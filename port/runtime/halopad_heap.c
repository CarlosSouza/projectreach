/* HaloPad guest heaps: HeapCreate/HeapAlloc/HeapFree/HeapReAlloc/HeapSize/HeapDestroy,
 * GetProcessHeap, and the fixed-memory forms of GlobalAlloc/LocalAlloc (G3).
 *
 * Blocks live in guest memory obtained from halopad_vmem.c; all bookkeeping (free
 * extents, block sizes, owning heap) is kept on the host, so a guest overflow cannot
 * corrupt it. Blocks are 16-byte aligned; blocks of 512 KiB or more get their own
 * region, as the Windows heap does. HeapSize returns the size that was requested. An
 * unknown pointer passed to free/size/realloc stops the program. */
#include "halopad_win32.h"
#include <pthread.h>

#define HEAP_NO_SERIALIZE 0x1u
#define HEAP_GENERATE_EXCEPTIONS 0x4u
#define HEAP_ZERO_MEMORY 0x8u
#define HEAP_REALLOC_IN_PLACE_ONLY 0x10u
#define ALIGN 16u
#define SEGMENT (8u << 20)
#define LARGE (512u << 10)
#define MAX_HEAPS 32

typedef struct { uint32_t addr, size; } extent;
typedef struct { uint32_t handle; extent *free; uint32_t nfree, cap; } heap;
typedef struct { uint32_t addr, rounded, requested; uint16_t heap; uint8_t large, used; } block;

static heap heaps[MAX_HEAPS];
static uint32_t process_heap;
static block *blocks;       /* open-addressing table keyed by guest address */
static uint32_t nblocks, ntomb, blockcap;

static uint32_t hash(uint32_t a) { return (a >> 4) * 2654435761u; }

static block *lookup(uint32_t a)
{
    if (!blockcap) return NULL;
    for (uint32_t i = hash(a) & (blockcap - 1);; i = (i + 1) & (blockcap - 1)) {
        if (!blocks[i].used && !blocks[i].addr) return NULL;
        if (blocks[i].used && blocks[i].addr == a) return &blocks[i];
    }
}

static void insert(block b);
static void grow_table(void)
{
    block *old = blocks; uint32_t oldcap = blockcap;
    if (!blockcap || nblocks * 4 > blockcap) blockcap = blockcap ? blockcap * 2 : 4096;   /* else rehash in place size */
    blocks = calloc(blockcap, sizeof *blocks);
    nblocks = 0; ntomb = 0;
    for (uint32_t i = 0; i < oldcap; i++) if (old[i].used) insert(old[i]);
    free(old);
}

static void insert(block b)
{
    if ((nblocks + ntomb + 1) * 2 > blockcap) grow_table();
    uint32_t i = hash(b.addr) & (blockcap - 1);
    while (blocks[i].used) i = (i + 1) & (blockcap - 1);
    if (blocks[i].addr) ntomb--;   /* reusing a tombstone */
    b.used = 1;
    blocks[i] = b;
    nblocks++;
}

static void erase(block *b)
{
    /* keep probe chains intact: mark as a tombstone (addr kept, used = 0) */
    b->used = 0;
    nblocks--; ntomb++;
}

static heap *heap_of(uint32_t handle)
{
    for (int i = 0; i < MAX_HEAPS; i++) if (heaps[i].handle && heaps[i].handle == handle) return &heaps[i];
    return NULL;
}

static void add_free(heap *h, uint32_t addr, uint32_t size)
{
    uint32_t lo = 0, hi = h->nfree;
    while (lo < hi) { uint32_t m = (lo + hi) / 2; if (h->free[m].addr < addr) lo = m + 1; else hi = m; }
    if (lo > 0 && h->free[lo - 1].addr + h->free[lo - 1].size == addr) {
        h->free[lo - 1].size += size;
        if (lo < h->nfree && addr + size == h->free[lo].addr) {
            h->free[lo - 1].size += h->free[lo].size;
            memmove(&h->free[lo], &h->free[lo + 1], (h->nfree - lo - 1) * sizeof(extent));
            h->nfree--;
        }
        return;
    }
    if (lo < h->nfree && addr + size == h->free[lo].addr) { h->free[lo].addr = addr; h->free[lo].size += size; return; }
    if (h->nfree == h->cap) { h->cap = h->cap ? h->cap * 2 : 256; h->free = realloc(h->free, h->cap * sizeof(extent)); }
    memmove(&h->free[lo + 1], &h->free[lo], (h->nfree - lo) * sizeof(extent));
    h->free[lo] = (extent){addr, size};
    h->nfree++;
}

static uint32_t take(heap *h, uint32_t size)
{
    for (uint32_t i = 0; i < h->nfree; i++) {
        if (h->free[i].size < size) continue;
        uint32_t a = h->free[i].addr;
        h->free[i].addr += size; h->free[i].size -= size;
        if (!h->free[i].size) { memmove(&h->free[i], &h->free[i + 1], (h->nfree - i - 1) * sizeof(extent)); h->nfree--; }
        return a;
    }
    uint32_t seg = size > SEGMENT ? (size + 0xFFFF) & ~0xFFFFu : SEGMENT;
    uint32_t base = halopad_vm_reserve(0, seg, 1);
    if (!base) return 0;
    if (seg > size) add_free(h, base + size, seg - size);
    return base;
}

/* Whether [a, a+n) is a single free extent start; used to grow in place. */
static int take_at(heap *h, uint32_t a, uint32_t n)
{
    for (uint32_t i = 0; i < h->nfree; i++) {
        if (h->free[i].addr != a) continue;
        if (h->free[i].size < n) return 0;
        h->free[i].addr += n; h->free[i].size -= n;
        if (!h->free[i].size) { memmove(&h->free[i], &h->free[i + 1], (h->nfree - i - 1) * sizeof(extent)); h->nfree--; }
        return 1;
    }
    return 0;
}

static uint32_t alloc(heap *h, uint32_t size, int zero)
{
    uint32_t rounded = size ? (size + ALIGN - 1) & ~(ALIGN - 1) : ALIGN;
    if (rounded < size) return 0;
    int large = rounded >= LARGE;
    uint32_t a = large ? halopad_vm_reserve(0, rounded, 1) : take(h, rounded);
    if (!a) return 0;
    if (zero || large) memset(halopad_guest_ptr(a), 0, rounded);
    insert((block){a, rounded, size, (uint16_t)(h - heaps), (uint8_t)large, 1});
    return a;
}

static void release(heap *h, block *b)
{
    if (b->large) halopad_vm_release(b->addr);
    else add_free(h, b->addr, b->rounded);
    erase(b);
}

static void check_flags(const char *what, uint32_t flags, uint32_t allowed)
{
    if (flags & ~allowed) hp_unsupported(what, "flags 0x%x", flags);
}

uint32_t HeapCreate_c(uint32_t options, uint32_t initial, uint32_t maximum);
uint32_t GetProcessHeap_c(void);
uint32_t HeapDestroy_c(uint32_t handle);
uint32_t HeapAlloc_c(uint32_t handle, uint32_t flags, uint32_t size);
uint32_t HeapFree_c(uint32_t handle, uint32_t flags, uint32_t a);
uint32_t HeapSize_c(uint32_t handle, uint32_t flags, uint32_t a);
uint32_t HeapReAlloc_c(uint32_t handle, uint32_t flags, uint32_t a, uint32_t size);
uint32_t GlobalLock_c(uint32_t a);
uint32_t GlobalUnlock_c(uint32_t a);

static uint32_t HeapCreate_c_unlocked(uint32_t options, uint32_t initial, uint32_t maximum)
{
    (void)initial;
    check_flags("HeapCreate", options, HEAP_NO_SERIALIZE);
    if (maximum) hp_unsupported("HeapCreate", "fixed maximum size 0x%x", maximum);
    for (int i = 0; i < MAX_HEAPS; i++) {
        if (heaps[i].handle) continue;
        /* the handle is a guest address, as on Windows: a small runtime-owned page */
        heaps[i].handle = halopad_vm_reserve(0, 0x1000, 1);
        return heaps[i].handle;
    }
    hp_unsupported("HeapCreate", "more than %d heaps", MAX_HEAPS);
}

static uint32_t GetProcessHeap_c_unlocked(void)
{
    if (!process_heap) process_heap = HeapCreate_c(0, 0, 0);
    return process_heap;
}

static uint32_t HeapDestroy_c_unlocked(uint32_t handle)
{
    heap *h = heap_of(handle);
    if (!h || handle == process_heap) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    for (uint32_t i = 0; i < blockcap; i++)
        if (blocks[i].used && &heaps[blocks[i].heap] == h) release(h, &blocks[i]);
    /* segments stay reserved for reuse by later heaps' large blocks */
    free(h->free);
    *h = (heap){0};
    return 1;
}

static uint32_t HeapAlloc_c_unlocked(uint32_t handle, uint32_t flags, uint32_t size)
{
    check_flags("HeapAlloc", flags, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY | HEAP_GENERATE_EXCEPTIONS);
    heap *h = heap_of(handle);
    if (!h) hp_unsupported("HeapAlloc", "heap handle 0x%08x", handle);
    uint32_t a = alloc(h, size, (flags & HEAP_ZERO_MEMORY) != 0);
    if (!a && (flags & HEAP_GENERATE_EXCEPTIONS)) hp_unsupported("HeapAlloc", "out of memory exception for 0x%x bytes", size);
    if (!a) halopad_last_error = HP_ERROR_NOT_ENOUGH_MEMORY;
    return a;
}

static block *owned(const char *what, uint32_t handle, uint32_t a)
{
    block *b = lookup(a);
    if (!b || !heap_of(handle) || &heaps[b->heap] != heap_of(handle))
        hp_unsupported(what, "pointer 0x%08x that heap 0x%08x did not allocate", a, handle);
    return b;
}

static uint32_t HeapFree_c_unlocked(uint32_t handle, uint32_t flags, uint32_t a)
{
    check_flags("HeapFree", flags, HEAP_NO_SERIALIZE);
    if (!a) return 1;
    block *b = owned("HeapFree", handle, a);
    release(&heaps[b->heap], b);
    return 1;
}

static uint32_t HeapSize_c_unlocked(uint32_t handle, uint32_t flags, uint32_t a)
{
    check_flags("HeapSize", flags, HEAP_NO_SERIALIZE);
    return owned("HeapSize", handle, a)->requested;
}

static uint32_t HeapReAlloc_c_unlocked(uint32_t handle, uint32_t flags, uint32_t a, uint32_t size)
{
    check_flags("HeapReAlloc", flags, HEAP_NO_SERIALIZE | HEAP_ZERO_MEMORY | HEAP_REALLOC_IN_PLACE_ONLY | HEAP_GENERATE_EXCEPTIONS);
    block *b = owned("HeapReAlloc", handle, a);
    heap *h = &heaps[b->heap];
    uint32_t old = b->requested, rounded = size ? (size + ALIGN - 1) & ~(ALIGN - 1) : ALIGN;
    if (rounded <= b->rounded || (!b->large && take_at(h, b->addr + b->rounded, rounded - b->rounded))) {
        if (rounded > b->rounded) b->rounded = rounded;
        b->requested = size;
        if ((flags & HEAP_ZERO_MEMORY) && size > old) memset((uint8_t *)halopad_guest_ptr(a) + old, 0, size - old);
        return a;
    }
    if (flags & HEAP_REALLOC_IN_PLACE_ONLY) { halopad_last_error = HP_ERROR_NOT_ENOUGH_MEMORY; return 0; }
    uint32_t n = alloc(h, size, 0);
    if (!n) { halopad_last_error = HP_ERROR_NOT_ENOUGH_MEMORY; return 0; }
    memcpy(halopad_guest_ptr(n), halopad_guest_ptr(a), old < size ? old : size);
    if ((flags & HEAP_ZERO_MEMORY) && size > old) memset((uint8_t *)halopad_guest_ptr(n) + old, 0, size - old);
    release(h, lookup(a));
    return n;
}

/* Runtime-owned guest data (strings handed to the game, etc.). */
uint32_t halopad_heap_alloc(uint32_t size, int zero)
{
    return HeapAlloc_c(GetProcessHeap_c(), zero ? HEAP_ZERO_MEMORY : 0, size);
}

void halopad_heap_free(uint32_t a) { HeapFree_c(GetProcessHeap_c(), 0, a); }

/* GlobalAlloc/LocalAlloc: fixed memory only (GMEM_FIXED/GPTR, LMEM_FIXED/LPTR). */
#define GMEM_MOVEABLE 0x2u
#define GMEM_ZEROINIT 0x40u

static uint32_t fixed_alloc(const char *what, uint32_t flags, uint32_t size)
{
    if (flags & ~GMEM_ZEROINIT) hp_unsupported(what, "flags 0x%x (movable memory)", flags);
    return HeapAlloc_c(GetProcessHeap_c(), (flags & GMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0, size);
}

uint32_t GlobalAlloc_c(uint32_t flags, uint32_t size) { return fixed_alloc("GlobalAlloc", flags, size); }
uint32_t LocalAlloc_c(uint32_t flags, uint32_t size) { return fixed_alloc("LocalAlloc", flags, size); }
uint32_t GlobalFree_c(uint32_t a) { return HeapFree_c(GetProcessHeap_c(), 0, a) ? 0 : a; }
uint32_t LocalFree_c(uint32_t a) { return HeapFree_c(GetProcessHeap_c(), 0, a) ? 0 : a; }
uint32_t GlobalReAlloc_c(uint32_t a, uint32_t size, uint32_t flags)
{
    if (flags & ~GMEM_ZEROINIT) hp_unsupported("GlobalReAlloc", "flags 0x%x", flags);
    return HeapReAlloc_c(GetProcessHeap_c(), (flags & GMEM_ZEROINIT) ? HEAP_ZERO_MEMORY : 0, a, size);
}
/* For fixed memory the handle is the pointer; the lock count is not tracked. */
static uint32_t GlobalLock_c_unlocked(uint32_t a) { if (a && !lookup(a)) hp_unsupported("GlobalLock", "handle 0x%08x", a); return a; }
static uint32_t GlobalUnlock_c_unlocked(uint32_t a) { if (a && !lookup(a)) hp_unsupported("GlobalUnlock", "handle 0x%08x", a); halopad_last_error = 0; return 0; }

/* ---- thread safety (Halo allocates from several threads): every entry point holds one recursive lock ---- */
static pthread_mutex_t heap_lock;
__attribute__((constructor)) static void heap_lock_init(void)
{
    pthread_mutexattr_t a;
    pthread_mutexattr_init(&a);
    pthread_mutexattr_settype(&a, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&heap_lock, &a);
}
uint32_t HeapCreate_c(uint32_t options, uint32_t initial, uint32_t maximum) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapCreate_c_unlocked(options, initial, maximum); pthread_mutex_unlock(&heap_lock); if (!r) HP_TRACE_FAIL("HeapCreate(0x%x, 0x%x, 0x%x)", options, initial, maximum); return r; }
uint32_t GetProcessHeap_c(void) { pthread_mutex_lock(&heap_lock); uint32_t r = GetProcessHeap_c_unlocked(); pthread_mutex_unlock(&heap_lock); return r; }
uint32_t HeapDestroy_c(uint32_t handle) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapDestroy_c_unlocked(handle); pthread_mutex_unlock(&heap_lock); return r; }
uint32_t HeapAlloc_c(uint32_t handle, uint32_t flags, uint32_t size) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapAlloc_c_unlocked(handle, flags, size); pthread_mutex_unlock(&heap_lock); if (!r) HP_TRACE_FAIL("HeapAlloc(0x%08x, 0x%x, 0x%x)", handle, flags, size); return r; }
uint32_t HeapFree_c(uint32_t handle, uint32_t flags, uint32_t a) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapFree_c_unlocked(handle, flags, a); pthread_mutex_unlock(&heap_lock); return r; }
uint32_t HeapSize_c(uint32_t handle, uint32_t flags, uint32_t a) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapSize_c_unlocked(handle, flags, a); pthread_mutex_unlock(&heap_lock); return r; }
uint32_t HeapReAlloc_c(uint32_t handle, uint32_t flags, uint32_t a, uint32_t size) { pthread_mutex_lock(&heap_lock); uint32_t r = HeapReAlloc_c_unlocked(handle, flags, a, size); pthread_mutex_unlock(&heap_lock); if (!r) HP_TRACE_FAIL("HeapReAlloc(0x%08x, 0x%x, 0x%08x, 0x%x)", handle, flags, a, size); return r; }
uint32_t GlobalLock_c(uint32_t a) { pthread_mutex_lock(&heap_lock); uint32_t r = GlobalLock_c_unlocked(a); pthread_mutex_unlock(&heap_lock); return r; }
uint32_t GlobalUnlock_c(uint32_t a) { pthread_mutex_lock(&heap_lock); uint32_t r = GlobalUnlock_c_unlocked(a); pthread_mutex_unlock(&heap_lock); return r; }

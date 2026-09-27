/* HaloPad guest virtual memory: VirtualAlloc/VirtualFree/VirtualQuery/VirtualProtect
 * over the reserved 4 GiB guest region (G3). Reservations use Windows' 64 KiB
 * granularity and 4 KiB pages; committed pages read as zero. Regions are tracked on the
 * host, so guest code cannot corrupt the bookkeeping. Execute permissions are accepted
 * but nothing in guest memory is ever executed: control flow only reaches compiled code
 * through dispatch, so running generated code would stop there with its address. */
#include "halopad_win32.h"
#include <sys/mman.h>
#include <unistd.h>

extern uint64_t halopad_guest_base;

#define MEM_COMMIT 0x1000u
#define MEM_RESERVE 0x2000u
#define MEM_DECOMMIT 0x4000u
#define MEM_RELEASE 0x8000u
#define MEM_FREE 0x10000u
#define MEM_PRIVATE 0x20000u
#define MEM_TOP_DOWN 0x100000u
#define GRAN 0x10000u
#define PAGE 0x1000u
#define VM_LO 0x01000000u    /* allocations start above the image (0x400000-0x82c000) */
#define VM_HI 0x7FFD0000u    /* below the TLS/TEB pages */

typedef struct { uint32_t base, size, protect; uint8_t *committed; } region;
static region regions[4096];
static uint32_t nregions;

static region *find(uint32_t a)
{
    for (uint32_t i = 0; i < nregions; i++)
        if (a >= regions[i].base && a - regions[i].base < regions[i].size) return &regions[i];
    return NULL;
}

static int overlaps(uint32_t base, uint32_t size)
{
    for (uint32_t i = 0; i < nregions; i++)
        if (base < regions[i].base + regions[i].size && regions[i].base < base + size) return 1;
    return 0;
}

/* Fixed ranges already in use (image, stack, thread pages) are registered at startup. */
void halopad_vm_mark(uint32_t base, uint32_t size)
{
    if (nregions == sizeof regions / sizeof regions[0]) hp_unsupported("VirtualAlloc", "more than %u regions", nregions);
    regions[nregions++] = (region){base, size, 4, NULL};
}

/* The host may use larger pages than Windows' 4 KiB (16 KiB on Apple Silicon). Granting
 * access rounds outward (harmless); removing access rounds inward, so it never takes
 * access away from memory the guest still owns. */
static uint32_t host_page(void) { static uint32_t hp; if (!hp) hp = (uint32_t)sysconf(_SC_PAGESIZE); return hp; }

static int host_range(uint32_t a, uint32_t size, int outward, uint32_t *lo, uint32_t *hi)
{
    uint32_t hp = host_page();
    uint64_t end = (uint64_t)a + size;
    *lo = outward ? a & ~(hp - 1) : (a + hp - 1) & ~(hp - 1);
    *hi = (uint32_t)(outward ? (end + hp - 1) & ~(uint64_t)(hp - 1) : end & ~(uint64_t)(hp - 1));
    return *hi > *lo;
}

static void set_committed(region *r, uint32_t a, uint32_t size, int on)
{
    for (uint32_t p = (a - r->base) / PAGE; p < (a - r->base + size + PAGE - 1) / PAGE; p++) r->committed[p] = (uint8_t)on;
}

static void protect(uint32_t a, uint32_t size, int rw)
{
    uint32_t lo, hi;
    if (!host_range(a, size, rw, &lo, &hi)) return;
    void *host = (void *)(uintptr_t)(halopad_guest_base + lo);
    if (rw) {
        if (mprotect(host, hi - lo, PROT_READ | PROT_WRITE) != 0) { perror("HALOPAD: mprotect"); abort(); }
    } else if (mmap(host, hi - lo, PROT_NONE, MAP_PRIVATE | MAP_ANON | MAP_FIXED, -1, 0) == MAP_FAILED) {
        perror("HALOPAD: mmap"); abort();
    }
}

uint32_t halopad_vm_reserve(uint32_t address, uint32_t size, int commit_now)
{
    size = (size + PAGE - 1) & ~(PAGE - 1);
    uint32_t span = (size + GRAN - 1) & ~(GRAN - 1);
    uint32_t base = 0;
    if (address) {
        base = address & ~(GRAN - 1);
        span = ((address + size + GRAN - 1) & ~(GRAN - 1)) - base;
        if (base < 0x10000 || (uint64_t)base + span > VM_HI || overlaps(base, span)) return 0;
    } else {
        for (uint64_t b = VM_LO; b + span <= VM_HI; b += GRAN)
            if (!overlaps((uint32_t)b, span)) { base = (uint32_t)b; break; }
        if (!base) return 0;
    }
    if (nregions == sizeof regions / sizeof regions[0]) hp_unsupported("VirtualAlloc", "more than %u regions", nregions);
    region *r = &regions[nregions++];
    *r = (region){base, span, 4, calloc(span / PAGE, 1)};
    if (commit_now) { protect(base, span, 1); set_committed(r, base, span, 1); }
    return address ? address : base;
}

int halopad_vm_release(uint32_t address)
{
    region *r = find(address);
    if (!r || r->base != address || !r->committed) return 0;
    protect(r->base, r->size, 0);
    free(r->committed);
    *r = regions[--nregions];
    return 1;
}

uint32_t VirtualAlloc_c(uint32_t address, uint32_t size, uint32_t type, uint32_t prot)
{
    uint32_t known = MEM_COMMIT | MEM_RESERVE | MEM_TOP_DOWN;
    if (type & ~known) hp_unsupported("VirtualAlloc", "allocation type 0x%x", type);
    if (prot != 0x04 && prot != 0x40 && prot != 0x01) hp_unsupported("VirtualAlloc", "protection 0x%x", prot);
    if (!size) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    region *r = address ? find(address) : NULL;
    if (r && (type & MEM_COMMIT) && !(type & MEM_RESERVE) && r->committed) {
        /* commit inside an existing reservation */
        uint32_t a = address & ~(PAGE - 1), end = (address + size + PAGE - 1) & ~(PAGE - 1);
        if (end - r->base > r->size) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
        if (prot != 0x01) { protect(a, end - a, 1); set_committed(r, a, end - a, 1); }
        return a;
    }
    if (!(type & MEM_RESERVE) && address) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    uint32_t got = halopad_vm_reserve(address, size, (type & MEM_COMMIT) && prot != 0x01);
    if (!got) halopad_last_error = address ? 487 /* ERROR_INVALID_ADDRESS */ : HP_ERROR_NOT_ENOUGH_MEMORY;
    return got;
}

uint32_t VirtualFree_c(uint32_t address, uint32_t size, uint32_t type)
{
    region *r = find(address);
    if (!r || !r->committed) { halopad_last_error = 487; return 0; }
    if (type == MEM_RELEASE) {
        if (size != 0 || r->base != address) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
        return (uint32_t)halopad_vm_release(address);
    }
    if (type == MEM_DECOMMIT) {
        uint32_t a = address & ~(PAGE - 1);
        uint32_t end = size ? (address + size + PAGE - 1) & ~(PAGE - 1) : r->base + r->size;
        protect(a, end - a, 0);
        set_committed(r, a, end - a, 0);
        return 1;
    }
    hp_unsupported("VirtualFree", "free type 0x%x", type);
}

uint32_t VirtualQuery_c(uint32_t address, uint32_t info, uint32_t length)
{
    if (length < 28) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    region *r = find(address);
    if (!r) hp_unsupported("VirtualQuery", "address 0x%08x outside known regions", address);
    uint32_t page = address & ~(PAGE - 1);
    uint32_t on = r->committed ? r->committed[(page - r->base) / PAGE] : 1, end = page;
    while (end < r->base + r->size && (r->committed ? r->committed[(end - r->base) / PAGE] : 1) == on) end += PAGE;
    wr32(info + 0, page);
    wr32(info + 4, r->base);
    wr32(info + 8, r->protect);
    wr32(info + 12, end - page);
    wr32(info + 16, on ? MEM_COMMIT : MEM_RESERVE);
    wr32(info + 20, on ? r->protect : 0);
    wr32(info + 24, r->committed ? MEM_PRIVATE : 0x1000000 /* MEM_IMAGE */);
    return 28;
}

uint32_t VirtualProtect_c(uint32_t address, uint32_t size, uint32_t prot, uint32_t old)
{
    region *r = find(address);
    if (!r) hp_unsupported("VirtualProtect", "address 0x%08x outside known regions", address);
    int rw;
    if (prot == 0x04 || prot == 0x40) rw = 1;          /* PAGE_READWRITE, PAGE_EXECUTE_READWRITE */
    else if (prot == 0x02 || prot == 0x20) rw = 0;     /* PAGE_READONLY, PAGE_EXECUTE_READ */
    else hp_unsupported("VirtualProtect", "protection 0x%x at 0x%08x", prot, address);
    if (old) wr32(old, r->protect);
    uint32_t lo, hi;
    if (host_range(address, size ? size : 1, rw, &lo, &hi)
        && mprotect((void *)(uintptr_t)(halopad_guest_base + lo), hi - lo, rw ? PROT_READ | PROT_WRITE : PROT_READ) != 0) {
        perror("HALOPAD: mprotect"); abort();
    }
    r->protect = prot;
    return 1;
}

/* The image's read-only parts (headers, .text, .rdata, .rsrc) are read-only, as on
 * Windows. Translated code never reads its own instructions as code, so a write to .text
 * (self-modifying code) would silently diverge; read-only pages make it fault and name
 * the guest address instead. A host page is protected only if every 4 KiB page in it is
 * read-only in the image. */
void halopad_protect_image(uint32_t image_base)
{
    uint32_t pe = image_base + rd32(image_base + 0x3C);
    uint32_t nsec = rd32(pe + 6) & 0xFFFF, opt = rd32(pe + 0x14) & 0xFFFF, image_size = rd32(pe + 0x50);
    uint32_t pages = image_size / PAGE;
    uint8_t *ro = calloc(pages, 1);
    memset(ro, 1, (pe + 0x18 + opt + 40 * nsec - image_base + PAGE - 1) / PAGE);   /* headers */
    for (uint32_t i = 0; i < nsec; i++) {
        uint32_t sh = pe + 0x18 + opt + 40 * i;
        uint32_t rva = rd32(sh + 12), size = rd32(sh + 8), flags = rd32(sh + 36);
        if (flags & 0x80000000u) continue;                        /* IMAGE_SCN_MEM_WRITE */
        for (uint32_t p = rva / PAGE; p < (rva + size + PAGE - 1) / PAGE && p < pages; p++) ro[p] = 1;
    }
    uint32_t per = host_page() / PAGE;
    for (uint32_t hp = 0; hp + per <= pages; hp += per) {
        int all = 1;
        for (uint32_t k = 0; k < per; k++) all &= ro[hp + k];
        if (all && mprotect((void *)(uintptr_t)(halopad_guest_base + image_base + hp * PAGE), host_page(), PROT_READ) != 0) {
            perror("HALOPAD: mprotect"); abort();
        }
    }
    free(ro);
}

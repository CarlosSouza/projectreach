/* HaloPad thread environments (G3): the Windows thread environment block (TEB) that
 * translated code reaches through fs:, one per thread, with its static TLS block.
 *
 * The main thread's TEB is at 0x7FFDE000 and its TLS array and block at 0x7FFD0000, the
 * addresses Windows XP used. Threads Halo creates get a two-page region: the TEB, then
 * the TLS array and a fresh copy of the image's TLS template. Only the TEB fields Halo is
 * known to use are populated (SEH chain, stack bounds, self, process and thread IDs, TLS
 * pointer, TlsSlots); any other fs: offset stops with its offset. */
#include <pthread.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CCALL
#define TEB_VA 0x7FFDE000u
#define TLS_VA 0x7FFD0000u   /* TLS pointer array + main-thread TLS block */

uint32_t halopad_guest_commit(uint32_t guest, uint32_t size);
void *halopad_guest_ptr(uint32_t guest);
uint32_t halopad_vm_reserve(uint32_t address, uint32_t size, int commit_now);
int halopad_vm_release(uint32_t address);
extern _Thread_local uint32_t halopad_current_tid;

static _Thread_local uint32_t *teb;
static _Thread_local uint32_t teb_va;
static struct { uint32_t start, end, zero, index; int present; } tls;
static uint32_t live[256];                 /* every thread's TEB (TlsAlloc clears a slot in all of them) */
static pthread_mutex_t live_lock = PTHREAD_MUTEX_INITIALIZER;

static void put(uint32_t guest, uint32_t v) { memcpy(halopad_guest_ptr(guest), &v, 4); }
static uint32_t get(uint32_t guest) { uint32_t v; memcpy(&v, halopad_guest_ptr(guest), 4); return v; }

static void build(uint32_t va, uint32_t tls_va, uint32_t stack_base, uint32_t stack_limit)
{
    teb = halopad_guest_ptr(va);
    teb_va = va;
    memset(teb, 0, 0x1000);
    teb[0x00 / 4] = 0xFFFFFFFFu;   /* ExceptionList: end of the SEH chain */
    teb[0x04 / 4] = stack_base;    /* StackBase (top) */
    teb[0x08 / 4] = stack_limit;   /* StackLimit */
    teb[0x18 / 4] = va;            /* Self */
    teb[0x20 / 4] = 0x1000;        /* ClientId.UniqueProcess (GetCurrentProcessId) */
    teb[0x24 / 4] = halopad_current_tid;   /* ClientId.UniqueThread */
    teb[0x2C / 4] = tls_va;        /* ThreadLocalStoragePointer */
    if (tls.present) {
        uint32_t block = tls_va + 0x100;
        memcpy(halopad_guest_ptr(block), halopad_guest_ptr(tls.start), tls.end - tls.start);
        memset((uint8_t *)halopad_guest_ptr(block) + (tls.end - tls.start), 0, tls.zero);
        put(tls_va, block);        /* slot 0: the image's TLS index is 0 */
    }
    pthread_mutex_lock(&live_lock);
    for (int i = 0; i < 256; i++) if (!live[i]) { live[i] = va; break; }
    pthread_mutex_unlock(&live_lock);
}

/* Build the TEB for the main thread and its static TLS block from the image's TLS
 * directory (template start/end, index address, zero fill). */
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base)
{
    halopad_guest_commit(TEB_VA, 0x1000);
    halopad_guest_commit(TLS_VA, 0x1000);
    uint32_t pe = image_base + get(image_base + 0x3C);
    uint32_t tls_rva = get(pe + 0x78 + 9 * 8);          /* data directory 9: TLS */
    if (tls_rva) {
        uint32_t dir = image_base + tls_rva;
        uint32_t callbacks = get(dir + 12);
        tls.start = get(dir); tls.end = get(dir + 4); tls.index = get(dir + 8); tls.zero = get(dir + 16);
        if (tls.end - tls.start + tls.zero > 0x1000 - 0x100) { fprintf(stderr, "HALOPAD TRAP: TLS block of %u bytes\n", tls.end - tls.start + tls.zero); abort(); }
        if (callbacks && get(callbacks)) {
            fprintf(stderr, "HALOPAD TRAP: TLS callback 0x%08x present; TLS callbacks are not run yet\n", get(callbacks));
            abort();
        }
        tls.present = 1;
        put(tls.index, 0);         /* the image's TLS index */
    }
    build(TEB_VA, TLS_VA, stack_base, stack_limit);
}

/* A new thread's environment (called on that thread); returns its TEB address. */
uint32_t halopad_thread_attach(uint32_t stack_base, uint32_t stack_limit)
{
    uint32_t va = halopad_vm_reserve(0, 0x2000, 1);
    if (!va) { fprintf(stderr, "HALOPAD TRAP: no guest memory for a thread environment\n"); abort(); }
    build(va, va + 0x1000, stack_base, stack_limit);
    return va;
}

void halopad_thread_detach(void)
{
    pthread_mutex_lock(&live_lock);
    for (int i = 0; i < 256; i++) if (live[i] == teb_va) live[i] = 0;
    pthread_mutex_unlock(&live_lock);
    if (teb_va != TEB_VA) halopad_vm_release(teb_va);
    teb = NULL;
    teb_va = 0;
}

uint32_t halopad_teb(void) { return teb_va; }

/* Run fn on every live TEB (under the registry lock). */
void halopad_teb_each(void (*fn)(uint32_t teb_address, uint32_t arg), uint32_t arg)
{
    pthread_mutex_lock(&live_lock);
    for (int i = 0; i < 256; i++) if (live[i]) fn(live[i], arg);
    pthread_mutex_unlock(&live_lock);
}

static int known_offset(uint32_t ofs)
{
    return ofs == 0x00 || ofs == 0x04 || ofs == 0x08 || ofs == 0x18 || ofs == 0x20 || ofs == 0x24 || ofs == 0x2C;
}

uint32_t CCALL X86_ReadFsDword(uint32_t ofs)
{
    if (!teb || !known_offset(ofs)) {
        fprintf(stderr, "HALOPAD TRAP: fs:[0x%x] read; this thread-environment field is not provided\n", ofs);
        abort();
    }
    return teb[ofs / 4];
}

void CCALL X86_WriteFsDword(uint32_t ofs, uint32_t value)
{
    if (!teb || ofs != 0x00) {
        fprintf(stderr, "HALOPAD TRAP: fs:[0x%x] write of 0x%x; only the SEH chain head is writable\n", ofs, value);
        abort();
    }
    teb[0] = value;
}

/* HaloPad main-thread environment (G3): the Windows thread environment block that
 * translated code reaches through fs:, in guest memory at the address Windows XP used.
 * Only the fields Halo is known to use are populated; any other fs: offset stops with
 * its offset. The PEB is not provided yet: fs:[0x30] traps until something needs it. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CCALL
#define TEB_VA 0x7FFDE000u
#define TLS_VA 0x7FFD0000u   /* TLS pointer array + main-thread TLS block */

uint32_t halopad_guest_commit(uint32_t guest, uint32_t size);
void *halopad_guest_ptr(uint32_t guest);

static uint32_t *teb;

static void put(uint32_t guest, uint32_t v) { memcpy(halopad_guest_ptr(guest), &v, 4); }
static uint32_t get(uint32_t guest) { uint32_t v; memcpy(&v, halopad_guest_ptr(guest), 4); return v; }

/* Build the TEB for the main thread and its static TLS block from the image's TLS
 * directory (template start/end, index address, zero fill). */
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base)
{
    halopad_guest_commit(TEB_VA, 0x1000);
    halopad_guest_commit(TLS_VA, 0x1000);
    teb = halopad_guest_ptr(TEB_VA);
    teb[0x00 / 4] = 0xFFFFFFFFu;   /* ExceptionList: end of the SEH chain */
    teb[0x04 / 4] = stack_base;    /* StackBase (top) */
    teb[0x08 / 4] = stack_limit;   /* StackLimit */
    teb[0x18 / 4] = TEB_VA;        /* Self */
    teb[0x2C / 4] = TLS_VA;        /* ThreadLocalStoragePointer */

    uint32_t pe = image_base + get(image_base + 0x3C);
    uint32_t tls_rva = get(pe + 0x78 + 9 * 8);          /* data directory 9: TLS */
    if (!tls_rva) return;
    uint32_t dir = image_base + tls_rva;
    uint32_t start = get(dir), end = get(dir + 4), index = get(dir + 8), callbacks = get(dir + 12), zero = get(dir + 16);
    uint32_t size = end - start + zero;
    if (size > 0x1000 - 0x100) { fprintf(stderr, "HALOPAD TRAP: TLS block of %u bytes\n", size); abort(); }
    if (callbacks && get(callbacks)) {
        fprintf(stderr, "HALOPAD TRAP: TLS callback 0x%08x present; TLS callbacks are not run yet\n", get(callbacks));
        abort();
    }
    uint32_t block = TLS_VA + 0x100;
    memcpy(halopad_guest_ptr(block), halopad_guest_ptr(start), end - start);
    put(TLS_VA, block);   /* slot 0 */
    put(index, 0);        /* the image's TLS index */
}

static int known_offset(uint32_t ofs)
{
    return ofs == 0x00 || ofs == 0x04 || ofs == 0x08 || ofs == 0x18 || ofs == 0x2C;
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

/* G2d slice 1 harness: Halo's stdcall CRC32 (0x59f2a2) translated by SRW/llasm and
 * compiled for ARM64. Reads cases (u32 len, u32 crc_in, len bytes) and prints
 * "eax esp_delta" per case for comparison with the x86 oracle. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern void c_halo_crc32(_cpu *);
extern void (*ptr_initialize_pointers)(uint64_t);

static uint32_t stack_words[4096];
static uint8_t buffer[1 << 20];

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s cases.bin\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    uintptr_t offset = (uintptr_t)&c_halo_crc32 & ~(uintptr_t)UINT32_MAX;
    uintptr_t lo = (uintptr_t)stack_words < (uintptr_t)buffer ? (uintptr_t)stack_words : (uintptr_t)buffer;
    uintptr_t hi = (uintptr_t)(buffer + sizeof buffer) > (uintptr_t)(stack_words + 4096) ? (uintptr_t)(buffer + sizeof buffer) : (uintptr_t)(stack_words + 4096);
    if (lo < offset || hi - offset > UINT32_MAX) {
        fprintf(stderr, "FAIL: upstream offset model cannot represent this layout\n");
        return 2;
    }
    ptr_initialize_pointers(offset);
    uint32_t len, crc;
    while (fread(&len, 4, 1, f) == 1 && fread(&crc, 4, 1, f) == 1) {
        if (len > sizeof buffer || fread(buffer, 1, len, f) != len) { fprintf(stderr, "FAIL: bad case file\n"); return 2; }
        uint32_t *sp = &stack_words[2048];
        sp[0] = crc;
        sp[1] = (uint32_t)((uintptr_t)buffer - offset);
        sp[2] = len;
        _cpu state;
        memset(&state, 0, sizeof state);
        state._esp = (uint32_t)((uintptr_t)sp - offset);
        state._pointer_offset = offset;
        c_halo_crc32(&state);
        printf("%08" PRIx32 " %" PRId32 "\n", state._eax, (int32_t)(state._esp - (uint32_t)((uintptr_t)sp - offset)));
    }
    return 0;
}

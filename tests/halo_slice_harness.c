/* G2d generic slice harness: calls translated Halo functions (exported through
 * global_aliases.sci) on cases from a binary file and prints observable results.
 *
 * Case: u32 fn, u32 fpcw, u32 nargs, then per argument
 *   kind 0: u32 value              (integer)
 *   kind 1: u32 len, len bytes     (fresh 16-byte aligned buffer)
 *   kind 2: u32 arg, u32 offset    (pointer into another buffer argument)
 * Output line: eax esp_delta eax_ref [buffer hex ...]
 *   eax_ref is B<i>+<off> when eax points into buffer argument i, else V<eax>. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern void (*const halopad_slice_fns[])(_cpu *);
extern const unsigned halopad_slice_count;
/* llasm's global constructor stores its data-pointer initializer here; it must run once
 * with the pointer offset before any translated code reads a stored code/data address. */
extern void (*ptr_initialize_pointers)(uint64_t);

static uint32_t stack_words[16384];
static uint8_t arena[16 << 20];

static int rd(FILE *f, uint32_t *v) { return fread(v, 4, 1, f) == 1; }

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s cases.bin\n", argv[0]); return 2; }
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    uintptr_t offset = (uintptr_t)halopad_slice_fns[0] & ~(uintptr_t)UINT32_MAX;
    uintptr_t lo = (uintptr_t)stack_words < (uintptr_t)arena ? (uintptr_t)stack_words : (uintptr_t)arena;
    uintptr_t hi1 = (uintptr_t)(arena + sizeof arena), hi2 = (uintptr_t)(stack_words + 16384);
    uintptr_t hi = hi1 > hi2 ? hi1 : hi2;
    if (lo < offset || hi - offset > UINT32_MAX) { fprintf(stderr, "FAIL: layout outside the offset window\n"); return 2; }
    if (!ptr_initialize_pointers) { fprintf(stderr, "FAIL: llasm pointer initializer missing\n"); return 2; }
    ptr_initialize_pointers(offset);
    uint32_t fn, fpcw, nargs;
    while (rd(f, &fn) && rd(f, &fpcw) && rd(f, &nargs)) {
        if (fn >= halopad_slice_count || nargs > 16) { fprintf(stderr, "FAIL: bad case header\n"); return 2; }
        uint32_t values[16], kinds[16], lens[16], ptr_arg[16], ptr_off[16];
        uintptr_t hosts[16];
        size_t used = 0;
        for (uint32_t i = 0; i < nargs; i++) {
            uint32_t kind, a, b;
            if (!rd(f, &kind) || !rd(f, &a)) return 2;
            kinds[i] = kind;
            if (kind == 0) { values[i] = a; hosts[i] = 0; lens[i] = 0; }
            else if (kind == 1) {
                used = (used + 15) & ~(size_t)15;
                if (used + a + 64 > sizeof arena || fread(arena + used, 1, a, f) != a) return 2;
                hosts[i] = (uintptr_t)(arena + used); lens[i] = a;
                values[i] = (uint32_t)(hosts[i] - offset);
                used += a + 0x40;
            } else if (kind == 2) {
                if (!rd(f, &b)) return 2;
                ptr_arg[i] = a; ptr_off[i] = b; hosts[i] = 0; lens[i] = 0;
            } else return 2;
        }
        for (uint32_t i = 0; i < nargs; i++) {
            if (kinds[i] != 2) continue;
            if (ptr_arg[i] >= nargs || kinds[ptr_arg[i]] != 1) return 2;
            values[i] = values[ptr_arg[i]] + ptr_off[i];
        }
        uint32_t *sp = &stack_words[8192];
        for (uint32_t i = 0; i < nargs; i++) sp[i] = values[i];
        _cpu state;
        memset(&state, 0, sizeof state);
        state._esp = (uint32_t)((uintptr_t)sp - offset);
        state._pointer_offset = offset;
        state._st_cw = fpcw;
        halopad_slice_fns[fn](&state);
        printf("%08" PRIx32 " %" PRId32, state._eax, (int32_t)(state._esp - (uint32_t)((uintptr_t)sp - offset)));
        int found = 0;
        for (uint32_t i = 0; i < nargs && !found; i++)
            if (kinds[i] == 1 && state._eax >= values[i] && state._eax <= values[i] + lens[i]) {
                printf(" B%u+%u", i, state._eax - values[i]); found = 1;
            }
        if (!found) printf(" V%08" PRIx32, state._eax);
        for (uint32_t i = 0; i < nargs; i++) {
            if (kinds[i] != 1) continue;
            putchar(' ');
            if (lens[i] == 0) { putchar('-'); continue; }
            for (uint32_t k = 0; k < lens[i]; k++) printf("%02x", ((uint8_t *)hosts[i])[k]);
        }
        putchar('\n');
    }
    return 0;
}

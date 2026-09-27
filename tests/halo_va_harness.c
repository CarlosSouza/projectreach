/* G2e harness: translated Halo functions in the original-address model.
 * Case format as tests/halo_slice_harness.c, except the first word is the function's
 * original address. Buffers live in guest memory; pointers are guest addresses.
 * With HALOPAD_ENTER_VIA_SLOT=1 the first word is instead a guest address holding the
 * entry (as 'call [slot]' or 'mov esi, [slot]; call esi' read it), e.g. an import slot. */
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
uint32_t halopad_guest_alloc(uint32_t size, uint32_t align);
void *halopad_guest_ptr(uint32_t guest);
void halopad_enter(_cpu *cpu, uint32_t va);
extern void (*ptr_initialize_pointers)(uint64_t) __attribute__((weak));

static int rd(FILE *f, uint32_t *v) { return fread(v, 4, 1, f) == 1; }

int main(int argc, char **argv)
{
    if (argc != 2) { fprintf(stderr, "usage: %s cases.bin\n", argv[0]); return 2; }
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) { fprintf(stderr, "HALOPAD_IMAGE not set\n"); return 2; }
    uint32_t stack_top = halopad_guest_init(image, 0x400000);
    if (&ptr_initialize_pointers && ptr_initialize_pointers) ptr_initialize_pointers(halopad_guest_base);
    FILE *f = fopen(argv[1], "rb");
    if (!f) { perror(argv[1]); return 2; }
    const char *via = getenv("HALOPAD_ENTER_VIA_SLOT");
    int via_slot = via && via[0] == '1';
    uint32_t va, fpcw, nargs;
    while (rd(f, &va) && rd(f, &fpcw) && rd(f, &nargs)) {
        if (nargs > 16) return 2;
        uint32_t values[16], kinds[16], lens[16], ptr_arg[16], ptr_off[16];
        for (uint32_t i = 0; i < nargs; i++) {
            uint32_t kind, a, b;
            if (!rd(f, &kind) || !rd(f, &a)) return 2;
            kinds[i] = kind; lens[i] = 0;
            if (kind == 0) values[i] = a;
            else if (kind == 1) {
                values[i] = halopad_guest_alloc(a + 64, 16);
                lens[i] = a;
                if (fread(halopad_guest_ptr(values[i]), 1, a, f) != a) return 2;
            } else if (kind == 2) {
                if (!rd(f, &b)) return 2;
                ptr_arg[i] = a; ptr_off[i] = b;
            } else return 2;
        }
        for (uint32_t i = 0; i < nargs; i++)
            if (kinds[i] == 2) values[i] = values[ptr_arg[i]] + ptr_off[i];
        uint32_t nregs, reg_ids[8], reg_vals[8];
        if (!rd(f, &nregs) || nregs > 8) return 2;
        for (uint32_t r = 0; r < nregs; r++) {
            uint32_t kind, a, b = 0;
            if (!rd(f, &reg_ids[r]) || !rd(f, &kind) || !rd(f, &a)) return 2;
            if (kind == 2) { if (!rd(f, &b)) return 2; reg_vals[r] = values[a] + b; }
            else reg_vals[r] = a;
        }
        uint32_t sp = stack_top - 4 * nargs;
        for (uint32_t i = 0; i < nargs; i++) memcpy(halopad_guest_ptr(sp + 4 * i), &values[i], 4);
        _cpu state;
        memset(&state, 0, sizeof state);
        state._esp = sp;
        state._pointer_offset = halopad_guest_base;
        state._st_cw = fpcw;
        for (uint32_t r = 0; r < nregs; r++) {
            uint32_t *slot[8] = {&state._eax, &state._ecx, &state._edx, &state._ebx, NULL, &state._ebp, &state._esi, &state._edi};
            if (reg_ids[r] > 7 || !slot[reg_ids[r]]) return 2;
            *slot[reg_ids[r]] = reg_vals[r];
        }
        uint32_t entry = va;
        if (via_slot) memcpy(&entry, halopad_guest_ptr(va), 4);
        halopad_enter(&state, entry);
        printf("%08" PRIx32 " %" PRId32, state._eax, (int32_t)(state._esp - sp));
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
            for (uint32_t k = 0; k < lens[i]; k++) printf("%02x", ((uint8_t *)halopad_guest_ptr(values[i]))[k]);
        }
        putchar('\n');
        fflush(stdout);
    }
    return 0;
}

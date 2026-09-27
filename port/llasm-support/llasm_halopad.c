/* HaloPad additions to the llasm support (G3): cpuid under the plain-CPU contract,
 * rdtsc, and the counting part of repe/repne cmpsw/cmpsd.
 * Not part of SR's llasm support; same MIT terms as the files it sits beside. */
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "llasm_cpu.h"

/* Plain-CPU contract (docs/G2D-SIMD-SCOPE.md): GenuineIntel, family 6 model 3 stepping 3,
   x87 + TSC + CX8 + CMOV, no MMX/SSE/3DNow!. Any other leaf stops the program. */
EXTERNC void CCALL x86_cpuid(CPU)
{
    switch (eax)
    {
    case 0:
        eax = 1; ebx = 0x756E6547; edx = 0x49656E69; ecx = 0x6C65746E;   /* "GenuineIntel" */
        return;
    case 1:
        eax = 0x633; ebx = 0; ecx = 0; edx = (1u << 0) | (1u << 4) | (1u << 8) | (1u << 15);
        return;
    case 0x80000000:
        eax = 0x80000001; ebx = 0; ecx = 0; edx = 0;
        return;
    case 0x80000001:
        eax = 0x633; ebx = 0; ecx = 0; edx = 0;
        return;
    }
    fprintf(stderr, "HALOPAD TRAP: cpuid leaf 0x%x (sub-leaf 0x%x) is outside the plain-CPU contract\n", eax, ecx);
    abort();
}

/* Time-stamp counter: the reference machine's 2.4 GHz, from the host's monotonic clock
   (nanoseconds times 12/5). Halo measures it against the performance counter at start-up
   (0x580e70) and reads 2400 MHz: its Radeon 9700 PRO class, not the low-spec branch it takes at
   1000 MHz or less (default resolution, detail defaults: 0x51a2ad, 0x53d70a, 0x53e3c0). */
EXTERNC void CCALL x86_rdtsc(CPU)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    uint64_t ns = (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec;
    uint64_t v = ns / 5 * 12 + ns % 5 * 12 / 5;
    eax = (uint32_t)v;
    edx = (uint32_t)(v >> 32);
}

/* repe/repne cmpsw/cmpsd: called with ecx >= 1. Skips the elements the repeated compare
   would pass over, stopping before the last element it compares (the first mismatch for
   repe, the first match for repne, or the final element). The translated code then
   performs that last compare itself, which sets the flags exactly as x86 does. */
#define PREP(name, type, step, cond) \
EXTERNC void CCALL name(CPU) \
{ \
    int32_t dir = (eflags & DF) ? -(step) : (step); \
    while (ecx > 1 && (*(type *)REG2PTR(esi) cond *(type *)REG2PTR(edi))) \
    { \
        esi += dir; edi += dir; ecx--; \
    } \
}
PREP(x86_repe_cmpsw_prep, uint16_t, 2, ==)
PREP(x86_repne_cmpsw_prep, uint16_t, 2, !=)
PREP(x86_repe_cmpsd_prep, uint32_t, 4, ==)
PREP(x86_repne_cmpsd_prep, uint32_t, 4, !=)

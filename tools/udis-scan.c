/* Decode every audited instruction start with SRW's own udis86 1.7.2 and report
 * mnemonic counts and segment-prefixed forms, so SRW's llasm coverage can be
 * compared exactly. Usage: udis-scan <image.bin> <base> <instructions.u32> */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include "udis86.h"
#ifdef WITH_SRW_FLAGS
#include "udis86_dep.h"
#endif

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "usage\n"); return 2; }
    FILE *fi = fopen(argv[1], "rb"); FILE *fa = fopen(argv[3], "rb");
    if (!fi || !fa) { perror("open"); return 2; }
    fseek(fi, 0, SEEK_END); long n = ftell(fi); fseek(fi, 0, SEEK_SET);
    uint8_t *img = malloc(n); fread(img, 1, n, fi);
    uint32_t base = strtoul(argv[2], NULL, 0), addr;
    static long counts[UD_MAX_MNEMONIC_CODE];
    ud_t u; ud_init(&u); ud_set_mode(&u, 32); ud_set_syntax(&u, UD_SYN_INTEL);
#ifdef WITH_SRW_FLAGS
    init_udis86_dep();
#endif
    long total = 0, invalid = 0;
    while (fread(&addr, 4, 1, fa) == 1) {
        uint32_t o = addr - base;
        ud_set_input_buffer(&u, img + o, (size_t)(n - o) < 16 ? (size_t)(n - o) : 16);
        ud_set_pc(&u, addr);
        if (!ud_disassemble(&u)) continue;
        total++;
        if (u.mnemonic == UD_Iinvalid) { invalid++; printf("INVALID 0x%x\n", addr); continue; }
        counts[u.mnemonic]++;
        if (getenv("UDIS_SCAN_ALL"))
#ifdef WITH_SRW_FLAGS
            printf("I 0x%x %u %s %s 0x%lx 0x%lx\n", addr, ud_insn_len(&u), ud_lookup_mnemonic(u.mnemonic),
                   u.pfx_seg == UD_R_FS ? "fs" : (u.pfx_seg == UD_R_GS ? "gs" : "-"),
                   (unsigned long)flags_needed[u.mnemonic], (unsigned long)flags_modified[u.mnemonic]);
#else
            printf("I 0x%x %u %s %s\n", addr, ud_insn_len(&u), ud_lookup_mnemonic(u.mnemonic),
                   u.pfx_seg == UD_R_FS ? "fs" : (u.pfx_seg == UD_R_GS ? "gs" : "-"));
#endif
        if (u.pfx_seg == UD_R_FS || u.pfx_seg == UD_R_GS)
            printf("SEG 0x%x %s %s\n", addr, u.pfx_seg == UD_R_FS ? "fs" : "gs", ud_insn_asm(&u));
    }
    for (int i = 0; i < UD_MAX_MNEMONIC_CODE; i++)
        if (counts[i]) printf("MN %s %ld\n", ud_lookup_mnemonic(i), counts[i]);
    printf("TOTAL %ld INVALID %ld\n", total, invalid);
    return 0;
}

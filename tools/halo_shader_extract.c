/* Extract Halo's shader collections for analysis (G4 scoping): runs Halo's own file loader
 * and decryptor (0x51d0f0, translated) on shaders\vsh.enc and the three pixel shader
 * collections, and writes the decrypted data to generated/analysis/shaders/. Nothing
 * here is shipped; HaloPad's Metal layer receives shaders as bytecode at run time.
 * Linked in place of the core: scripts/run-core.py --main tools/halo_shader_extract.c */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE"), *root = getenv("HALOPAD_REPO_ROOT");
    if (!image || !root) return 2;
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    char outdir[1024];
    snprintf(outdir, sizeof outdir, "%s/generated/analysis/shaders", root);
    mkdir(outdir, 0755);
    const char *files[] = {"shaders\\vsh.enc", "shaders\\EffectCollection_ps_1_1.enc", "shaders\\EffectCollection_ps_1_4.enc",
                           "shaders\\EffectCollection_ps_2_0.enc"};
    int failed = 0;
    for (int i = 0; i < 4; i++) {
        uint32_t name = halopad_heap_alloc(64, 1);
        strcpy(halopad_guest_ptr(name), files[i]);
        uint32_t pbuf = halopad_heap_alloc(4, 1), psize = halopad_heap_alloc(4, 1);
        uint32_t args[2] = {pbuf, psize};
        uint32_t ok = halopad_call_guest_ex(0x51D0F0, 2, args, name, 0) & 0xFF;   /* ecx = file name; cdecl */
        uint32_t buf, size;
        memcpy(&buf, halopad_guest_ptr(pbuf), 4);
        memcpy(&size, halopad_guest_ptr(psize), 4);
        const char *base = strrchr(files[i], '\\') + 1;
        char path[1200];
        snprintf(path, sizeof path, "%s/%.*s.bin", outdir, (int)(strlen(base) - 4), base);
        if (!ok) { printf("%s: Halo's loader failed\n", files[i]); failed = 1; continue; }
        FILE *f = fopen(path, "wb");
        fwrite(halopad_guest_ptr(buf), 1, size, f);
        fclose(f);
        printf("%s: %u bytes -> %s\n", files[i], size, path);
    }
    return failed;
}

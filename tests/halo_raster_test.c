/* Rasterizer initialization test (G3): Halo's 0x51a240 on HaloPad, the graphics start-up that
 * follows the machine checks (WinMain -> 0x5442e0 -> 0x515610 -> 0x51a240): the game window
 * (0x5191d0), Direct3D and the adapter, the shader path, the presentation parameters
 * (0x519860), CreateDevice, the splash (0x519080), the shader effects and the rasterizer's
 * subsystems, and the chat windows (0x51cdb0).
 *
 * This is a component test, like the splash and chat tests: it sets only what 0x51a240 reads
 * from WinMain (the window class values of 0x5446a7-0x5446fe, the Direct3D library pointers
 * of 0x5449c7, the machine measurement 0x580e70, strings.dll and the Keystone loader
 * 0x545ee0). The core runner remains the only path through WinMain, and it stops at the
 * license and the product ID as Halo does.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_raster_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_dialog.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);
void *halopad_d3d9_device_target(uint32_t g);
void halopad_metal_read_image(void *p, uint32_t *out, uint32_t w, uint32_t h);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    fprintf(stderr, "HALOPAD TEST: %s\n", what);
    printf("%-72s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}

static int dialogs;
int halopad_host_dialog(const halopad_dialog_view *v)
{
    dialogs++;
    printf("    DIALOG \"%s\":", v->title);
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1006) printf(" %s", v->item[i].text);
    printf("\n");
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1005) return i;   /* Exit */
    return HPD_CLOSE;
}

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    halopad_protect_image(0x400000);
    halopad_modules_init();
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;

    /* the C runtime, as the entry point 0x5ccac7 starts it */
    uint32_t one = 1;
    halopad_call_guest_ex(0x5d6ba6, 1, &one, 0, 0);
    halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0);
    uint32_t k32 = LoadLibraryA_c(str("kernel32.dll")), user32 = LoadLibraryA_c(str("user32.dll"));
    wr(0x6bece8, halopad_call_guest(GetProcAddress_c(k32, str("GetCommandLineA")), 0, NULL));
    wr(0x63e2f4, halopad_call_guest_ex(0x5d6a6a, 0, NULL, 0, 0));
    halopad_call_guest_ex(0x5d69c8, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5d6795, 0, NULL, 0, 0);
    check("CRT started (_cinit)", halopad_call_guest_ex(0x5caa87, 1, &one, 0, 0), 0);

    /* what 0x51a240 reads from WinMain */
    wr(0x6bde88, LoadLibraryA_c(str("strings.dll")));             /* 0x582590 */
    wr(0x6e1480, 0x400000);                                       /* hInstance */
    wr(0x6e148c, 1);                                              /* nCmdShow: SW_SHOWNORMAL */
    wr(0x6e1490, 0x544f40);                                       /* the window procedure */
    strcpy(halopad_guest_ptr(0x6e1494), "Halo");                  /* class name */
    strcpy(halopad_guest_ptr(0x6e14d4), "Halo");                  /* title */
    uint32_t cur = 0x7f00;
    wr(0x67e57c, halopad_call_guest(GetProcAddress_c(user32, str("LoadCursorA")), 2, (uint32_t[]){0, cur}));
    halopad_call_guest_ex(0x580e70, 0, NULL, 0, 0);               /* memory, CPU, video memory */
    uint32_t d3d9 = LoadLibraryA_c(str("d3d9.dll")), create = GetProcAddress_c(d3d9, str("Direct3DCreate9")), sdk = 0x1f;
    wr(0x6e1524, d3d9);
    wr(0x6e1534, create);
    wr(0x6bd168, halopad_call_guest(create, 1, &sdk));
    halopad_call_guest_ex(0x545ee0, 0, NULL, 0, 0);               /* Keystone.dll and its entry points */
    check("Keystone loaded (0x545ee0)", rd(0x6bd16c) != 0, 1);

    /* Halo's rasterizer initialization */
    uint32_t ok = halopad_call_guest_ex(0x51a240, 0, NULL, 0, 0) & 0xFF;
    check("0x51a240 succeeded", ok, 1);
    check("  the game window (0x6e1484)", rd(0x6e1484) != 0, 1);
    uint32_t device = rd(0x6b840c);
    check("  the Direct3D device (0x6b840c)", device != 0, 1);
    check("  no warning or error dialog", (uint32_t)dialogs, 0);
    if (device) {
        uint32_t w = 0, h = 0;
        uint32_t bb = halopad_heap_alloc(4, 1), desc = halopad_heap_alloc(32, 1);
        uint32_t args[4] = {device, 0, 0, 0};
        (void)args;
        uint32_t a[5] = {device, 0, 0, 0 /* D3DBACKBUFFER_TYPE_MONO */, bb};
        halopad_call_guest(rd(rd(device) + 4 * 18), 5, a);                  /* GetBackBuffer */
        if (rd(bb)) {
            uint32_t d[2] = {rd(bb), desc};
            halopad_call_guest(rd(rd(rd(bb)) + 4 * 12), 2, d);              /* IDirect3DSurface9::GetDesc */
            w = rd(desc + 24); h = rd(desc + 28);
            halopad_call_guest(rd(rd(rd(bb)) + 4 * 2), 1, (uint32_t[]){rd(bb)});   /* Release */
        }
        printf("    back buffer %ux%u\n", w, h);
        check("  the reference machine's default resolution, 800x600", w << 16 | h, 800u << 16 | 600u);
        uint32_t *img = malloc(w * h * 4);
        halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
        const char *reg = getenv("HALOPAD_REGISTRY");
        if (reg && strrchr(reg, '/')) {
            char path[1200];
            snprintf(path, sizeof path, "%.*s/raster.ppm", (int)(strrchr(reg, '/') - reg), reg);
            FILE *f = fopen(path, "wb");
            if (f) {
                fprintf(f, "P6\n%u %u\n255\n", w, h);
                for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
                fclose(f);
            }
        }
        free(img);
    }
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

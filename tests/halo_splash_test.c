/* Direct3D splash test (G3): Halo's own splash, 0x519080, on a real device. Right after the
 * device is created, Halo calls it with eax = 1: CreateOffscreenPlainSurface(640x480
 * X8R8G8B8), the statically linked D3DX's D3DXLoadSurfaceFromResourceA (0x582dfc) for bitmap
 * 0x86 in strings.dll (the handle Halo keeps at 0x6bde88), GetRenderTarget, StretchRect onto
 * the back buffer, its present wrapper 0x51ba30 (Present, once), and the StretchRect again so
 * the next back buffer holds the picture too. With eax = 0 it clears both instead. The test decodes the
 * bitmap itself from strings.dll's resource directory (independently of the runtime's
 * resource services and of D3DX) and compares every pixel of the back buffer with it.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_splash_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

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
static uint16_t rd16(uint32_t g) { uint16_t v; memcpy(&v, halopad_guest_ptr(g), 2); return v; }
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    fprintf(stderr, "HALOPAD TEST: %s\n", what);
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}

/* One entry of a PE resource directory level, by integer id (the first entry when id is 0). */
static uint32_t res_entry(uint32_t root, uint32_t dir, uint32_t id)
{
    uint32_t n = rd16(dir + 12) + rd16(dir + 14);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t e = dir + 16 + 8 * i, name = rd(e), off = rd(e + 4);
        if (!(name & 0x80000000u) && (id == 0 || name == id)) return root + (off & 0x7FFFFFFFu);
    }
    return 0;
}

/* Halo's splash 0x519080 takes its case in eax (0 black, 1 the splash bitmap); nothing between
   here and its entry changes eax. */
static void splash(_cpu *cpu, uint32_t which) { cpu->_eax = which; halopad_call_guest_ex(0x519080, 0, NULL, 0, 0); }

static void save(const char *name, const uint32_t *img)
{
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (!reg || !strrchr(reg, '/')) return;
    char path[1200];
    snprintf(path, sizeof path, "%.*s/%s", (int)(strrchr(reg, '/') - reg), reg, name);
    FILE *f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n640 480\n255\n");
    for (int i = 0; i < 640 * 480; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
    fclose(f);
    fprintf(stderr, "HALOPAD TEST: frame saved to %s\n", path);
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

    /* Halo's C runtime start-up up to its I/O set-up (D3DX allocates through the CRT heap) */
    uint32_t crt_arg = 1;
    check("CRT _heap_init (0x5d6ba6)", halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0) != 0, 1);
    check("CRT _mtinit (0x5cf966)", halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0) != 0, 1);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    check("CRT _ioinit (0x5cf3f2)", (int32_t)halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0) >= 0, 1);

    /* window and device as in tests/halo_d3d9_test.c */
    uint32_t d3d9 = LoadLibraryA_c(str("d3d9.dll")), sdk = 0x1f;
    uint32_t d3d = halopad_call_guest(GetProcAddress_c(d3d9, str("Direct3DCreate9")), 1, &sdk);
    uint32_t user32 = LoadLibraryA_c(str("user32.dll"));
    uint32_t wc = halopad_heap_alloc(48, 1), cls = str("HaloPadSplash");
    uint32_t wcv[12] = {48, 0, GetProcAddress_c(user32, str("DefWindowProcA")), 0, 0, 0x400000, 0, 0, 0, 0, cls, 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    halopad_call_guest(GetProcAddress_c(user32, str("RegisterClassExA")), 1, &wc);
    uint32_t cw[12] = {0, cls, str("HaloPad splash test"), 0x00CF0000, 100, 100, 640 + 8, 480 + 27, 0, 0, 0x400000, 0};
    uint32_t hwnd = halopad_call_guest(GetProcAddress_c(user32, str("CreateWindowExA")), 12, cw);
    uint32_t pp = halopad_heap_alloc(56, 1);
    uint32_t ppv[14] = {0, 0, 0, 1, 0, 0, 1, hwnd, 1, 1, 75, 0, 0, 0x80000000};
    memcpy(halopad_guest_ptr(pp), ppv, 56);
    uint32_t pdev = halopad_heap_alloc(4, 1);
    check("device (640x480 windowed)", method(d3d, 16, 6, (uint32_t[]){0, 1, hwnd, 0x40, pp, pdev}), 0);
    uint32_t device = rd(pdev);
    cpu._st_cw = 0x027F;

    /* what Halo's start-up leaves behind: the device and strings.dll (0x582590) */
    uint32_t strings = LoadLibraryA_c(str("strings.dll"));
    check("LoadLibraryA(strings.dll)", strings, 0x3F800000);
    wr(0x6b840c, device);
    wr(0x6bde88, strings);

    /* the reference: bitmap 0x86 from strings.dll's resource directory, read here directly */
    uint32_t rsrc = strings + rd(strings + rd(strings + 0x3c) + 0x88);          /* data directory 2 */
    uint32_t l1 = res_entry(rsrc, rsrc, 2), l2 = l1 ? res_entry(rsrc, l1, 0x86) : 0, l3 = l2 ? res_entry(rsrc, l2, 0) : 0;
    check("strings.dll has bitmap 0x86", l3 != 0, 1);
    if (!l3) { printf("FAIL: %d failure(s)\n", failures + 1); return 1; }
    uint32_t bmi = strings + rd(l3);
    check("  a 640x480 24-bit bitmap", rd(bmi + 4) == 640 && rd(bmi + 8) == 480 && rd16(bmi + 14) == 24, 1);
    uint32_t *want = malloc(640 * 480 * 4);
    const uint8_t *bits = halopad_guest_ptr(bmi + rd(bmi));
    for (int y = 0; y < 480; y++)                                                 /* bottom-up rows, BGR */
        for (int x = 0; x < 640; x++) {
            const uint8_t *p = bits + (479 - y) * 640 * 3 + x * 3;
            want[y * 640 + x] = (uint32_t)p[2] << 16 | (uint32_t)p[1] << 8 | p[0];
        }

    /* Halo's splash, from translated code */
    uint32_t frames0 = rd(0x637d00);
    splash(&cpu, 1);
    check("the splash presented once (Halo's frame counter 0x637d00)", rd(0x637d00) - frames0, 1);
    check("  and did not mark the device lost (0x75c408)", *(uint8_t *)halopad_guest_ptr(0x75c408), 0);
    uint32_t *img = malloc(640 * 480 * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, 640, 480);
    save("splash.ppm", img);
    uint32_t diff = 0, first = ~0u;
    for (int i = 0; i < 640 * 480; i++)
        if ((img[i] & 0xFFFFFF) != want[i]) { if (first == ~0u) first = (uint32_t)i; diff++; }
    if (diff) printf("    first difference at %u,%u: got %06x, want %06x\n", first % 640, first / 640, img[first] & 0xFFFFFF, want[first]);
    check("every back-buffer pixel equals bitmap 0x86", diff, 0);

    /* the other case: both buffers cleared to black */
    frames0 = rd(0x637d00);
    splash(&cpu, 0);
    check("case 0 presented once", rd(0x637d00) - frames0, 1);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, 640, 480);
    uint32_t lit = 0;
    for (int i = 0; i < 640 * 480; i++) lit += (img[i] & 0xFFFFFF) != 0;
    check("  and the back buffer is black", lit, 0);
    free(img);
    free(want);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

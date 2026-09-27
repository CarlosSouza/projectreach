/* Keystone chat UI test (G3): Halo's own chat set-up running on a real device. The test sets
 * the globals Halo's loader (0x545ee0) leaves behind: the Keystone module handle, the 17
 * function pointers it resolves, the game window, the Direct3D device, the current directory
 * as a wide string and the screen size. It then runs Halo's 0x51cdb0 from translated code:
 * KeystoneCreate(hwnd, device, directory) and KsCreateWindow for KeystoneEditbox and
 * KeystoneChatLog from content/480editbox.ksml and content/480log.ksml, all inside the
 * translated Keystone.dll. Then it draws frames with KsUpdate between BeginScene and EndScene
 * as Halo does (0x51b923).
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_keystone_ui_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t wstr(const char *s)
{
    uint32_t n = (uint32_t)strlen(s), g = halopad_heap_alloc(2 * n + 2, 1);
    for (uint32_t i = 0; i < n; i++) ((uint16_t *)halopad_guest_ptr(g))[i] = (uint8_t)s[i];
    return g;
}
static void check(const char *what, uint32_t got, uint32_t want)
{
    fprintf(stderr, "HALOPAD TEST: %s\n", what);   /* orders the checks among runtime traces */
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}
/* Keystone's Call_* exports are cdecl: the caller pops */
static uint32_t ks(uint32_t global, uint32_t n, const uint32_t *args) { return halopad_call_guest_ex(rd(global), n, args, 0, 0); }
#define KS(global, ...) ks(global, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})

/* Halo's globals (0x545f9d..0x5460ed, 0x51cdb0) */
enum { G_MODULE = 0x6bd16c, G_CREATE = 0x6bd170, G_KS = 0x6bd174, G_DIR = 0x6bd178, G_KSRELEASE = 0x6bd17c,
       G_XLATE = 0x6bd180, G_CREATEWINDOW = 0x6bd184, G_GETWINDOW = 0x6bd188, G_UPDATE = 0x6bd18c,
       G_DISPATCH = 0x6bd190, G_SETFOCUSWINDOW = 0x6bd194, G_KWRELEASE = 0x6bd198, G_GETCONTROL = 0x6bd19c,
       G_RELAYOUT = 0x6bd1a0, G_SETFOCUSCONTROL = 0x6bd1a4, G_ADDDIRTY = 0x6bd1a8, G_SHOW = 0x6bd1ac,
       G_GETATTR = 0x6bd1b0, G_SETATTR = 0x6bd1b4, G_SENDMSG = 0x6bd1b8,
       G_HWND = 0x6e1484, G_DEVICE = 0x6b840c, G_WIDTH = 0x75b760, G_HEIGHT = 0x75b764,
       G_EDITBOX_NAME = 0x637d50, G_LOG_NAME = 0x637d54 };

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

    /* Halo's C runtime start-up up to its I/O set-up, as its entry point 0x5ccac7 runs it
       before WinMain: the chat set-up reaches CRT code that needs the heap and per-thread data */
    uint32_t crt_arg = 1;
    check("CRT _heap_init (0x5d6ba6)", halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0) != 0, 1);
    check("CRT _mtinit (0x5cf966)", halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0) != 0, 1);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    check("CRT _ioinit (0x5cf3f2)", (int32_t)halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0) >= 0, 1);

    /* window and device as in tests/halo_d3d9_test.c */
    uint32_t d3d9 = LoadLibraryA_c(str("d3d9.dll")), sdk = 0x1f;
    uint32_t d3d = halopad_call_guest(GetProcAddress_c(d3d9, str("Direct3DCreate9")), 1, &sdk);
    uint32_t user32 = LoadLibraryA_c(str("user32.dll"));
    uint32_t wc = halopad_heap_alloc(48, 1), cls = str("HaloPadKeystone");
    uint32_t wcv[12] = {48, 0, GetProcAddress_c(user32, str("DefWindowProcA")), 0, 0, 0x400000, 0, 0, 0, 0, cls, 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    halopad_call_guest(GetProcAddress_c(user32, str("RegisterClassExA")), 1, &wc);
    uint32_t cw[12] = {0, cls, str("HaloPad Keystone test"), 0x00CF0000, 100, 100, 640 + 8, 480 + 27, 0, 0, 0x400000, 0};
    uint32_t hwnd = halopad_call_guest(GetProcAddress_c(user32, str("CreateWindowExA")), 12, cw);
    uint32_t pp = halopad_heap_alloc(56, 1);
    uint32_t ppv[14] = {0, 0, 0, 1, 0, 0, 1, hwnd, 1, 1, 75, 0, 0, 0x80000000};
    memcpy(halopad_guest_ptr(pp), ppv, 56);
    uint32_t pdev = halopad_heap_alloc(4, 1);
    check("device (640x480 windowed)", method(d3d, 16, 6, (uint32_t[]){0, 1, hwnd, 0x40, pp, pdev}), 0);
    uint32_t device = rd(pdev);
    cpu._st_cw = 0x027F;

    /* what Halo's 0x545ee0 leaves behind */
    uint32_t h = LoadLibraryA_c(str("keystone.dll"));
    check("keystone.dll loaded", h, 0x10200000);
    static const struct { uint32_t global; const char *name; } fns[] = {
        {G_CREATE, "KeystoneCreate"}, {G_XLATE, "Call_KsTranslateAccelerator"}, {G_CREATEWINDOW, "Call_KsCreateWindow"},
        {G_GETWINDOW, "Call_KsGetWindow"}, {G_UPDATE, "Call_KsUpdate"}, {G_DISPATCH, "Call_KsDispatchMessage"},
        {G_KWRELEASE, "Call_KW_Release"}, {G_SETFOCUSWINDOW, "Call_KsSetFocusWindow"}, {G_GETCONTROL, "Call_KW_GetControlByID"},
        {G_GETATTR, "Call_KC_GetAttribute"}, {G_SETATTR, "Call_KC_SetAttribute"}, {G_SENDMSG, "Call_KC_SendMessage"},
        {G_RELAYOUT, "Call_KW_ReLayout"}, {G_SETFOCUSCONTROL, "Call_KW_SetFocusControl"}, {G_ADDDIRTY, "Call_KW_AddDirtyControl"},
        {G_KSRELEASE, "Call_KsRelease"}, {G_SHOW, "Call_KW_ShowWindow"}};
    wr(G_MODULE, h);
    for (unsigned i = 0; i < sizeof fns / sizeof fns[0]; i++) wr(fns[i].global, GetProcAddress_c(h, str(fns[i].name)));
    wr(G_DIR, wstr("C:\\Program Files\\Microsoft Games\\Halo Custom Edition"));
    wr(G_HWND, hwnd);
    wr(G_DEVICE, device);
    wr(G_WIDTH, 640);
    wr(G_HEIGHT, 480);

    /* Halo's chat set-up, from translated code */
    halopad_call_guest(0x51cdb0, 0, NULL);
    uint32_t k = rd(G_KS);
    check("KeystoneCreate gave Halo a Keystone object", k != 0, 1);
    uint32_t ew = KS(G_GETWINDOW, k, rd(G_EDITBOX_NAME));
    check("KsGetWindow(\"KeystoneEditbox\")", ew != 0, 1);
    uint32_t lw = KS(G_GETWINDOW, k, rd(G_LOG_NAME));
    check("KsGetWindow(\"KeystoneChatLog\")", lw != 0, 1);

    /* Frames as Halo draws them (0x51b923), with its message pump. Keystone reads each .ksml
       asynchronously (ReadFileEx) and lays the window out once MSXML has parsed and validated it,
       so the controls appear after a few frames. */
    float one = 1.0f; uint32_t onebits; memcpy(&onebits, &one, 4);
    uint32_t peek = GetProcAddress_c(user32, str("PeekMessageA")), translate = GetProcAddress_c(user32, str("TranslateMessage"));
    uint32_t dispatch = GetProcAddress_c(user32, str("DispatchMessageA")), msg = halopad_heap_alloc(28, 1);
    uint32_t edit = 0, list = 0, frames = 0, bad = 0;
    uint32_t oe = wstr("oEditbox"), ol = wstr("oListbox");
    for (; frames < 600 && !(edit && list); frames++) {
        while (halopad_call_guest(peek, 5, (uint32_t[]){msg, 0, 0, 0, 1})) {
            halopad_call_guest(translate, 1, &msg);
            halopad_call_guest(dispatch, 1, &msg);
        }
        bad += method(device, 41, 0, NULL) != 0;                                         /* BeginScene */
        method(device, 43, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});             /* Clear */
        bad += (int32_t)KS(G_UPDATE, k) < 0;
        bad += method(device, 42, 0, NULL) != 0;                                         /* EndScene */
        bad += method(device, 17, 4, (uint32_t[]){0, 0, 0, 0}) != 0;                     /* Present */
        usleep(5000);
        edit = KS(G_GETCONTROL, ew, oe);
        list = KS(G_GETCONTROL, lw, ol);
    }
    fprintf(stderr, "HALOPAD TEST: %u frames drawn\n", frames);
    check("frames: BeginScene, KsUpdate, EndScene and Present all succeed", bad, 0);
    check("  edit box control oEditbox (content/480editbox.ksml laid out)", edit != 0, 1);
    check("  list box control oListbox (content/480log.ksml laid out)", list != 0, 1);

    /* a chat line, added the way Halo adds one (0x4ae8a0: LB_ADDSTRING to oListbox, scroll, relayout),
       then frames; the frame is saved next to the evidence as chat.ppm */
    wr(0x647828, 10000000); wr(0x64782c, 0);                     /* Halo's QueryPerformanceFrequency copy (start-up sets it) */
    uint32_t line = wstr("HaloPad: chat through translated Keystone, MSXML 4 and GDI on CoreText");
    halopad_call_guest_ex(0x4ae8a0, 1, &line, 0, 0);
    /* and the chat input open, as Halo opens it (0x4ada50): the prompt's and the edit box's text,
       then KW_ShowWindow(SW_SHOW) on the edit box window */
    uint32_t prompt = KS(G_GETCONTROL, ew, wstr("oPrompt"));
    check("  prompt label oPrompt", prompt != 0, 1);
    KS(G_SETATTR, prompt, wstr("text"), wstr("Say:"));
    KS(G_SETATTR, edit, wstr("text"), wstr("typed on a Mac"));
    KS(G_SHOW, ew, 5);
    for (int i = 0; i < 4; i++) {
        method(device, 41, 0, NULL);
        method(device, 43, 6, (uint32_t[]){0, 0, 3, 0xFF203040, onebits, 0});
        KS(G_UPDATE, k);
        method(device, 42, 0, NULL);
        if (i < 3) method(device, 17, 4, (uint32_t[]){0, 0, 0, 0});
    }
    uint32_t *img = malloc(640 * 480 * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, 640, 480);
    uint32_t lit = 0, lit_edit = 0;
    for (int y = 260; y < 390; y++) for (int x = 5; x < 640; x++) if ((img[y * 640 + x] & 0xFF) > 0xC0 && (img[y * 640 + x] >> 16 & 0xFF) > 0xC0) lit++;
    for (int y = 450; y < 480; y++) for (int x = 0; x < 640; x++) if ((img[y * 640 + x] & 0xFF) > 0xC0 && (img[y * 640 + x] >> 16 & 0xFF) > 0xC0) lit_edit++;
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (reg) {
        char path[1200];
        snprintf(path, sizeof path, "%.*s/chat.ppm", (int)(strrchr(reg, '/') - reg), reg);
        FILE *f = fopen(path, "wb");
        if (f) {
            fprintf(f, "P6\n640 480\n255\n");
            for (int i = 0; i < 640 * 480; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
            fclose(f);
            fprintf(stderr, "HALOPAD TEST: frame saved to %s\n", path);
        }
    }
    check("the chat line is drawn in the log area (white text pixels)", lit > 100, 1);
    check("the prompt and the typed text are drawn along the bottom (the edit box image is fully transparent)", lit_edit > 50, 1);
    free(img);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

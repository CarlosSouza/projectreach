/* iPadOS/iOS development scene (G8/G9, never shipped): Halo from its main menu, played in the
 * app with the touch controls and the three-dot menu (port/ios/HaloPadOverlay.m).
 *
 * Linked by scripts/build-ios-app.py --scene tests/halo_app_scene.c in place of the core's start
 * (halopad_app_entry). It prepares what WinMain has prepared by the time it calls main, as the
 * component tests do (tests/halo_host_test.c, tests/halo_connect_test.c): the C runtime, the
 * command line (HALOPAD_ARGS, e.g. "-connect 203.0.113.5:2302"), hInstance and strings.dll,
 * Halo's memory, Keystone, the DirectX libraries, the key string from Halo's own 0x5829e0 (empty
 * without the DigitalProductID its installer writes; nothing is written or made up) and
 * GameSpy's set-up; then the game's systems (0x5442e0) and Halo's main (0x4ca9c0), which runs its
 * frame loop until the player quits. Nothing is pressed for the player. */
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

static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }

int halopad_app_entry(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) { fprintf(stderr, "HALOPAD SCENE: no HALOPAD_IMAGE\n"); return 2; }
    if (!getenv("HALOPAD_ARGS")) setenv("HALOPAD_ARGS", "", 1);
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    halopad_protect_image(0x400000);
    halopad_modules_init();
    static _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;

    uint32_t one = 1;
    halopad_call_guest_ex(0x5d6ba6, 1, &one, 0, 0);               /* _heap_init */
    halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0);               /* _mtinit */
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0);               /* _ioinit */
    uint32_t k32 = LoadLibraryA_c(str("kernel32.dll"));
    wr(0x6bece8, halopad_call_guest(GetProcAddress_c(k32, str("GetCommandLineA")), 0, NULL));
    wr(0x63e2f4, halopad_call_guest_ex(0x5d6a6a, 0, NULL, 0, 0));
    halopad_call_guest_ex(0x5d69c8, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5d6795, 0, NULL, 0, 0);
    if (halopad_call_guest_ex(0x5caa87, 1, &one, 0, 0)) { fprintf(stderr, "HALOPAD SCENE: _cinit failed\n"); return 3; }
    {   /* WinMain's arguments (0x5447ad): 0x545a00 splits the command line after the program name */
        const char *cl = (const char *)halopad_guest_ptr(rd(0x6bece8));
        const char *tail = strstr(cl, "\" ") ? strstr(cl, "\" ") + 2 : "";
        uint32_t edi0 = cpu._edi, count = 0x6bd164;
        cpu._edi = str(tail);
        wr(0x6bd160, halopad_call_guest_ex(0x545a00, 1, &count, 0, 0));
        cpu._edi = edi0;
        fprintf(stderr, "HALOPAD SCENE: command line \"%s\"\n", tail);
    }
    uint32_t user32 = LoadLibraryA_c(str("user32.dll"));
    wr(0x6bde88, LoadLibraryA_c(str("strings.dll")));
    wr(0x6e1480, 0x400000);
    wr(0x6e148c, 1);
    wr(0x6e1490, 0x544f40);
    strcpy(halopad_guest_ptr(0x6e1494), "Halo");
    strcpy(halopad_guest_ptr(0x6e14d4), "Halo");
    wr(0x67e57c, halopad_call_guest(GetProcAddress_c(user32, str("LoadCursorA")), 2, (uint32_t[]){0, 0x7f00}));
    halopad_call_guest_ex(0x580e70, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x445230, 0, NULL, 0, 0);               /* Halo's memory set-up (WinMain 0x5449c2) */
    halopad_call_guest_ex(0x545ee0, 0, NULL, 0, 0);               /* Keystone */
    static const struct { const char *dll, *fn; uint32_t module, entry; } libs[] = {
        {"d3d9.dll", "Direct3DCreate9", 0x6e1524, 0x6e1534}, {"dsound.dll", "DirectSoundCreate8", 0x6e151c, 0x6e1530},
        {"dinput8.dll", "DirectInput8Create", 0x6e1518, 0x6e1528}, {"shfolder.dll", "SHGetFolderPathA", 0x6e1520, 0x6e152c}};
    for (unsigned i = 0; i < 4; i++) {
        uint32_t h = LoadLibraryA_c(str(libs[i].dll));
        wr(libs[i].module, h);
        wr(libs[i].entry, GetProcAddress_c(h, str(libs[i].fn)));
    }
    /* WinMain 0x544c21: the key string from Halo's own 0x5829e0 (nothing written or made up) */
    wr(0x6e1468, halopad_call_guest_ex(0x5829e0, 0, NULL, 0, 0));
    {   /* WinMain 0x544d16: GameSpy's set-up (0x5797e0), the key read from WinMain's own code at 0x544d27 */
        uint32_t keybuf = halopad_heap_alloc(8, 1);
        for (uint32_t k = 0; k < 6; k++) ((uint8_t *)halopad_guest_ptr(keybuf))[k] = *(uint8_t *)halopad_guest_ptr(0x544d27 + 4 * k + 3);
        uint32_t port = rd(0x6337f8), eax0 = cpu._eax, esi0 = cpu._esi, edi0 = cpu._edi;
        cpu._eax = 0x610654; cpu._esi = keybuf; cpu._edi = 0;
        halopad_call_guest_ex(0x5797e0, 1, &port, 0, 0);
        cpu._eax = eax0; cpu._esi = esi0; cpu._edi = edi0;
    }
    uint32_t sdk = 0x1f;
    wr(0x6bd168, halopad_call_guest(rd(0x6e1534), 1, &sdk));     /* IDirect3D9 (0x544a2f) */
    if (!(halopad_call_guest_ex(0x5442e0, 0, NULL, 0, 0) & 0xFF)) { fprintf(stderr, "HALOPAD SCENE: the game's systems did not start\n"); return 4; }
    fprintf(stderr, "HALOPAD SCENE: Halo's main\n");
    halopad_call_guest_ex(0x4ca9c0, 0, NULL, 0, 0);
    fprintf(stderr, "HALOPAD SCENE: main returned\n");
    return 0;
}

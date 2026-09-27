/* Game start test (G3/G4): Halo's main (0x4ca9c0) up to its frame loop, then the main menu.
 *
 * After the systems start (0x5442e0), main initializes the game's systems (0x4c96d0,
 * 0x4dad80, 0x4ad8f0, 0x45b330, 0x4980e0, 0x4e6730, 0x4c9790, 0x4cd1d0, 0x4cd330, 0x4cd080),
 * plays the intro movies unless the command line turns them off (0x43ed20), and enters its
 * frame loop (0x4cab41), which loads the main menu (0x4cbc90: levels\ui\ui through 0x45b810,
 * 0x45b920 and 0x4cc960). This component test does the same with the movies off (main's own
 * -novideo branch), loads the main menu with 0x4cbc90, and records any dialog. The core runner
 * remains the only path through WinMain, gated by the license and the product ID.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_game_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
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
extern int halopad_audio_manual;

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
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1005) return i;
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
    halopad_audio_manual = 1;                                     /* no audio device in tests */

    uint32_t one = 1;
    halopad_call_guest_ex(0x5d6ba6, 1, &one, 0, 0);
    halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0);
    uint32_t k32 = LoadLibraryA_c(str("kernel32.dll"));
    wr(0x6bece8, halopad_call_guest(GetProcAddress_c(k32, str("GetCommandLineA")), 0, NULL));
    wr(0x63e2f4, halopad_call_guest_ex(0x5d6a6a, 0, NULL, 0, 0));
    halopad_call_guest_ex(0x5d69c8, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5d6795, 0, NULL, 0, 0);
    check("CRT started (_cinit)", halopad_call_guest_ex(0x5caa87, 1, &one, 0, 0), 0);
    /* what WinMain has set up by 0x5442e0 (see tests/halo_raster_test.c) */
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
    /* the libraries WinMain's checks load (0x5449c7-0x544b20), module and entry point */
    static const struct { const char *dll, *fn; uint32_t module, entry; } libs[] = {
        {"d3d9.dll", "Direct3DCreate9", 0x6e1524, 0x6e1534}, {"dsound.dll", "DirectSoundCreate8", 0x6e151c, 0x6e1530},
        {"dinput8.dll", "DirectInput8Create", 0x6e1518, 0x6e1528}, {"shfolder.dll", "SHGetFolderPathA", 0x6e1520, 0x6e152c}};
    for (unsigned i = 0; i < 4; i++) {
        uint32_t h = LoadLibraryA_c(str(libs[i].dll));
        wr(libs[i].module, h);
        wr(libs[i].entry, GetProcAddress_c(h, str(libs[i].fn)));
    }
    uint32_t sdk = 0x1f;
    wr(0x6bd168, halopad_call_guest(rd(0x6e1534), 1, &sdk));     /* IDirect3D9 (0x544a2f) */

    /* the game's systems: resource maps, graphics, input, sound (0x5442e0 loads the DirectX
       libraries itself) */
    check("the game's systems start (0x5442e0)", halopad_call_guest_ex(0x5442e0, 0, NULL, 0, 0) & 0xFF, 1);
    check("  resource maps, graphics, input and sound without a dialog", (uint32_t)dialogs, 0);
    check("  the texture and sound caches exist (0x647468, 0x647458)", rd(0x647468) && rd(0x647458), 1);


    /* main (0x4ca9c0) up to its frame loop */
    wr(0x6b47e7 & ~3u, rd(0x6b47e7 & ~3u));                       /* (touch: the globals below are bytes) */
    *(uint8_t *)halopad_guest_ptr(0x6b4908) = 0;
    *(uint8_t *)halopad_guest_ptr(0x6b47e7) = 1;
    *(uint16_t *)halopad_guest_ptr(0x6b47e4) = 0xffff;
    *(uint8_t *)halopad_guest_ptr(0x6b47f9) = 1;
    strcpy(halopad_guest_ptr(0x6b4809), "levels\\b30\\b30");
    *(uint8_t *)halopad_guest_ptr(0x6b47a9) = 0;
    static const uint32_t inits[] = {0x4c96d0, 0x4dad80, 0x4ad8f0, 0x45b330, 0x4980e0};
    for (unsigned i = 0; i < 5; i++) halopad_call_guest_ex(inits[i], 0, NULL, 0, 0);
    snprintf(halopad_guest_ptr(0x6b7398), 260, "%s\\banned.txt", (const char *)halopad_guest_ptr(0x647830));
    wr(0x6534cc, 0x44); wr(0x6534d0, 0); wr(0x6534d4, 0);
    halopad_call_guest_ex(0x4e6730, 0, NULL, 0, 0);
    wr(0x6534d8, 8); wr(0x6534dc, 0); wr(0x6534e0, 0);
    static const uint32_t inits2[] = {0x4c9790, 0x4cd1d0, 0x4cd330, 0x4cd080};
    for (unsigned i = 0; i < 4; i++) halopad_call_guest_ex(inits2[i], 0, NULL, 0, 0);
    check("main's initialization up to the frame loop, without a dialog", (uint32_t)dialogs, 0);

    /* the main menu (0x4cbc90) */
    halopad_call_guest_ex(0x4cbc90, 0, NULL, 0, 0);
    const char *name = (const char *)halopad_guest_ptr(0x643064);
    printf("    loaded map \"%s\", scenario tag 0x%08x\n", name, rd(rd(0x643844) + 4));
    check("the main menu's map is loaded (0x4cbc90)", !strcmp(name, "ui"), 1);
    check("no dialog", (uint32_t)dialogs, 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

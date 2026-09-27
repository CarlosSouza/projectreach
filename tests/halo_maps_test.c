/* Map loading test (G3/G4): Halo's cache-file system on HaloPad, as a component.
 *
 * Custom Edition keeps its shared resources in maps\bitmaps.map, maps\sounds.map and
 * maps\loc.map, opened at start-up by 0x442ff0 (inside 0x5442e0). A scenario is opened with
 * 0x443c50(path in eax, fatal flag): its header is read through Halo's I/O thread and
 * validated into a cache slot (0x644318, 0x80c bytes each; 0x443ed0). 0x442290 then loads it:
 * the header into the current map's (0x643044), the tag data into tag memory, the tag index
 * (0x817144), and every tag's references fixed up. The test brings the game's systems up
 * first with 0x5442e0 (resource maps, graphics, input, sound: the caches the load uses), then
 * loads the main menu (levels\ui\ui) and Blood Gulch, and records any dialog.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_maps_test.c. */
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

/* 0x443c50 (open, header into a cache slot), then 0x442290 (the tags into tag memory), which
   returns the scenario tag's datum handle, or -1 */
static uint32_t open_map(_cpu *cpu, const char *path)
{
    uint32_t p = str(path), fatal = 1;
    cpu->_eax = p;
    uint32_t ok = halopad_call_guest_ex(0x443c50, 1, &fatal, 0, 0) & 0xFF;
    if (!ok) return 0xFFFFFFFFu;
    cpu->_eax = p;
    return halopad_call_guest_ex(0x442290, 0, NULL, 0, 0);
}

/* The loaded map: header 0x643044 (version, name), tag index 0x817144 (32-byte entries: class
   at +0, id at +0xc, name at +0x10), and the scenario tag the load returned */
static void describe(const char *label, const char *want_name, uint32_t scenario, const char *want_tag)
{
    const char *name = (const char *)halopad_guest_ptr(0x643064);
    uint32_t index = rd(0x817144), count = rd(rd(0x643844) + 0xc);
    printf("    %s: header version %u, name \"%s\", tag data %u bytes at 0x%08x, %u tags, scenario 0x%08x\n", label, rd(0x643048), name,
           rd(0x643058), rd(0x643844), count, scenario);
    char what[160];
    snprintf(what, sizeof what, "  %s: the header Halo loaded is the map's (\"%s\", version 609)", label, want_name);
    check(what, !strcmp(name, want_name) && rd(0x643048) == 0x261, 1);
    snprintf(what, sizeof what, "  %s: the tag index sits in Halo's tag memory", label);
    check(what, index >= rd(0x643844) && index < rd(0x643844) + rd(0x643058), 1);
    uint32_t e = index + (scenario & 0xFFFF) * 32;
    const char *tag = rd(e + 0x10) ? (const char *)halopad_guest_ptr(rd(e + 0x10)) : "";
    printf("    scenario tag: class %c%c%c%c, id 0x%08x, \"%s\"\n", (int)(rd(e) >> 24), (int)(rd(e) >> 16 & 0xFF), (int)(rd(e) >> 8 & 0xFF),
           (int)(rd(e) & 0xFF), rd(e + 0xc), tag);
    snprintf(what, sizeof what, "  %s: the load returns its scenario tag, scnr \"%s\"", label, want_tag);
    check(what, scenario != 0xFFFFFFFFu && rd(e) == 0x73636e72u && rd(e + 0xc) == scenario && !strcmp(tag, want_tag), 1);
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

    uint32_t sc = open_map(&cpu, "levels\\ui\\ui");
    check("the main menu (maps\\ui.map) loads (0x442290)", sc != 0xFFFFFFFFu, 1);
    describe("ui", "ui", sc, "levels\\ui\\ui");
    sc = open_map(&cpu, "levels\\test\\bloodgulch\\bloodgulch");
    check("Blood Gulch (maps\\bloodgulch.map) loads", sc != 0xFFFFFFFFu, 1);
    describe("bloodgulch", "bloodgulch", sc, "levels\\test\\bloodgulch\\bloodgulch");
    check("no dialog", (uint32_t)dialogs, 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

/* Vehicle test (G4): a player walks to Blood Gulch's Warthog, gets in, drives and gets out.
 *
 * Blood Gulch is loaded through Halo's start-up script as in tests/halo_play_test.c. The test steers
 * with the mouse and W: out of the red base, then to the driver's side of the Warthog that spawns
 * outside it (vehicles\warthog\mp_warthog near (102.3, -144.7)). Checks in Halo's game state:
 *  - up close, Halo offers the Warthog as the player's interaction (player +0x24 the vehicle,
 *    +0x28 the interaction type, 8 = enter a seat) -- the "Hold E" prompt;
 *  - E puts the player in the driver's seat (the unit's parent, +0x11c, becomes the Warthog);
 *  - W drives the Warthog more than 4 units, and the player is still in it when it stops;
 *  - E again gets out (the parent is -1 again and the player stands beside it).
 * Frames: warthog-01 (at the door), -02 (driving), -03 (out). */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
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
uint32_t CreateFileA_c(uint32_t name, uint32_t access, uint32_t share, uint32_t sec, uint32_t disp, uint32_t flags, uint32_t tmpl);
uint32_t WriteFile_c(uint32_t h, uint32_t buf, uint32_t n, uint32_t written, uint32_t ov);
uint32_t CloseHandle_c(uint32_t h);
uint32_t DeleteFileA_c(uint32_t name);
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


#include "../port/runtime/halopad_input.h"
void *halopad_d3d9_device_target(uint32_t g);
void halopad_metal_read_image(void *p, uint32_t *out, uint32_t w, uint32_t h);
extern void (*halopad_d3d9_present_hook)(uint32_t device);

/* Halo's game state (from its own set-up code): the players table (0x476170: "players", 0x200-byte
   entries, pointer at 0x815920) and the object table (0x4f83e0: "object", 12-byte entries whose
   +8 is the object's address, pointer at 0x7fb710). A data table keeps its capacity at +0x20 and
   its first entry at +0x34; a handle is salt << 16 | index. Player +0x34 is the player's unit.
   Object +0x5c is the position and +0x68 the velocity; unit +0x23c is where it looks (i, j, k),
   +0x118 the weapon in hand; weapon +0x2b8 the rounds in the magazine. */
static float f32(uint32_t g) { float v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint16_t u16(uint32_t g) { uint16_t v; memcpy(&v, halopad_guest_ptr(g), 2); return v; }
static uint32_t object(uint32_t h)
{
    uint32_t t = rd(0x7fb710);
    if (!t || h == 0xffffffff || (h & 0xffff) >= u16(t + 0x20)) return 0;
    uint32_t e = rd(t + 0x34) + (h & 0xffff) * 12;
    return u16(e) == h >> 16 ? rd(e + 8) : 0;
}
static uint32_t player_unit(void) { uint32_t t = rd(0x815920); return t ? object(rd(rd(t + 0x34) + 0x34)) : 0; }

static void key(int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = 'W'; e.scan = 0x11; e.down = down; halopad_input_event(&e); }
static void mouse_right(int dx) { hp_input e = {0}; e.kind = HPI_MOUSEMOVE; e.x = 400; e.y = 300; e.dx = dx; halopad_input_event(&e); }
static void trigger(int down) { hp_input e = {0}; e.kind = HPI_BUTTON; e.x = 400; e.y = 300; e.button = 0; e.down = down; halopad_input_event(&e); }

static int frames;
static uint32_t lit, bare;
static float pos100[3], pos119[3], pos221[3], vel229[3], look229[3], look262[3];
static uint32_t unit_seen, rounds_before = 0xffff, rounds_after = 0xffff;
static void get3(uint32_t g, float *v) { for (int i = 0; i < 3; i++) v[i] = f32(g + 4 * i); }
static void save_frame(uint32_t device)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    for (uint32_t i = 0; i < w * h; i++) { lit += (img[i] & 0xFFFFFF) != 0; bare += (img[i] & 0xFFFFFF) == 0xFFE6C4; }
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (reg && strrchr(reg, '/')) {
        char path[1200];
        snprintf(path, sizeof path, "%.*s/play.ppm", (int)(strrchr(reg, '/') - reg), reg);
        FILE *f = fopen(path, "wb");
        if (f) {
            fprintf(f, "P6\n%u %u\n255\n", w, h);
            for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
            fclose(f);
        }
    }
    free(img);
}
/* Frame by frame, like a player at the keyboard: stand still, hold W for 100 frames, let go,
   move the mouse right by 300 counts over 30 frames, then hold the left button for 20 frames. */
static void keyx(uint32_t vk, uint32_t scan, int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = vk; e.scan = scan; e.down = down; halopad_input_event(&e); }
static void mouse(int dx, int dy) { hp_input e = {0}; e.kind = HPI_MOUSEMOVE; e.x = 400; e.y = 300; e.dx = dx; e.dy = dy; halopad_input_event(&e); }
static void shot(uint32_t device, int n)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    const char *reg = getenv("HALOPAD_REGISTRY");
    char path[1200];
    snprintf(path, sizeof path, "%.*s/warthog-%02d.ppm", (int)(strrchr(reg, '/') - reg), reg, n);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
    fclose(f); free(img);
}
/* the Warthog outside the red base, by its tag name and spawn position */
static uint32_t find_hog(void)
{
    uint32_t ot = rd(0x7fb710), tags = rd(0x817144);
    for (uint32_t i = 0; i < u16(ot + 0x20); i++) {
        uint32_t e = rd(ot + 0x34) + i * 12;
        if (!u16(e)) continue;
        uint32_t o = rd(e + 8), te = tags + (rd(o) & 0xffff) * 32;
        if (strcmp((const char *)halopad_guest_ptr(rd(te + 0x10)), "vehicles\\warthog\\mp_warthog")) continue;
        if (fabsf(f32(o + 0x5c) - 102.28f) < 4 && fabsf(f32(o + 0x60) + 144.72f) < 4) return (uint32_t)u16(e) << 16 | i;
    }
    return 0xffffffff;
}
static int steer(float tx, float ty, const float *p, const float *look)
{
    float yaw = atan2f(look[1], look[0]) * 57.29578f, b = atan2f(ty - p[1], tx - p[0]) * 57.29578f, err = b - yaw;
    while (err > 180) err -= 360; while (err < -180) err += 360;
    int dx = (int)(-err * 6); if (dx > 60) dx = 60; if (dx < -60) dx = -60;
    if (dx) mouse(dx, 0);
    return fabsf(err) < 20;
}
#define QUIT_AFTER 3000
static int phase, phase_frame, w_down, offered_type, done, ejected;
static uint32_t seated_before_exit;
static uint32_t hog = 0xffffffff, offered, parent_in, parent_out = 0x12345678;
static float target[2], hog_start[3], hog_moved, out_dist;
static void on_present(uint32_t device)
{
    frames++;
    uint32_t pl = rd(0x815920) ? rd(rd(0x815920) + 0x34) : 0, u = player_unit();
    if (!u || frames < 100 || done) goto end;
    if (hog == 0xffffffff) hog = find_hog();
    uint32_t h = object(hog);
    if (!h) goto end;
    unit_seen = u;
    float p[3], look[3], hp[3]; get3(u + 0x5c, p); get3(u + 0x23c, look); get3(h + 0x5c, hp);
    if (phase == 0) { phase = 1; target[0] = 105.5f; target[1] = -160.5f; }
    if (phase == 1 || phase == 2) {                 /* out of the base, then beside the driver's door */
        float d = hypotf(target[0] - p[0], target[1] - p[1]);
        int ok = steer(target[0], target[1], p, look), want = ok && d > 0.5f;
        if (want != w_down) { keyx('W', 0x11, want); w_down = want; }
        if (!want && ok && d < 0.8f) {
            if (phase == 1) { phase = 2; target[0] = hp[0] - 1.6f; target[1] = hp[1]; }
            else { phase = 3; phase_frame = frames; }
        }
        if (frames > 1500) done = 1;
    }
    if (phase == 3) {                               /* face it and step in until Halo offers the seat */
        int t = frames - phase_frame;
        steer(hp[0], hp[1], p, look);
        if (t == 20) keyx('W', 0x11, 1);
        if (t > 20 && rd(pl + 0x24) == hog && (int16_t)u16(pl + 0x28) == 8 && !offered) {
            offered = hog; offered_type = 8; keyx('W', 0x11, 0); shot(device, 1);
            phase = 4; phase_frame = frames;
        }
        if (t == 200) { keyx('W', 0x11, 0); done = 1; }
    }
    if (phase == 4) {                               /* E: in */
        int t = frames - phase_frame;
        if (t == 10) keyx('E', 0x12, 1);
        if (t == 60) { keyx('E', 0x12, 0); parent_in = rd(u + 0x11c); memcpy(hog_start, hp, 12); phase = 5; phase_frame = frames; }
    }
    if (phase == 5) {                               /* W: drive */
        int t = frames - phase_frame;
        if (t == 10) keyx('W', 0x11, 1);
        if (t == 80) shot(device, 2);
        if (t == 100) { keyx('W', 0x11, 0); }         /* before the cliff: a longer drive climbs it and Halo tips the driver out */
        if (t > 100 && rd(u + 0x11c) != hog) ejected = 1;
        if (t == 260) { hog_moved = hypotf(hp[0] - hog_start[0], hp[1] - hog_start[1]); phase = 6; phase_frame = frames; }
    }
    if (phase == 6) {                               /* stopped: E gets out */
        int t = frames - phase_frame;
        if (t == 1) seated_before_exit = rd(u + 0x11c);
        if (t == 10) keyx('E', 0x12, 1);
        if (t == 25) keyx('E', 0x12, 0);
        if (t == 120) { parent_out = rd(u + 0x11c); out_dist = hypotf(hp[0] - p[0], hp[1] - p[1]); shot(device, 3); done = 1; }
    }
end:
    if (frames == QUIT_AFTER || (done && frames % 50 == 0)) *(uint8_t *)halopad_guest_ptr(0x6b47eb) = 1;
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

    /* the start-up script, in the game folder's writable layer */
    const char *script_name = "halopad_vehicle.txt", *script = "map_name levels\\test\\bloodgulch\\bloodgulch\r\n";
    setenv("HALOPAD_ARGS", "-exec halopad_vehicle.txt", 1);
    {
        uint32_t n = str(script_name), buf = str(script), w = halopad_heap_alloc(4, 1);
        uint32_t h = CreateFileA_c(n, 0x40000000, 0, 0, 2 /* CREATE_ALWAYS */, 0x80, 0);
        WriteFile_c(h, buf, (uint32_t)strlen(script), w, 0);
        CloseHandle_c(h);
    }
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
    /* WinMain's arguments (0x5447ad): 0x545a00 splits the command line after the program name */
    {
        const char *cl = (const char *)halopad_guest_ptr(rd(0x6bece8));
        const char *tail = strstr(cl, "\" ") ? strstr(cl, "\" ") + 2 : "";
        uint32_t edi0 = cpu._edi, count = 0x6bd164;
        cpu._edi = str(tail);
        wr(0x6bd160, halopad_call_guest_ex(0x545a00, 1, &count, 0, 0));
        cpu._edi = edi0;
        printf("    command line \"%s\": %u argument(s)\n", tail, rd(0x6bd164));
    }
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



    /* main: initialization, then frames until the hook asks it to quit */
    halopad_d3d9_present_hook = on_present;
    halopad_call_guest_ex(0x4ca9c0, 0, NULL, 0, 0);
    printf("    %d frames presented; main returned\n", frames);
    check("main ran and returned when asked to quit", frames > 0, 1);
    const char *loaded = (const char *)halopad_guest_ptr(0x643064);
    printf("    map loaded: \"%s\"\n", loaded);
    check("  Blood Gulch loaded", !strcmp(loaded, "bloodgulch"), 1);
    check("  the player has a unit (players 0x815920, objects 0x7fb710)", unit_seen != 0, 1);
    printf("    Warthog %08x; offered %08x (type %d); parent after E %08x; driven %.2f units; parent after E again %08x, %.2f from it\n", hog, offered, offered_type, parent_in, hog_moved, parent_out, out_dist);
    check("  up close, Halo offers the Warthog's seat (player +0x24, type 8)", offered == hog && hog != 0xffffffff, 1);
    check("  E puts the player in the Warthog (unit +0x11c is the vehicle)", parent_in == hog && hog != 0xffffffff, 1);
    check("  W drives the Warthog more than 4 units", hog_moved > 4.0f, 1);
    check("  the player is still driving when it stops", !ejected && seated_before_exit == hog, 1);
    check("  E again gets out (no parent, beside the Warthog)", parent_out == 0xffffffff && out_dist < 6.0f, 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    DeleteFileA_c(str(script_name));
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

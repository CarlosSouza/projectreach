/* Scripted play component test (G4): keyboard/mouse events in Blood Gulch, through Halo's own input.
 *
 * Like tests/halo_bloodgulch_test.c, Halo's start-up script (-exec, "map_name
 * levels\test\bloodgulch\bloodgulch") loads the map through main. Host input goes in as the
 * shell delivers it (halopad_host_post_input), so it reaches Halo through USER32 and DirectInput 8:
 * the buffered keyboard (0x4946b7) and the exclusive mouse (0x4947b2). The test stands still,
 * holds W, lets go, moves the mouse right and holds the left button, and checks each effect in
 * Halo's game state: the unit's position, velocity, where it looks, and the rounds in its
 * weapon. It saves the frame while firing as play.ppm. The core runner remains the only path
 * through WinMain, gated by the license and the product ID.
 * HALOPAD_TEST_CAPTURE_MOTION=1 retains 30 consecutive pan frames with guest pose metadata.
 * Capture readback/file I/O changes pacing; this is not real-time performance, human touch,
 * physical-controller or normal WinMain acceptance.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_play_test.c. */
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

static void key(int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = 'W'; e.scan = 0x11; e.down = down; halopad_host_post_input(&e); }
static void mouse_right(int dx) { hp_input e = {0}; e.kind = HPI_MOUSEMOVE; e.x = 400; e.y = 300; e.dx = dx; halopad_host_post_input(&e); }
static void trigger(int down) { hp_input e = {0}; e.kind = HPI_BUTTON; e.x = 400; e.y = 300; e.button = 0; e.down = down; halopad_host_post_input(&e); }

#define QUIT_AFTER 300
static int frames;
static int capture_motion, motion_frames;
static uint32_t lit, bare;
static float pos100[3], pos119[3], pos221[3], vel229[3], look229[3], look262[3];
static uint32_t unit_seen, rounds_before = 0xffff, rounds_after = 0xffff;
static void get3(uint32_t g, float *v) { for (int i = 0; i < 3; i++) v[i] = f32(g + 4 * i); }
static int save_frame(uint32_t device, const char *name, int count_pixels)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    if (!img) return 0;
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    if (count_pixels) for (uint32_t i = 0; i < w * h; i++) { lit += (img[i] & 0xFFFFFF) != 0; bare += (img[i] & 0xFFFFFF) == 0xFFE6C4; }
    int saved = 0;
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (reg && strrchr(reg, '/')) {
        char path[1200];
        snprintf(path, sizeof path, "%.*s/%s", (int)(strrchr(reg, '/') - reg), reg, name);
        FILE *f = fopen(path, "wb");
        if (f) {
            uint8_t *rgb = malloc(w * h * 3);
            if (rgb) {
                for (uint32_t i = 0; i < w * h; i++) {
                    rgb[3 * i] = img[i] >> 16; rgb[3 * i + 1] = img[i] >> 8; rgb[3 * i + 2] = img[i];
                }
                int header = fprintf(f, "P6\n%u %u\n255\n", w, h);
                saved = header > 0 && fwrite(rgb, 1, w * h * 3, f) == w * h * 3;
                free(rgb);
            }
            if (fclose(f)) saved = 0;
        }
    }
    free(img);
    return saved;
}
/* Scripted frame sequence: stand still, hold W for 100 frames, let go,
   move the mouse right by 300 counts over 30 frames, then hold the left button for 20 frames. */
static void on_present(uint32_t device)
{
    frames++;
    uint32_t u = player_unit();
    if (u) unit_seen = u;
    if (u && frames == 100) get3(u + 0x5c, pos100);
    if (u && frames == 119) get3(u + 0x5c, pos119);
    if (frames == 120) key(1);
    if (frames == 220) key(0);
    if (u && frames == 221) get3(u + 0x5c, pos221);
    if (u && frames == 229) { get3(u + 0x68, vel229); get3(u + 0x23c, look229); }
    if (frames >= 230 && frames < 260) mouse_right(10);
    /* Explicit test-only opt-in: 30 consecutive presented views during the pan.
       Readback/file I/O changes pacing; this is not a real-time FPS measurement. */
    if (capture_motion && frames >= 230 && frames < 260) {
        char name[40];
        snprintf(name, sizeof name, "motion-%03d.ppm", frames);
        if (u && save_frame(device, name, 0)) {
            motion_frames++;
            printf("HALOPAD MOTION frame %d file %s position %.9g %.9g %.9g look %.9g %.9g %.9g\n",
                   frames, name, f32(u + 0x5c), f32(u + 0x60), f32(u + 0x64),
                   f32(u + 0x23c), f32(u + 0x240), f32(u + 0x244));
        }
    }
    if (u && frames == 262) { get3(u + 0x23c, look262); uint32_t w = object(rd(u + 0x118)); if (w) rounds_before = u16(w + 0x2b8); }
    if (frames == 265) trigger(1);
    if (frames == 280) check("  saved firing frame", save_frame(device, "play.ppm", 1), 1);
    if (frames == 285) trigger(0);
    if (u && frames == 295) { uint32_t w = object(rd(u + 0x118)); if (w) rounds_after = u16(w + 0x2b8); }
    if (frames == QUIT_AFTER) *(uint8_t *)halopad_guest_ptr(0x6b47eb) = 1;
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
    const char *motion = getenv("HALOPAD_TEST_CAPTURE_MOTION");
    capture_motion = motion && !strcmp(motion, "1");
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
    const char *script_name = "halopad_play.txt", *script = "map_name levels\\test\\bloodgulch\\bloodgulch\r\n";
    setenv("HALOPAD_ARGS", "-exec halopad_play.txt", 1);
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
    check("main ran and returned when asked to quit", frames >= QUIT_AFTER, 1);
    const char *loaded = (const char *)halopad_guest_ptr(0x643064);
    printf("    map loaded: \"%s\"\n", loaded);
    check("  Blood Gulch loaded", !strcmp(loaded, "bloodgulch"), 1);
    check("  the player has a unit (players 0x815920, objects 0x7fb710)", unit_seen != 0, 1);
    printf("    standing: (%.3f, %.3f, %.3f) at frame 100, (%.3f, %.3f, %.3f) at frame 119\n", pos100[0], pos100[1], pos100[2], pos119[0], pos119[1], pos119[2]);
    check("  without input the player stays put", !memcmp(pos100, pos119, sizeof pos100), 1);
    float dx = pos221[0] - pos119[0], dy = pos221[1] - pos119[1];
    printf("    W held for 100 frames: moved (%.3f, %.3f, %.3f)\n", dx, dy, pos221[2] - pos119[2]);
    check("  W (DirectInput keyboard) walks the player forward, along +x, over 3 units", dx > 3.0f && fabsf(dy) < 0.5f, 1);
    printf("    velocity 9 frames after letting go: (%.3f, %.3f, %.3f)\n", vel229[0], vel229[1], vel229[2]);
    check("  letting go stops the player", fabsf(vel229[0]) < 0.005f && fabsf(vel229[1]) < 0.005f, 1);
    float yaw0 = atan2f(look229[1], look229[0]) * 57.29578f, yaw1 = atan2f(look262[1], look262[0]) * 57.29578f;
    printf("    looking (%.3f, %.3f, %.3f) -> (%.3f, %.3f, %.3f): yaw %.1f -> %.1f degrees\n", look229[0], look229[1], look229[2], look262[0], look262[1], look262[2], yaw0, yaw1);
    check("  300 mouse counts to the right (DirectInput mouse) turn the view right by 10-60 degrees", yaw0 - yaw1 > 10.0f && yaw0 - yaw1 < 60.0f, 1);
    printf("    rounds in the magazine: %u before, %u after 20 frames of the trigger\n", rounds_before, rounds_after);
    check("  the left button fires the assault rifle (the magazine empties by 5 or more)", rounds_before != 0xffff && rounds_after + 5 <= rounds_before, 1);
    check("  the frame while firing is not blank", lit > 1000, 1);
    printf("    pixels of the bare clear colour (Blood Gulch's fog, 0xffe6c4) while firing: %u of 480000\n", bare);
    check("  the world is drawn while firing (under 10% bare clear colour)", bare < 48000, 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    if (capture_motion) check("  captured all 30 consecutive motion frames", motion_frames, 30);
    DeleteFileA_c(str(script_name));
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

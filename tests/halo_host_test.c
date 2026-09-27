/* Host test (G4): a local Slayer game started through Halo's own menus, then played.
 *
 * From the main menu, with keys as a player presses them: Multiplayer (creating profile "New001"
 * in a fresh state folder: run with scripts/run-core.py --fresh-state), Create Game > LAN, the
 * first map (Battle Creek), the Slayer gametype, Server Setup > Start Game. Halo hosts the game
 * itself: the Slayer engine runs, with its rules, spawning and respawning. Then, in Halo's game
 * state (players 0x815920, objects 0x7fb710; see tests/halo_play_test.c): fire (projectiles
 * appear), melee (F), look down and throw frag grenades at the player's own feet (right
 * button; unit +0x31e counts frags, +0xe0/+0xe4 are health and shields), die from them and
 * respawn as a new unit. WinMain's GameSpy set-up and key string are set as in the other
 * component tests (see tests/halo_connect_test.c). */
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

static void keyx(uint32_t vk, uint32_t scan, int ext, int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = vk; e.scan = scan; e.extended = ext; e.down = down; halopad_input_event(&e); }
static void shot(uint32_t device, int n)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    const char *reg = getenv("HALOPAD_REGISTRY");
    char path[1200];
    snprintf(path, sizeof path, "%.*s/host-%02d.ppm", (int)(strrchr(reg, '/') - reg), reg, n);
    FILE *f = fopen(path, "wb");
    fprintf(f, "P6\n%u %u\n255\n", w, h);
    for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
    fclose(f); free(img);
}
/* The menu route, as keys (100 frames apart; "wait" lets a screen settle). */
static const char *const route[] = {
    "enter", "enter",                                  /* Multiplayer; accept the new profile's name */
    "down", "down", "down", "down", "enter",          /* Create Game > LAN */
    "enter",                                           /* Select Map: the first, Battle Creek */
    "down", "down", "down", "down", "down", "down", "down", "down", "down", "down", "down", "down",
    "enter",                                           /* Select Gametype: focus down to OK (Slayer stays chosen) */
    "wait", "wait", "down", "down", "down", "enter",  /* Server Setup > Start Game */
};
#define NROUTE (int)(sizeof route / sizeof route[0])
static float f32(uint32_t g) { float v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint16_t u16(uint32_t g) { uint16_t v; memcpy(&v, halopad_guest_ptr(g), 2); return v; }
static uint32_t object(uint32_t h)
{
    uint32_t ot = rd(0x7fb710);
    if (!ot || h == 0xffffffff || (h & 0xffff) >= u16(ot + 0x20)) return 0;
    uint32_t e = rd(ot + 0x34) + (h & 0xffff) * 12;
    return u16(e) == h >> 16 ? rd(e + 8) : 0;
}
static uint32_t local_player(void) { uint32_t pt = rd(0x815920); return pt ? rd(pt + 0x34) : 0; }
static uint32_t live_objects(void)
{
    uint32_t ot = rd(0x7fb710), n = 0;
    if (!ot) return 0;
    for (uint32_t i = 0; i < u16(ot + 0x20); i++) n += u16(rd(ot + 0x34) + i * 12) != 0;
    return n;
}
static void mouse(int dx, int dy) { hp_input e = {0}; e.kind = HPI_MOUSEMOVE; e.x = 400; e.y = 300; e.dx = dx; e.dy = dy; halopad_input_event(&e); }
static void button(int b, int down) { hp_input e = {0}; e.kind = HPI_BUTTON; e.x = 400; e.y = 300; e.button = b; e.down = down; halopad_input_event(&e); }
#define QUIT_AFTER 4800
static int frames, spawn_frame, death_frame, respawn_frame;
static uint32_t first_unit, second_unit, objects_before, objects_most, frags0 = 0xff, frags1 = 0xff, melee_seen, name_ok;
static float health_min = 2, shield_min = 2, look_down;
static char map_at_spawn[32];
static void on_present(uint32_t device)
{
    frames++;
    int k = (frames - 200) / 100, s2 = (frames - 200) % 100;
    if (frames > 200 && k < NROUTE) {
        static const struct { const char *n; uint32_t vk, scan; int ext; } map[] = {{"enter", 13, 0x1c, 0}, {"down", 40, 0x50, 1}};
        for (unsigned m = 0; m < 2; m++) if (!strcmp(map[m].n, route[k])) {
            if (s2 == 1) keyx(map[m].vk, map[m].scan, map[m].ext, 1);
            if (s2 == 6) keyx(map[m].vk, map[m].scan, map[m].ext, 0);
        }
    }
    uint32_t p = local_player(), u = p ? object(rd(p + 0x34)) : 0;
    if (!spawn_frame && u && frames > 200 + 100 * NROUTE) {
        spawn_frame = frames; first_unit = rd(p + 0x34);
        snprintf(map_at_spawn, sizeof map_at_spawn, "%s", (const char *)halopad_guest_ptr(0x643064));
        /* the player's name (UTF-16 at player +0x4) is the profile's */
        const uint16_t *nm = halopad_guest_ptr(p + 4); name_ok = nm[0] == 'N' && nm[1] == 'e' && nm[2] == 'w' && nm[3] == '0';
        shot(device, 1);
    }
    if (!spawn_frame) goto end;
    int t = frames - spawn_frame;
    if (u && rd(p + 0x34) == first_unit) {
        float hp = f32(u + 0xe0), sh = f32(u + 0xe4);
        if (t > 300 && hp < health_min) health_min = hp;
        if (t > 300 && sh < shield_min) shield_min = sh;
    }
    /* fire */
    if (t == 60) objects_before = live_objects();
    if (t == 70) button(0, 1);
    if (t > 70 && t < 110) { uint32_t n = live_objects(); if (n > objects_most) objects_most = n; }
    if (t == 90) shot(device, 2);
    if (t == 100) button(0, 0);
    /* melee */
    if (t == 180) keyx('F', 0x21, 0, 1);
    if (t > 180 && t < 230 && u && rd(u + 0x2ac)) melee_seen = 1;
    if (t == 186) shot(device, 3);
    if (t == 192) keyx('F', 0x21, 0, 0);
    /* look down, then frag grenades at the player's feet until it dies */
    if (t >= 260 && t < 290) mouse(0, 40);
    if (t == 295 && u) { look_down = f32(u + 0x244); frags0 = *(uint8_t *)halopad_guest_ptr(u + 0x31e); }
    /* both frags close together, so they go off at the player's feet together; again later if
       the player still stands (a grenade can bounce away) */
    if (t == 300 || t == 340 || t == 900 || t == 940) button(1, 1);
    if (t == 310 || t == 350 || t == 910 || t == 950) button(1, 0);
    if (t == 330) shot(device, 4);
    if (t == 330 && u) frags1 = *(uint8_t *)halopad_guest_ptr(u + 0x31e);   /* after the first throw */
    if (!death_frame && t > 300 && (!u || rd(p + 0x34) != first_unit)) { death_frame = frames; shot(device, 5); }
    if (death_frame && !respawn_frame && u && rd(p + 0x34) != first_unit) { respawn_frame = frames; second_unit = rd(p + 0x34); }
    if (respawn_frame && frames == respawn_frame + 60) shot(device, 6);
end:
    if (frames == QUIT_AFTER || (respawn_frame && frames == respawn_frame + 100)) *(uint8_t *)halopad_guest_ptr(0x6b47eb) = 1;
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
    const char *script_name = "halopad_play.txt", *script = "map_name levels\\test\\bloodgulch\\bloodgulch\r\n";
    setenv("HALOPAD_ARGS", "", 1);
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
    /* WinMain 0x544c21: the key string from Halo's own 0x5829e0 (empty without a DigitalProductID;
       WinMain itself would stop there), as in tests/halo_connect_test.c. Nothing is made up. */
    wr(0x6e1468, halopad_call_guest_ex(0x5829e0, 0, NULL, 0, 0));
    /* WinMain 0x544d16-0x544d51: GameSpy's set-up (0x5797e0: game name "halom" in eax, the key in
       esi, -ip in edi, the port on the stack), which registers Halo's own query keys
       (qr2_register_key 0x5c0850). The key is read from WinMain's own instructions (six
       'mov byte [ebp-0x30+k], imm' at 0x544d27), not written here. */
    {
        uint32_t keybuf = halopad_heap_alloc(8, 1);
        for (uint32_t k = 0; k < 6; k++) ((uint8_t *)halopad_guest_ptr(keybuf))[k] = *(uint8_t *)halopad_guest_ptr(0x544d27 + 4 * k + 3);
        uint32_t port = rd(0x6337f8), eax0 = cpu._eax, esi0 = cpu._esi, edi0 = cpu._edi;
        cpu._eax = 0x610654; cpu._esi = keybuf; cpu._edi = 0;
        halopad_call_guest_ex(0x5797e0, 1, &port, 0, 0);
        cpu._eax = eax0; cpu._esi = esi0; cpu._edi = edi0;
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
    printf("    the menus started a game on \"%s\"; the player spawned at frame %d\n", map_at_spawn, spawn_frame);
    check("  Multiplayer > Create Game (LAN) > Battle Creek > Slayer > Start Game loads beavercreek", !strcmp(map_at_spawn, "beavercreek"), 1);
    check("  the player is New001, the profile made in the menus", name_ok, 1);
    printf("    live objects %u before firing, up to %u while firing\n", objects_before, objects_most);
    check("  the trigger fires (projectiles appear)", objects_most > objects_before, 1);
    check("  F melees (unit +0x2ac set during the swing)", melee_seen, 1);
    printf("    looking down: k = %.2f; frag grenades %u -> %u; lowest health %.2f, shields %.2f\n", look_down, frags0, frags1, health_min, shield_min);
    check("  the mouse looks down", look_down < -0.5f, 1);
    check("  the right button throws a frag grenade (one fewer)", frags0 != 0xff && frags1 + 1 == frags0, 1);
    check("  the grenade at the player's feet does damage", shield_min < 1.0f || health_min < 1.0f, 1);
    printf("    died at frame %d, respawned at frame %d as unit %08x (was %08x)\n", death_frame, respawn_frame, second_unit, first_unit);
    check("  the player dies", death_frame > 0, 1);
    check("  Slayer respawns the player as a new unit", respawn_frame > death_frame && second_unit && second_unit != first_unit, 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    DeleteFileA_c(str(script_name));
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

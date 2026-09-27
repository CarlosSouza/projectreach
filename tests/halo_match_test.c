/* Match test (G5, step 2): two HaloPad clients in one game on the original dedicated server.
 *
 * Run by scripts/reference-join.sh with --clients 2 (scripts/run-core.py starts both at once, as
 * HALOPAD_CLIENT 0 and 1). Each joins as in tests/halo_connect_test.c, on client port 2305 + i.
 * Two installations on one machine need their own ports: -port 2320 + i as well (WinMain 0x544c63).
 * Client 0 plays: 400 frames after the server spawns it, it holds W for 200 frames, then fires.
 * Client 1 watches: every frame it looks for the other player in its own players table (a
 * player whose local index, +2, is -1) and follows that player's unit. The other player's
 * movement can reach client 1 only through the server, so this checks input, the server's
 * simulation and replication together. Checks: both join and are spawned; client 0 moves;
 * client 1 sees a remote player and sees it move by more than 1.5 units. */
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


void *halopad_d3d9_device_target(uint32_t g);
void halopad_metal_read_image(void *p, uint32_t *out, uint32_t w, uint32_t h);
extern void (*halopad_d3d9_present_hook)(uint32_t device);
#include "../port/runtime/halopad_input.h"
#define QUIT_AFTER 1800
static int frames, map_frame, spawn_frame, client;
static uint32_t unit_seen;
static float mine0[3], mine1[3], remote0[3], remote_max;
static int remote_frames, remote_seen_frame;
static uint32_t objects_before, objects_most;
static uint16_t u16(uint32_t g) { uint16_t v; memcpy(&v, halopad_guest_ptr(g), 2); return v; }
static float f32(uint32_t g) { float v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t object(uint32_t h)
{
    uint32_t ot = rd(0x7fb710);
    if (!ot || h == 0xffffffff || (h & 0xffff) >= u16(ot + 0x20)) return 0;
    uint32_t e = rd(ot + 0x34) + (h & 0xffff) * 12;
    return u16(e) == h >> 16 ? rd(e + 8) : 0;
}
/* players 0x815920: 0x200-byte entries, +0 salt (0 = free), +2 local player index (-1 = remote),
   +0x34 the player's unit (see tests/halo_play_test.c) */
static uint32_t player_unit(int want_local)
{
    uint32_t pt = rd(0x815920);
    if (!pt) return 0;
    for (uint32_t i = 0; i < u16(pt + 0x20); i++) {
        uint32_t p = rd(pt + 0x34) + i * 0x200;
        if (!u16(p)) continue;
        int local = (int16_t)u16(p + 2) != -1;
        if (local == want_local) { uint32_t u = object(rd(p + 0x34)); if (u) return u; }
    }
    return 0;
}
/* live objects in the object table (a non-zero salt); firing adds projectiles */
static uint32_t live_objects(void)
{
    uint32_t ot = rd(0x7fb710), n = 0;
    if (!ot) return 0;
    for (uint32_t i = 0; i < u16(ot + 0x20); i++) n += u16(rd(ot + 0x34) + i * 12) != 0;
    return n;
}
static void key(int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = 'W'; e.scan = 0x11; e.down = down; halopad_input_event(&e); }
static void trigger(int down) { hp_input e = {0}; e.kind = HPI_BUTTON; e.x = 400; e.y = 300; e.button = 0; e.down = down; halopad_input_event(&e); }
static float dist(const float *a, const float *b) { float d = 0; for (int k = 0; k < 3; k++) d += (a[k] - b[k]) * (a[k] - b[k]); return sqrtf(d); }
static void save(uint32_t device)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (reg && strrchr(reg, '/')) {
        char path[1200];
        snprintf(path, sizeof path, "%.*s/match.ppm", (int)(strrchr(reg, '/') - reg), reg);
        FILE *f = fopen(path, "wb");
        if (f) {
            fprintf(f, "P6\n%u %u\n255\n", w, h);
            for (uint32_t i = 0; i < w * h; i++) { uint8_t rgb[3] = {(uint8_t)(img[i] >> 16), (uint8_t)(img[i] >> 8), (uint8_t)img[i]}; fwrite(rgb, 1, 3, f); }
            fclose(f);
        }
    }
    free(img);
}
static void on_present(uint32_t device)
{
    frames++;
    if (!map_frame && !strcmp((const char *)halopad_guest_ptr(0x643064), "bloodgulch")) map_frame = frames;
    uint32_t u = map_frame ? player_unit(1) : 0;
    if (u) { unit_seen = u; if (!spawn_frame) { spawn_frame = frames; memcpy(mine0, halopad_guest_ptr(u + 0x5c), 12); } }
    if (client == 0 && spawn_frame) {
        int t = frames - spawn_frame;
        if (t == 399) memcpy(mine0, halopad_guest_ptr(u + 0x5c), 12);
        if (t == 400) key(1);
        if (t == 600) key(0);
        if (t == 610 && u) { memcpy(mine1, halopad_guest_ptr(u + 0x5c), 12); objects_before = live_objects(); }
        if (t == 615) trigger(1);
        if (t > 615 && t <= 660) { uint32_t n = live_objects(); if (n > objects_most) objects_most = n; }
        if (t == 645) trigger(0);
        if (t == 500) save(device);
    }
    if (client == 1 && map_frame) {
        uint32_t r = player_unit(0);
        if (r) {
            float p[3]; memcpy(p, halopad_guest_ptr(r + 0x5c), 12);
            if (!remote_frames++) { memcpy(remote0, p, 12); remote_seen_frame = frames; }
            float d = dist(p, remote0);
            if (d > remote_max && d < 1000) remote_max = d;
            if (remote_frames == 900) save(device);
        }
    }
    if (frames == 600 && !spawn_frame) save(device);               /* not in the game by now: show why */
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

    client = getenv("HALOPAD_CLIENT") ? atoi(getenv("HALOPAD_CLIENT")) : 0;
    const char *server = getenv("HALOPAD_TEST_SERVER") ? getenv("HALOPAD_TEST_SERVER") : "127.0.0.1:2310";
    char args[128];
    snprintf(args, sizeof args, "-connect %s", server);
    setenv("HALOPAD_ARGS", args, 1);
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
    /* WinMain 0x544c21: the key string from Halo's own 0x5829e0, which reads the DigitalProductID
       its installer writes. On this machine there is none and it returns its empty string
       (0x5f363c); WinMain would stop here with "Your product key is invalid". The component
       test records what the original server does with that; nothing is written or made up. */
    wr(0x6e1468, halopad_call_guest_ex(0x5829e0, 0, NULL, 0, 0));
    printf("    key string from 0x5829e0: 0x%08x \"%s\"\n", rd(0x6e1468), (const char *)halopad_guest_ptr(rd(0x6e1468)));
    /* WinMain's -cport (0x544c93): the client's port, with the flag that a port was given */
    wr(0x6337fc, 2305 + (uint32_t)client);
    wr(0x6337f8, 2320 + (uint32_t)client);                       /* and -port (0x544c63): each client's own server port */
    *(uint8_t *)halopad_guest_ptr(0x6b7360) = 1;
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
    printf("    client %d joined %s: map at frame %d, spawned at frame %d\n", client, server, map_frame, spawn_frame);
    check("  the server's map (bloodgulch) loaded through the connection", map_frame > 0, 1);
    check("  the server spawned this player's unit", unit_seen != 0, 1);
    if (client == 0) {
        printf("    W held for 200 frames: moved %.2f units; live objects %u before firing, up to %u while firing\n", dist(mine0, mine1), objects_before, objects_most);
        check("  client 0: W moves the player (over 1.5 units)", dist(mine0, mine1) > 1.5f, 1);
        check("  client 0: the trigger fires (projectiles appear)", objects_most > objects_before, 1);
    } else {
        printf("    the other player: seen for %d frames from frame %d, moved up to %.2f units\n", remote_frames, remote_seen_frame, remote_max);
        check("  client 1: another player is in the game", remote_frames > 0, 1);
        check("  client 1: sees the other player move (over 1.5 units, through the server)", remote_max > 1.5f, 1);
    }
    check("  no dialog", (uint32_t)dialogs, 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

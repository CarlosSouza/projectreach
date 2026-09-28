/* Lifecycle test (G4): menu return, map reload, quitting from the menu, and a clean relaunch.
 *
 * Run with scripts/run-core.py --relaunch: two processes, one after the other, on one fresh state
 * folder (HALOPAD_LAUNCH 1 and 2). Only keys, as a player presses them:
 *  launch 1: Multiplayer (creating profile "New001"), Create Game > LAN > Battle Creek > Slayer >
 *    Start Game; in the game, Escape > Leave Game back to the main menu; the same again (the map
 *    loads a second time); then Quit > OK. Halo's own main returns: the test never sets the quit
 *    flag unless a 20,000-frame guard runs out, which fails the test.
 *  launch 2: Multiplayer goes straight to the multiplayer menu (the profile Halo saved is loaded,
 *    so there is no name prompt), the same game starts, the player is New001; Leave Game; Quit > OK.
 * Checks: the map sequence, the player's name, main returning by itself, and the saved profile
 * files on disk between the launches. WinMain's GameSpy set-up and key string are set as in the
 * other component tests (see tests/halo_connect_test.c). */
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
extern int halopad_host_input_off;

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
    snprintf(path, sizeof path, "%.*s/life-%02d.ppm", (int)(strrchr(reg, '/') - reg), reg, n);
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
#define QUIT_AFTER 6400
static int frames, spawn_frame, death_frame, respawn_frame;
static uint32_t first_unit, second_unit, objects_before, objects_most, frags0 = 0xff, frags1 = 0xff, melee_seen, name_ok;
static float health_min = 2, shield_min = 2, look_down;
static char map_at_spawn[32];
static uint32_t pickup, weapons_before, picked; static int w_down, e_down, offered_pickup, searched, nskip, progress_frame;
static uint32_t skip[16]; static float best_dist, spawn_pos[2], spawn_yaw; static int tp0, combat_start, stuck_tries, strafe_until; static uint32_t strafe_key;
/* The sound Halo makes: DirectSound's mix, pulled at real-time rate as Core Audio would pull it
   (tests have no audio device, halopad_audio_manual), recorded for the whole session. */
void halopad_dsound_mix(float *out, uint32_t frames);
extern int halopad_audio_manual;
#include <time.h>
#include <pthread.h>
static int16_t *rec; static uint32_t rec_n, rec_cap;
static uint32_t mark_fire, mark_frag, mark_shot, mark_death;
static double now_s(void) { struct timespec ts; clock_gettime(CLOCK_MONOTONIC, &ts); return ts.tv_sec + ts.tv_nsec * 1e-9; }
static pthread_mutex_t rec_lock = PTHREAD_MUTEX_INITIALIZER;
static volatile int tap_run;
/* the audio device's thread: every 10 ms, mix what real time has consumed */
static void *audio_tap(void *arg)
{
    (void)arg;
    static float buf[2 * 44100];
    double start = now_s(), done = 0;
    while (tap_run) {
        struct timespec ts = {0, 10 * 1000 * 1000};
        nanosleep(&ts, NULL);
        uint32_t n = (uint32_t)((now_s() - start - done) * 44100);
        if (!n) continue;
        if (n > 44100) n = 44100;
        done += n / 44100.0;
        halopad_dsound_mix(buf, n);
        pthread_mutex_lock(&rec_lock);
        if (rec_n + n > rec_cap) { rec_cap = (rec_n + n) * 2; rec = realloc(rec, rec_cap * 4); }
        for (uint32_t i = 0; i < 2 * n; i++) rec[2 * rec_n + i] = (int16_t)(buf[i] * 32767);
        rec_n += n;
        pthread_mutex_unlock(&rec_lock);
    }
    return NULL;
}
/* RMS in dBFS of frames [a, b) (both channels) */
static double rms_db(uint32_t a, uint32_t b)
{
    if (b > rec_n) b = rec_n;
    if (a >= b) return -200;
    double e = 0;
    for (uint32_t i = 2 * a; i < 2 * b; i++) e += (double)rec[i] * rec[i];
    return 10 * log10(e / (2.0 * (b - a)) / (32767.0 * 32767.0) + 1e-20);
}
static int cmp_d(const void *x, const void *y) { double a = *(const double *)x, b = *(const double *)y; return (a > b) - (a < b); }
static double median_db(uint32_t a, uint32_t b)          /* the median 100 ms level in [a, b) */
{
    static double v[4096]; int n = 0;
    for (uint32_t x = a; x + 4410 <= b && n < 4096; x += 4410) v[n++] = rms_db(x, x + 4410);
    if (!n) return -200;
    qsort(v, n, sizeof v[0], cmp_d);
    return v[n / 2];
}
static double loudest_db(uint32_t a, uint32_t b)       /* the loudest 50 ms window in [a, b) */
{
    double m = -200;
    for (uint32_t x = a; x + 2205 <= b && x + 2205 <= rec_n; x += 1102) { double d = rms_db(x, x + 2205); if (d > m) m = d; }
    return m;
}
static void save_wav(void)
{
    const char *reg = getenv("HALOPAD_REGISTRY");
    char path[1200];
    snprintf(path, sizeof path, "%.*s/session.wav", (int)(strrchr(reg, '/') - reg), reg);
    FILE *f = fopen(path, "wb");
    uint32_t data = rec_n * 4, v;
    fwrite("RIFF", 1, 4, f); v = 36 + data; fwrite(&v, 4, 1, f); fwrite("WAVEfmt ", 1, 8, f);
    v = 16; fwrite(&v, 4, 1, f); uint16_t h[2] = {1, 2}; fwrite(h, 2, 2, f);
    v = 44100; fwrite(&v, 4, 1, f); v = 44100 * 4; fwrite(&v, 4, 1, f); h[0] = 4; h[1] = 16; fwrite(h, 2, 2, f);
    fwrite("data", 1, 4, f); fwrite(&data, 4, 1, f); fwrite(rec, 4, rec_n, f); fclose(f);
}
static char maps_seen[256];
static int quit_by_test, quit_confirm_sent, launch = 1, spawned_as_new001;
static void on_present(uint32_t device)
{
    frames++;
    static const char *const start[] = {"down", "down", "down", "down", "enter", "enter",
        "down", "down", "down", "down", "down", "down", "down", "down", "down", "down", "down", "down", "enter",
        "wait", "wait", "down", "down", "down", "enter", "wait", "wait", "wait", "wait"};
    static const char *const leave[] = {"esc", "wait", "down", "down", "down", "enter", "wait"};
    static const char *const quit[] = {"down", "down", "down", "down", "enter", "wait", "left", "wait", "enter", "wait"};
    static const char *route[128]; static int n = -1;
    if (n < 0) {
        n = 0;
        route[n++] = "enter";                                           /* Multiplayer */
        if (launch == 1) route[n++] = "enter";                          /* accept the new profile's name */
        for (int rep = 0; rep < (launch == 1 ? 2 : 1); rep++) {
            for (unsigned q = 0; q < sizeof start / sizeof *start; q++) route[n++] = start[q];
            for (unsigned q = 0; q < sizeof leave / sizeof *leave; q++) route[n++] = leave[q];
            if (rep == 0 && launch == 1) route[n++] = "enter";          /* Multiplayer again */
        }
        for (unsigned q = 0; q < sizeof quit / sizeof *quit; q++) route[n++] = quit[q];
    }
    int k = (frames - 200) / 100, s2 = (frames - 200) % 100;
    if (frames >= 200 && k < n) {
        if (s2 == 0 && getenv("HALOPAD_TRACE_LIFECYCLE")) {
            printf("    route %d frame %d: %s\n", k, frames, route[k]);
            shot(device, 100 + k);
        }
        static const struct { const char *n; uint32_t vk, scan; int ext; } map[] = {{"enter", 13, 0x1c, 0}, {"esc", 27, 1, 0}, {"down", 40, 0x50, 1}, {"left", 37, 0x4b, 1}};
        for (unsigned m = 0; m < sizeof map / sizeof map[0]; m++) if (!strcmp(map[m].n, route[k])) {
            if (s2 == 1) {
                keyx(map[m].vk, map[m].scan, map[m].ext, 1);
                if (k == n - 2) quit_confirm_sent = 1;
            }
            if (s2 == 6) keyx(map[m].vk, map[m].scan, map[m].ext, 0);
        }
    }
    const char *m = (const char *)halopad_guest_ptr(0x643064);
    const char *last = strrchr(maps_seen, ' ') ? strrchr(maps_seen, ' ') + 1 : maps_seen;
    if (*m && strcmp(m, last) && strlen(maps_seen) + strlen(m) + 2 < sizeof maps_seen) { if (*maps_seen) strcat(maps_seen, " "); strcat(maps_seen, m); shot(device, (int)strlen(maps_seen) % 100); }
    uint32_t p = local_player(), u = p ? object(rd(p + 0x34)) : 0;
    if (u && !strcmp(m, "beavercreek")) { const uint16_t *nm = halopad_guest_ptr(p + 4); if (nm[0] == 'N' && nm[1] == 'e' && nm[2] == 'w' && nm[3] == '0' && nm[4] == '0' && nm[5] == '1') spawned_as_new001 = 1; }
    if (frames == 20000) { quit_by_test = 1; *(uint8_t *)halopad_guest_ptr(0x6b47eb) = 1; }
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

static int profile_on_disk(void)
{
    const char *st = getenv("HALOPAD_STATE_ROOT");
    char path[1200];
    snprintf(path, sizeof path, "%s/C/Documents and Settings/Player/My Documents/My Games/Halo CE/savegames/New001/blam.sav", st ? st : ".");
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    fseek(f, 0, SEEK_END);
    long n = ftell(f);
    fclose(f);
    return n == 8192;
}
int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    /* This test owns its input. Desktop pointer/focus events can otherwise select a
       different menu item or pause Halo while the frame-driven route is running.
       Focus/lifecycle input itself is covered separately by halo_user32_test. */
    halopad_host_input_off = 1;
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
    launch = getenv("HALOPAD_LAUNCH") ? atoi(getenv("HALOPAD_LAUNCH")) : 1;
    if (launch == 2) check("launch 2 starts from the state launch 1 left (profile New001 on disk)", profile_on_disk(), 1);
    tap_run = 1;
    pthread_t tap;
    pthread_create(&tap, NULL, audio_tap, NULL);
    halopad_d3d9_present_hook = on_present;
    halopad_call_guest_ex(0x4ca9c0, 0, NULL, 0, 0);
    printf("    %d frames presented; main returned\n", frames);
    printf("    launch %d: %d frames; maps in order: %s\n", launch, frames, maps_seen);
    check("main returned after the test sent Quit > OK (no forced stop)", !quit_by_test && quit_confirm_sent, 1);
    if (launch == 1)
        check("  maps: menu, Battle Creek, menu (Leave Game), Battle Creek again, menu", !strcmp(maps_seen, "ui beavercreek ui beavercreek ui"), 1);
    else
        check("  maps: menu, Battle Creek (no profile prompt: the saved profile is loaded), menu", !strcmp(maps_seen, "ui beavercreek ui"), 1);
    check("  the player in the game is New001", spawned_as_new001, 1);
    check("  the profile is saved on disk", profile_on_disk(), 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    DeleteFileA_c(str(script_name));
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

/* Host test (G4): a local Slayer game started through Halo's own menus, then played.
 *
 * From the main menu, with keys as a player presses them: Multiplayer (creating profile "New001"
 * in a fresh state folder: run with scripts/run-core.py --fresh-state), Create Game > LAN, the
 * first map (Battle Creek), the Slayer gametype, Server Setup > Start Game. Halo hosts the game
 * itself: the Slayer engine runs, with its rules, spawning and respawning. Then, in Halo's game
 * state (players 0x815920, objects 0x7fb710; see tests/halo_play_test.c): walk to a loose weapon
 * and pick it up (it joins the unit's weapons at +0x2f8), walk back to the spawn point, fire (projectiles
 * appear), melee (F), look down and throw frag grenades at the player's own feet (right
 * button; unit +0x31e counts frags, +0xe0/+0xe4 are health and shields), die from them and
 * respawn as a new unit. WinMain's GameSpy set-up and key string are set as in the other
 * component tests (see tests/halo_connect_test.c). The whole session's sound is pulled from
 * DirectSound's mix at real-time rate and saved as session.wav; the menu music, gunfire and the
 * grenade explosions are checked in it. HALOPAD_TEST_CAPTURE_MOTION=1 additionally
 * retains 30 consecutive downward-pan frames and guest pose metadata. Readback
 * changes pacing: not human input, normal WinMain, hardware or FPS acceptance. */
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

static void keyx(uint32_t vk, uint32_t scan, int ext, int down) { hp_input e = {0}; e.kind = HPI_KEY; e.vk = e.side_vk = vk; e.scan = scan; e.extended = ext; e.down = down; halopad_host_post_input(&e); }
static int shot(uint32_t device, int n)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    if (!img) { failures++; return 0; }
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (!reg || !strrchr(reg, '/')) { free(img); failures++; return 0; }
    char path[1200];
    snprintf(path, sizeof path, "%.*s/host-%02d.ppm", (int)(strrchr(reg, '/') - reg), reg, n);
    FILE *f = fopen(path, "wb");
    int saved = 0;
    if (f) {
        uint8_t *rgb = malloc(w * h * 3);
        if (rgb) {
            for (uint32_t i = 0; i < w * h; i++) {
                rgb[3 * i] = img[i] >> 16; rgb[3 * i + 1] = img[i] >> 8; rgb[3 * i + 2] = img[i];
            }
            saved = fprintf(f, "P6\n%u %u\n255\n", w, h) > 0 &&
                fwrite(rgb, 1, w * h * 3, f) == w * h * 3;
            free(rgb);
        }
        if (fclose(f)) saved = 0;
    }
    free(img);
    failures += !saved;
    return saved;
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
static void mouse(int dx, int dy) { hp_input e = {0}; e.kind = HPI_MOUSEMOVE; e.x = 400; e.y = 300; e.dx = dx; e.dy = dy; halopad_host_post_input(&e); }
static void button(int b, int down) { hp_input e = {0}; e.kind = HPI_BUTTON; e.x = 400; e.y = 300; e.button = b; e.down = down; halopad_host_post_input(&e); }
#define QUIT_AFTER 6400
static int frames, spawn_frame, death_frame, respawn_frame;
static int capture_motion, motion_frames;
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
static void on_present(uint32_t device)
{
    frames++;
    if (frames == 150 || frames == 2900) printf("    checkpoint frame %d: map \"%s\", %.1f s of sound mixed\n", frames, (const char *)halopad_guest_ptr(0x643064), rec_n / 44100.0);
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
    /* after the respawn (Slayer picks a random spawn point): walk to the nearest loose weapon on the
       player's level that it does not hold, and take it when Halo offers it (player +0x24, "Hold E
       to pick up"). Blocked (no progress for 60 frames): strafe left, then right; then try the next
       weapon. */
    if (!combat_start) combat_start = spawn_frame;

    if (respawn_frame) {
        int tp = frames - respawn_frame;
        /* choose (again, if the way is blocked: no progress for 60 frames) the nearest loose weapon */
        if (u && !picked && pickup && tp - progress_frame > 60 && stuck_tries < 2) {       /* blocked: strafe */
            strafe_key = stuck_tries == 0 ? 'A' : 'D'; strafe_until = tp + 40; stuck_tries++; progress_frame = tp + 40;
            if (w_down) { keyx('W', 0x11, 0, 0); w_down = 0; }
            keyx(strafe_key, strafe_key == 'A' ? 0x1e : 0x20, 0, 1);
        }
        if (strafe_key && tp >= strafe_until) { keyx(strafe_key, strafe_key == 'A' ? 0x1e : 0x20, 0, 0); strafe_key = 0; }
        if (u && !picked && tp >= 20 && tp < 1000 && (tp == 20 || (pickup && tp - progress_frame > 60))) {
            stuck_tries = 0;
            if (pickup && nskip < 16) skip[nskip++] = pickup;
            uint32_t ot = rd(0x7fb710), held = object(rd(u + 0x2f8));
            float best = 25.0f, up[3] = {f32(u + 0x5c), f32(u + 0x60), f32(u + 0x64)};
            pickup = 0;
            for (uint32_t i = 0; i < u16(ot + 0x20); i++) {
                uint32_t e = rd(ot + 0x34) + i * 12;
                if (!u16(e)) continue;
                uint32_t o = rd(e + 8), hnd = (uint32_t)u16(e) << 16 | i;
                int skipped = 0; for (int q = 0; q < nskip; q++) skipped |= skip[q] == hnd;
                if (skipped || (int16_t)u16(o + 0xb4) != 2 || rd(o + 0xcc) != 0xffffffff || (held && (rd(o) & 0xffff) == (rd(held) & 0xffff))) continue;
                float d = hypotf(f32(o + 0x5c) - up[0], f32(o + 0x60) - up[1]);
                if (fabsf(f32(o + 0x64) - up[2]) < 1.0f && d < best) { best = d; pickup = hnd; }
            }
            if (tp == 20) weapons_before = rd(u + 0x2f8 + 4);
            progress_frame = tp; best_dist = 1e9f;
            if (1) { uint32_t oo = object(pickup); printf("      pickup choose t%d: %08x at %.2f %.2f %.2f; player %.2f %.2f %.2f\n", tp, pickup, oo ? f32(oo + 0x5c) : 0, oo ? f32(oo + 0x60) : 0, oo ? f32(oo + 0x64) : 0, up[0], up[1], up[2]); }
        }
        if (u && pickup && object(pickup)) {
            uint32_t o = object(pickup);
            float d = hypotf(f32(o + 0x5c) - f32(u + 0x5c), f32(o + 0x60) - f32(u + 0x60));
            if (d < best_dist - 0.3f) { best_dist = d; progress_frame = tp; }
        }
        if (tp > 20 && tp < 1000 && pickup && u && !picked && !strafe_key) {
            uint32_t o = object(pickup);
            float up[3] = {f32(u + 0x5c), f32(u + 0x60), f32(u + 0x64)}, look[3] = {f32(u + 0x23c), f32(u + 0x240), f32(u + 0x244)};
            if (o) {
                float yaw = atan2f(look[1], look[0]) * 57.29578f, b = atan2f(f32(o + 0x60) - up[1], f32(o + 0x5c) - up[0]) * 57.29578f, err = b - yaw;
                while (err > 180) err -= 360; while (err < -180) err += 360;
                int dx = (int)(-err * 6); if (dx > 60) dx = 60; if (dx < -60) dx = -60;
                if (dx) mouse(dx, 0);
                int want = fabsf(err) < 20;
                if (want != w_down) { keyx('W', 0x11, 0, want); w_down = want; }
            }
            if (tp % 80 == 0) printf("      pickup t%d player %.2f %.2f %.2f weapon %.2f %.2f %.2f interaction %08x/%d\n", tp, up[0], up[1], up[2], o ? f32(o + 0x5c) : 0, o ? f32(o + 0x60) : 0, o ? f32(o + 0x64) : 0, rd(p + 0x24), (int16_t)u16(p + 0x28));
            /* Halo offers the weapon (player +0x24, the "Hold E to pick up" prompt); hold E to take it */
            if (rd(p + 0x24) == pickup && !e_down) { offered_pickup = (int16_t)u16(p + 0x28); keyx('E', 0x12, 0, 1); e_down = frames; }
            if (e_down && frames - e_down == 40) keyx('E', 0x12, 0, 0);
            for (uint32_t q = 0; q < 4; q++) if (rd(u + 0x2f8 + 4 * q) == pickup) { picked = frames; shot(device, 7); }
        }
        if ((picked || tp == 1000) && w_down) { keyx('W', 0x11, 0, 0); w_down = 0; }
        if ((picked || tp == 1000) && e_down > 0 && frames - e_down < 40) { keyx('E', 0x12, 0, 0); e_down = -1; }
    }
    if (frames < combat_start) goto end;
    int t = frames - combat_start;
    if (u && rd(p + 0x34) == first_unit) {
        float hp = f32(u + 0xe0), sh = f32(u + 0xe4);
        if (t > 300 && hp < health_min) health_min = hp;
        if (t > 300 && sh < shield_min) shield_min = sh;
    }
    /* fire */
    if (t == 60) objects_before = live_objects();
    if (t == 70) { button(0, 1); mark_fire = rec_n; }
    if (t > 70 && t < 110) { uint32_t n = live_objects(); if (n > objects_most) { if (n > objects_before && !mark_shot) mark_shot = rec_n; objects_most = n; } }
    if (t == 90) shot(device, 2);
    if (t == 100) button(0, 0);
    /* melee */
    if (t == 180) keyx('F', 0x21, 0, 1);
    /* Player melee timer: original 0x55d226 starts it, 0x55d263 decrements it.
       +0x2ac is an animation frame adjacent to a usually-ffff index; testing
       that whole word falsely passed even while idle. +0x289 is AI melee. */
    if (t > 180 && t < 230 && u && *(uint8_t *)halopad_guest_ptr(u + 0x505)) melee_seen = 1;
    if (t == 186) shot(device, 3);
    if (t == 192) keyx('F', 0x21, 0, 0);
    /* look down, then frag grenades at the player's feet until it dies */
    /* look down: 1,200 counts over 30 frames, done well before the first throw (k -0.76 in 52
       runs, where the two frags killed; a slower closed-loop turn still moving at the throw, or
       k -0.91, left the player wounded) */
    if (t >= 240 && t < 270) mouse(0, 40);
    if (capture_motion && u && t >= 240 && t < 270 && shot(device, 1000 + frames)) {
        motion_frames++;
        printf("HALOPAD HOST MOTION frame %d file host-%02d.ppm position %.9g %.9g %.9g look %.9g %.9g %.9g\n",
               frames, 1000 + frames, f32(u + 0x5c), f32(u + 0x60), f32(u + 0x64),
               f32(u + 0x23c), f32(u + 0x240), f32(u + 0x244));
    }
    if (t == 325 && u) { look_down = f32(u + 0x244); frags0 = *(uint8_t *)halopad_guest_ptr(u + 0x31e); }
    /* grenades at the player's feet until it dies: a throw every 40 frames (a press while the last
       throw is still in its animation is ignored, so fixed times sometimes threw one frag, which
       leaves the player at 0.40 health); when the frags are gone, G switches to plasma grenades
       (unit +0x31e frags, +0x31f plasma) */
    if (t == 330) mark_frag = rec_n;
    if (t >= 330 && t < 1400 && !death_frame && u && rd(p + 0x34) == first_unit) {
        uint8_t frags = *(uint8_t *)halopad_guest_ptr(u + 0x31e), plasma = *(uint8_t *)halopad_guest_ptr(u + 0x31f);
        static int switched;
        if ((t - 330) % 40 == 0 && (frags || (switched && plasma))) button(1, 1);
        if ((t - 330) % 40 == 10) button(1, 0);
        if (!frags && plasma && !switched && (t - 330) % 40 == 20) { keyx('G', 0x22, 0, 1); switched = t; }
        if (switched && t == switched + 6) keyx('G', 0x22, 0, 0);
    }
    if (t == 365) shot(device, 4);
    if (t == 360 && u) frags1 = *(uint8_t *)halopad_guest_ptr(u + 0x31e);   /* after the first throw */
    if (!death_frame && t > 300 && (!u || rd(p + 0x34) != first_unit)) { death_frame = frames; mark_death = rec_n; button(1, 0); shot(device, 5); }
    if (death_frame && !respawn_frame && u && rd(p + 0x34) != first_unit) { respawn_frame = frames; second_unit = rd(p + 0x34); }
    if (respawn_frame && frames == respawn_frame + 10) shot(device, 6);
end:
    if (frames == QUIT_AFTER || (respawn_frame && (frames == respawn_frame + 1060 || (picked && frames == picked + 60)))) *(uint8_t *)halopad_guest_ptr(0x6b47eb) = 1;
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
    tap_run = 1;
    pthread_t tap;
    pthread_create(&tap, NULL, audio_tap, NULL);
    halopad_d3d9_present_hook = on_present;
    halopad_call_guest_ex(0x4ca9c0, 0, NULL, 0, 0);
    printf("    %d frames presented; main returned\n", frames);
    check("main ran and returned when asked to quit", frames > 0, 1);
    printf("    the menus started a game on \"%s\"; the player spawned at frame %d\n", map_at_spawn, spawn_frame);
    check("  Multiplayer > Create Game (LAN) > Battle Creek > Slayer > Start Game loads beavercreek", !strcmp(map_at_spawn, "beavercreek"), 1);
    check("  the player is New001, the profile made in the menus", name_ok, 1);
    tap_run = 0;
    pthread_join(tap, NULL);
    save_wav();
    {
        uint32_t sec = 44100;
        double menu = loudest_db(sec * 2, sec * 20 < mark_fire ? sec * 20 : mark_fire);
        /* the game's ambience: the median 100 ms level over the 8.5 s before the shot (spawn and
           announcer sounds are short and do not move it) */
        double quiet = mark_shot > 10 * sec ? median_db(mark_shot - 10 * sec, mark_shot - 3 * sec / 2) : 0;
        double shot = loudest_db(mark_shot > sec / 4 ? mark_shot - sec / 4 : 0, mark_shot + sec / 2);
        double boom = loudest_db(mark_death > sec / 2 ? mark_death - sec / 2 : 0, mark_death + sec / 4);
        printf("    audio marks: trigger %.2f s, projectile %.2f s, first frag %.2f s, death %.2f s\n", mark_fire / 44100.0, mark_shot / 44100.0, mark_frag / 44100.0, mark_death / 44100.0);
        printf("    audio: %.1f s recorded (session.wav); menu loudest %.1f dBFS; game ambience %.1f dBFS; the shot %.1f dBFS; the explosion that kills %.1f dBFS\n",
               rec_n / 44100.0, menu, quiet, shot, boom);
        check("  the menu makes sound (music, louder than -40 dBFS)", menu > -40, 1);
        check("  the shot is heard when its projectile appears (15 dB over the ambience)", mark_shot && shot > quiet + 15, 1);
        check("  the explosion is heard when it kills the player (20 dB over the ambience)", mark_death && boom > quiet + 20, 1);
    }
    printf("    pickup: weapon %08x (offered as interaction type %d), second slot %08x before, taken at frame %d\n", pickup, offered_pickup, weapons_before, picked);
    check("  walking over a loose weapon picks it up (it joins the unit's weapons, +0x2f8)", pickup && picked, 1);
    printf("    live objects %u before firing, up to %u while firing\n", objects_before, objects_most);
    check("  the trigger fires (projectiles appear)", objects_most > objects_before, 1);
    check("  F melees (player melee timer +0x505)", melee_seen, 1);
    printf("    looking down: k = %.2f; frag grenades %u -> %u; lowest health %.2f, shields %.2f\n", look_down, frags0, frags1, health_min, shield_min);
    check("  the mouse looks down", look_down < -0.5f, 1);
    check("  the right button throws a frag grenade (one fewer)", frags0 != 0xff && frags1 + 1 == frags0, 1);
    check("  the grenade at the player's feet does damage", shield_min < 1.0f || health_min < 1.0f, 1);
    printf("    died at frame %d, respawned at frame %d as unit %08x (was %08x)\n", death_frame, respawn_frame, second_unit, first_unit);
    check("  the player dies", death_frame > 0, 1);
    check("  Slayer respawns the player as a new unit", respawn_frame > death_frame && second_unit && second_unit != first_unit, 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    if (capture_motion) check("  captured all 30 consecutive motion frames", motion_frames, 30);
    DeleteFileA_c(str(script_name));
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

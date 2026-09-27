/* DirectSound 8 test (G4/G9): Halo's set-up sequence (0x549270) and the software mixer.
 * The Core Audio device is not started (halopad_audio_manual); the test mixes 44,100 Hz
 * stereo float itself with halopad_dsound_mix and checks samples against DirectSound's
 * rules. Methods are called through the guest vtables.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_dsound_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
extern int halopad_audio_manual;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);
void halopad_dsound_mix(float *out, uint32_t frames);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t bytes(const void *p, uint32_t n) { uint32_t g = halopad_heap_alloc(n, 0); memcpy(halopad_guest_ptr(g), p, n); return g; }
static uint32_t F(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static void near(const char *what, double got, double want)
{
    int ok = fabs(got - want) < 0.002;
    printf("%-66s %s (got %.4f, want %.4f)\n", what, ok ? "PASS" : "FAIL", got, want);
    failures += !ok;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}
#define M(obj, index, ...) method(obj, index, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define M0(obj, index) method(obj, index, 0, NULL)

static float out[2 * 44100];
static void mix(uint32_t frames) { halopad_dsound_mix(out, frames); }

enum { Release = 2, QI = 0, CreateSoundBuffer = 3, GetCaps = 4, Duplicate = 5, SetCooperativeLevel = 6,
       GetCurrentPosition = 4, GetFormat = 5, GetStatus = 9, Lock = 11, Play = 12, SetCurrentPosition = 13, SetFormat = 14,
       SetVolume = 15, SetPan = 16, SetFrequency = 17, Stop = 18, Unlock = 19,
       SetConeAngles = 13, SetConeOrientation = 14, SetConeOutsideVolume = 15, SetMinDistance = 17, SetMode = 18, SetPosition3 = 19,
       SetVelocity3 = 20,
       LSetDistanceFactor = 11, LSetDopplerFactor = 12, LSetPosition = 14, LSetRolloffFactor = 15, LCommit = 17 };

static uint32_t ds, pcur, wcur, lk[4];
static uint32_t wfx(uint16_t ch, uint32_t rate, uint16_t bits)
{
    uint16_t w[9] = {1, ch, (uint16_t)rate, (uint16_t)(rate >> 16), (uint16_t)(rate * ch * bits / 8), (uint16_t)((rate * ch * bits / 8) >> 16),
                     (uint16_t)(ch * bits / 8), bits, 0};
    return bytes(w, 18);
}
static uint32_t make(uint32_t flags, uint16_t ch, uint32_t rate, uint16_t bits, uint32_t size, uint32_t *code)
{
    uint32_t d = bytes((uint32_t[]){36, flags, size, 0, wfx(ch, rate, bits), 0, 0, 0, 0}, 36), p = halopad_heap_alloc(4, 1);
    *code = M(ds, CreateSoundBuffer, d, p, 0);
    return rd(p);
}
/* fill a 16-bit buffer with f(i) */
static void fill16(uint32_t b, uint32_t size, int16_t (*f)(uint32_t))
{
    M(b, Lock, 0, size, lk[0], lk[1], lk[2], lk[3], 0);
    int16_t *s = halopad_guest_ptr(rd(lk[0]));
    for (uint32_t i = 0; i < size / 2; i++) s[i] = f(i);
    M(b, Unlock, rd(lk[0]), rd(lk[1]), 0, 0);
}
static int16_t half(uint32_t i) { (void)i; return 16384; }
static int16_t ramp(uint32_t i) { return (int16_t)(i * 100); }
static uint32_t cursor(uint32_t b) { M(b, GetCurrentPosition, pcur, wcur); return rd(pcur); }

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    halopad_audio_manual = 1;
    pcur = halopad_heap_alloc(4, 1); wcur = halopad_heap_alloc(4, 1);
    for (int i = 0; i < 4; i++) lk[i] = halopad_heap_alloc(4, 1);
    uint32_t code;

    uint32_t create = GetProcAddress_c(LoadLibraryA_c(str("dsound.dll")), str("DirectSoundCreate8"));
    uint32_t pds = halopad_heap_alloc(4, 1);
    static const uint8_t other[16] = {1, 2, 3, 4};
    check("DirectSoundCreate8(an unknown device) has no driver", halopad_call_guest(create, 3, (uint32_t[]){bytes(other, 16), pds, 0}), 0x88780078);
    check("DirectSoundCreate8(NULL)", halopad_call_guest(create, 3, (uint32_t[]){0, pds, 0}), 0);
    ds = rd(pds);
    make(0, 1, 22050, 16, 1000, &code);
    check("CreateSoundBuffer before SetCooperativeLevel: DSERR_PRIOLEVELNEEDED", code, 0x88780046);
    check("SetCooperativeLevel(DSSCL_PRIORITY)", M(ds, SetCooperativeLevel, 0, 2), 0);
    uint32_t caps = halopad_heap_alloc(96, 1);
    memcpy(halopad_guest_ptr(caps), (uint32_t[]){96}, 4);
    check("GetCaps", M(ds, GetCaps, caps), 0);
    check("  max secondary rate 200,000 (Halo mixes at 44,100)", rd(caps + 12), 200000);
    check("  no hardware 3D voices", rd(caps + 44), 0);

    /* Halo's primary buffer and listener */
    uint32_t pd = bytes((uint32_t[]){36, 0x11, 0, 0, 0, 0, 0, 0, 0}, 36), pp = halopad_heap_alloc(4, 1);
    check("primary buffer (PRIMARYBUFFER | CTRL3D)", M(ds, CreateSoundBuffer, pd, pp, 0), 0);
    uint32_t prim = rd(pp);
    static const uint8_t iid_listener[16] = {0x84, 0xFA, 0x9A, 0x27, 0x81, 0x49, 0xCE, 0x11, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60};
    static const uint8_t iid_3dbuf[16] = {0x86, 0xFA, 0x9A, 0x27, 0x81, 0x49, 0xCE, 0x11, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60};
    static const uint8_t iid_ks[16] = {0x30, 0xAC, 0xEF, 0x31, 0x5C, 0x51, 0xD0, 0x11, 0xA9, 0xAA, 0x00, 0xAA, 0x00, 0x61, 0xBE, 0x93};
    uint32_t pl = halopad_heap_alloc(4, 1);
    check("  QueryInterface(IDirectSound3DListener)", M(prim, QI, bytes(iid_listener, 16), pl), 0);
    uint32_t lis = rd(pl);
    check("  SetFormat(44,100 Hz 16-bit stereo)", M(prim, SetFormat, wfx(2, 44100, 16)), 0);
    uint32_t gf = halopad_heap_alloc(18, 1);
    M(prim, GetFormat, gf, 18, 0);
    check("  GetFormat reads it back", rd(gf + 4), 44100);
    check("  listener: SetDistanceFactor(3.048), SetRolloffFactor, SetDopplerFactor(0)",
          M(lis, LSetDistanceFactor, F(3.048f), 0) | M(lis, LSetRolloffFactor, F(1.0f), 0) | M(lis, LSetDopplerFactor, 0, 0), 0);
    make(0x116, 1, 22050, 16, 4096, &code);
    check("hardware probe (STATIC | LOCHARDWARE | CTRL3D) fails: software only", code, 0x88780032);

    /* a 2D buffer: 22,050 Hz mono 16-bit, constant 0.5 */
    uint32_t b = make(0xE0 | 0x8000, 1, 22050, 16, 44100, &code);
    check("secondary buffer (CTRLVOLUME|PAN|FREQUENCY, GLOBALFOCUS)", code, 0);
    fill16(b, 44100, half);
    M(b, Play, 0, 0, 0);
    mix(441);
    near("mix: 0.5 in both channels", out[0], 0.5);
    near("  right", out[1], 0.5);
    check("  play cursor after 10 ms at 22,050 Hz: 440 bytes", cursor(b), 440);
    check("  write cursor ~10 ms ahead", rd(wcur), 440 + 220 * 2);
    M(b, SetVolume, (uint32_t)-600);
    mix(1);
    near("SetVolume(-600): 0.5 x 10^(-0.3)", out[0], 0.5 * pow(10, -0.3));
    M(b, SetVolume, 0); M(b, SetPan, 10000);
    mix(1);
    near("SetPan(10000): left silent", out[0], 0);
    near("  right full", out[1], 0.5);
    M(b, SetPan, (uint32_t)-600);
    mix(1);
    near("SetPan(-600): right attenuated 6 dB", out[1], 0.5 * pow(10, -0.3));
    M(b, SetPan, 0);
    M(b, SetCurrentPosition, 0);
    M(b, SetFrequency, 44100);
    mix(441);
    check("SetFrequency(44100): 441 frames in 10 ms (882 bytes)", cursor(b), 882);
    M(b, SetFrequency, 0);
    mix(44100);
    uint32_t st = halopad_heap_alloc(4, 1);
    M(b, GetStatus, st);
    check("a one-shot buffer stops at its end", rd(st) & 1, 0);
    check("  and rewinds to 0", cursor(b), 0);
    M(b, Play, 0, 0, 1);
    M(b, GetStatus, st);
    check("Play(LOOPING): PLAYING | LOOPING | LOCSOFTWARE", rd(st), 0x15);
    mix(44100); mix(1000);
    M(b, GetStatus, st);
    check("  still playing past its end", rd(st) & 1, 1);
    M0(b, Stop);
    /* resampling: a ramp at 22,050 Hz, output frame k reads source frame k/2 */
    fill16(b, 44100, ramp);
    M(b, SetCurrentPosition, 0);
    M(b, Play, 0, 0, 0);
    mix(8);
    near("resampling: frame 3 is between source frames 1 and 2", out[6], 150.0 / 32768);
    near("  frame 4 is source frame 2", out[8], 200.0 / 32768);
    M0(b, Stop);
    /* focus: a buffer without GLOBALFOCUS is silent while nothing of ours is in front */
    uint32_t nf = make(0x80, 1, 22050, 16, 4410, &code);
    fill16(nf, 4410, half);
    M(nf, Play, 0, 0, 0);
    mix(441);
    near("without GLOBALFOCUS in the background: silent", out[0], 0);
    check("  but still playing", cursor(nf), 440);
    M0(nf, Release);

    /* 3D */
    uint32_t b3 = make(0x10 | 0x80 | 0x20 | 0x8000, 1, 44100, 16, 44100, &code);
    check("3D buffer (mono)", code, 0);
    make(0x10, 2, 44100, 16, 4000, &code);
    check("a stereo 3D buffer is invalid", code, 0x80070057);
    uint32_t p3 = halopad_heap_alloc(4, 1);
    check("QueryInterface(IDirectSound3DBuffer)", M(b3, QI, bytes(iid_3dbuf, 16), p3), 0);
    uint32_t d3 = rd(p3);
    check("QueryInterface(IKsPropertySet) (EAX): E_NOINTERFACE", M(b3, QI, bytes(iid_ks, 16), p3), 0x80004002);
    fill16(b3, 44100, half);
    M(b3, Play, 0, 0, 1);
    M(d3, SetPosition3, 0, 0, F(1), 0);
    mix(1);
    near("3D: ahead at min distance: full in both", out[0], 0.5);
    near("  right", out[1], 0.5);
    M(d3, SetPosition3, F(10), 0, 0, 0);
    mix(1);
    near("3D: 10 to the right: left silent", out[0], 0);
    near("  right 0.5 x 1/(1 + 9)", out[1], 0.05);
    M(d3, SetPosition3, F(-2), 0, 0, 0);
    mix(1);
    near("3D: 2 to the left: left 0.5 x 1/2", out[0], 0.25);
    near("  right silent", out[1], 0);
    M(d3, SetPosition3, 0, 0, F(1), 0);
    M(d3, SetPosition3, 0, 0, F(5), 1);                              /* DS3D_DEFERRED */
    mix(1);
    near("deferred position: unchanged before commit", out[0], 0.5);
    M0(lis, LCommit);
    mix(1);
    near("  after CommitDeferredSettings: 0.5 x 1/(1 + 4)", out[0], 0.1);
    M(d3, SetMinDistance, F(2), 1);
    M(d3, SetPosition3, 0, 0, F(4), 0);                               /* immediate while a change is pending */
    M0(lis, LCommit);
    mix(1);
    near("  an immediate change survives the commit: 0.5 x 2/(2 + 2)", out[0], 0.25);
    M(d3, SetMinDistance, F(1), 0);
    M(lis, LSetPosition, F(100), 0, 0, 0);
    M(d3, SetMode, 1, 0);                                            /* HEADRELATIVE */
    M(d3, SetPosition3, 0, 0, F(1), 0);
    mix(1);
    near("head-relative: the listener's position does not matter", out[0], 0.5);
    M(d3, SetMode, 0, 0);
    M(lis, LSetPosition, 0, 0, 0, 0);
    M(d3, SetConeAngles, 90, 90, 0);
    M(d3, SetConeOutsideVolume, (uint32_t)-10000, 0);
    M(d3, SetConeOrientation, 0, 0, F(1), 0);
    mix(1);
    near("cone facing away: outside volume (silent)", out[0], 0);
    M(d3, SetConeOrientation, 0, 0, F(-1), 0);
    mix(1);
    near("  facing the listener: full", out[0], 0.5);
    M(d3, SetConeAngles, 360, 360, 0);
    M(lis, LSetDistanceFactor, F(1), 0);
    M(lis, LSetDopplerFactor, F(1), 0);
    M(d3, SetVelocity3, 0, 0, F(343.3f), 0);                          /* moving away at the speed of sound */
    M(b3, SetCurrentPosition, 0);
    mix(441);
    check("Doppler: moving away at c halves the pitch (441 frames -> 440 bytes)", cursor(b3), 440);
    M(lis, LSetDopplerFactor, 0, 0);

    /* duplicates, lock wrap-around, errors */
    uint32_t pdup = halopad_heap_alloc(4, 1);
    check("DuplicateSoundBuffer", M(ds, Duplicate, b, pdup), 0);
    uint32_t dup = rd(pdup);
    fill16(b, 44100, half);
    M0(b3, Stop);
    M(dup, Play, 0, 0, 0);
    mix(1);
    near("  the duplicate plays the original's data", out[0], 0.5);
    check("Lock across the end: two regions", M(b, Lock, 44096, 8, lk[0], lk[1], lk[2], lk[3], 0) == 0 && rd(lk[1]) == 4 && rd(lk[3]) == 4, 1);
    make(0x80, 1, 22050, 24, 1000, &code);
    check("24-bit PCM: DSERR_BADFORMAT", code, 0x88780064);
    uint32_t plain = make(0, 1, 22050, 16, 1000, &code);
    check("SetVolume without CTRLVOLUME: DSERR_CONTROLUNAVAIL", M(plain, SetVolume, 0), 0x8878001E);
    M0(plain, Release);

    M0(dup, Release);
    check("Release the 3D buffer interface", M0(d3, Release) > 0, 1);
    M0(b3, Release);
    M0(b, Release);
    M0(lis, Release);
    check("Release the primary buffer", M0(prim, Release), 0);
    check("Release IDirectSound8", M0(ds, Release), 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

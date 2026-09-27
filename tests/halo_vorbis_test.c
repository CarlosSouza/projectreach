/* vorbisfile test (G4): an Ogg Vorbis stream made by libvorbis's reference encoder (tools/vorbis_encode.c), decoded through HaloPad's
 * vorbisfile.dll with Halo's own memory-reader callbacks (read 0x5481a0, seek 0x5481f0,
 * close 0x548210, tell 0x548230, over its {pos, data, size, eof} reader), the way its
 * sound code does (0x548300/0x548620), compared sample by sample with ffmpeg's own
 * native decoder (an independent implementation: float rounding may differ by a count or
 * two), and each channel checked against the tone it should carry.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_vorbis_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-70s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint8_t *run(const char *cmd, size_t *n)
{
    FILE *p = popen(cmd, "r");
    if (!p) return NULL;
    size_t cap = 1 << 20, len = 0;
    uint8_t *b = malloc(cap);
    size_t r;
    while ((r = fread(b + len, 1, cap - len, p)) > 0) { len += r; if (len == cap) b = realloc(b, cap *= 2); }
    pclose(p);
    *n = len;
    return b;
}
static uint32_t ov_open, ov_read, ov_clear, ov_crosslap;
static uint32_t reader(uint32_t data, uint32_t size)          /* Halo's reader: pos, data, size, eof */
{
    uint32_t r = halopad_heap_alloc(16, 1);
    memcpy(halopad_guest_ptr(r), (uint32_t[]){0, data, size, 0}, 16);
    return r;
}
static uint32_t open_stream(uint32_t rd, uint32_t vf)
{
    uint32_t a[8] = {rd, vf, 0, 0, 0x5481a0, 0x5481f0, 0x548210, 0x548230};
    return halopad_call_guest_ex(ov_open, 8, a, 0, 0);
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
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;

    /* 2 s of stereo 22,050 Hz (440 Hz left, 660 Hz right), made by libvorbis's reference encoder */
    const char *root = getenv("HALOPAD_REPO_ROOT");
    char cmd[4096];
    snprintf(cmd, sizeof cmd, "clang -O2 -w -I%s/generated/xiph/include -I%s/ref/xiph/ogg/include -I%s/ref/xiph/vorbis/include -I%s/ref/xiph/vorbis/lib "
             "%s/tools/vorbis_encode.c %s/ref/xiph/vorbis/lib/vorbisenc.c %s/generated/xiph/arm64-apple-macosx14.0.0/libhalopad-xiph.a "
             "-o /tmp/halopad-vorbis-encode && /tmp/halopad-vorbis-encode > /tmp/halopad-vorbis-test.ogg",
             root, root, root, root, root, root, root);
    system(cmd);
    size_t on = 0, rn = 0;
    uint8_t *ogg = run("cat /tmp/halopad-vorbis-test.ogg", &on);
    uint8_t *ref = run("/opt/homebrew/bin/ffmpeg -loglevel error -c:a vorbis -i /tmp/halopad-vorbis-test.ogg -f s16le -", &rn);
    check("the reference encoder made a stream; ffmpeg decoded it for comparison", on > 1000 && rn > 100000, 1);

    uint32_t dll = LoadLibraryA_c(str("vorbisfile.dll"));
    ov_open = GetProcAddress_c(dll, str("ov_open_callbacks")); ov_read = GetProcAddress_c(dll, str("ov_read"));
    ov_clear = GetProcAddress_c(dll, str("ov_clear")); ov_crosslap = GetProcAddress_c(dll, str("ov_crosslap"));
    uint32_t data = halopad_heap_alloc((uint32_t)on, 0);
    memcpy(halopad_guest_ptr(data), ogg, on);
    uint32_t vf = halopad_heap_alloc(720, 1), vf2 = halopad_heap_alloc(720, 1);
    check("ov_open_callbacks with Halo's reader callbacks", open_stream(reader(data, (uint32_t)on), vf), 0);

    /* read it all as Halo does: 16-bit signed little-endian */
    uint32_t buf = halopad_heap_alloc(4096, 0), bs = halopad_heap_alloc(4, 1);
    uint8_t *pcm = malloc(rn + 65536);
    size_t got = 0;
    for (;;) {
        uint32_t a[7] = {vf, buf, 4096, 0, 2, 1, bs};
        int32_t r = (int32_t)halopad_call_guest_ex(ov_read, 7, a, 0, 0);
        if (r <= 0) { check("ov_read ends with 0 (end of stream)", (uint32_t)r, 0); break; }
        memcpy(pcm + got, halopad_guest_ptr(buf), (size_t)r);
        got += (size_t)r;
        if (got > rn + 32768) break;
    }
    check("as many samples as ffmpeg decodes", (uint32_t)got, (uint32_t)rn);
    int maxd = 0;
    size_t n = (got < rn ? got : rn) / 2;
    for (size_t i = 0; i < n; i++) {
        int d = abs(((int16_t *)pcm)[i] - ((int16_t *)ref)[i]);
        if (d > maxd) maxd = d;
    }
    printf("    (largest difference from ffmpeg's decoder: %d of 32768)\n", maxd);
    check("every sample within 2 counts of ffmpeg's decoder", maxd <= 2, 1);
    /* each channel carries its tone: amplitude 0.125 x 32768 = 4096 by correlation */
    for (int ch = 0; ch < 2; ch++) {
        double f = ch ? 660.0 : 440.0, s = 0, c = 0;
        size_t frames = got / 4, from = 2000, to = frames - 2000;
        for (size_t i = from; i < to; i++) {
            double x = ((int16_t *)pcm)[2 * i + (size_t)ch], ph = 2 * M_PI * f * (double)i / 22050.0;
            s += x * sin(ph); c += x * cos(ph);
        }
        double amp = 2 * sqrt(s * s + c * c) / (double)(to - from);
        char what[96];
        snprintf(what, sizeof what, "  channel %d carries %.0f Hz at amplitude ~4096 (%.0f)", ch, f, amp);
        check(what, fabs(amp - 4096) < 60, 1);
    }

    /* a second stream on the same data, crossfaded into, as for looping sounds */
    check("ov_open_callbacks on a second structure", open_stream(reader(data, (uint32_t)on), vf2), 0);
    uint32_t a1[7] = {vf, buf, 4096, 0, 2, 1, bs};
    halopad_call_guest_ex(ov_read, 7, a1, 0, 0);
    uint32_t cl[2] = {vf, vf2};
    check("ov_crosslap(first, second)", halopad_call_guest_ex(ov_crosslap, 2, cl, 0, 0), 0);
    check("ov_clear both", halopad_call_guest_ex(ov_clear, 1, &vf, 0, 0) | halopad_call_guest_ex(ov_clear, 1, &vf2, 0, 0), 0);
    uint32_t junk = halopad_heap_alloc(4096, 1);
    check("a stream that is not Vorbis: OV_ENOTVORBIS", open_stream(reader(junk, 4096), vf), (uint32_t)-132);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

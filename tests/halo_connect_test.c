/* Join test (G5, step 1): Halo on HaloPad joins the original Custom Edition 1.10 dedicated server.
 *
 * Run by scripts/reference-join.sh, which starts the reference server (haloceded.exe in the
 * project's CrossOver bottle, private, 127.0.0.1:2310) and checks the server's own log for the
 * join. Halo takes the address from its command line (-connect, handled by main's 0x4cd080 ->
 * 0x4cb800). What WinMain sets up before main is set here as in the other component tests:
 * its -cport value (0x544c93; 2305, since the server holds 127.0.0.1:2302-2303) and the key
 * string from Halo's own 0x5829e0 (0x544c21). This machine has no DigitalProductID, so that is
 * Halo's empty string, and the private server accepts it (its log records the MD5 of ""); WinMain
 * itself would stop there with "Your product key is invalid". Nothing is written or made up.
 * Checks: the server's map loads, the server spawns the player's unit, no dialog; frame 900 is
 * saved as join.ppm. */
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
static int QUIT_AFTER = 1200;
static int frames, map_frame;
static const char *expect_map = "bloodgulch";     /* HALOPAD_TEST_MAP: the map the server runs */
/* HALOPAD_TEST_VIA=console: no -connect; at frame 300 the test opens Halo's console (the grave
   key) and types "connect <server>" and Enter, the keys and characters the iOS app's Join Server
   sends (port/ios/HaloPadOverlay.m) */
static const char *via_console;
/* typed keys wait in a queue: each key goes down on one frame and up two frames later, so Halo's
   once-a-frame keyboard read sees it */
static hp_input typed[512];
static int ntyped, next_typed;
static void queue_key(uint32_t vk, uint32_t side, uint32_t scan, int down, uint16_t ch)
{
    if (ntyped >= 512) return;
    hp_input e = {0};
    e.kind = HPI_KEY; e.vk = vk; e.side_vk = side ? side : vk; e.scan = scan; e.down = down;
    if (down && ch) { e.chars[0] = ch; e.nchars = 1; }
    typed[ntyped++] = e;
}
static void key_char(uint32_t vk, uint32_t scan, int shift, uint16_t ch)
{
    if (shift) queue_key(0x10, 0xA0, 0x2a, 1, 0);
    queue_key(vk, 0, scan, 1, ch);
    queue_key(vk, 0, scan, 0, 0);
    if (shift) queue_key(0x10, 0xA0, 0x2a, 0, 0);
}
static void type_text(const char *s)
{
    static const char row1[] = "1234567890", q[] = "qwertyuiop", a[] = "asdfghjkl", z[] = "zxcvbnm";
    for (; *s; s++) {
        const char *p;
        char c = *s;
        if ((p = strchr(row1, c))) key_char((uint32_t)c, 0x02 + (uint32_t)(p - row1), 0, (uint16_t)c);
        else if ((p = strchr(q, c))) key_char((uint32_t)(c - 32), 0x10 + (uint32_t)(p - q), 0, (uint16_t)c);
        else if ((p = strchr(a, c))) key_char((uint32_t)(c - 32), 0x1e + (uint32_t)(p - a), 0, (uint16_t)c);
        else if ((p = strchr(z, c))) key_char((uint32_t)(c - 32), 0x2c + (uint32_t)(p - z), 0, (uint16_t)c);
        else if (c == ' ') key_char(0x20, 0x39, 0, ' ');
        else if (c == '.') key_char(0xBE, 0x34, 0, '.');
        else if (c == ':') key_char(0xBA, 0x27, 1, ':');
        else if (c == '"') key_char(0xDE, 0x28, 1, '"');
        else if (c == '\n') key_char(0x0D, 0x1c, 0, '\r');
    }
}
static uint32_t unit_seen;
static uint16_t u16(uint32_t g) { uint16_t v; memcpy(&v, halopad_guest_ptr(g), 2); return v; }
/* the player's unit: players table 0x815920, object table 0x7fb710 (see tests/halo_play_test.c) */
static uint32_t player_unit(void)
{
    uint32_t pt = rd(0x815920), ot = rd(0x7fb710);
    if (!pt || !ot) return 0;
    uint32_t h = rd(rd(pt + 0x34) + 0x34);
    if (h == 0xffffffff || (h & 0xffff) >= u16(ot + 0x20)) return 0;
    uint32_t e = rd(ot + 0x34) + (h & 0xffff) * 12;
    return u16(e) == h >> 16 ? rd(e + 8) : 0;
}
static const char *save_name = "join.ppm";
static void save(uint32_t device)
{
    uint32_t w = 800, h = 600, *img = malloc(w * h * 4);
    halopad_metal_read_image(halopad_d3d9_device_target(device), img, w, h);
    const char *reg = getenv("HALOPAD_REGISTRY");
    if (reg && strrchr(reg, '/')) {
        char path[1200];
        snprintf(path, sizeof path, "%.*s/%s", (int)(strrchr(reg, '/') - reg), reg, save_name);
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
    if (!map_frame && !strcasecmp((const char *)halopad_guest_ptr(0x643064), expect_map)) map_frame = frames;
    uint32_t u = player_unit();
    if (u && map_frame) unit_seen = u;
    if (frames == 900) save(device);
    if (via_console && frames == 300) {
        key_char(0xC0, 0x29, 0, 0);                              /* Halo's console */
        char line[160];
        snprintf(line, sizeof line, "connect %s \"\"\n", via_console);     /* Halo's connect: address and password */
        type_text(line);
        printf("    frame 300: typed \"%s\" into Halo's console\n", line);
    }
    if (via_console && frames == 480) { save_name = "console-typing.ppm"; save(device); save_name = "join.ppm"; }
    if (via_console && frames == 1500) save(device);
    if (next_typed < ntyped && frames % 2 == 0) halopad_input_event(&typed[next_typed++]);
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

    const char *server = getenv("HALOPAD_TEST_SERVER") ? getenv("HALOPAD_TEST_SERVER") : "127.0.0.1:2310";
    if (getenv("HALOPAD_TEST_MAP")) expect_map = getenv("HALOPAD_TEST_MAP");
    char args[128];
    if (getenv("HALOPAD_TEST_VIA") && !strcmp(getenv("HALOPAD_TEST_VIA"), "console")) { via_console = server; QUIT_AFTER = 2100; }
    if (via_console) snprintf(args, sizeof args, "-console");     /* Halo's console needs its -console switch */
    else snprintf(args, sizeof args, "-connect %s", server);
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
    wr(0x6337fc, 2305);
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
    printf("    joined %s: the server's map loaded at frame %d\n", server, map_frame);
    printf("    expected map \"%s\"; the map in memory is \"%s\"\n", expect_map, (const char *)halopad_guest_ptr(0x643064));
    check("  the server's map loaded through the connection", map_frame > 0, 1);
    check("  the server spawned the player's unit", unit_seen != 0, 1);
    check("  no dialog", (uint32_t)dialogs, 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

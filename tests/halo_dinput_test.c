/* DirectInput 8 test (G9): the system keyboard and mouse the way Halo sets them up, with
 * Halo's own c_dfDIKeyboard (0x5ec4ec) and c_dfDIMouse2 (0x5ec6f4) from the image, driven by
 * host input through halopad_input_event. Methods are called through the guest vtables.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_dinput_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_input.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
extern int halopad_host_input_off;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t bytes(const void *p, uint32_t n) { uint32_t g = halopad_heap_alloc(n, 0); memcpy(halopad_guest_ptr(g), p, n); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-64s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}
#define M(obj, index, ...) method(obj, index, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define M0(obj, index) method(obj, index, 0, NULL)
static uint32_t user32;
static uint32_t api(const char *name, uint32_t n, const uint32_t *args) { return halopad_call_guest(GetProcAddress_c(user32, str(name)), n, args); }
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
static void input(hp_input e) { halopad_input_event(&e); }
static void key(uint32_t scan, int ext, int down) { input((hp_input){.kind = HPI_KEY, .vk = 'A', .scan = scan, .extended = ext, .down = down}); }

enum { Release = 2, CreateDevice = 3, EnumDevices = 4,
       GetProperty = 5, SetProperty = 6, Acquire = 7, Unacquire = 8, GetDeviceState = 9, GetDeviceData = 10,
       SetDataFormat = 11, SetCooperativeLevel = 13, Poll = 25 };

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
    halopad_host_input_off = 1;
    user32 = LoadLibraryA_c(str("user32.dll"));

    /* a foreground window, as Halo's is when it sets up input */
    uint32_t wc = halopad_heap_alloc(48, 1);
    uint32_t wcv[12] = {48, 0, GetProcAddress_c(user32, str("DefWindowProcA")), 0, 0, 0x400000, 0, 0, 0, 0, str("HaloPadDITest"), 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    API("RegisterClassExA", wc);
    uint32_t hwnd = API("CreateWindowExA", 0, str("HaloPadDITest"), str("DI"), 0x00CF0000, 0, 0, 648, 507, 0, 0, 0x400000, 0);
    API("ShowWindow", hwnd, 5);

    uint32_t dinput8 = LoadLibraryA_c(str("dinput8.dll"));
    uint32_t create = GetProcAddress_c(dinput8, str("DirectInput8Create"));
    check("DirectInput8Create is exported", create != 0, 1);
    static const uint8_t iid[16] = {0x30, 0x80, 0x79, 0xBF, 0x3A, 0x48, 0xA2, 0x4D, 0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00};
    static const uint8_t kbd[16] = {0x61, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
    static const uint8_t mouse[16] = {0x60, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
    static const uint8_t other[16] = {0x62, 0x2B, 0x1D, 0x6F, 0xA0, 0xD5, 0xCF, 0x11, 0xBF, 0xC7, 0x44, 0x45, 0x53, 0x54, 0x00, 0x00};
    uint32_t giid = bytes(iid, 16), gkbd = bytes(kbd, 16), gmouse = bytes(mouse, 16), gother = bytes(other, 16);
    uint32_t pdi = halopad_heap_alloc(4, 1);
    check("DirectInput8Create(0x0700) is an old version", halopad_call_guest(create, 5, (uint32_t[]){0x400000, 0x700, giid, pdi, 0}), 0x8007047E);
    check("DirectInput8Create(0x0800, IID_IDirectInput8A)", halopad_call_guest(create, 5, (uint32_t[]){0x400000, 0x800, giid, pdi, 0}), 0);
    uint32_t di = rd(pdi);
    uint32_t pk = halopad_heap_alloc(4, 1), pm = halopad_heap_alloc(4, 1);
    check("CreateDevice(GUID_SysKeyboard)", M(di, CreateDevice, gkbd, pk, 0), 0);
    check("CreateDevice(GUID_SysMouse)", M(di, CreateDevice, gmouse, pm, 0), 0);
    check("CreateDevice(an unknown GUID) is not registered", M(di, CreateDevice, gother, pk + 0, 0) == 0x80040154 && rd(pk) == 0, 1);
    M(di, CreateDevice, gkbd, pk, 0);
    uint32_t k = rd(pk), m = rd(pm);
    check("EnumDevices(GAMECTRL, ATTACHEDONLY) finds none yet", M(di, EnumDevices, 4, 0x494b30, 0, 1), 0);

    /* keyboard, as Halo sets it up */
    check("keyboard: SetCooperativeLevel(EXCLUSIVE|NONEXCLUSIVE) is invalid", M(k, SetCooperativeLevel, hwnd, 3 | 4), 0x80070057);
    check("keyboard: SetCooperativeLevel(NONEXCLUSIVE|FOREGROUND|NOWINKEY)", M(k, SetCooperativeLevel, hwnd, 0x16), 0);
    check("keyboard: Acquire before SetDataFormat is invalid", M0(k, Acquire), 0x80070057);
    check("keyboard: SetDataFormat(Halo's c_dfDIKeyboard)", M(k, SetDataFormat, 0x5ec4ec), 0);
    uint32_t prop = bytes((uint32_t[]){20, 16, 0, 0, 32}, 20);
    check("keyboard: SetProperty(DIPROP_BUFFERSIZE, 32)", M(k, SetProperty, 1, prop), 0);
    uint32_t od = halopad_heap_alloc(20 * 64, 1), n = halopad_heap_alloc(4, 1);
    memcpy(halopad_guest_ptr(n), (uint32_t[]){1}, 4);
    check("keyboard: GetDeviceData before Acquire: DIERR_NOTACQUIRED", M(k, GetDeviceData, 20, od, n, 0), 0x8007000C);
    check("keyboard: Acquire", M0(k, Acquire), 0);
    check("keyboard: Acquire again: DI_NOEFFECT", M0(k, Acquire), 1);
    key(0x1E, 0, 1); key(0x1E, 0, 1); key(0x1E, 0, 1);            /* A held: autorepeat */
    uint32_t st = halopad_heap_alloc(256, 1);
    M(k, GetDeviceState, 256, st);
    check("keyboard: GetDeviceState: DIK_A down", ((uint8_t *)halopad_guest_ptr(st))[0x1E], 0x80);
    key(0x1E, 0, 0);
    key(0x1D, 1, 1);                                                /* right Ctrl: DIK_RCONTROL 0x9D */
    uint32_t got[3][4], ok = 1;
    for (int i = 0; i < 3; i++) {
        memcpy(halopad_guest_ptr(n), (uint32_t[]){1}, 4);
        ok &= M(k, GetDeviceData, 20, od, n, 0) == 0 && rd(n) == 1;
        for (int j = 0; j < 4; j++) got[i][j] = rd(od + 4 * j);
    }
    check("keyboard: buffered events one at a time, repeats dropped", ok, 1);
    check("  A down, A up, RCONTROL down", got[0][0] == 0x1E && got[0][1] == 0x80 && got[1][0] == 0x1E && got[1][1] == 0
          && got[2][0] == 0x9D && got[2][1] == 0x80, 1);
    check("  sequence numbers increase", got[0][3] < got[1][3] && got[1][3] < got[2][3], 1);
    memcpy(halopad_guest_ptr(n), (uint32_t[]){1}, 4);
    check("  then empty: DI_OK, 0 events", M(k, GetDeviceData, 20, od, n, 0) == 0 && rd(n) == 0, 1);
    key(0x1D, 1, 0);
    for (int i = 0; i < 20; i++) { key(0x10, 0, 1); key(0x10, 0, 0); }   /* 41 events into a buffer of 32 */
    memcpy(halopad_guest_ptr(n), (uint32_t[]){1}, 4);
    check("overflow: DI_BUFFEROVERFLOW", M(k, GetDeviceData, 20, od, n, 0), 1);
    memcpy(halopad_guest_ptr(n), (uint32_t[]){0xFFFFFFFF}, 4);
    check("  flush (NULL, INFINITE) removes the other 31", M(k, GetDeviceData, 20, 0, n, 0) == 0 && rd(n) == 31, 1);

    /* losing the foreground */
    input((hp_input){.kind = HPI_ACTIVATE, .down = 0});
    memcpy(halopad_guest_ptr(n), (uint32_t[]){1}, 4);
    check("after deactivation: DIERR_INPUTLOST", M(k, GetDeviceData, 20, od, n, 0), 0x8007001E);
    check("  then DIERR_NOTACQUIRED", M(k, GetDeviceData, 20, od, n, 0), 0x8007000C);
    check("  Acquire in the background: DIERR_OTHERAPPHASPRIO", M0(k, Acquire), 0x80070005);
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1});
    check("  Acquire once active again", M0(k, Acquire), 0);

    /* mouse, as Halo sets it up */
    check("mouse: SetCooperativeLevel(EXCLUSIVE|FOREGROUND)", M(m, SetCooperativeLevel, hwnd, 5), 0);
    check("mouse: SetDataFormat(Halo's c_dfDIMouse2)", M(m, SetDataFormat, 0x5ec6f4), 0);
    uint32_t gp = bytes((uint32_t[]){20, 16, 8, 1, 0}, 20);
    check("mouse: GetProperty(DIPROP_GRANULARITY, wheel by offset)", M(m, GetProperty, 3, gp) == 0 && rd(gp + 16) == 120, 1);
    check("mouse: Acquire", M0(m, Acquire), 0);
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 10, .y = 10, .dx = 3, .dy = -2});
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 10, .y = 10, .dx = 3, .dy = -2});
    input((hp_input){.kind = HPI_WHEEL, .x = 10, .y = 10, .wheel = 120});
    input((hp_input){.kind = HPI_BUTTON, .x = 10, .y = 10, .button = 1, .down = 1});
    uint32_t ms = halopad_heap_alloc(20, 1);
    check("mouse: GetDeviceState(DIMOUSESTATE2)", M(m, GetDeviceState, 20, ms), 0);
    check("  lX 6, lY -4, lZ 120, right button down", rd(ms) == 6 && rd(ms + 4) == (uint32_t)-4 && rd(ms + 8) == 120
          && ((uint8_t *)halopad_guest_ptr(ms))[13] == 0x80, 1);
    M(m, GetDeviceState, 20, ms);
    check("  relative: the next read is 0, 0, 0 with the button still down", rd(ms) == 0 && rd(ms + 4) == 0 && rd(ms + 8) == 0
          && ((uint8_t *)halopad_guest_ptr(ms))[13] == 0x80, 1);
    check("mouse: GetDeviceState of the wrong size is invalid", M(m, GetDeviceState, 16, ms), 0x80070057);
    check("mouse: GetDeviceData unbuffered: DIERR_NOTBUFFERED", M(m, GetDeviceData, 20, od, n, 0), 0x80040207);
    check("mouse: Poll on an interrupt device: DI_NOEFFECT", M0(m, Poll), 1);
    check("mouse: Unacquire", M0(m, Unacquire), 0);

    check("Release the mouse", M0(m, Release), 0);
    check("Release the keyboard", M0(k, Release), 0);
    check("Release IDirectInput8", M0(di, Release), 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

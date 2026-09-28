/* DirectInput 8 test (G9): the system keyboard and mouse the way Halo sets them up, with
 * Halo's own c_dfDIKeyboard (0x5ec4ec) and c_dfDIMouse2 (0x5ec6f4) from the image, driven by
 * host input through halopad_input_event. Methods are called through the guest vtables.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_dinput_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <TargetConditionals.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_input.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
extern int halopad_host_input_off;
extern hp_gamepad halopad_gamepad_test[4];
extern int halopad_gamepad_test_count;
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
void halopad_host_pump(void);
static void queued_input(hp_input e)
{
#if TARGET_OS_IPHONE
    halopad_host_post_input(&e);
#else
    input(e);
#endif
}

static void touch_button(int button, int down)
{ queued_input((hp_input){.kind = HPI_BUTTON, .flags = HPI_TOUCH, .button = button, .down = down}); }
static void pump_many(void) { for (int i = 0; i < 8; i++) halopad_host_pump(); }

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
    halopad_gamepad_test_count = 0;                                 /* no controllers yet */
    check("EnumDevices(GAMECTRL, ATTACHEDONLY) with no controllers: DI_OK", M(di, EnumDevices, 4, 0x494b30, 0, 1), 0);

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

    /* Exercise Halo's translated keyboard consumer, not just DirectInput's queue.
       0x493520 drains buffered events and defers a same-update release until the
       next update. The table at 0x5fa358 maps DIK offsets to Halo key indices. */
    uint32_t saved_keyboard = rd(0x64c730);
    uint8_t saved_enabled = *(uint8_t *)halopad_guest_ptr(0x64c528);
    memcpy(halopad_guest_ptr(0x64c730), &k, 4);
    *(uint8_t *)halopad_guest_ptr(0x64c528) = 1;
    const uint32_t quick_scans[] = {0x39, 0x13, 0x12, 0x21, 0x0f, 0x2c, 0x1d, 0x10, 0x22, 0x3b, 0x01};
#if TARGET_OS_IPHONE
    halopad_host_input_off = 0;
#endif
    for (uint32_t i = 0; i < sizeof quick_scans / sizeof *quick_scans; i++) {
        uint32_t scan = quick_scans[i];
        uint16_t index;
        memcpy(&index, halopad_guest_ptr(0x5fa358 + scan * 2), 2);
        check("overlay key has a valid Halo key index", index < 109, 1);
        if (index >= 109) continue;
        queued_input((hp_input){.kind = HPI_KEY, .flags = HPI_TOUCH, .vk = 'A', .scan = scan, .down = 1});
        queued_input((hp_input){.kind = HPI_KEY, .flags = HPI_TOUCH, .vk = 'A', .scan = scan, .down = 0});
        pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        char label[128];
        snprintf(label, sizeof label, "Halo preserves short key scan %02x after eight host pumps", scan);
        check(label, *(uint8_t *)halopad_guest_ptr(0x64c550 + index), 1);
        check("Halo records its deferred release", *(uint8_t *)halopad_guest_ptr(0x64c5bd + index), 1);
        halopad_call_guest(0x493520, 0, NULL);
        check("next Halo update releases the short key", *(uint8_t *)halopad_guest_ptr(0x64c550 + index), 0);
        halopad_call_guest(0x493520, 0, NULL);
        check("later Halo update does not replay it", *(uint8_t *)halopad_guest_ptr(0x64c550 + index), 0);
    }
    /* Native UI cancellation must also reach the buffered keyboard consumer. */
    uint16_t jump_index;
    memcpy(&jump_index, halopad_guest_ptr(0x5fa358 + 0x39 * 2), 2);
    uint8_t *jump_state = halopad_guest_ptr(0x64c550 + jump_index);
    hp_input jump = {.kind = HPI_KEY, .flags = HPI_TOUCH, .vk = 0x20, .side_vk = 0x20, .scan = 0x39, .down = 1};
    for (int pumped = 0; pumped < 2; pumped++) {
        jump.down = 1; queued_input(jump);
        jump.down = 0; queued_input(jump);
        if (pumped) pump_many();
        queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        check(pumped ? "cancel removes already-buffered touch key tap" : "cancel removes host-queued touch key tap", *jump_state, 0);
        halopad_call_guest(0x493520, 0, NULL);
    }
    jump.down = 1; queued_input(jump); pump_many();
    halopad_call_guest(0x493520, 0, NULL);
    check("held touch key reaches Halo", *jump_state, 1);
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    halopad_call_guest(0x493520, 0, NULL);
    check("cancel releases touch key already read by Halo", *jump_state, 0);
    check("cancel clears USER32 touch key", API("GetAsyncKeyState", 0x20) & 0x8000, 0);
    for (int physical_first = 0; physical_first < 2; physical_first++) {
        jump.flags = physical_first ? 0 : HPI_TOUCH; jump.down = 1; queued_input(jump);
        jump.flags = physical_first ? HPI_TOUCH : 0; queued_input(jump); pump_many();
        queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        check("cancel preserves physical key in either ownership order", *jump_state != 0, 1);
        check("cancel preserves physical USER32 key", API("GetAsyncKeyState", 0x20) & 0x8000, 0x8000);
        jump.flags = 0; jump.down = 0; queued_input(jump); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        check("physical release after cancel releases Halo key", *jump_state, 0);
    }
    for (int release_touch = 0; release_touch < 2; release_touch++) {
        jump.flags = 0; jump.down = 1; queued_input(jump);
        jump.flags = HPI_TOUCH; queued_input(jump); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        jump.flags = release_touch ? HPI_TOUCH : 0; jump.down = 0; queued_input(jump); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        check("releasing either key source preserves the other in Halo", *jump_state != 0, 1);
        check("releasing either key source preserves USER32 hold", API("GetAsyncKeyState", 0x20) & 0x8000, 0x8000);
        jump.flags = release_touch ? 0 : HPI_TOUCH; queued_input(jump); pump_many();
        halopad_call_guest(0x493520, 0, NULL);
        check("last key owner releases Halo", *jump_state, 0);
        queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    }
    jump.flags = HPI_TOUCH; jump.down = 1; queued_input(jump); pump_many();
    input((hp_input){.kind = HPI_ACTIVATE, .down = 0});
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1}); M0(k, Acquire);
    halopad_call_guest(0x493520, 0, NULL);
    check("focus loss removes unread virtual key before reacquire", *jump_state, 0);
    /* A physical tap queued beside a canceled virtual tap must still be read. */
    jump.flags = 0; queued_input(jump); jump.down = 0; queued_input(jump);
    jump.flags = HPI_TOUCH; jump.scan = 0x13; jump.vk = 'R'; jump.side_vk = 'R'; jump.down = 1; queued_input(jump);
    jump.down = 0; queued_input(jump); pump_many();
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    halopad_call_guest(0x493520, 0, NULL);
    check("cancel preserves unrelated buffered physical tap", *jump_state, 1);
    halopad_call_guest(0x493520, 0, NULL);
    jump.scan = 0x39; jump.vk = jump.side_vk = 0x20;
    jump.flags = HPI_TOUCH; jump.down = 1; queued_input(jump); pump_many();
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH});
    queued_input(jump); jump.down = 0; queued_input(jump); pump_many();
    halopad_call_guest(0x493520, 0, NULL);
    check("fresh touch tap after cancel is retained", *jump_state, 1);
    halopad_call_guest(0x493520, 0, NULL);
    check("fresh tap releases normally", *jump_state, 0);
#if TARGET_OS_IPHONE
    halopad_host_input_off = 1;
#endif
    memcpy(halopad_guest_ptr(0x64c730), &saved_keyboard, 4);
    *(uint8_t *)halopad_guest_ptr(0x64c528) = saved_enabled;

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

    /* A real UIKit-style tap can reach two host pumps before Halo samples the
       unbuffered mouse. Test the queue, not just a direct handler hold. */
#if TARGET_OS_IPHONE
    halopad_host_input_off = 0;
#endif
    touch_button(0, 1); touch_button(0, 0);
    for (int i = 0; i < 8; i++) halopad_host_pump();
    M(m, GetDeviceState, 20, ms);
    check("touch tap survives eight pumps before mouse state read", ((uint8_t *)halopad_guest_ptr(ms))[12], 0x80);
    M(m, GetDeviceState, 20, ms);
    check("touch tap releases on the following state read", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    touch_button(0, 1); touch_button(0, 1); touch_button(0, 0);
    touch_button(0, 1); touch_button(0, 0); pump_many();
    M(m, GetDeviceState, 16, ms); M(m, GetDeviceState, 20, 0); M0(m, Poll);
    M(k, GetDeviceState, 256, st);
    int sequenceOK = 1;
    for (int i = 0; i < 5; i++) {
        M(m, GetDeviceState, 20, ms);
        sequenceOK &= ((uint8_t *)halopad_guest_ptr(ms))[12] == (i < 4 && !(i & 1) ? 0x80 : 0);
    }
    check("two taps retain edges; duplicates, invalid reads, Poll and keyboard reads do not consume them", sequenceOK, 1);
    touch_button(0, 1); pump_many();
    M(m, GetDeviceState, 20, ms); M(m, GetDeviceState, 20, ms);
    check("held touch stays down after pending edge is read", ((uint8_t *)halopad_guest_ptr(ms))[12], 0x80);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 1});
    touch_button(0, 0); pump_many(); M(m, GetDeviceState, 20, ms);
    check("touch release preserves physical mouse hold", ((uint8_t *)halopad_guest_ptr(ms))[12], 0x80);
    check("USER32 also preserves physical hold", API("GetAsyncKeyState", 1) & 0x8000, 0x8000);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 0});
    M(m, GetDeviceState, 20, ms);
    check("last source release clears mouse", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    touch_button(0, 1); touch_button(0, 0); pump_many();
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    M(m, GetDeviceState, 20, ms);
    check("native menu cancellation discards already-pumped unread tap", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    touch_button(0, 1); touch_button(0, 0);
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    M(m, GetDeviceState, 20, ms);
    check("native menu cancellation discards tap still in host queue", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    touch_button(0, 1); pump_many(); M(m, GetDeviceState, 20, ms);
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many(); M(m, GetDeviceState, 20, ms);
    check("cancellation releases previously-read held touch", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    check("cancellation releases USER32 touch state", API("GetAsyncKeyState", 1) & 0x8000, 0);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 1});
    touch_button(0, 1); pump_many();
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many(); M(m, GetDeviceState, 20, ms);
    check("cancellation leaves physical hold in DirectInput and USER32",
          ((uint8_t *)halopad_guest_ptr(ms))[12] == 0x80 && (API("GetAsyncKeyState", 1) & 0x8000), 1);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 0});
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH});
    touch_button(0, 1); touch_button(0, 0); pump_many();
    M(m, GetDeviceState, 20, ms);
    check("new tap posted after cancellation is retained", ((uint8_t *)halopad_guest_ptr(ms))[12], 0x80);
    M(m, GetDeviceState, 20, ms);
    touch_button(0, 1); touch_button(0, 0); pump_many();
    M0(m, Unacquire); M0(m, Acquire); M(m, GetDeviceState, 20, ms);
    check("reacquire does not replay unread touch taps", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    touch_button(0, 1); touch_button(0, 0); pump_many();
    input((hp_input){.kind = HPI_ACTIVATE, .down = 0});
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1});
    check("focus cycle loses acquisition even with no intervening state read", M(m, GetDeviceState, 20, ms), 0x8007001e);
    M0(m, Acquire); M(m, GetDeviceState, 20, ms);
    check("foreground regain does not replay unread touch taps", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
#if TARGET_OS_IPHONE
    halopad_host_input_off = 1;
#endif
    check("mouse: Unacquire", M0(m, Unacquire), 0);


    /* Buffered clients consume the ordinary event stream, not a second replay. */
    check("buffered mouse setup", M(m, SetProperty, 1, prop), 0);
    M0(m, Acquire);
    input((hp_input){.kind = HPI_BUTTON, .flags = HPI_TOUCH, .button = 0, .down = 1});
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    memcpy(halopad_guest_ptr(n), (uint32_t[]){32}, 4);
    M(m, GetDeviceData, 20, od, n, 0);
    check("buffered touch cancellation supplies down and release", rd(n) == 2 && rd(od) == 12 && rd(od + 4) == 0x80 && rd(od + 20) == 12 && rd(od + 24) == 0, 1);
    M(m, GetDeviceState, 20, ms);
    check("buffered mouse state does not replay consumed tap", ((uint8_t *)halopad_guest_ptr(ms))[12], 0);
    M0(m, Unacquire);

    /* a game controller, set up by Halo's own code: 0x494840 builds its 80-object format at
       0x815400 (32 axes, 16 hats, 32 buttons, all optional) and enumerates game controllers with
       its callback 0x494b30, which creates each device, sets EXCLUSIVE|FOREGROUND and the format,
       reads the capabilities, and through EnumObjects (0x494a10) gives every axis the range
       -4096..4096 and a 10% dead zone */
    halopad_gamepad_test_count = 1;
    halopad_gamepad_test[0] = (hp_gamepad){.id = 7, .lx = 1.0f, .ly = 1.0f, .rx = -0.05f, .ry = 0, .lt = 1.0f, .rt = 0,
                                           .buttons = 1u | 1u << 7, .dpad = 2};   /* stick right and up, A and Start, d-pad right */
    memcpy(halopad_guest_ptr(0x64c52c), &di, 4);                   /* Halo's IDirectInput8 */
    check("Halo's game controller set-up (0x494840) returns TRUE", halopad_call_guest(0x494840, 0, NULL) & 0xFF, 1);
    check("  one controller counted (0x64c774)", rd(0x64c774), 1);
    uint32_t pad = rd(0x64c778);
    check("  its device (0x64c778)", pad != 0, 1);
    {
        const char *want = "Controller (XBOX 360 For Windows)";
        const uint16_t *name = halopad_guest_ptr(0x64c798);       /* Halo keeps it as UTF-16 (0x55ae60) */
        int same = 1;
        for (size_t i = 0; i <= strlen(want); i++) same &= name[i] == (uint8_t)want[i];
        check("  named \"Controller (XBOX 360 For Windows)\", as XP names an Xbox 360 pad", same, 1);
    }
    check("  capabilities Halo keeps: 5 axes, 10 buttons, 1 hat", rd(0x64c798 + 0x234) == 5 && rd(0x64c798 + 0x238) == 10
          && rd(0x64c798 + 0x23c) == 1, 1);
    uint32_t rp = bytes((uint32_t[]){24, 16, 0, 1, 0, 0}, 24);    /* DIPROPRANGE of the axis at offset 0 */
    check("  the range Halo set: -4096..4096", M(pad, GetProperty, 4, rp) == 0 && rd(rp + 16) == (uint32_t)-4096 && rd(rp + 20) == 4096, 1);
    uint32_t dz = bytes((uint32_t[]){20, 16, 4, 1, 0}, 20);
    check("  the dead zone Halo set: 1000 (10%)", M(pad, GetProperty, 5, dz) == 0 && rd(dz + 16) == 1000, 1);
    check("gamepad: GetDeviceState before Acquire: DIERR_NOTACQUIRED", M(pad, GetDeviceState, 224, halopad_heap_alloc(224, 1)), 0x8007000C);
    check("gamepad: Acquire", M0(pad, Acquire), 0);
    check("gamepad: Poll (a polled device)", M0(pad, Poll), 0);
    uint32_t js = halopad_heap_alloc(224, 1);
    memset(halopad_guest_ptr(js), 0x55, 224);
    check("gamepad: GetDeviceState(224 bytes, Halo's format)", M(pad, GetDeviceState, 224, js), 0);
    const uint8_t *jb = halopad_guest_ptr(js);
    check("  X 4096 (stick right), Y -4096 (stick up)", rd(js) == 4096 && rd(js + 4) == (uint32_t)-4096, 1);
    check("  Z 4096 (left trigger), Rx 0 (inside the dead zone), Ry 0", rd(js + 8) == 4096 && rd(js + 12) == 0 && rd(js + 16) == 0, 1);
    check("  unmatched axis entries read 0", rd(js + 0x14) == 0 && rd(js + 0x7C) == 0, 1);
    check("  hat 9000 (right); unmatched hats centred (0xFFFFFFFF)", rd(js + 0x80) == 9000 && rd(js + 0x84) == 0xFFFFFFFF
          && rd(js + 0xBC) == 0xFFFFFFFF, 1);
    check("  buttons: 0 (A) and 7 (Start) down, 1 up, unmatched 0", jb[0xC0] == 0x80 && jb[0xC7] == 0x80 && jb[0xC1] == 0 && jb[0xDF] == 0, 1);
    halopad_gamepad_test[0].lx = 0.5f;                             /* halfway: past the dead zone, rescaled */
    M0(pad, Poll);
    M(pad, GetDeviceState, 224, js);
    check("  half right: (0.5 - 0.1) / 0.9 of 4096 = 1820", rd(js), 1820);
    halopad_gamepad_test_count = 0;                                /* unplugged */
    check("gamepad unplugged: Poll gives DIERR_INPUTLOST", M0(pad, Poll), 0x8007001E);
    check("  then GetDeviceState: DIERR_NOTACQUIRED", M(pad, GetDeviceState, 224, js), 0x8007000C);
    check("  and Acquire: DIERR_UNPLUGGED", M0(pad, Acquire), 0x80040209);
    check("Release the gamepad", M0(pad, Release), 0);
    check("Release the mouse", M0(m, Release), 0);
    check("Release the keyboard", M0(k, Release), 0);
    check("Release IDirectInput8", M0(di, Release), 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

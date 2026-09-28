/* DirectInput 8 test (G9): the system keyboard and mouse the way Halo sets them up, with
 * Halo's own c_dfDIKeyboard (0x5ec4ec) and c_dfDIMouse2 (0x5ec6f4) from the image, driven by
 * host input through halopad_input_event. Methods are called through the guest vtables.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_dinput_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
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
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
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

    /* Opening native UI must discard unread aiming, without swallowing a real mouse. */
    input((hp_input){.kind = HPI_MOUSEMOVE, .flags = HPI_TOUCH, .dx = 90, .dy = -45});
    input((hp_input){.kind = HPI_MOUSEMOVE, .dx = 7, .dy = -3});
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    M(m, GetDeviceState, 20, ms);
    check("cancel discards unread touch look but preserves physical deltas", rd(ms) == 7 && rd(ms + 4) == (uint32_t)-3, 1);
    input((hp_input){.kind = HPI_MOUSEMOVE, .flags = HPI_TOUCH, .dx = 11});
    M(m, GetDeviceState, 20, ms);
    check("fresh look after cancel still reaches the reader", rd(ms), 11);
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    M(m, GetDeviceState, 20, ms);
    check("cancel after a consumed look does not create reverse motion", rd(ms) == 0 && rd(ms + 4) == 0, 1);
#if TARGET_OS_IPHONE
    halopad_host_input_off = 0;
    halopad_host_post_input(&(hp_input){.kind = HPI_KEY, .vk = 'W', .scan = 0x11, .down = 1});
    halopad_host_post_input(&(hp_input){.kind = HPI_KEY, .vk = 'W', .scan = 0x11, .down = 0});
    halopad_host_post_input(&(hp_input){.kind = HPI_MOUSEMOVE, .flags = HPI_TOUCH, .dx = 50});
    halopad_host_pump(); /* leave the look behind the physical-key release barrier */
    halopad_host_post_input(&(hp_input){.kind = HPI_CANCEL_TOUCH});
    halopad_host_post_input(&(hp_input){.kind = HPI_MOUSEMOVE, .flags = HPI_TOUCH, .dx = 4});
    halopad_host_post_input(&(hp_input){.kind = HPI_MOUSEMOVE, .dx = 3});
    for (int i = 0; i < 8; i++) halopad_host_pump();
    M(m, GetDeviceState, 20, ms);
    check("cancel purges queued look behind barriers but retains fresh and physical motion", rd(ms), 7);
    /* Consume the unrelated physical tap so it cannot contaminate later movement fixtures. */
    memcpy(halopad_guest_ptr(n), (uint32_t[]){32}, 4);
    M(k, GetDeviceData, 20, od, n, 0);
    halopad_host_input_off = 1;
#endif

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
    input((hp_input){.kind = HPI_MOUSEMOVE, .flags = HPI_TOUCH, .dx = 90, .dy = 45});
    input((hp_input){.kind = HPI_MOUSEMOVE, .dx = 7});
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    memcpy(halopad_guest_ptr(n), (uint32_t[]){32}, 4);
    M(m, GetDeviceData, 20, od, n, 0);
    check("buffered cancel removes only touch motion", rd(n) == 1 && rd(od) == 0 && rd(od + 4) == 7, 1);
    M(m, GetDeviceState, 20, ms);
    check("buffered cancel also preserves only physical state delta", rd(ms) == 7 && rd(ms + 4) == 0, 1);
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
    halopad_touch_move_enable(1);
    halopad_gamepad_test[0] = (hp_gamepad){.id = 7, .lx = 1.0f, .ly = 1.0f, .rx = -0.05f, .ry = 0, .lt = 1.0f, .rt = 0,
                                           .buttons = 1u | 1u << 7, .dpad = 2};   /* stick right and up, A and Start, d-pad right */
    memcpy(halopad_guest_ptr(0x64c52c), &di, 4);                   /* Halo's IDirectInput8 */
    check("Halo's game controller set-up (0x494840) returns TRUE", halopad_call_guest(0x494840, 0, NULL) & 0xFF, 1);
    check("  physical controller and distinct touch controller counted", rd(0x64c774), 2);
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
    uint32_t touchpad = rd(0x64c77c);
    check("touch controller has its own DirectInput handle", touchpad != 0 && touchpad != pad, 1);
    const uint16_t *touchname = halopad_guest_ptr(0x64c798 + 0x240);
    int named = 1;
    for (unsigned i = 0; i < sizeof "HaloPad Touch Move"; i++) named &= touchname[i] == (uint8_t)"HaloPad Touch Move"[i];
    check("touch controller has a distinct displayed name", named, 1);
    check("touch controller acquire", M0(touchpad, Acquire), 0);
    input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = .5f, .move_y = .25f});
    M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("touch axes preserve partial magnitude", rd(js) == 1820 && rd(js + 4) == (uint32_t)-683, 1);
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    M(touchpad, GetDeviceState, 224, js);
    check("cancel clears an already-polled touch snapshot", rd(js) == 0 && rd(js + 4) == 0, 1);
    M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("poll cannot resurrect canceled touch axes", rd(js) == 0 && rd(js + 4) == 0, 1);
    M0(pad, Poll); M(pad, GetDeviceState, 224, js);
    check("touch cancel preserves physical controller axes", rd(js), 1820);
    input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = 1});
    input((hp_input){.kind = HPI_ACTIVATE, .down = 0});
    input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = 1});
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1});
    check("touch pad loses foreground acquisition", M(touchpad, GetDeviceState, 224, js), 0x8007001e);
    M0(touchpad, Acquire); M(touchpad, GetDeviceState, 224, js);
    check("reacquire does not replay inactive touch motion", rd(js), 0);
    input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = NAN, .move_y = INFINITY});
    M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("nonfinite touch axes become neutral", rd(js) == 0 && rd(js + 4) == 0, 1);
    input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = 1, .move_y = 1});
    M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("diagonal touch input is radially bounded", rd(js) > 0 && rd(js) < 4096 && (int32_t)rd(js + 4) == -(int32_t)rd(js), 1);
#if TARGET_OS_IPHONE
    halopad_host_input_off = 0;
    queued_input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = 1});
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH});
    pump_many(); M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("cancel drops axes still waiting in host queue", rd(js), 0);
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH});
    queued_input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = .5f});
    pump_many(); M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("fresh touch movement after cancel is retained", rd(js), 1820);
    for (int i = 0; i < 1024; i++) queued_input((hp_input){.kind = HPI_MOUSEMOVE});
    queued_input((hp_input){.kind = HPI_TOUCH_MOVE, .move_x = 1});
    queued_input((hp_input){.kind = HPI_TOUCH_MOVE});
    pump_many(); M0(touchpad, Poll); M(touchpad, GetDeviceState, 224, js);
    check("touch release survives a saturated host queue", rd(js), 0);
    halopad_host_input_off = 1;
#endif
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    check("release touch controller", M0(touchpad, Release), 0);
    halopad_touch_move_enable(0);
    /* Follow the axes beyond DirectInput into Halo's actual movement consumer.
       Test-only bindings: pad slot 0 X +/- -> right/left, Y +/- -> back/forward;
       keyboard W remains independently bound. Original 0x493520 polls and copies
       signed axes to +0x20 of the logical pad; 0x48f850 combines configured inputs.
       None of these fixture writes belong in the app or runtime. */
    uint8_t saved_bindings[0x868], saved_state[0x600], saved_pad_map[16];
    memcpy(saved_bindings, halopad_guest_ptr(0x6ab328), sizeof saved_bindings);
    memcpy(saved_state, halopad_guest_ptr(0x6ad498), sizeof saved_state);
    memcpy(saved_pad_map, halopad_guest_ptr(0x64dc18), sizeof saved_pad_map);
    uint32_t saved_mouse = rd(0x64c734), saved_slot = rd(0x64c9c8);
    saved_keyboard = rd(0x64c730);
    saved_enabled = *(uint8_t *)halopad_guest_ptr(0x64c528);
    uint32_t saved_source = rd(0x815900);
    memset(halopad_guest_ptr(0x6ab328), 0, sizeof saved_bindings);
    for (uint32_t a = 0x6ab330; a < 0x6abb36; a += 2)
        memcpy(halopad_guest_ptr(a), &(uint16_t){0x7fff}, 2);
    for (int i = 0; i < 4; i++) {
        memcpy(halopad_guest_ptr(0x64dc18 + 4 * i), &(uint32_t){i ? 0xffffffff : 0}, 4);
        memcpy(halopad_guest_ptr(0x6ab526 + 4 * i), &(uint32_t){0xffffffff}, 4);
    }
    memcpy(halopad_guest_ptr(0x64c9c8), &(uint32_t){0}, 4);
    memcpy(halopad_guest_ptr(0x64c730), &k, 4);
    memcpy(halopad_guest_ptr(0x64c734), &m, 4);
    *(uint8_t *)halopad_guest_ptr(0x64c528) = 1;
    /* Resolve W through the original DIK table; unit digital throttle and pad threshold. */
    uint16_t w_index; memcpy(&w_index, halopad_guest_ptr(0x5fa358 + 0x11 * 2), 2);
    /* Use Halo's original binding setter (also used by its bind command), which
       leaves keyboard and controller mappings independent. ECX is the descriptor;
       EBX is the action index. This is a test fixture, not automatic user rebinding. */
    for (unsigned i = 0; i < 5; i++) {
        uint16_t descriptor[6] = {3, 0, 1, i / 2, i % 2 + 1, 0};
        uint32_t action = (uint32_t[]){22, 21, 20, 19, 19}[i];
        if (i == 4) { descriptor[0] = 1; descriptor[2] = 0; descriptor[3] = w_index; descriptor[4] = 0; }
        uint32_t binding = bytes(descriptor, sizeof descriptor), saved_ebx = cpu._ebx;
        cpu._ebx = action;
        check("original setter accepts independent movement binding", halopad_call_guest_ex(0x48e360, 0, NULL, binding, 0) & 0xff, 1);
        cpu._ebx = saved_ebx;
    }
    memcpy(halopad_guest_ptr(0x6abb38), &(float){1}, 4);
    memcpy(halopad_guest_ptr(0x6abb3c), &(float){1}, 4);
    memcpy(halopad_guest_ptr(0x6abb58), &(float){1}, 4);
    memcpy(halopad_guest_ptr(0x6abb5c), &(float){1}, 4);
    M0(k, Acquire); M0(m, Acquire);
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    static const struct { float x, y, forward, strafe; const char *name; } movement[] = {
        {0, 0, 0, 0, "neutral"}, {0.05f, 0.05f, 0, 0, "inside dead zone"},
        {0, 0.25f, 683.0f/4096, 0, "quarter forward"},
        {0, 0.5f, 1820.0f/4096, 0, "half forward"},
        {0, 1, 1, 0, "full forward"}, {0, -0.5f, -1820.0f/4096, 0, "half backward"},
        {0.5f, 0, 0, -1820.0f/4096, "half right"}, {-1, 0, 0, 1, "full left"},
        {0.5f, 0.5f, 1820.0f/4096, -1820.0f/4096, "diagonal"},
        {0, 0, 0, 0, "release"}
    };
    for (unsigned i = 0; i < sizeof movement / sizeof *movement; i++) {
        halopad_gamepad_test[0] = (hp_gamepad){.id = 7, .lx = movement[i].x, .ly = movement[i].y, .dpad = -1};
        halopad_call_guest(0x493520, 0, NULL);
        halopad_call_guest(0x48f850, 0, NULL);
        float forward, strafe;
        memcpy(&forward, halopad_guest_ptr(0x6ad4b8), 4);
        memcpy(&strafe, halopad_guest_ptr(0x6ad4bc), 4);
        char label[100]; snprintf(label, sizeof label, "Halo movement consumer: %s", movement[i].name);
        printf("    axes %.2f/%.2f -> forward %.6f, strafe %.6f; bits %08x/%08x\n", movement[i].x, movement[i].y, forward, strafe, rd(0x6ad4b8), rd(0x6ad4bc));
        check(label, fabsf(forward - movement[i].forward) < 0.00001f && fabsf(strafe - movement[i].strafe) < 0.00001f, 1);
    }
    /* Touch actions follow original menu bindings, not the default keys. Use
       the original setter for every remap and exercise real DirectInput reads. */
    halopad_host_input_off = 0;
    M0(m, Unacquire);
    uint32_t no_buffer = bytes((uint32_t[]){20, 16, 0, 0, 0}, 20);
    M(m, SetProperty, 1, no_buffer); M0(m, Acquire);
    uint16_t j_index, space_index;
    memcpy(&j_index, halopad_guest_ptr(0x5fa358 + 0x24 * 2), 2);
    memcpy(&space_index, halopad_guest_ptr(0x5fa358 + 0x39 * 2), 2);
    uint32_t action_descriptor = bytes((uint16_t[]){1, 0, 0, j_index, 0, 0}, 12);
    cpu._ebx = 0; /* jump */
    check("original setter remaps JUMP to J", halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0) & 255, 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    M(k, GetDeviceState, 256, st);
    check("touch JUMP follows J, leaving Space neutral", ((uint8_t *)halopad_guest_ptr(st))[0x24] == 128 && !((uint8_t *)halopad_guest_ptr(st))[0x39], 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0});
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    halopad_call_guest(0x493520, 0, NULL);
    queued_input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    queued_input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0}); pump_many();
    halopad_call_guest(0x493520, 0, NULL);
    check("original Halo poll retains a remapped short JUMP tap", *(uint8_t *)halopad_guest_ptr(0x64c550 + j_index), 1);
    halopad_call_guest(0x493520, 0, NULL);
    check("original Halo poll releases remapped short JUMP", *(uint8_t *)halopad_guest_ptr(0x64c550 + j_index), 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    /* Change the mapping while the finger is held. Its release must use J. */
    cpu._ebx = 0x7fff;
    halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0);
    memcpy(halopad_guest_ptr(action_descriptor), (uint16_t[]){1, 0, 0, space_index, 0, 0}, 12);
    cpu._ebx = 0;
    halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0});
    M(k, GetDeviceState, 256, st);
    check("remapped held action releases its down-time key", !((uint8_t *)halopad_guest_ptr(st))[0x24] && !((uint8_t *)halopad_guest_ptr(st))[0x39], 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    M(k, GetDeviceState, 256, st);
    check("next touch uses the new Space binding", ((uint8_t *)halopad_guest_ptr(st))[0x39], 128);
    input((hp_input){.kind = HPI_KEY, .vk = 0x20, .scan = 0x39, .down = 1});
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0});
    M(k, GetDeviceState, 256, st);
    check("action cancellation preserves physical Space", ((uint8_t *)halopad_guest_ptr(st))[0x39], 128);
    input((hp_input){.kind = HPI_KEY, .vk = 0x20, .scan = 0x39});
    cpu._ebx = 0x7fff;
    halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    M(k, GetDeviceState, 256, st);
    check("unbound action does not fall back to an unrelated default", !((uint8_t *)halopad_guest_ptr(st))[0x39], 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0});
    memcpy(halopad_guest_ptr(action_descriptor), (uint16_t[]){2, 0, 0, 2, 0, 0}, 12);
    cpu._ebx = 7; /* fire */
    check("original setter remaps FIRE to middle mouse", halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0) & 255, 1);
    queued_input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 7, .down = 1});
    queued_input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 7}); pump_many();
    M(m, GetDeviceState, 20, ms);
    check("remapped FIRE tap survives host pumps", ((uint8_t *)halopad_guest_ptr(ms))[14] == 128 && !((uint8_t *)halopad_guest_ptr(ms))[12], 1);
    M(m, GetDeviceState, 20, ms);
    check("remapped FIRE tap releases on the next read", ((uint8_t *)halopad_guest_ptr(ms))[14], 0);
    queued_input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 7, .down = 1});
    queued_input((hp_input){.kind = HPI_CANCEL_TOUCH}); pump_many();
    M(m, GetDeviceState, 20, ms);
    check("cancel drops unread remapped actions", ((uint8_t *)halopad_guest_ptr(ms))[14], 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 7, .down = 1});
    M(m, GetDeviceState, 20, ms);
    cpu._ebx = 0; /* same mouse button rebound to JUMP while FIRE remains held */
    halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 7});
    M(m, GetDeviceState, 20, ms);
    check("remap collision preserves the second touch owner", ((uint8_t *)halopad_guest_ptr(ms))[14], 128);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0});
    M(m, GetDeviceState, 20, ms);
    check("last touch owner releases a shared remapped button", ((uint8_t *)halopad_guest_ptr(ms))[14], 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    input((hp_input){.kind = HPI_ACTIVATE});
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1}); M0(m, Acquire); M0(k, Acquire);
    M(m, GetDeviceState, 20, ms);
    check("focus loss cancels remapped actions without replay", ((uint8_t *)halopad_guest_ptr(ms))[14], 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 0, .down = 1});
    M(m, GetDeviceState, 20, ms);
    check("fresh touch after focus regain works", ((uint8_t *)halopad_guest_ptr(ms))[14], 128);
    cpu._ebx = 0x7fff;
    halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0);
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    halopad_host_input_off = 1;

    /* Halo's player-command builder (0x473c70) applies this original quantizer
       when its game-mode word is nonzero. Presentation may occur on either side
       of a simulation tick; a later +/-1 does not mean analog polling failed. */
    const float quantized_inputs[] = {-1, -.5924479f, -.050001f, -.05f, -.02f, 0,
                                      .02f, .05f, .050001f, .222330734f, .5924479f, 1};
    for (unsigned i = 0; i < sizeof quantized_inputs / sizeof *quantized_inputs; i++) {
        float value = quantized_inputs[i]; uint32_t bits;
        memcpy(&bits, &value, 4);
        uint32_t old_top = cpu._st_top;
        halopad_call_guest_ex(0x473c30, 1, &bits, 0, 0);
        float actual = (float)cpu._st[cpu._st_top & 7];
        cpu._st_top = old_top; /* caller consumes the original x87 return */
        uint32_t out_bits; memcpy(&out_bits, &actual, 4);
        printf("    quantize %08x -> %08x\n", bits, out_bits);
        check("original player-command movement quantization", actual == (value > .05f ? 1 : value < -.05f ? -1 : 0), 1);
    }
    /* The app must use a spare slot, not assume logical controller zero. */
    memcpy(halopad_guest_ptr(0x64dc18), &(uint32_t){0xffffffff}, 4);
    memcpy(halopad_guest_ptr(0x64dc24), &(uint32_t){0}, 4);
    memcpy(halopad_guest_ptr(0x64c9c8), &(uint32_t){3}, 4);
    memcpy(halopad_guest_ptr(0x6ab536 + 3 * 128), halopad_guest_ptr(0x6ab536), 128);
    halopad_gamepad_test[0].ly = .5f;
    halopad_call_guest(0x493520, 0, NULL);
    halopad_call_guest(0x48f850, 0, NULL);
    check("spare logical slot preserves partial movement", rd(0x6ad4b8), 0x3ee38000);
    memcpy(halopad_guest_ptr(0x6abb58), &(float){.75f}, 4);
    halopad_call_guest(0x48f850, 0, NULL);
    float profile_throttle; memcpy(&profile_throttle, halopad_guest_ptr(0x6ad4b8), 4);
    check("spare slot with live profile threshold preserves magnitude", fabsf(profile_throttle - 1820.0f / 4096 / .75f) < .00001f, 1);
    memcpy(halopad_guest_ptr(0x6abb58), &(float){1}, 4);
    memcpy(halopad_guest_ptr(0x64dc18), &(uint32_t){0}, 4);
    memcpy(halopad_guest_ptr(0x64dc24), &(uint32_t){0xffffffff}, 4);
    memcpy(halopad_guest_ptr(0x64c9c8), &(uint32_t){0}, 4);
    halopad_gamepad_test[0].ly = 0;
    input((hp_input){.kind = HPI_KEY, .vk = 'W', .scan = 0x11, .down = 1});
    halopad_call_guest(0x493520, 0, NULL);
    halopad_call_guest(0x48f850, 0, NULL);
    check("neutral controller preserves physical W movement", rd(0x6ad4b8), 0x3f800000);
    halopad_gamepad_test[0].lx = 0.5f;
    halopad_call_guest(0x493520, 0, NULL);
    halopad_call_guest(0x48f850, 0, NULL);
    check("physical W and partial pad strafe coexist", rd(0x6ad4b8) == 0x3f800000 && rd(0x6ad4bc) == 0xbee38000, 1);
    input((hp_input){.kind = HPI_KEY, .vk = 'W', .scan = 0x11, .down = 0});
    halopad_call_guest(0x493520, 0, NULL);
    halopad_call_guest(0x48f850, 0, NULL);
    check("keyboard release preserves pad strafe", rd(0x6ad4b8) == 0 && rd(0x6ad4bc) == 0xbee38000, 1);
    halopad_gamepad_test[0].lx = 0;
    halopad_call_guest(0x493520, 0, NULL);
    halopad_call_guest(0x48f850, 0, NULL);
    check("both sources released return movement to neutral", rd(0x6ad4b8) == 0 && rd(0x6ad4bc) == 0, 1);
    /* Digital touch fallback must follow movement remaps through the original
       consumer as well. Make the old W key move BACKWARD to catch any accidental
       fallback to a default; I is now the only forward keyboard binding. */
    uint16_t i_index;
    memcpy(&i_index, halopad_guest_ptr(0x5fa358 + 0x17 * 2), 2);
    memcpy(halopad_guest_ptr(action_descriptor), (uint16_t[]){1, 0, 0, w_index, 0, 0}, 12);
    cpu._ebx = 20;
    check("original setter assigns old W key to backward", halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0) & 255, 1);
    memcpy(halopad_guest_ptr(action_descriptor), (uint16_t[]){1, 0, 0, i_index, 0, 0}, 12);
    cpu._ebx = 19;
    check("original setter assigns I to forward", halopad_call_guest_ex(0x48e360, 0, NULL, action_descriptor, 0) & 255, 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 19, .down = 1});
    halopad_call_guest(0x493520, 0, NULL); halopad_call_guest(0x48f850, 0, NULL);
    check("digital MOVE uses remapped I and never the old W key", rd(0x6ad4b8) == 0x3f800000 &&
          *(uint8_t *)halopad_guest_ptr(0x64c550 + i_index) && !*(uint8_t *)halopad_guest_ptr(0x64c550 + w_index), 1);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 19});
    halopad_call_guest(0x493520, 0, NULL); halopad_call_guest(0x48f850, 0, NULL);
    check("remapped digital MOVE release is neutral", rd(0x6ad4b8), 0);
    input((hp_input){.kind = HPI_ACTION, .flags = HPI_TOUCH, .action = 19, .down = 1});
    halopad_call_guest(0x493520, 0, NULL); halopad_call_guest(0x48f850, 0, NULL);
    input((hp_input){.kind = HPI_CANCEL_TOUCH});
    halopad_call_guest(0x493520, 0, NULL); halopad_call_guest(0x48f850, 0, NULL);
    check("native-menu cancellation stops remapped digital MOVE", rd(0x6ad4b8), 0);
    memcpy(halopad_guest_ptr(0x6ab328), saved_bindings, sizeof saved_bindings);
    memcpy(halopad_guest_ptr(0x6ad498), saved_state, sizeof saved_state);
    memcpy(halopad_guest_ptr(0x64dc18), saved_pad_map, sizeof saved_pad_map);
    memcpy(halopad_guest_ptr(0x64c9c8), &saved_slot, 4);
    memcpy(halopad_guest_ptr(0x64c730), &saved_keyboard, 4);
    memcpy(halopad_guest_ptr(0x64c734), &saved_mouse, 4);
    memcpy(halopad_guest_ptr(0x815900), &saved_source, 4);
    *(uint8_t *)halopad_guest_ptr(0x64c528) = saved_enabled;
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

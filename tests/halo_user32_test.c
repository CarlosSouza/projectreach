/* USER32 message test (G3/G9): the message queue, activation and focus, window state and
 * host input, called the way Halo calls them (guest addresses from GetProcAddress, entered
 * through dispatch and the llasm wrappers). The window procedure is DefWindowProcA itself,
 * so default processing runs for real; a trace hook records every message delivered to it.
 * Host input goes through halopad_input_event, the entry point the AppKit layer uses.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_user32_test.c. */
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
extern void (*halopad_user32_trace)(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp);
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
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-64s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}

static uint32_t user32, kernel32;
static uint32_t api(const char *name, uint32_t n, const uint32_t *args)
{
    uint32_t mod = strncmp(name, "k:", 2) ? user32 : kernel32;
    uint32_t va = GetProcAddress_c(mod, str(strncmp(name, "k:", 2) ? name : name + 2));
    if (!va) { printf("no export %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define API0(name) api(name, 0, NULL)

/* the trace: messages delivered to the window procedure */
static uint32_t tmsg[256], twp[256], tlp[256], tn;
static void trace(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp) { (void)hwnd; if (tn < 256) { tmsg[tn] = msg; twp[tn] = wp; tlp[tn] = lp; tn++; } }
static int seq(const uint32_t *want, uint32_t n)
{
    int ok = tn == n;
    for (uint32_t i = 0; ok && i < n; i++) ok = tmsg[i] == want[i];
    if (!ok) { printf("    trace:"); for (uint32_t i = 0; i < tn; i++) printf(" %04x", tmsg[i]); printf("\n"); }
    return ok;
}
static int find(uint32_t msg) { for (uint32_t i = 0; i < tn; i++) if (tmsg[i] == msg) return (int)i; return -1; }
#define SEQ(...) seq((uint32_t[]){__VA_ARGS__}, sizeof((uint32_t[]){__VA_ARGS__}) / 4)

static void input(hp_input e) { halopad_input_event(&e); }
static uint32_t msg;                                  /* a guest MSG */
static uint32_t peek(uint32_t lo, uint32_t hi, uint32_t flags) { return API("PeekMessageA", msg, 0, lo, hi, flags); }
static uint32_t M(int i) { return rd(msg + 4 * i); }  /* 0 hwnd, 1 message, 2 wParam, 3 lParam, 4 time, 5 x, 6 y */

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
    kernel32 = LoadLibraryA_c(str("kernel32.dll"));
    msg = halopad_heap_alloc(28, 1);

    /* a CS_DBLCLKS class whose procedure is DefWindowProcA; a 640x480 client area */
    uint32_t defproc = GetProcAddress_c(user32, str("DefWindowProcA"));
    uint32_t cls = str("HaloPadMsgTest");
    uint32_t wc = halopad_heap_alloc(48, 1);
    uint32_t wcv[12] = {48, 0x8, defproc, 0, 0, 0x400000, 0, API("LoadCursorA", 0, 32512), 0, 0, cls, 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    check("RegisterClassExA (CS_DBLCLKS, DefWindowProcA)", API("RegisterClassExA", wc) != 0, 1);
    uint32_t hwnd = API("CreateWindowExA", 0, cls, str("HaloPad"), 0x00CF0000, 100, 100, 648, 507, 0, 0, 0x400000, 0);
    check("CreateWindowExA", hwnd != 0, 1);
    halopad_user32_trace = trace;

    /* first show: Windows' order */
    tn = 0;
    check("ShowWindow(SW_SHOW) on a hidden window returns 0", API("ShowWindow", hwnd, 5), 0);
    check("  WM_SHOWWINDOW, WM_WINDOWPOSCHANGING, WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE, WM_SETFOCUS, WM_WINDOWPOSCHANGED, WM_SIZE, WM_MOVE",
          SEQ(0x18, 0x46, 0x1C, 0x86, 0x06, 0x07, 0x47, 0x05, 0x03), 1);
    int sz = find(0x05);
    check("  WM_SIZE 640x480", sz >= 0 ? tlp[sz] : 0, 480u << 16 | 640);
    int mv = find(0x03);
    check("  WM_MOVE to the client origin (104,123)", mv >= 0 ? tlp[mv] : 0, 123u << 16 | 104);
    check("  active, foreground", API0("GetActiveWindow") == hwnd && API0("GetForegroundWindow") == hwnd, 1);
    check("  GWL_STYLE has WS_VISIBLE", (API("GetWindowLongA", hwnd, (uint32_t)-16) & 0x10000000) != 0, 1);
    check("ShowWindow again returns 1 (was visible)", API("ShowWindow", hwnd, 5), 1);

    /* WM_PAINT until DefWindowProc validates */
    check("PeekMessageA: WM_PAINT for the new window", peek(0, 0, 1) && M(1) == 0x0F && M(0) == hwnd, 1);
    API("DispatchMessageA", msg);
    check("  after DefWindowProc, the queue is empty", peek(0, 0, 1), 0);

    /* keys */
    hp_input a = {.kind = HPI_KEY, .vk = 'A', .scan = 0x1E, .down = 1, .chars = {'a'}, .nchars = 1};
    input(a);
    check("GetAsyncKeyState('A') while down", API("GetAsyncKeyState", 'A') & 0xFFFF, 0x8001);
    check("GetKeyState('A') before the message is retrieved", API("GetKeyState", 'A') & 0x8000, 0);
    check("key down: WM_KEYDOWN 'A', lParam 0x001E0001", peek(0, 0, 1) && M(1) == 0x100 && M(2) == 'A' && M(3) == 0x001E0001, 1);
    check("  GetKeyState('A') once retrieved", API("GetKeyState", 'A') & 0x8000, 0x8000);
    check("  TranslateMessage", API("TranslateMessage", msg), 1);
    check("  next: WM_CHAR 'a'", peek(0, 0, 1) && M(1) == 0x102 && M(2) == 'a', 1);
    input(a);
    check("autorepeat: previous-state bit set", peek(0, 0, 1) && M(1) == 0x100 && M(3) == 0x401E0001, 1);
    a.down = 0; a.nchars = 0;
    input(a);
    check("key up: WM_KEYUP lParam 0xC01E0001", peek(0, 0, 1) && M(1) == 0x101 && M(3) == 0xC01E0001, 1);
    check("  TranslateMessage on WM_KEYUP makes no character", API("TranslateMessage", msg) == 1 && peek(0, 0, 1) == 0, 1);
    hp_input caps = {.kind = HPI_KEY, .vk = 0x14, .scan = 0x3A, .down = 1};
    input(caps); caps.down = 0; input(caps);
    while (peek(0, 0, 1)) {}
    check("Caps Lock toggles GetKeyState bit 0", API("GetKeyState", 0x14) & 1, 1);
    hp_input left = {.kind = HPI_KEY, .vk = 0x25, .scan = 0x4B, .extended = 1, .down = 1};
    input(left);
    check("extended key: lParam bit 24", peek(0, 0, 1) && M(3) == 0x014B0001, 1);
    left.down = 0; input(left); peek(0, 0, 1);

    /* mouse */
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 5, .y = 6});
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 10, .y = 20});
    check("mouse moves coalesce: one WM_MOUSEMOVE at (10,20)", peek(0, 0, 1) && M(1) == 0x200 && M(3) == (20u << 16 | 10) && !peek(0, 0, 0), 1);
    uint32_t pt = halopad_heap_alloc(8, 1);
    API("GetCursorPos", pt);
    check("GetCursorPos is client (10,20) on screen (114,143)", rd(pt) == 114 && rd(pt + 4) == 143, 1);
    uint32_t cp = halopad_heap_alloc(8, 1);
    memcpy(halopad_guest_ptr(cp), (uint32_t[]){10, 20}, 8);
    API("ClientToScreen", hwnd, cp);
    check("ClientToScreen agrees", rd(cp) == 114 && rd(cp + 4) == 143, 1);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 1, .x = 10, .y = 20});
    check("left button: WM_LBUTTONDOWN with MK_LBUTTON", peek(0, 0, 1) && M(1) == 0x201 && M(2) == 1, 1);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 0, .x = 10, .y = 20});
    check("  WM_LBUTTONUP", peek(0, 0, 1) && M(1) == 0x202 && M(2) == 0, 1);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 1, .x = 10, .y = 20});
    check("  a second click at once is WM_LBUTTONDBLCLK (CS_DBLCLKS)", peek(0, 0, 1) && M(1) == 0x203, 1);
    input((hp_input){.kind = HPI_BUTTON, .button = 0, .down = 0, .x = 10, .y = 20});
    peek(0, 0, 1);
    input((hp_input){.kind = HPI_WHEEL, .wheel = 120, .x = 10, .y = 20});
    check("wheel: WM_MOUSEWHEEL, delta 120 in the high word", peek(0, 0, 1) && M(1) == 0x20A && M(2) == 0x00780000, 1);

    /* filters and PM_NOREMOVE */
    input((hp_input){.kind = HPI_KEY, .vk = 'B', .scan = 0x30, .down = 1});
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 1, .y = 1});
    check("filter WM_MOUSEFIRST..WM_MOUSELAST takes the later mouse message", peek(0x200, 0x20E, 1) && M(1) == 0x200, 1);
    check("PM_NOREMOVE leaves WM_KEYDOWN", peek(0, 0, 0) && M(1) == 0x100 && peek(0, 0, 0) && M(1) == 0x100, 1);
    check("hwnd -1 (thread messages only) finds nothing", API("PeekMessageA", msg, 0xFFFFFFFF, 0, 0, 1), 0);
    peek(0, 0, 1);
    input((hp_input){.kind = HPI_KEY, .vk = 'B', .scan = 0x30, .down = 0});
    peek(0, 0, 1);

    /* window data */
    check("SetWindowLongA(GWL_USERDATA)", API("SetWindowLongA", hwnd, (uint32_t)-21, 0x1234) == 0 && API("GetWindowLongA", hwnd, (uint32_t)-21) == 0x1234, 1);
    uint32_t pn = str("HaloPadProp");
    check("SetPropA/GetPropA/RemovePropA", API("SetPropA", hwnd, pn, 77) && API("GetPropA", hwnd, pn) == 77
          && API("RemovePropA", hwnd, pn) == 77 && API("GetPropA", hwnd, pn) == 0, 1);
    check("FindWindowA by class", API("FindWindowA", cls, 0), hwnd);
    check("FindWindowA by title", API("FindWindowA", 0, str("HaloPad")), hwnd);
    check("FindWindowA with no match", API("FindWindowA", str("NoSuchClass"), 0), 0);
    check("GetSystemMetrics(SM_SWAPBUTTON)", API("GetSystemMetrics", 23), 0);
    check("PtInRect", API("PtInRect", cp - 8 + 8, 0, 0) == 0, 1);
    tn = 0;
    API("SetWindowPos", hwnd, 0, 50, 60, 0, 0, 0x1 | 0x4 | 0x10);
    check("SetWindowPos move: CHANGING, CHANGED, WM_MOVE (no WM_SIZE)", SEQ(0x46, 0x47, 0x03), 1);
    check("  WM_MOVE to (54,83)", tlp[2], 83u << 16 | 54);
    check("SetWindowTextA through WM_SETTEXT", API("SetWindowTextA", hwnd, str("Renamed")) == 1 && API("FindWindowA", 0, str("Renamed")) == hwnd, 1);

    /* waits */
    check("MsgWaitForMultipleObjects(0 handles, 30 ms) on an empty queue times out", API("MsgWaitForMultipleObjects", 0, 0, 0, 30, 0xFF), 0x102);
    input((hp_input){.kind = HPI_MOUSEMOVE, .x = 2, .y = 2});
    check("  returns WAIT_OBJECT_0 + 0 once input is queued", API("MsgWaitForMultipleObjects", 0, 0, 0, 1000, 0xFF), 0);
    peek(0, 0, 1);
    uint32_t ev = API("k:CreateEventA", 0, 1, 1, 0);
    uint32_t hs = halopad_heap_alloc(4, 1);
    memcpy(halopad_guest_ptr(hs), &ev, 4);
    check("  a signaled event wins: WAIT_OBJECT_0", API("MsgWaitForMultipleObjects", 1, hs, 0, 1000, 0xFF), 0);

    /* wsprintfA (cdecl) */
    uint32_t buf = halopad_heap_alloc(128, 1);
    uint32_t wsargs[7] = {buf, str("%s-%d-%05u-%x-%c-%%"), str("halo"), (uint32_t)-7, 42, 0xBEEF, 'Z'};
    uint32_t n = halopad_call_guest_ex(GetProcAddress_c(user32, str("wsprintfA")), 7, wsargs, 0, 0);
    check("wsprintfA returns the length", n, 22);
    check("  text \"halo--7-00042-beef-Z-%\"", strcmp(halopad_guest_ptr(buf), "halo--7-00042-beef-Z-%") == 0, 1);

    /* the application loses and regains the foreground */
    tn = 0;
    input((hp_input){.kind = HPI_ACTIVATE, .down = 0});
    check("deactivation: WM_NCACTIVATE, WM_ACTIVATE, WM_KILLFOCUS, WM_ACTIVATEAPP(FALSE)", SEQ(0x86, 0x06, 0x08, 0x1C), 1);
    check("  no foreground window, no async key state", API0("GetForegroundWindow") == 0 && API("GetAsyncKeyState", 'A') == 0, 1);
    tn = 0;
    input((hp_input){.kind = HPI_ACTIVATE, .down = 1});
    check("reactivation: WM_ACTIVATEAPP(TRUE), WM_NCACTIVATE, WM_ACTIVATE, WM_SETFOCUS", SEQ(0x1C, 0x86, 0x06, 0x07), 1);

    /* minimise and restore */
    tn = 0;
    API("ShowWindow", hwnd, 6);
    check("SW_MINIMIZE: WM_SIZE(SIZE_MINIMIZED) and deactivation", tmsg[0] == 0x05 && twp[0] == 1 && API0("GetActiveWindow") == 0, 1);
    tn = 0;
    API("ShowWindow", hwnd, 9);
    check("SW_RESTORE: reactivated, WM_SIZE(SIZE_RESTORED, 640x480)", API0("GetActiveWindow") == hwnd && find(0x05) >= 0
          && twp[find(0x05)] == 0 && tlp[find(0x05)] == (480u << 16 | 640), 1);

    /* Alt+F4 through DefWindowProc, then the close button */
    input((hp_input){.kind = HPI_KEY, .vk = 0x12, .side_vk = 0xA4, .scan = 0x38, .down = 1});
    check("Alt: WM_SYSKEYDOWN VK_MENU with the context bit", peek(0, 0, 1) && M(1) == 0x104 && M(2) == 0x12 && (M(3) & (1u << 29)), 1);
    input((hp_input){.kind = HPI_KEY, .vk = 0x73, .scan = 0x3E, .down = 1});
    check("F4 with Alt: WM_SYSKEYDOWN VK_F4", peek(0, 0, 1) && M(1) == 0x104 && M(2) == 0x73, 1);
    tn = 0;
    API("DispatchMessageA", msg);
    check("  DefWindowProc: WM_SYSCOMMAND(SC_CLOSE), WM_CLOSE, then the window is destroyed",
          find(0x112) >= 0 && find(0x10) > find(0x112) && find(0x02) > find(0x10) && find(0x82) > find(0x02), 1);
    check("  the window is gone", API("GetWindowLongA", hwnd, (uint32_t)-16), 0);
    input((hp_input){.kind = HPI_KEY, .vk = 0x73, .scan = 0x3E, .down = 0});
    input((hp_input){.kind = HPI_KEY, .vk = 0x12, .side_vk = 0xA4, .scan = 0x38, .down = 0});
    while (peek(0, 0, 1)) {}

    uint32_t hwnd2 = API("CreateWindowExA", 0, cls, str("Second"), 0x00CF0000, 0, 0, 200, 200, 0, 0, 0x400000, 0);
    API("ShowWindow", hwnd2, 5);
    while (peek(0, 0, 1)) API("DispatchMessageA", msg);
    input((hp_input){.kind = HPI_CLOSE});
    check("close button: WM_SYSCOMMAND(SC_CLOSE) is posted", peek(0, 0, 1) && M(1) == 0x112 && M(2) == 0xF060 && M(0) == hwnd2, 1);
    tn = 0;
    API("DispatchMessageA", msg);
    check("  dispatched: WM_CLOSE then WM_DESTROY, WM_NCDESTROY", find(0x10) >= 0 && find(0x02) > find(0x10) && find(0x82) > find(0x02), 1);
    API("PostQuitMessage", 3);
    check("PostQuitMessage(3): WM_QUIT with wParam 3", peek(0, 0, 1) && M(1) == 0x12 && M(2) == 3 && M(0) == 0, 1);
    check("  then the queue is empty", peek(0, 0, 1), 0);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

/* HaloPad USER32 shim, part 1 (G3): icons/cursors, window classes, the desktop window,
 * window geometry and CreateWindowExA with its creation messages, DefWindowProcA.
 *
 * Windows are host-side records; the AppKit/Metal view attaches when something first
 * draws (graphics work). Messages go to Halo's window procedure through
 * halopad_call_guest. Frame metrics follow Windows XP's classic theme (caption 19,
 * sizing frame 4). The desktop is the Mac's main display. Anything outside this set
 * (message numbers, styles) stops with its value. */
#include "halopad_win32.h"
#include <strings.h>
#if defined(__APPLE__)
#include <TargetConditionals.h>
#if TARGET_OS_OSX
#include <CoreGraphics/CoreGraphics.h>
#endif
#endif

uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t FindResourceExA_c(uint32_t module, uint32_t type, uint32_t name, uint32_t lang);

#define CAPTION 19
#define FRAME 4
#define WS_CAPTION 0x00C00000u
#define WS_THICKFRAME 0x00040000u
#define WS_BORDER 0x00800000u
#define WS_CHILD 0x40000000u
#define WS_POPUP 0x80000000u

/* ---- handles: icons, cursors, windows, classes ---- */
enum { H_FREE, H_ICON, H_CURSOR, H_WINDOW };
typedef struct { int kind; uint32_t resource; uint32_t cls; uint32_t style, exstyle; int32_t x, y, w, h; uint32_t parent, instance;
                 char text[256]; uint32_t userdata; int visible;
                 uint32_t wndproc;                       /* the window's procedure (the class's, unless subclassed) */
                 int disabled, minimized, invalid, sizemove_sent;
                 void *host;                             /* the host window, once shown (top-level windows) */
                 struct { char name[32]; uint32_t atom, data; } props[8]; } uobj;
#define UBASE 0x00010010u
#define MAXU 256
static uobj uobjs[MAXU];

static uint32_t unew(int kind)
{
    for (uint32_t i = 1; i < MAXU; i++) if (uobjs[i].kind == H_FREE) { memset(&uobjs[i], 0, sizeof uobjs[i]); uobjs[i].kind = kind; return UBASE + 4 * i; }
    hp_unsupported("USER32", "more than %d user objects", MAXU);
}

static uobj *uget(uint32_t h, int kind)
{
    if (h < UBASE || (h - UBASE) % 4 || (h - UBASE) / 4 >= MAXU) return NULL;
    uobj *o = &uobjs[(h - UBASE) / 4];
    return o->kind == kind ? o : NULL;
}

#define DESKTOP UBASE   /* slot 0 */

void halopad_desktop_size(int32_t *w, int32_t *h);
static void desktop_size(int32_t *w, int32_t *h) { halopad_desktop_size(w, h); }

/* The Mac's main display, in pixels: the Windows desktop HaloPad presents. */
void halopad_desktop_size(int32_t *w, int32_t *h)
{
#if defined(__APPLE__) && TARGET_OS_OSX
    CGDirectDisplayID d = CGMainDisplayID();
    *w = (int32_t)CGDisplayPixelsWide(d);
    *h = (int32_t)CGDisplayPixelsHigh(d);
#else
    void halopad_host_screen_size(int32_t *w, int32_t *h);    /* port/apple/halopad_host_ios.m */
    halopad_host_screen_size(w, h);
#endif
}

uint32_t LoadIconA_c(uint32_t instance, uint32_t name)
{
    if (!instance) hp_unsupported("LoadIconA", "system icon %u", name);
    uint32_t r = FindResourceExA_c(instance, 14 /* RT_GROUP_ICON */, name, 0);
    if (!r) return 0;
    uint32_t h = unew(H_ICON);
    uget(h, H_ICON)->resource = r;
    return h;
}

uint32_t LoadCursorA_c(uint32_t instance, uint32_t name)
{
    static uint32_t system[32];
    if (instance) hp_unsupported("LoadCursorA", "application cursor %u", name);
    if (name < 32512 || name > 32650) hp_unsupported("LoadCursorA", "system cursor %u", name);
    uint32_t slot = (name - 32512) % 32;
    if (!system[slot]) { system[slot] = unew(H_CURSOR); uget(system[slot], H_CURSOR)->resource = name; }
    return system[slot];
}

typedef struct { char name[64]; uint32_t style, wndproc, extra_cls, extra_wnd, instance, icon, cursor, brush, icon_sm; int used; } wclass;
static wclass classes[32];

static wclass *find_class(uint32_t name)
{
    for (int i = 0; i < 32; i++) {
        if (!classes[i].used) continue;
        if (name < 0x10000 ? name == 0xC000u + (uint32_t)i : !strcasecmp(classes[i].name, G(name))) return &classes[i];
    }
    return NULL;
}

uint32_t RegisterClassExA_c(uint32_t wc)
{
    if (rd32(wc) != 48) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    uint32_t name = rd32(wc + 40);
    if (name < 0x10000) hp_unsupported("RegisterClassExA", "class atom %u as name", name);
    if (find_class(name)) { halopad_last_error = 1410; return 0; }        /* ERROR_CLASS_ALREADY_EXISTS */
    if (rd32(wc + 12) || rd32(wc + 16)) hp_unsupported("RegisterClassExA", "class/window extra bytes %u/%u", rd32(wc + 12), rd32(wc + 16));
    for (int i = 0; i < 32; i++) {
        if (classes[i].used) continue;
        wclass *c = &classes[i];
        snprintf(c->name, sizeof c->name, "%s", (const char *)G(name));
        c->style = rd32(wc + 4); c->wndproc = rd32(wc + 8); c->instance = rd32(wc + 20);
        c->icon = rd32(wc + 24); c->cursor = rd32(wc + 28); c->brush = rd32(wc + 32); c->icon_sm = rd32(wc + 44);
        c->used = 1;
        return 0xC000u + (uint32_t)i;
    }
    hp_unsupported("RegisterClassExA", "more than 32 classes");
}

uint32_t UnregisterClassA_c(uint32_t name, uint32_t instance)
{
    (void)instance;
    wclass *c = find_class(name);
    if (!c) { halopad_last_error = 1411; return 0; }                    /* ERROR_CLASS_DOES_NOT_EXIST */
    c->used = 0;
    return 1;
}

uint32_t GetDesktopWindow_c(void)
{
    if (uobjs[0].kind != H_WINDOW) {
        uobjs[0].kind = H_WINDOW;
        desktop_size(&uobjs[0].w, &uobjs[0].h);
        uobjs[0].visible = 1;
    }
    return DESKTOP;
}

/* Non-client insets for a style (XP classic theme). */
static void frame_insets(uint32_t style, int32_t *l, int32_t *t, int32_t *r, int32_t *b)
{
    int32_t f = (style & WS_THICKFRAME) ? FRAME : ((style & (WS_BORDER | WS_CAPTION)) ? 3 : 0);
    if ((style & WS_CAPTION) == WS_BORDER) f = 1;
    *l = *r = *b = f;
    *t = f + (((style & WS_CAPTION) == WS_CAPTION) ? CAPTION : 0);
}

uint32_t AdjustWindowRect_c(uint32_t rect, uint32_t style, uint32_t menu)
{
    if (menu) hp_unsupported("AdjustWindowRect", "windows with menus");
    int32_t l, t, r, b;
    frame_insets(style, &l, &t, &r, &b);
    wr32(rect + 0, rd32(rect + 0) - (uint32_t)l);
    wr32(rect + 4, rd32(rect + 4) - (uint32_t)t);
    wr32(rect + 8, rd32(rect + 8) + (uint32_t)r);
    wr32(rect + 12, rd32(rect + 12) + (uint32_t)b);
    return 1;
}

uint32_t GetWindowRect_c(uint32_t hwnd, uint32_t rect)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) { halopad_last_error = 1400; return 0; }                    /* ERROR_INVALID_WINDOW_HANDLE */
    wr32(rect, (uint32_t)w->x); wr32(rect + 4, (uint32_t)w->y);
    wr32(rect + 8, (uint32_t)(w->x + w->w)); wr32(rect + 12, (uint32_t)(w->y + w->h));
    return 1;
}

/* For the graphics layer: a window's client size and title. */
int halopad_window_client_size(uint32_t hwnd, int32_t *cw, int32_t *ch)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    int32_t l, t, r, b;
    frame_insets(hwnd == DESKTOP ? 0 : w->style, &l, &t, &r, &b);
    *cw = w->w - l - r;
    *ch = w->h - t - b;
    return 1;
}

const char *halopad_window_text(uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    return w ? w->text : "HaloPad";
}

uint32_t GetClientRect_c(uint32_t hwnd, uint32_t rect)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) { halopad_last_error = 1400; return 0; }
    int32_t l, t, r, b;
    frame_insets(hwnd == DESKTOP ? 0 : w->style, &l, &t, &r, &b);
    wr32(rect, 0); wr32(rect + 4, 0);
    wr32(rect + 8, (uint32_t)(w->w - l - r)); wr32(rect + 12, (uint32_t)(w->h - t - b));
    return 1;
}

/* Test support: called with every message delivered to a window procedure. */
void (*halopad_user32_trace)(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp);

static uint32_t send(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;                                   /* destroyed meanwhile */
    if (halopad_user32_trace) halopad_user32_trace(hwnd, msg, wp, lp);
    uint32_t args[4] = {hwnd, msg, wp, lp};
    return halopad_call_guest(w->wndproc, 4, args);
}
static uint32_t default_message(uobj *w, uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp);

#define WM_CREATE 0x0001
#define WM_GETMINMAXINFO 0x0024
#define WM_NCCREATE 0x0081
#define WM_NCCALCSIZE 0x0083

uint32_t CreateWindowExA_c(uint32_t exstyle, uint32_t clsname, uint32_t title, uint32_t style, uint32_t x, uint32_t y,
                           uint32_t w, uint32_t h, uint32_t parent, uint32_t menu, uint32_t instance, uint32_t param)
{
    wclass *c = find_class(clsname);
    if (!c) { halopad_last_error = 1407; return 0; }                    /* ERROR_CANNOT_FIND_WND_CLASS */
    if (exstyle || menu || (style & (WS_CHILD | 0x10000000u /* WS_VISIBLE */)) || (parent && parent != DESKTOP))
        hp_unsupported("CreateWindowExA", "exstyle 0x%x, style 0x%x, parent 0x%x, menu 0x%x", exstyle, style, parent, menu);
    if ((int32_t)x == (int32_t)0x80000000 || (int32_t)w == (int32_t)0x80000000) hp_unsupported("CreateWindowExA", "CW_USEDEFAULT");
    uint32_t hwnd = unew(H_WINDOW);
    uobj *o = uget(hwnd, H_WINDOW);
    o->cls = (uint32_t)(c - classes); o->style = style; o->exstyle = exstyle; o->instance = instance; o->parent = parent;
    o->wndproc = c->wndproc;
    o->x = (int32_t)x; o->y = (int32_t)y; o->w = (int32_t)w; o->h = (int32_t)h;

    /* WM_GETMINMAXINFO (XP classic metrics on the host desktop) */
    int32_t dw, dh;
    desktop_size(&dw, &dh);
    uint32_t mmi = halopad_heap_alloc(40, 1);
    wr32(mmi + 8, (uint32_t)(dw + 2 * FRAME)); wr32(mmi + 12, (uint32_t)(dh + 2 * FRAME));
    wr32(mmi + 16, (uint32_t)-FRAME); wr32(mmi + 20, (uint32_t)-FRAME);
    wr32(mmi + 24, 112); wr32(mmi + 28, 27);
    wr32(mmi + 32, (uint32_t)(dw + 2 * FRAME + 12)); wr32(mmi + 36, (uint32_t)(dh + 2 * FRAME + 12));
    send(hwnd, WM_GETMINMAXINFO, 0, mmi);
    halopad_heap_free(mmi);

    uint32_t cs = halopad_heap_alloc(48, 1);
    wr32(cs + 0, param); wr32(cs + 4, instance); wr32(cs + 8, menu); wr32(cs + 12, parent);
    wr32(cs + 16, h); wr32(cs + 20, w); wr32(cs + 24, y); wr32(cs + 28, x);
    wr32(cs + 32, style); wr32(cs + 36, title); wr32(cs + 40, clsname); wr32(cs + 44, exstyle);
    uint32_t ok = send(hwnd, WM_NCCREATE, 0, cs);
    if (ok) {
        uint32_t rc = halopad_heap_alloc(16, 1);
        wr32(rc, x); wr32(rc + 4, y); wr32(rc + 8, x + w); wr32(rc + 12, y + h);
        send(hwnd, WM_NCCALCSIZE, 0, rc);
        halopad_heap_free(rc);
        if (send(hwnd, WM_CREATE, 0, cs) == 0xFFFFFFFFu) ok = 0;
    }
    halopad_heap_free(cs);
    if (!ok) { o->kind = H_FREE; return 0; }   /* the window procedure refused creation */
    return hwnd;
}

uint32_t DefWindowProcA_c(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    switch (msg) {
    case WM_NCCREATE: {
        uint32_t t = rd32(lp + 36);
        snprintf(w->text, sizeof w->text, "%s", t ? (const char *)G(t) : "");
        return 1;
    }
    case WM_NCCALCSIZE: {                         /* window rect -> client rect in place */
        int32_t l, t, r, b;
        frame_insets(w->style, &l, &t, &r, &b);
        /* with NCCALCSIZE_PARAMS, its first rectangle is the new window rectangle */
        wr32(lp, rd32(lp) + (uint32_t)l); wr32(lp + 4, rd32(lp + 4) + (uint32_t)t);
        wr32(lp + 8, rd32(lp + 8) - (uint32_t)r); wr32(lp + 12, rd32(lp + 12) - (uint32_t)b);
        return 0;
    }
    case WM_GETMINMAXINFO:
    case WM_CREATE:
        return 0;
    }
    return default_message(w, hwnd, msg, wp, lp);
}


/* ==== part 2 (G3/G9): the message queue, activation and focus, window state, input ====
 *
 * One thread's queue: posted messages and input in arrival order, then WM_QUIT once
 * posted, then WM_PAINT for an invalid window. Host input (keyboard, mouse, application
 * activation, the close button) arrives through halopad_input_event, called by the host
 * layer while PeekMessageA pumps host events; keyboard input goes to the focus window,
 * mouse input to the capture or active window. Activation follows Windows' order
 * (WM_ACTIVATEAPP, WM_NCACTIVATE, WM_ACTIVATE, then DefWindowProc's SetFocus), and a first
 * ShowWindow sends WM_SHOWWINDOW, the position change pair and WM_SIZE/WM_MOVE. Key state
 * is kept both as input arrives (GetAsyncKeyState) and as messages are retrieved
 * (GetKeyState). */
#include "halopad_input.h"

void halopad_host_pump(void);
void halopad_host_cursor(int visible);
uint32_t GetTickCount_c(void);
int halopad_wait_poll(uint32_t n, uint32_t handles);
void Sleep_c(uint32_t ms);

#define WM_DESTROY 0x0002
#define WM_MOVE 0x0003
#define WM_SIZE 0x0005
#define WM_ACTIVATE 0x0006
#define WM_SETFOCUS 0x0007
#define WM_KILLFOCUS 0x0008
#define WM_ENABLE 0x000A
#define WM_SETTEXT 0x000C
#define WM_GETTEXT 0x000D
#define WM_GETTEXTLENGTH 0x000E
#define WM_PAINT 0x000F
#define WM_CLOSE 0x0010
#define WM_QUIT 0x0012
#define WM_ERASEBKGND 0x0014
#define WM_SHOWWINDOW 0x0018
#define WM_ACTIVATEAPP 0x001C
#define WM_SETCURSOR 0x0020
#define WM_WINDOWPOSCHANGING 0x0046
#define WM_WINDOWPOSCHANGED 0x0047
#define WM_INPUTLANGCHANGE 0x0051
#define WM_STYLECHANGING 0x007C
#define WM_STYLECHANGED 0x007D
#define WM_DISPLAYCHANGE 0x007E
#define WM_NCDESTROY 0x0082
#define WM_NCHITTEST 0x0084
#define WM_NCACTIVATE 0x0086
#define WM_KEYDOWN 0x0100
#define WM_KEYUP 0x0101
#define WM_CHAR 0x0102
#define WM_SYSKEYDOWN 0x0104
#define WM_SYSKEYUP 0x0105
#define WM_SYSCHAR 0x0106
#define WM_SYSCOMMAND 0x0112
#define WM_TIMER 0x0113
#define WM_MOUSEMOVE 0x0200
#define WM_LBUTTONDOWN 0x0201
#define WM_MOUSEWHEEL 0x020A
#define WM_CAPTURECHANGED 0x0215
#define SC_MINIMIZE 0xF020
#define SC_CLOSE 0xF060
#define SC_KEYMENU 0xF100
#define SC_RESTORE 0xF120
#define SC_SCREENSAVE 0xF140
#define SC_MONITORPOWER 0xF170
#define SWP_NOSIZE 0x1
#define SWP_NOMOVE 0x2
#define SWP_NOZORDER 0x4
#define SWP_NOACTIVATE 0x10
#define SWP_FRAMECHANGED 0x20
#define SWP_SHOWWINDOW 0x40
#define SWP_HIDEWINDOW 0x80
#define SWP_NOSENDCHANGING 0x400
#define WS_VISIBLE 0x10000000u
#define WS_DISABLED 0x08000000u
#define WS_MINIMIZE 0x20000000u
#define WS_EX_TOPMOST 0x8u
#define VK_LBUTTON 0x01
#define VK_SHIFT 0x10
#define VK_CONTROL 0x11
#define VK_MENU 0x12
#define VK_CAPITAL 0x14
#define VK_F4 0x73
#define VK_F10 0x79
#define VK_NUMLOCK 0x90
#define VK_SCROLL 0x91

typedef struct { uint32_t hwnd, msg, wp, lp, time; int32_t x, y; uint16_t chars[4]; uint8_t nchars; } qmsg;
#define QMAX 4096
static qmsg queue[QMAX];
static uint32_t qcount;
static int quit_posted;
static uint32_t quit_code;
static qmsg last_key;                                 /* the key message last retrieved, with its characters */
static uint32_t active, last_active, focus, capture;
static int app_active;
static int32_t cursor_x, cursor_y;                    /* screen */
static int cursor_count;                              /* ShowCursor's display count */
static uint32_t cursor_handle;
static uint8_t async_keys[256], async_pressed[256], sync_keys[256], toggles[256];
static uint32_t last_click_time, last_click_msg;
static int32_t last_click_x, last_click_y;

static int is_window(uint32_t h) { return h && h != DESKTOP && uget(h, H_WINDOW) != NULL; }

static void post(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp, const uint16_t *chars, int n)
{
    if (qcount == QMAX) hp_unsupported("USER32", "a message queue of more than %u messages (0x%04x)", QMAX, msg);
    qmsg *m = &queue[qcount++];
    memset(m, 0, sizeof *m);
    m->hwnd = hwnd; m->msg = msg; m->wp = wp; m->lp = lp; m->time = GetTickCount_c(); m->x = cursor_x; m->y = cursor_y;
    for (int i = 0; i < n && i < 4; i++) m->chars[i] = chars[i];
    m->nchars = (uint8_t)(n < 4 ? n : 4);
}

static void window_screen_client(uobj *w, int32_t *x, int32_t *y)
{
    int32_t l, t, r, b;
    frame_insets(w->style, &l, &t, &r, &b);
    *x = w->x + l; *y = w->y + t;
}

/* ---- host windows ---- */

void *halopad_host_window_create(uint32_t width, uint32_t height, const char *title, int visible);
void halopad_host_window_destroy(void *w);
void halopad_host_window_show(void *w, int visible);
void halopad_host_window_title(void *w, const char *title);
void halopad_host_window_resize(void *w, uint32_t width, uint32_t height);

static void host_sync(uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w || hwnd == DESKTOP || (w->style & WS_CHILD)) return;
    int32_t cw, ch;
    halopad_window_client_size(hwnd, &cw, &ch);
    int shown = w->visible && !w->minimized;
    if (!w->host) {
        if (!shown || cw <= 0 || ch <= 0) return;
        w->host = halopad_host_window_create((uint32_t)cw, (uint32_t)ch, w->text, 1);
        return;
    }
    if (cw > 0 && ch > 0) halopad_host_window_resize(w->host, (uint32_t)cw, (uint32_t)ch);
    halopad_host_window_show(w->host, shown);
}

/* Direct3D's window: its host window, created (and shown) if it has none yet. */
void *halopad_window_host(uint32_t hwnd, uint32_t width, uint32_t height)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) hp_unsupported("Direct3D", "a device window 0x%x that is not a window", hwnd);
    if (!w->host) w->host = halopad_host_window_create(width, height, w->text, 1);
    return w->host;
}
/* GDI: the host window if the window is on screen, else NULL. */
void *halopad_window_host_if_shown(uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    return w && w->visible && !w->minimized ? w->host : NULL;
}

/* ---- activation and focus ---- */

static void set_focus(uint32_t hwnd)
{
    if (focus == hwnd) return;
    uint32_t old = focus;
    focus = hwnd;
    if (is_window(old)) send(old, WM_KILLFOCUS, hwnd, 0);
    if (is_window(hwnd)) send(hwnd, WM_SETFOCUS, old, 0);
}

static void activate(uint32_t hwnd)
{
    if (active == hwnd) return;
    uint32_t old = active;
    if (hwnd && !app_active) { app_active = 1; send(hwnd, WM_ACTIVATEAPP, 1, 0); }
    active = hwnd;
    if (hwnd) last_active = hwnd;
    if (is_window(old)) { send(old, WM_NCACTIVATE, 0, 0); send(old, WM_ACTIVATE, 0 /* WA_INACTIVE */, hwnd); }
    if (is_window(hwnd)) {
        uobj *w = uget(hwnd, H_WINDOW);
        send(hwnd, WM_NCACTIVATE, 1, 0);
        send(hwnd, WM_ACTIVATE, 1 /* WA_ACTIVE */ | (w->minimized ? 0x10000u : 0), old);
    } else if (focus) {
        set_focus(0);
    }
}

/* the application lost or regained the foreground (host) */
static void app_activation(int on)
{
    if (on == app_active) return;
    if (!on) {
        uint32_t a = active;
        if (is_window(a)) { send(a, WM_NCACTIVATE, 0, 0); send(a, WM_ACTIVATE, 0, 0); }
        set_focus(0);
        active = 0;
        app_active = 0;
        if (is_window(a)) send(a, WM_ACTIVATEAPP, 0, 0);
        memset(async_keys, 0, sizeof async_keys);
    } else if (is_window(last_active) && uget(last_active, H_WINDOW)->visible) {
        activate(last_active);
    } else {
        app_active = 1;
    }
}

/* ---- window position and visibility ---- */

static void pos_changed(uint32_t hwnd, uint32_t after, int32_t x, int32_t y, int32_t cx, int32_t cy, uint32_t flags)
{
    uint32_t wp = halopad_heap_alloc(28, 1);
    wr32(wp, hwnd); wr32(wp + 4, after); wr32(wp + 8, (uint32_t)x); wr32(wp + 12, (uint32_t)y);
    wr32(wp + 16, (uint32_t)cx); wr32(wp + 20, (uint32_t)cy); wr32(wp + 24, flags);
    send(hwnd, WM_WINDOWPOSCHANGED, 0, wp);
    halopad_heap_free(wp);
}

static uint32_t set_window_pos(uint32_t hwnd, uint32_t after, int32_t x, int32_t y, int32_t cx, int32_t cy, uint32_t flags)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w || hwnd == DESKTOP) { halopad_last_error = 1400; return 0; }   /* ERROR_INVALID_WINDOW_HANDLE */
    if (flags & ~(0x1u | 0x2u | 0x4u | 0x8u | 0x10u | 0x20u | 0x40u | 0x80u | 0x100u | 0x200u | 0x400u | 0x2000u | 0x4000u))
        hp_unsupported("SetWindowPos", "flags 0x%x", flags);
    if (flags & SWP_NOMOVE) { x = w->x; y = w->y; }
    if (flags & SWP_NOSIZE) { cx = w->w; cy = w->h; }
    if (!(flags & SWP_NOSENDCHANGING)) {
        uint32_t wp = halopad_heap_alloc(28, 1);
        wr32(wp, hwnd); wr32(wp + 4, after); wr32(wp + 8, (uint32_t)x); wr32(wp + 12, (uint32_t)y);
        wr32(wp + 16, (uint32_t)cx); wr32(wp + 20, (uint32_t)cy); wr32(wp + 24, flags);
        send(hwnd, WM_WINDOWPOSCHANGING, 0, wp);
        if (!(flags & SWP_NOMOVE)) { x = (int32_t)rd32(wp + 8); y = (int32_t)rd32(wp + 12); }
        if (!(flags & SWP_NOSIZE)) { cx = (int32_t)rd32(wp + 16); cy = (int32_t)rd32(wp + 20); }
        halopad_heap_free(wp);
        if (!(w = uget(hwnd, H_WINDOW))) return 1;
    }
    if (!(flags & SWP_NOZORDER)) {
        if (after == 0xFFFFFFFFu) w->exstyle |= WS_EX_TOPMOST;               /* HWND_TOPMOST */
        else if (after == 0xFFFFFFFEu) w->exstyle &= ~WS_EX_TOPMOST;         /* HWND_NOTOPMOST */
    }
    if (flags & SWP_FRAMECHANGED) {
        uint32_t p = halopad_heap_alloc(68, 1);
        wr32(p, (uint32_t)x); wr32(p + 4, (uint32_t)y); wr32(p + 8, (uint32_t)(x + cx)); wr32(p + 12, (uint32_t)(y + cy));
        send(hwnd, WM_NCCALCSIZE, 1, p);
        halopad_heap_free(p);
    }
    w->x = x; w->y = y; w->w = cx; w->h = cy;
    if (flags & SWP_SHOWWINDOW) { w->visible = 1; w->invalid = 1; }
    if (flags & SWP_HIDEWINDOW) w->visible = 0;
    if (!(flags & SWP_NOACTIVATE) && w->visible && !(w->style & WS_CHILD)) activate(hwnd);
    host_sync(hwnd);
    pos_changed(hwnd, after, x, y, cx, cy, flags);
    if ((flags & SWP_HIDEWINDOW) && active == hwnd) activate(0);
    return 1;
}

uint32_t SetWindowPos_c(uint32_t hwnd, uint32_t after, uint32_t x, uint32_t y, uint32_t cx, uint32_t cy, uint32_t flags)
{
    return set_window_pos(hwnd, after, (int32_t)x, (int32_t)y, (int32_t)cx, (int32_t)cy, flags);
}

uint32_t MoveWindow_c(uint32_t hwnd, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t repaint)
{
    return set_window_pos(hwnd, 0, (int32_t)x, (int32_t)y, (int32_t)w, (int32_t)h, SWP_NOZORDER | SWP_NOACTIVATE | (repaint ? 0 : 0x8u));
}

uint32_t ShowWindow_c(uint32_t hwnd, uint32_t cmd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w || hwnd == DESKTOP) { halopad_last_error = 1400; return 0; }
    int was = w->visible;
    switch (cmd) {
    case 0:                                                          /* SW_HIDE */
        if (!was) return 0;
        send(hwnd, WM_SHOWWINDOW, 0, 0);
        set_window_pos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        return (uint32_t)was;
    case 1: case 5: case 9: case 10: case 4: case 8: {               /* SHOWNORMAL, SHOW, RESTORE, SHOWDEFAULT, NOACTIVATE, NA */
        int restore = w->minimized && (cmd == 1 || cmd == 9);
        if (!was) send(hwnd, WM_SHOWWINDOW, 1, 0);
        if (restore) { w->minimized = 0; host_sync(hwnd); }
        uint32_t f = SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_SHOWWINDOW;
        if (cmd == 4 || cmd == 8) f |= SWP_NOACTIVATE;
        if (was && !restore) f |= SWP_NOACTIVATE * (active == hwnd);
        set_window_pos(hwnd, 0, 0, 0, 0, 0, f);
        if (restore && (w = uget(hwnd, H_WINDOW))) {
            int32_t cw, ch;
            halopad_window_client_size(hwnd, &cw, &ch);
            send(hwnd, WM_SIZE, 0 /* SIZE_RESTORED */, (uint32_t)(cw & 0xFFFF) | (uint32_t)ch << 16);
        }
        return (uint32_t)was;
    }
    case 6: case 2: case 7:                                          /* SW_MINIMIZE, SHOWMINIMIZED, SHOWMINNOACTIVE */
        if (!was) send(hwnd, WM_SHOWWINDOW, 1, 0);
        w->minimized = 1;
        w->visible = 1;
        host_sync(hwnd);
        send(hwnd, WM_SIZE, 1 /* SIZE_MINIMIZED */, 0);
        if (active == hwnd) { set_focus(0); activate(0); }
        return (uint32_t)was;
    }
    hp_unsupported("ShowWindow", "command %u", cmd);
}

uint32_t DestroyWindow_c(uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w || hwnd == DESKTOP) { halopad_last_error = 1400; return 0; }
    if (w->visible) set_window_pos(hwnd, 0, 0, 0, 0, 0, SWP_HIDEWINDOW | SWP_NOSIZE | SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    if (focus == hwnd) set_focus(0);
    if (active == hwnd) {
        activate(0);
        if (app_active) { app_active = 0; send(hwnd, WM_ACTIVATEAPP, 0, 0); }
    }
    if (capture == hwnd) capture = 0;
    send(hwnd, WM_DESTROY, 0, 0);
    send(hwnd, WM_NCDESTROY, 0, 0);
    uint32_t n = 0;
    for (uint32_t i = 0; i < qcount; i++) if (queue[i].hwnd != hwnd) queue[n++] = queue[i];
    qcount = n;
    if (last_active == hwnd) last_active = 0;
    if ((w = uget(hwnd, H_WINDOW))) {
        if (w->host) halopad_host_window_destroy(w->host);
        w->host = NULL;
        w->kind = H_FREE;
    }
    return 1;
}

uint32_t IsWindow_c(uint32_t hwnd) { return (uint32_t)(hwnd == DESKTOP || is_window(hwnd)); }

uint32_t SetForegroundWindow_c(uint32_t hwnd)
{
    if (!is_window(hwnd)) return 0;
    app_active = 1;
    activate(hwnd);
    return 1;
}
uint32_t SetActiveWindow_c(uint32_t hwnd)
{
    if (hwnd && !is_window(hwnd)) { halopad_last_error = 1400; return 0; }
    uint32_t old = active;
    activate(hwnd);
    return old;
}
uint32_t SetFocus_c(uint32_t hwnd)
{
    if (hwnd && !is_window(hwnd)) { halopad_last_error = 1400; return 0; }
    uint32_t old = focus;
    if (hwnd && active != hwnd && !(uget(hwnd, H_WINDOW)->style & WS_CHILD)) activate(hwnd);
    set_focus(hwnd);
    return old;
}
uint32_t GetForegroundWindow_c(void) { return app_active ? active : 0; }
uint32_t GetActiveWindow_c(void) { return active; }
uint32_t GetFocus_c(void) { return focus; }
/* Classes are registered only through the A functions (RegisterClassW and the other W forms
 * stop the program), so every window is an ANSI window. */
uint32_t IsWindowUnicode_c(uint32_t hwnd) { (void)hwnd; return 0; }

uint32_t SetCapture_c(uint32_t hwnd) { uint32_t old = capture; capture = hwnd; return old; }
uint32_t ReleaseCapture_c(void)
{
    uint32_t old = capture;
    capture = 0;
    if (is_window(old)) send(old, WM_CAPTURECHANGED, 0, 0);
    return 1;
}
uint32_t GetCapture_c(void) { return capture; }

static void update_cursor(void) { halopad_host_cursor(cursor_count >= 0 && cursor_handle != 0); }
uint32_t ShowCursor_c(uint32_t show)
{
    cursor_count += show ? 1 : -1;
    update_cursor();
    return (uint32_t)cursor_count;
}
uint32_t SetCursor_c(uint32_t h)
{
    uint32_t old = cursor_handle;
    if (h && !uget(h, H_CURSOR)) hp_unsupported("SetCursor", "handle 0x%x (not a cursor)", h);
    cursor_handle = h;
    update_cursor();
    return old;
}
uint32_t GetCursorPos_c(uint32_t pt)
{
    if (!pt) return 0;
    wr32(pt, (uint32_t)cursor_x); wr32(pt + 4, (uint32_t)cursor_y);
    return 1;
}
uint32_t ClientToScreen_c(uint32_t hwnd, uint32_t pt)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    int32_t x, y;
    window_screen_client(w, &x, &y);
    wr32(pt, rd32(pt) + (uint32_t)x); wr32(pt + 4, rd32(pt + 4) + (uint32_t)y);
    return 1;
}
uint32_t PtInRect_c(uint32_t rect, uint32_t x, uint32_t y)
{
    int32_t px = (int32_t)x, py = (int32_t)y;
    return px >= (int32_t)rd32(rect) && px < (int32_t)rd32(rect + 8) && py >= (int32_t)rd32(rect + 4) && py < (int32_t)rd32(rect + 12);
}
uint32_t GetDoubleClickTime_c(void) { return 500; }

uint32_t GetSystemMetrics_c(uint32_t i)
{
    int32_t dw, dh;
    desktop_size(&dw, &dh);
    switch (i) {
    case 0: case 78: return (uint32_t)dw;                           /* SM_CXSCREEN, SM_CXVIRTUALSCREEN */
    case 1: case 79: return (uint32_t)dh;
    case 16: return (uint32_t)dw;                                   /* SM_CXFULLSCREEN */
    case 17: return (uint32_t)(dh - CAPTION);
    case 4: return CAPTION;                                         /* SM_CYCAPTION */
    case 5: case 6: return 1;                                       /* SM_CXBORDER, SM_CYBORDER */
    case 7: case 8: return 3;                                       /* SM_CXDLGFRAME */
    case 32: case 33: return FRAME;                                 /* SM_CXFRAME */
    case 11: case 12: case 13: case 14: return 32;                  /* icons and cursors */
    case 49: case 50: return 16;                                    /* small icons */
    case 19: return 1;                                              /* SM_MOUSEPRESENT */
    case 23: return 0;                                              /* SM_SWAPBUTTON */
    case 36: case 37: return 4;                                     /* double-click rectangle */
    case 43: return 3;                                              /* SM_CMOUSEBUTTONS */
    case 76: case 77: return 0;                                     /* virtual screen origin */
    case 80: return 1;                                              /* SM_CMONITORS */
    case 0x1000: return 0;                                          /* SM_REMOTESESSION */
    }
    hp_unsupported("GetSystemMetrics", "index %u", i);
}

/* ---- window data ---- */

uint32_t GetWindowLongA_c(uint32_t hwnd, uint32_t index)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) { halopad_last_error = 1400; return 0; }
    switch ((int32_t)index) {
    case -4: return w->wndproc;                                     /* GWL_WNDPROC */
    case -6: return w->instance;                                    /* GWL_HINSTANCE */
    case -8: return w->parent == DESKTOP ? 0 : w->parent;           /* GWL_HWNDPARENT */
    case -12: return 0;                                             /* GWL_ID */
    case -16: return w->style | (w->visible ? WS_VISIBLE : 0) | (w->disabled ? WS_DISABLED : 0) | (w->minimized ? WS_MINIMIZE : 0);
    case -20: return w->exstyle;
    case -21: return w->userdata;
    }
    hp_unsupported("GetWindowLongA", "index %d", (int32_t)index);
}

uint32_t SetWindowLongA_c(uint32_t hwnd, uint32_t index, uint32_t v)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w || hwnd == DESKTOP) { halopad_last_error = 1400; return 0; }
    uint32_t old;
    switch ((int32_t)index) {
    case -4: old = w->wndproc; w->wndproc = v; return old;
    case -21: old = w->userdata; w->userdata = v; return old;
    case -16: case -20: {
        int ex = (int32_t)index == -20;
        old = ex ? w->exstyle : GetWindowLongA_c(hwnd, index);
        uint32_t ss = halopad_heap_alloc(8, 1);
        wr32(ss, old); wr32(ss + 4, v);
        send(hwnd, WM_STYLECHANGING, index, ss);
        uint32_t nv = rd32(ss + 4);
        if ((w = uget(hwnd, H_WINDOW))) {
            if (ex) w->exstyle = nv;
            else {
                w->style = nv & ~(WS_VISIBLE | WS_DISABLED | WS_MINIMIZE);
                w->visible = !!(nv & WS_VISIBLE); w->disabled = !!(nv & WS_DISABLED); w->minimized = !!(nv & WS_MINIMIZE);
            }
            send(hwnd, WM_STYLECHANGED, index, ss);
        }
        halopad_heap_free(ss);
        return old;
    }
    }
    hp_unsupported("SetWindowLongA", "index %d", (int32_t)index);
}

uint32_t GetParent_c(uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) { halopad_last_error = 1400; return 0; }
    return w->parent == DESKTOP ? 0 : w->parent;
}

uint32_t EnableWindow_c(uint32_t hwnd, uint32_t enable)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    int was_disabled = w->disabled;
    if (was_disabled == !enable) return (uint32_t)was_disabled;
    w->disabled = !enable;
    if (!enable && focus == hwnd) set_focus(0);
    send(hwnd, WM_ENABLE, enable ? 1 : 0, 0);
    return (uint32_t)was_disabled;
}

uint32_t SetWindowTextA_c(uint32_t hwnd, uint32_t text)
{
    if (!is_window(hwnd)) { halopad_last_error = 1400; return 0; }
    return send(hwnd, WM_SETTEXT, 0, text) ? 1 : 0;
}

uint32_t InvalidateRect_c(uint32_t hwnd, uint32_t rect, uint32_t erase)
{
    (void)rect; (void)erase;                                        /* any part: the window repaints */
    uobj *w = uget(hwnd, H_WINDOW);
    if (!hwnd) hp_unsupported("InvalidateRect", "all windows (NULL)");
    if (!w) return 0;
    w->invalid = 1;
    return 1;
}
uint32_t ValidateRect_c(uint32_t hwnd, uint32_t rect)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!hwnd) hp_unsupported("ValidateRect", "all windows (NULL)");
    if (!w) return 0;
    if (rect) hp_unsupported("ValidateRect", "part of a window");
    w->invalid = 0;
    return 1;
}

uint32_t FindWindowA_c(uint32_t cls, uint32_t title)
{
    for (uint32_t i = 1; i < MAXU; i++) {
        uobj *w = &uobjs[i];
        if (w->kind != H_WINDOW || (w->style & WS_CHILD)) continue;
        if (cls) {
            wclass *c = find_class(cls);
            if (!c || (uint32_t)(c - classes) != w->cls) continue;
        }
        if (title && strcmp(w->text, (const char *)G(title))) continue;
        return UBASE + 4 * i;
    }
    return 0;
}

uint32_t GetWindowPlacement_c(uint32_t hwnd, uint32_t p)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) { halopad_last_error = 1400; return 0; }
    wr32(p, 44); wr32(p + 4, 0); wr32(p + 8, w->minimized ? 2 : 1);
    wr32(p + 12, 0xFFFFFFFFu); wr32(p + 16, 0xFFFFFFFFu); wr32(p + 20, 0xFFFFFFFFu); wr32(p + 24, 0xFFFFFFFFu);
    wr32(p + 28, (uint32_t)w->x); wr32(p + 32, (uint32_t)w->y); wr32(p + 36, (uint32_t)(w->x + w->w)); wr32(p + 40, (uint32_t)(w->y + w->h));
    return 1;
}

static int prop_match(uobj *w, int i, uint32_t name)
{
    return name < 0x10000 ? w->props[i].atom == name : (!w->props[i].atom && w->props[i].name[0] && !strcmp(w->props[i].name, G(name)));
}
uint32_t SetPropA_c(uint32_t hwnd, uint32_t name, uint32_t data)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    int free_slot = -1;
    for (int i = 0; i < 8; i++) {
        if (prop_match(w, i, name)) { w->props[i].data = data; return 1; }
        if (free_slot < 0 && !w->props[i].atom && !w->props[i].name[0]) free_slot = i;
    }
    if (free_slot < 0) hp_unsupported("SetPropA", "more than 8 properties on a window");
    if (name < 0x10000) w->props[free_slot].atom = name;
    else snprintf(w->props[free_slot].name, sizeof w->props[free_slot].name, "%s", (const char *)G(name));
    w->props[free_slot].data = data;
    return 1;
}
uint32_t GetPropA_c(uint32_t hwnd, uint32_t name)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    for (int i = 0; i < 8; i++) if (prop_match(w, i, name)) return w->props[i].data;
    return 0;
}
uint32_t RemovePropA_c(uint32_t hwnd, uint32_t name)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (!w) return 0;
    for (int i = 0; i < 8; i++)
        if (prop_match(w, i, name)) { uint32_t d = w->props[i].data; memset(&w->props[i], 0, sizeof w->props[i]); return d; }
    return 0;
}

/* ---- messages ---- */

static int matches(const qmsg *m, uint32_t hwnd, uint32_t lo, uint32_t hi)
{
    if (hwnd == 0xFFFFFFFFu) { if (m->hwnd) return 0; }
    else if (hwnd && m->hwnd != hwnd) return 0;
    return (!lo && !hi) || (m->msg >= lo && m->msg <= hi);
}

static void write_msg(uint32_t out, const qmsg *m)
{
    wr32(out, m->hwnd); wr32(out + 4, m->msg); wr32(out + 8, m->wp); wr32(out + 12, m->lp);
    wr32(out + 16, m->time); wr32(out + 20, (uint32_t)m->x); wr32(out + 24, (uint32_t)m->y);
}

static void retrieved(const qmsg *m)
{
    if (m->msg >= WM_KEYDOWN && m->msg <= 0x0109) {
        last_key = *m;
        uint32_t vk = m->wp & 0xFF;
        if (m->msg == WM_KEYDOWN || m->msg == WM_SYSKEYDOWN) {
            if (!(sync_keys[vk] & 0x80) && (vk == VK_CAPITAL || vk == VK_NUMLOCK || vk == VK_SCROLL)) toggles[vk] ^= 1;
            sync_keys[vk] = 0x80;
        } else if (m->msg == WM_KEYUP || m->msg == WM_SYSKEYUP) {
            sync_keys[vk] = 0;
        }
    }
}

uint32_t PeekMessageA_c(uint32_t out, uint32_t hwnd, uint32_t lo, uint32_t hi, uint32_t flags)
{
    if (flags & ~0x3u) hp_unsupported("PeekMessageA", "flags 0x%x", flags);   /* PM_REMOVE, PM_NOYIELD */
    halopad_host_pump();
    for (uint32_t i = 0; i < qcount; i++) {
        if (!matches(&queue[i], hwnd, lo, hi)) continue;
        qmsg m = queue[i];
        if (out) write_msg(out, &m);
        if (flags & 1) {
            memmove(&queue[i], &queue[i + 1], (qcount - i - 1) * sizeof queue[0]);
            qcount--;
            retrieved(&m);
        }
        return 1;
    }
    qmsg q = {0};
    if (quit_posted && ((!lo && !hi) || (WM_QUIT >= lo && WM_QUIT <= hi))) {   /* whatever the window filter */
        q.msg = WM_QUIT; q.wp = quit_code; q.time = GetTickCount_c(); q.x = cursor_x; q.y = cursor_y;
        if (out) write_msg(out, &q);
        if (flags & 1) quit_posted = 0;
        return 1;
    }
    for (uint32_t i = 1; i < MAXU; i++) {
        uobj *w = &uobjs[i];
        if (w->kind != H_WINDOW || !w->invalid || !w->visible || w->minimized) continue;
        q.hwnd = UBASE + 4 * i; q.msg = WM_PAINT; q.time = GetTickCount_c(); q.x = cursor_x; q.y = cursor_y;
        if (!matches(&q, hwnd, lo, hi)) continue;
        if (out) write_msg(out, &q);                                /* WM_PAINT stays until the window is validated */
        return 1;
    }
    return 0;
}

/* UTF-16 from the host keyboard to Windows-1252, as an ANSI window receives WM_CHAR */
static uint32_t ansi_char(uint16_t c)
{
    static const uint16_t cp1252[32] = {0x20AC, 0, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0, 0x017D, 0,
                                        0, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0, 0x017E, 0x0178};
    if (c < 0x80 || (c >= 0xA0 && c <= 0xFF)) return c;
    for (int i = 0; i < 32; i++) if (cp1252[i] == c) return 0x80u + (uint32_t)i;
    return '?';
}

uint32_t TranslateMessage_c(uint32_t msg)
{
    uint32_t m = rd32(msg + 4);
    if (m != WM_KEYDOWN && m != WM_KEYUP && m != WM_SYSKEYDOWN && m != WM_SYSKEYUP) return 0;
    if ((m == WM_KEYDOWN || m == WM_SYSKEYDOWN) && last_key.msg == m && last_key.hwnd == rd32(msg) && last_key.wp == rd32(msg + 8)
        && last_key.lp == rd32(msg + 12) && last_key.time == rd32(msg + 16)) {
        /* the characters go ahead of anything queued since, as Windows posts them */
        uint32_t n = last_key.nchars;
        if (qcount + n > QMAX) hp_unsupported("TranslateMessage", "a full message queue");
        memmove(&queue[n], &queue[0], qcount * sizeof queue[0]);
        for (uint32_t i = 0; i < n; i++) {
            qmsg *c = &queue[i];
            memset(c, 0, sizeof *c);
            c->hwnd = last_key.hwnd; c->msg = m == WM_KEYDOWN ? WM_CHAR : WM_SYSCHAR; c->wp = ansi_char(last_key.chars[i]);
            c->lp = last_key.lp; c->time = last_key.time; c->x = last_key.x; c->y = last_key.y;
        }
        qcount += n;
        last_key.nchars = 0;
    }
    return 1;
}

uint32_t DispatchMessageA_c(uint32_t msg)
{
    uint32_t hwnd = rd32(msg), m = rd32(msg + 4), wp = rd32(msg + 8), lp = rd32(msg + 12);
    if (m == WM_TIMER && lp) hp_unsupported("DispatchMessageA", "WM_TIMER with a callback");
    if (!hwnd) return 0;
    if (!is_window(hwnd)) { halopad_last_error = 1400; return 0; }
    return send(hwnd, m, wp, lp);
}

uint32_t PostQuitMessage_c(uint32_t code) { quit_posted = 1; quit_code = code; return 0; }

uint32_t SendMessageA_c(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    if (hwnd == 0xFFFFu) hp_unsupported("SendMessageA", "HWND_BROADCAST (message 0x%04x)", msg);
    if (!is_window(hwnd)) { halopad_last_error = 1400; return 0; }
    return send(hwnd, msg, wp, lp);
}

uint32_t CallWindowProcA_c(uint32_t proc, uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    if (!proc) return 0;
    uint32_t args[4] = {hwnd, msg, wp, lp};
    return halopad_call_guest(proc, 4, args);
}

/* QS_* wake mask bits the queue can satisfy: all queued messages are posted or input */
uint32_t MsgWaitForMultipleObjects_c(uint32_t n, uint32_t handles, uint32_t all, uint32_t ms, uint32_t mask)
{
    if (all && n > 1) hp_unsupported("MsgWaitForMultipleObjects", "waiting for all of %u objects", n);
    if (mask & ~0x4FFu) hp_unsupported("MsgWaitForMultipleObjects", "wake mask 0x%x", mask);
    uint32_t start = GetTickCount_c();
    for (;;) {
        int i = n ? halopad_wait_poll(n, handles) : -1;
        if (i >= 0) return (uint32_t)i;                             /* WAIT_OBJECT_0 + i */
        halopad_host_pump();
        if (mask && (qcount || quit_posted)) return n;              /* WAIT_OBJECT_0 + n: input is available */
        if (ms != 0xFFFFFFFFu && GetTickCount_c() - start >= ms) return 0x102;   /* WAIT_TIMEOUT */
        Sleep_c(1);
    }
}

/* ---- key state ---- */

uint32_t GetAsyncKeyState_c(uint32_t vk)
{
    vk &= 0xFF;
    if (!app_active) return 0;                                      /* another application has the foreground */
    uint32_t r = (async_keys[vk] ? 0x8000u : 0) | (async_pressed[vk] ? 1u : 0);
    async_pressed[vk] = 0;
    return (uint32_t)(int32_t)(int16_t)r;
}
uint32_t GetKeyState_c(uint32_t vk)
{
    vk &= 0xFF;
    uint32_t r = (sync_keys[vk] & 0x80 ? 0x8000u : 0) | toggles[vk];
    return (uint32_t)(int32_t)(int16_t)r;
}

/* ---- host input ---- */

static uint32_t mk_flags(void)
{
    return (async_keys[1] ? 1u : 0) | (async_keys[2] ? 2u : 0) | (async_keys[VK_SHIFT] ? 4u : 0) | (async_keys[VK_CONTROL] ? 8u : 0)
         | (async_keys[4] ? 0x10u : 0);
}

static void set_key(uint32_t vk, int down)
{
    if (down && !async_keys[vk]) async_pressed[vk] = 1;
    async_keys[vk] = (uint8_t)down;
}

void halopad_input_event(const hp_input *e)
{
    halopad_dinput_event(e);
    uint32_t target = capture ? capture : active;
    uobj *w = target ? uget(target, H_WINDOW) : NULL;
    switch (e->kind) {
    case HPI_ACTIVATE:
        app_activation(e->down);
        return;
    case HPI_CLOSE:
        if (w) post(target, WM_SYSCOMMAND, SC_CLOSE, (uint32_t)(cursor_x & 0xFFFF) | (uint32_t)cursor_y << 16, NULL, 0);
        return;
    case HPI_KEY: {
        uint32_t vk = e->vk & 0xFF;
        int was = async_keys[vk];
        set_key(vk, e->down);
        if (e->side_vk) set_key(e->side_vk & 0xFF, e->down);
        if (!focus && !active) return;
        int alt = async_keys[VK_MENU] && !async_keys[VK_CONTROL];
        int sys = alt || vk == VK_F10 || (vk == VK_MENU && !async_keys[VK_CONTROL]);
        uint32_t msg = e->down ? (sys ? WM_SYSKEYDOWN : WM_KEYDOWN) : (sys ? WM_SYSKEYUP : WM_KEYUP);
        uint32_t lp = 1u | (e->scan & 0xFF) << 16 | (e->extended ? 1u << 24 : 0) | (alt ? 1u << 29 : 0)
                    | ((e->down ? was : 1) ? 1u << 30 : 0) | (e->down ? 0 : 1u << 31);
        post(focus ? focus : active, msg, vk, lp, e->down ? e->chars : NULL, e->down ? e->nchars : 0);
        return;
    }
    case HPI_MOUSEMOVE: case HPI_BUTTON: case HPI_WHEEL: {
        if (!w) return;
        int32_t ox, oy;
        window_screen_client(w, &ox, &oy);
        cursor_x = ox + e->x; cursor_y = oy + e->y;
        uint32_t lp = (uint32_t)(e->x & 0xFFFF) | (uint32_t)(e->y & 0xFFFF) << 16;
        if (e->kind == HPI_MOUSEMOVE) {
            if (qcount && queue[qcount - 1].msg == WM_MOUSEMOVE && queue[qcount - 1].hwnd == target) qcount--;   /* coalesced */
            post(target, WM_MOUSEMOVE, mk_flags(), lp, NULL, 0);
        } else if (e->kind == HPI_WHEEL) {
            post(focus ? focus : target, WM_MOUSEWHEEL, mk_flags() | (uint32_t)(e->wheel & 0xFFFF) << 16,
                 (uint32_t)(cursor_x & 0xFFFF) | (uint32_t)cursor_y << 16, NULL, 0);
        } else {
            static const uint32_t vk_of[3] = {1, 2, 4};             /* VK_LBUTTON, VK_RBUTTON, VK_MBUTTON */
            static const uint32_t base[3] = {WM_LBUTTONDOWN, 0x0204, 0x0207};
            if (e->button < 0 || e->button > 2) return;
            set_key(vk_of[e->button], e->down);
            uint32_t msg = base[e->button] + (e->down ? 0 : 1);
            if (e->down) {
                uint32_t now = GetTickCount_c();
                if ((classes[w->cls].style & 0x8) && last_click_msg == msg && now - last_click_time <= 500
                    && abs(last_click_x - cursor_x) <= 2 && abs(last_click_y - cursor_y) <= 2) {
                    msg += 2;                                        /* CS_DBLCLKS: the second click is a double click */
                    last_click_msg = 0;
                } else {
                    last_click_msg = msg; last_click_time = now; last_click_x = cursor_x; last_click_y = cursor_y;
                }
            }
            post(target, msg, mk_flags(), lp, NULL, 0);
        }
        return;
    }
    }
}

/* ---- DefWindowProcA beyond creation ---- */

static uint32_t default_message(uobj *w, uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    switch (msg) {
    case WM_CLOSE: DestroyWindow_c(hwnd); return 0;
    case WM_SYSCOMMAND:
        switch (wp & 0xFFF0) {
        case SC_CLOSE: send(hwnd, WM_CLOSE, 0, 0); return 0;
        case SC_MINIMIZE: ShowWindow_c(hwnd, 6); return 0;
        case SC_RESTORE: ShowWindow_c(hwnd, 9); return 0;
        case SC_KEYMENU: case SC_SCREENSAVE: case SC_MONITORPOWER: return 0;   /* no menu; no screen saver */
        }
        hp_unsupported("DefWindowProcA", "WM_SYSCOMMAND 0x%04x", wp);
    case WM_SYSKEYDOWN:
        if (wp == VK_F4 && (lp & (1u << 29))) send(hwnd, WM_SYSCOMMAND, SC_CLOSE, 0);
        return 0;
    case WM_ACTIVATE:
        if ((wp & 0xFFFF) && !(wp >> 16) && !w->minimized) set_focus(hwnd);
        return 0;
    case WM_NCACTIVATE: return 1;
    case WM_WINDOWPOSCHANGED: {
        uint32_t f = rd32(lp + 24);
        int first = !w->sizemove_sent;
        w->sizemove_sent = 1;
        if (!(f & SWP_NOSIZE) || first) {
            int32_t cw, ch;
            halopad_window_client_size(hwnd, &cw, &ch);
            send(hwnd, WM_SIZE, w->minimized ? 1 : 0, (uint32_t)(cw & 0xFFFF) | (uint32_t)ch << 16);
        }
        if ((!(f & SWP_NOMOVE) || first) && (w = uget(hwnd, H_WINDOW))) {
            int32_t x, y;
            window_screen_client(w, &x, &y);
            send(hwnd, WM_MOVE, 0, (uint32_t)(x & 0xFFFF) | (uint32_t)y << 16);
        }
        return 0;
    }
    case WM_PAINT: w->invalid = 0; return 0;
    case WM_ERASEBKGND: return classes[w->cls].brush ? 1 : 0;
    case WM_NCHITTEST: return 1;                                    /* HTCLIENT */
    case WM_SETCURSOR:
        if ((lp & 0xFFFF) == 1 && classes[w->cls].cursor) SetCursor_c(classes[w->cls].cursor);
        return 0;
    case WM_SETTEXT:
        snprintf(w->text, sizeof w->text, "%s", lp ? (const char *)G(lp) : "");
        if (w->host) halopad_host_window_title(w->host, w->text);
        return 1;
    case WM_GETTEXT: {
        if (!wp) return 0;
        uint32_t n = (uint32_t)strlen(w->text);
        if (n > wp - 1) n = wp - 1;
        memcpy(G(lp), w->text, n);
        ((char *)G(lp))[n] = 0;
        return n;
    }
    case WM_GETTEXTLENGTH: return (uint32_t)strlen(w->text);
    case WM_DESTROY: case WM_NCDESTROY: case WM_MOVE: case WM_SIZE: case WM_SETFOCUS: case WM_KILLFOCUS: case WM_ENABLE:
    case WM_SHOWWINDOW: case WM_WINDOWPOSCHANGING: case WM_ACTIVATEAPP: case WM_DISPLAYCHANGE: case WM_INPUTLANGCHANGE:
    case WM_STYLECHANGING: case WM_STYLECHANGED: case WM_CAPTURECHANGED:
    case WM_KEYDOWN: case WM_KEYUP: case WM_CHAR: case WM_SYSKEYUP: case WM_SYSCHAR:
    case WM_MOUSEWHEEL:
        return 0;
    }
    if (msg >= WM_MOUSEMOVE && msg <= 0x0209) return 0;             /* mouse movement and buttons */
    hp_unsupported("DefWindowProcA", "message 0x%04x (wParam 0x%x, lParam 0x%x)", msg, wp, lp);
}

/* ---- wsprintfA (cdecl, variable arguments on the guest stack) ---- */

uint32_t halopad_guest_esp(void);

uint32_t wsprintfA_c(uint32_t buf, uint32_t fmt)
{
    uint32_t ap = halopad_guest_esp() + 12;                         /* return address, buf, fmt, then the arguments */
    const char *f = G(fmt);
    char out[1025];
    size_t n = 0;
    while (*f && n < 1024) {
        if (*f != '%') { out[n++] = *f++; continue; }
        const char *start = f++;
        if (*f == '%') { out[n++] = '%'; f++; continue; }
        char spec[32];
        size_t k = 0;
        spec[k++] = '%';
        while (*f && strchr("-#0", *f) && k < 20) spec[k++] = *f++;
        while (*f >= '0' && *f <= '9' && k < 20) spec[k++] = *f++;
        if (*f == '.') { spec[k++] = *f++; while (*f >= '0' && *f <= '9' && k < 20) spec[k++] = *f++; }
        if (*f == 'l' || *f == 'h') f++;
        char c = *f ? *f++ : 0;
        char tmp[1100];
        switch (c) {
        case 'd': case 'i': case 'u': case 'x': case 'X':
            spec[k++] = c; spec[k] = 0;
            if (c == 'd' || c == 'i') snprintf(tmp, sizeof tmp, spec, (int32_t)rd32(ap));
            else snprintf(tmp, sizeof tmp, spec, rd32(ap));
            ap += 4;
            break;
        case 'c':
            spec[k++] = 'c'; spec[k] = 0;
            snprintf(tmp, sizeof tmp, spec, (int)(rd32(ap) & 0xFF));
            ap += 4;
            break;
        case 's': {
            spec[k++] = 's'; spec[k] = 0;
            uint32_t s = rd32(ap);
            ap += 4;
            snprintf(tmp, sizeof tmp, spec, s ? (const char *)G(s) : "(null)");
            break;
        }
        default:
            hp_unsupported("wsprintfA", "conversion '%.*s'", (int)(f - start), start);
        }
        for (const char *p = tmp; *p && n < 1024; p++) out[n++] = *p;
    }
    out[n] = 0;
    memcpy(G(buf), out, n + 1);
    return (uint32_t)n;
}

uint32_t MessageBoxA_c(uint32_t hwnd, uint32_t text, uint32_t caption, uint32_t type)
{
    (void)hwnd;
    hp_unsupported("MessageBoxA", "a message box (type 0x%x) \"%s\": \"%s\"", type, caption ? (const char *)G(caption) : "Error",
                   text ? (const char *)G(text) : "");
}

/* The reference machine's only keyboard layout: US English (HKL 0x04090409). */
uint32_t GetKeyboardLayout_c(uint32_t thread) { (void)thread; return 0x04090409u; }

/* CharNextExA in a single-byte code page: the next character, or the same pointer at the end. */
uint32_t CharNextExA_c(uint32_t cp, uint32_t p, uint32_t flags)
{
    if (flags) hp_unsupported("CharNextExA", "flags 0x%x", flags);
    uint32_t c = cp == 0 || cp == 3 ? 1252 : cp == 1 ? 437 : cp;
    if (c != 1252 && c != 437) hp_unsupported("CharNextExA", "code page %u", cp);
    if (!p) return 0;
    return *(uint8_t *)G(p) ? p + 1 : p;
}

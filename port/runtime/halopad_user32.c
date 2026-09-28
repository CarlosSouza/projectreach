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
                 char text[1024]; uint32_t userdata; int visible;
                 uint32_t id; int ctl;                   /* dialogs and their controls (part 3): control id, CTL_* */
                 uint32_t check, font, icon;             /* controls: check state, WM_SETFONT's font, a static's icon id */
                 uint32_t dlgproc, msgresult, dlguser, dlgresult; int ended;   /* dialogs: DWL_*, EndDialog */
                 uint32_t wndproc;                       /* the window's procedure (the class's, unless subclassed) */
                 int disabled, minimized, invalid, sizemove_sent;
                 void *host;                             /* the host window, once shown (top-level windows) */
                 struct { char name[32]; uint32_t atom, data; } props[8]; } uobj;
#define UBASE 0x00010010u
#define MAXU 256
static uobj uobjs[MAXU];
enum { CTL_NONE, CTL_DIALOG, CTL_BUTTON, CTL_STATIC };

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
    int32_t x = w->x, y = w->y;
    if (w->style & WS_CHILD) {                                          /* a child's rectangle is kept in its parent's client space */
        void halopad_window_client_origin(uint32_t hwnd, int32_t *x, int32_t *y);
        int32_t px, py;
        halopad_window_client_origin(w->parent, &px, &py);
        x += px; y += py;
    }
    wr32(rect, (uint32_t)x); wr32(rect + 4, (uint32_t)y);
    wr32(rect + 8, (uint32_t)(x + w->w)); wr32(rect + 12, (uint32_t)(y + w->h));
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
    if ((int32_t)x == (int32_t)0x80000000 || (int32_t)w == (int32_t)0x80000000) {
        /* CW_USEDEFAULT: a pop-up window gets position (0, 0) and, for the size, 0 by 0 (Windows'
           rule; only overlapped windows get a default placement) */
        if (!(style & WS_POPUP)) hp_unsupported("CreateWindowExA", "CW_USEDEFAULT for an overlapped window (default placement)");
        if ((int32_t)x == (int32_t)0x80000000) x = y = 0;
        if ((int32_t)w == (int32_t)0x80000000) w = h = 0;
    }
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
static int app_active, app_activation_pending;
static uint64_t system_event_serial;                 /* host notifications handled synchronously */
static int32_t cursor_x, cursor_y;                    /* screen */
static int cursor_count;                              /* ShowCursor's display count */
static uint32_t cursor_handle;
static uint8_t async_keys[256], async_pressed[256], sync_keys[256], toggles[256];
static uint8_t mouse_physical[3], mouse_touch[3];
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
    uobj *p = (w->style & WS_CHILD) ? uget(w->parent, H_WINDOW) : NULL;
    if (p) { int32_t px, py; window_screen_client(p, &px, &py); *x += px; *y += py; }
}
/* the screen position of a window's client area (0,0 for none or the desktop) */
void halopad_window_client_origin(uint32_t hwnd, int32_t *x, int32_t *y)
{
    uobj *w = hwnd == DESKTOP ? NULL : uget(hwnd, H_WINDOW);
    *x = *y = 0;
    if (w) window_screen_client(w, x, y);
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
    if (!w || hwnd == DESKTOP || (w->style & WS_CHILD) || w->ctl == CTL_DIALOG) return;   /* dialogs: the host shows them (part 3) */
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
    /* A host scene may become active before a guest window exists. No guest has
       received the app activation in that case; deliver it to the first window. */
    if (hwnd && (!app_active || app_activation_pending)) {
        app_active = 1;
        app_activation_pending = 0;
        send(hwnd, WM_ACTIVATEAPP, 1, 0);
    }
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
        app_activation_pending = 0;
        if (is_window(a)) send(a, WM_ACTIVATEAPP, 0, 0);
        memset(async_keys, 0, sizeof async_keys);
        memset(mouse_physical, 0, sizeof mouse_physical);
        memset(mouse_touch, 0, sizeof mouse_touch);
    } else if (is_window(last_active) && uget(last_active, H_WINDOW)->visible) {
        activate(last_active);
    } else {
        app_active = 1;
        app_activation_pending = 1;
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
    for (uint32_t i = 1; i < MAXU; i++) {                           /* then its children, as Windows destroys them */
        uobj *c = &uobjs[i];
        if (c->kind == H_WINDOW && (c->style & WS_CHILD) && c->parent == hwnd) DestroyWindow_c(UBASE + 4 * i);
    }
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
/* the owned pop-up last active for a window: HaloPad's only owned pop-ups are dialogs, and a
   modal dialog is the active window while it runs */
uint32_t GetLastActivePopup_c(uint32_t hwnd)
{
    if (!is_window(hwnd)) return hwnd;
    uobj *a = uget(active, H_WINDOW);
    return a && a->parent == hwnd && !(a->style & WS_CHILD) ? active : hwnd;
}
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
    case -12: return w->id;                                         /* GWL_ID */
    case 0: case 4: case 8:                                         /* DWL_MSGRESULT, DWL_DLGPROC, DWL_USER */
        if (w->ctl != CTL_DIALOG) break;
        return (int32_t)index == 0 ? w->msgresult : (int32_t)index == 4 ? w->dlgproc : w->dlguser;
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
    case -12: old = w->id; w->id = v; return old;
    case 0: case 4: case 8:
        if (w->ctl != CTL_DIALOG) break;
        if ((int32_t)index == 0) { old = w->msgresult; w->msgresult = v; }
        else if ((int32_t)index == 4) { old = w->dlgproc; w->dlgproc = v; }
        else { old = w->dlguser; w->dlguser = v; }
        return old;
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
    uint64_t system_events = system_event_serial;
    for (;;) {
        int i = n ? halopad_wait_poll(n, handles) : -1;
        if (i >= 0) return (uint32_t)i;                             /* WAIT_OBJECT_0 + i */
        halopad_host_pump();
        /* Foreground activation is a system-event wake even with dwWakeMask == 0.
           The pump already delivered its window callbacks, so it need not leave
           a posted message. Keep Halo from sleeping after it has been reactivated. */
        if (system_event_serial != system_events) return n;
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
    case HPI_CANCEL_TOUCH:
        for (int b = 0; b < 3; b++) if (mouse_touch[b]) {
            hp_input release = {.kind = HPI_BUTTON, .flags = HPI_TOUCH, .button = b};
            if (w) { int32_t ox, oy; window_screen_client(w, &ox, &oy); release.x = cursor_x - ox; release.y = cursor_y - oy; }
            halopad_input_event(&release);
        }
        return;
    case HPI_ACTIVATE:
        system_event_serial++;
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
            int b = e->button;
            int before = mouse_physical[b] || mouse_touch[b];
            if (e->flags & HPI_TOUCH) mouse_touch[b] = !!e->down;
            else mouse_physical[b] = !!e->down;
            int after = mouse_physical[b] || mouse_touch[b];
            if (before == after) return;
            set_key(vk_of[b], after);
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


/* ==== part 3 (G3): dialog boxes ====
 *
 * DialogBoxParamA/DialogBoxIndirectParamA build a real dialog from its template
 * (DLGTEMPLATE or DLGTEMPLATEEX): the dialog window (class #32770, window procedure
 * DefDlgProcA calling the application's dialog procedure) and its controls as child windows
 * of the built-in Button and Static classes, whose window procedures have guest addresses so
 * that an application can subclass them (Halo's hyperlink does). Messages follow Windows:
 * WM_SETFONT to the dialog and to each control, WM_INITDIALOG, WM_COMMAND notifications from
 * the controls, DefDlgProc's WM_CLOSE (IDCANCEL). Dialog units map to pixels with Windows XP's
 * MS Shell Dlg 2 (Tahoma 8 pt) at 96 DPI: 6 by 13 base units.
 *
 * The modal loop shows the dialog's current state through the host (halopad_dialog.h) and
 * turns the player's action into the message Windows would send: BM_CLICK to a button or
 * checkbox, button down and up to a notifying static (a link), SC_CLOSE to the dialog. Without
 * a screen, HALOPAD_DIALOG_ACTIONS scripts the actions (control ids, or "close", separated by
 * commas); with neither, the dialog is printed and the run stops, since only the player can
 * answer it. */
#include "halopad_dialog.h"

#define WM_SETFONT 0x0030
#define WM_GETFONT 0x0031
#define WM_INITDIALOG 0x0110
#define WM_COMMAND 0x0111
#define WM_CTLCOLORMSGBOX 0x0132
#define WM_CTLCOLORBTN 0x0135
#define WM_CTLCOLORDLG 0x0136
#define WM_CTLCOLORSTATIC 0x0138
#define WM_LBUTTONUP 0x0202
#define BM_GETCHECK 0x00F0
#define BM_SETCHECK 0x00F1
#define BM_GETSTATE 0x00F2
#define BM_SETSTATE 0x00F3
#define BM_CLICK 0x00F5
#define DS_FIXEDSYS 0x08u
#define DS_SETFONT 0x40u
#define SS_NOTIFY 0x100u
#define WS_TABSTOP 0x00010000u

uint32_t CreateFontA_c(uint32_t args);
uint32_t CreateCompatibleDC_c(uint32_t dc);
uint32_t DeleteDC_c(uint32_t dc);
uint32_t GetTextColor_c(uint32_t dc);
uint32_t SetTextColor_c(uint32_t dc, uint32_t color);
uint32_t LoadResource_c(uint32_t module, uint32_t hrsrc);
int halopad_host_path(const char *guest, char *out, size_t size);
extern const uint32_t halopad_import_count, halopad_import_base, halopad_import_stride;
extern const char *const halopad_import_names[];

/* no screen: the macOS runner and tests (the iPadOS app shell provides both) */
__attribute__((weak)) int halopad_host_dialog(const halopad_dialog_view *v) { (void)v; return HPD_NO_SCREEN; }
__attribute__((weak)) int halopad_host_open_url(const char *url) { (void)url; return 0; }
__attribute__((weak)) void halopad_host_dialog_done(void) {}
/* Test support: when set, ShellExecuteA hands URLs here instead of to the host. */
int (*halopad_shell_open_hook)(const char *url);

static uint32_t runtime_proc(const char *name)
{
    for (uint32_t i = 0; i < halopad_import_count; i++)
        if (!strcmp(halopad_import_names[i], name)) return halopad_import_base + halopad_import_stride * i;
    hp_unsupported("USER32", "window procedure %s has no guest address (config/runtime/dynamic-exports.txt)", name);
}

/* the built-in classes, registered on first use */
static uint32_t builtin_class(const char *name, const char *proc)
{
    for (int i = 0; i < 32; i++) if (classes[i].used && !strcasecmp(classes[i].name, name)) return (uint32_t)i;
    for (int i = 0; i < 32; i++) {
        if (classes[i].used) continue;
        memset(&classes[i], 0, sizeof classes[i]);
        snprintf(classes[i].name, sizeof classes[i].name, "%s", name);
        classes[i].wndproc = runtime_proc(proc);
        classes[i].used = 1;
        return (uint32_t)i;
    }
    hp_unsupported("USER32", "more than 32 classes");
}

static uint16_t g16(uint32_t a) { uint16_t v; memcpy(&v, G(a), 2); return v; }

/* a template field: an ordinal (returned) or a UTF-16 string (as Windows-1252 in out; 0 returned) */
static uint32_t tmpl_field(uint32_t *p, char *out, size_t n)
{
    out[0] = 0;
    if (g16(*p) == 0xFFFF) { uint32_t o = g16(*p + 2); *p += 4; return o; }
    size_t k = 0;
    for (uint16_t c; (c = g16(*p)) != 0; *p += 2) if (k + 1 < n) out[k++] = c < 0x100 ? (char)c : '?';
    out[k] = 0;
    *p += 2;
    return 0;
}

#define DLG_BASE_X 6
#define DLG_BASE_Y 13
static int32_t dlu_x(int32_t v) { return (v * DLG_BASE_X + (v >= 0 ? 2 : -2)) / 4; }   /* MapDialogRect: MulDiv(v, base, 4 or 8) */
static int32_t dlu_y(int32_t v) { return (v * DLG_BASE_Y + (v >= 0 ? 4 : -4)) / 8; }

static uint32_t guest_text(const char *s)
{
    uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0);
    memcpy(G(g), s, strlen(s) + 1);
    return g;
}

/* One window with the creation messages CreateWindowExA sends, for the dialog manager's own
   windows (the dialog and its controls). title_ord: a resource ordinal as the title (icons). */
static uint32_t dlg_window(uint32_t cls, int ctl, uint32_t style, uint32_t exstyle, int32_t x, int32_t y, int32_t w, int32_t h,
                           uint32_t parent, uint32_t id, const char *title, uint32_t title_ord, uint32_t instance, uint32_t param)
{
    uint32_t hwnd = unew(H_WINDOW);
    uobj *o = uget(hwnd, H_WINDOW);
    o->cls = cls; o->ctl = ctl; o->style = style & ~(WS_VISIBLE | WS_DISABLED);
    o->visible = !!(style & WS_VISIBLE); o->disabled = !!(style & WS_DISABLED);
    o->exstyle = exstyle; o->instance = instance; o->parent = parent; o->id = id;
    o->wndproc = classes[cls].wndproc;
    o->x = x; o->y = y; o->w = w; o->h = h;
    uint32_t t;
    if (title_ord) { t = halopad_heap_alloc(4, 0); wr32(t, 0xFFFFu | title_ord << 16); }
    else t = guest_text(title);
    uint32_t cs = halopad_heap_alloc(48, 1);
    wr32(cs + 0, param); wr32(cs + 4, instance); wr32(cs + 8, (style & WS_CHILD) ? id : 0); wr32(cs + 12, parent);
    wr32(cs + 16, (uint32_t)h); wr32(cs + 20, (uint32_t)w); wr32(cs + 24, (uint32_t)y); wr32(cs + 28, (uint32_t)x);
    wr32(cs + 32, style); wr32(cs + 36, t); wr32(cs + 40, 0); wr32(cs + 44, exstyle);
    uint32_t ok = send(hwnd, WM_NCCREATE, 0, cs);
    if (ok) {
        uint32_t rc = halopad_heap_alloc(16, 1);
        wr32(rc, (uint32_t)x); wr32(rc + 4, (uint32_t)y); wr32(rc + 8, (uint32_t)(x + w)); wr32(rc + 12, (uint32_t)(y + h));
        send(hwnd, WM_NCCALCSIZE, 0, rc);
        halopad_heap_free(rc);
        if (send(hwnd, WM_CREATE, 0, cs) == 0xFFFFFFFFu) ok = 0;
    }
    halopad_heap_free(cs);
    halopad_heap_free(t);
    if (!ok) { if ((o = uget(hwnd, H_WINDOW))) o->kind = H_FREE; return 0; }
    return hwnd;
}

/* The dialog font: LOGFONT height -MulDiv(points, 96, 72). */
static uint32_t dialog_font(uint32_t points, uint32_t weight, uint32_t italic, uint32_t charset, const char *face)
{
    uint32_t a = halopad_heap_alloc(56, 1), f = guest_text(face);
    wr32(a, (uint32_t)-(int32_t)((points * 96 + 36) / 72)); wr32(a + 16, weight ? weight : 400);
    wr32(a + 20, italic); wr32(a + 32, charset); wr32(a + 52, f);
    uint32_t h = CreateFontA_c(a);
    halopad_heap_free(a);
    halopad_heap_free(f);
    return h;
}

static uint32_t create_dialog(uint32_t instance, uint32_t tmpl, uint32_t owner, uint32_t proc, uint32_t param)
{
    int ext = g16(tmpl) == 1 && g16(tmpl + 2) == 0xFFFF;
    uint32_t style, exstyle, n, p;
    int16_t x, y, cx, cy;
    if (ext) { exstyle = rd32(tmpl + 8); style = rd32(tmpl + 12); n = g16(tmpl + 16); p = tmpl + 18; }
    else { style = rd32(tmpl); exstyle = rd32(tmpl + 4); n = g16(tmpl + 8); p = tmpl + 10; }
    x = (int16_t)g16(p); y = (int16_t)g16(p + 2); cx = (int16_t)g16(p + 4); cy = (int16_t)g16(p + 6); p += 8;
    char menu[64], cls[64], title[256], face[64] = "";
    uint32_t menu_ord = tmpl_field(&p, menu, sizeof menu), cls_ord = tmpl_field(&p, cls, sizeof cls);
    tmpl_field(&p, title, sizeof title);
    if (menu_ord || menu[0] || cls_ord || cls[0]) hp_unsupported("DialogBox", "a dialog with a menu or its own class (\"%s\", \"%s\")", menu, cls);
    uint32_t points = 0, weight = 0, italic = 0, charset = 1;           /* DEFAULT_CHARSET */
    if (style & DS_SETFONT) {
        points = g16(p); p += 2;
        if (ext) { weight = g16(p); italic = *(uint8_t *)G(p + 2); charset = *(uint8_t *)G(p + 3); p += 4; }
        tmpl_field(&p, face, sizeof face);
    }
    if ((style & (DS_SETFONT | DS_FIXEDSYS)) == (DS_SETFONT | DS_FIXEDSYS) && !strcmp(face, "MS Shell Dlg")) snprintf(face, sizeof face, "MS Shell Dlg 2");
    if (!(style & DS_SETFONT) || points != 8 || (strcmp(face, "MS Shell Dlg 2") && strcmp(face, "MS Shell Dlg")))
        hp_unsupported("DialogBox", "dialog font %u pt \"%s\" (only MS Shell Dlg 8 pt has base units)", points, face);
    if (style & 0x1u /* DS_ABSALIGN */) hp_unsupported("DialogBox", "DS_ABSALIGN");

    uint32_t dcls = builtin_class("#32770", "DefDlgProcA");
    uint32_t bcls = builtin_class("Button", "HaloPadButtonWndProc"), scls = builtin_class("Static", "HaloPadStaticWndProc");
    int32_t l, t, r, b;
    frame_insets(style, &l, &t, &r, &b);
    int32_t ox, oy;
    halopad_window_client_origin(owner, &ox, &oy);
    uint32_t font = dialog_font(points, weight, italic, charset, face);

    /* the dialog procedure sees the messages from WM_SETFONT on; creation messages go to DefDlgProc alone */
    uint32_t dlg = dlg_window(dcls, CTL_DIALOG, style & ~WS_VISIBLE, exstyle, ox + dlu_x(x), oy + dlu_y(y),
                              dlu_x(cx) + l + r, dlu_y(cy) + t + b, owner, 0, title, 0, instance, param);
    if (!dlg) hp_unsupported("DialogBox", "the dialog window refused creation");
    uobj *d = uget(dlg, H_WINDOW);
    d->dlgproc = proc;
    d->font = font;
    send(dlg, WM_SETFONT, font, 0);

    for (uint32_t i = 0; i < n; i++) {
        p = (p + 3) & ~3u;
        uint32_t istyle, iex, id;
        int16_t ix, iy, icx, icy;
        if (ext) { iex = rd32(p + 4); istyle = rd32(p + 8); ix = (int16_t)g16(p + 12); iy = (int16_t)g16(p + 14);
                   icx = (int16_t)g16(p + 16); icy = (int16_t)g16(p + 18); id = rd32(p + 20); p += 24; }
        else { istyle = rd32(p); iex = rd32(p + 4); ix = (int16_t)g16(p + 8); iy = (int16_t)g16(p + 10);
               icx = (int16_t)g16(p + 12); icy = (int16_t)g16(p + 14); id = g16(p + 16); p += 18; }
        char icls[64], itext[1024];
        uint32_t icls_ord = tmpl_field(&p, icls, sizeof icls), itext_ord = tmpl_field(&p, itext, sizeof itext);
        uint32_t extra = g16(p);
        p += 2 + extra;
        if (extra) hp_unsupported("DialogBox", "control %u with creation data", id);
        uint32_t c; int ctl;
        if (icls_ord == 0x80 || (!icls_ord && !strcasecmp(icls, "Button"))) { c = bcls; ctl = CTL_BUTTON; }
        else if (icls_ord == 0x82 || (!icls_ord && !strcasecmp(icls, "Static"))) { c = scls; ctl = CTL_STATIC; }
        else hp_unsupported("DialogBox", "control %u of class %s%u", id, icls, icls_ord);
        uint32_t k = istyle & (ctl == CTL_BUTTON ? 0xFu : 0x1Fu);
        if (ctl == CTL_BUTTON && k != 0 && k != 1 && k != 2 && k != 3) hp_unsupported("DialogBox", "button %u of type %u", id, k);
        if (ctl == CTL_STATIC && k != 0 && k != 1 && k != 2 && k != 3) hp_unsupported("DialogBox", "static %u of type %u", id, k);
        uint32_t hc = dlg_window(c, ctl, istyle | WS_CHILD, iex, dlu_x(ix), dlu_y(iy), dlu_x(icx), dlu_y(icy), dlg, id & 0xFFFF,
                                 itext, itext_ord, instance, 0);
        if (!hc) hp_unsupported("DialogBox", "control %u refused creation", id);
        send(hc, WM_SETFONT, font, 0);
    }

    uint32_t first = 0;                                             /* the first tab stop that can take the focus */
    for (uint32_t i = 1; i < MAXU && !first; i++) {
        uobj *c = &uobjs[i];
        if (c->kind == H_WINDOW && c->parent == dlg && (c->style & WS_TABSTOP) && c->visible && !c->disabled) first = UBASE + 4 * i;
    }
    if (send(dlg, WM_INITDIALOG, first, param) && first && is_window(first)) set_focus(first);
    return dlg;
}

/* Mnemonics: "&Exit" shows as "Exit", "&&" as "&". */
static void plain_text(const char *in, char *out, size_t n)
{
    size_t k = 0;
    for (; *in && k + 1 < n; in++) {
        if (*in == '&') { if (in[1] == '&') in++; else continue; }
        out[k++] = *in;
    }
    out[k] = 0;
}

static uint32_t text_color(uint32_t dlg, uint32_t ctl)
{
    uint32_t dc = CreateCompatibleDC_c(0);
    SetTextColor_c(dc, 0);                                          /* COLOR_WINDOWTEXT */
    send(dlg, WM_CTLCOLORSTATIC, dc, ctl);
    uint32_t c = GetTextColor_c(dc);
    DeleteDC_c(dc);
    return c;
}

/* The dialog's visible controls as the host shows them; hwnds[i] is item i's window. */
static void dialog_view(uint32_t dlg, halopad_dialog_view *v, uint32_t *hwnds)
{
    uobj *d = uget(dlg, H_WINDOW);
    memset(v, 0, sizeof *v);
    snprintf(v->title, sizeof v->title, "%s", d->text);
    halopad_window_client_size(dlg, &v->w, &v->h);
    for (uint32_t i = 1; i < MAXU; i++) {
        uobj *c = &uobjs[i];
        if (c->kind != H_WINDOW || c->parent != dlg || !(c->style & WS_CHILD) || !c->visible) continue;
        if (v->count == HPD_MAX_ITEMS) hp_unsupported("DialogBox", "more than %d visible controls", HPD_MAX_ITEMS);
        uint32_t h = UBASE + 4 * i;
        halopad_dialog_item *it = &v->item[v->count];
        it->id = c->id; it->enabled = !c->disabled && !d->disabled;
        it->x = c->x; it->y = c->y; it->w = c->w; it->h = c->h;
        plain_text(c->text, it->text, sizeof it->text);
        uint32_t k = c->style & 0x1F;
        if (c->ctl == CTL_BUTTON) {
            it->kind = (k & 0xF) >= 2 ? HPD_CHECKBOX : HPD_BUTTON;
            it->is_default = (k & 0xF) == 1;
            it->checked = c->check == 1;
        } else if (k == 3) {
            it->kind = HPD_ICON;
            snprintf(it->text, sizeof it->text, "#%u", c->icon);
        } else {
            it->kind = (c->style & SS_NOTIFY) ? HPD_LINK : HPD_TEXT;
            it->align = (int)k;
            it->color = text_color(dlg, h);
            if (!(c = uget(h, H_WINDOW))) continue;                 /* destroyed while asked for its colour */
        }
        hwnds[v->count++] = h;
    }
}

static void print_dialog(const halopad_dialog_view *v)
{
    static const char *kinds[] = {"button", "checkbox", "text", "link", "icon"};
    fprintf(stderr, "HALOPAD DIALOG: \"%s\" (%dx%d)\n", v->title, v->w, v->h);
    for (int i = 0; i < v->count; i++) {
        const halopad_dialog_item *it = &v->item[i];
        fprintf(stderr, "HALOPAD DIALOG:   %-8s %5d %s%s%s \"%s\"\n", kinds[it->kind], (int32_t)it->id, it->enabled ? "" : "(disabled) ",
                it->kind == HPD_CHECKBOX ? (it->checked ? "[x] " : "[ ] ") : "", it->is_default ? "(default) " : "", it->text);
    }
}

/* the next scripted action (HALOPAD_DIALOG_ACTIONS), consumed as it is used */
static int scripted_action(const halopad_dialog_view *v)
{
    static char actions[512];
    static int loaded, pos;
    if (!loaded) { const char *e = getenv("HALOPAD_DIALOG_ACTIONS"); snprintf(actions, sizeof actions, "%s", e ? e : ""); loaded = 1; }
    while (actions[pos] == ',' || actions[pos] == ' ') pos++;
    if (!actions[pos]) return HPD_NO_SCREEN;
    char tok[32];
    int n = 0;
    while (actions[pos] && actions[pos] != ',' && n < 31) tok[n++] = actions[pos++];
    tok[n] = 0;
    if (!strcmp(tok, "close")) return HPD_CLOSE;
    uint32_t id = (uint32_t)strtoul(tok, NULL, 0);
    for (int i = 0; i < v->count; i++)
        if (v->item[i].id == id && v->item[i].enabled && v->item[i].kind != HPD_TEXT && v->item[i].kind != HPD_ICON) return i;
    print_dialog(v);
    hp_unsupported("DialogBox", "scripted action \"%s\": no enabled button, checkbox or link with that id", tok);
}

static uint32_t modal_loop(uint32_t dlg, uint32_t owner)
{
    int owner_was_disabled = owner && is_window(owner) ? (int)EnableWindow_c(owner, 0) : 1;
    uobj *d = uget(dlg, H_WINDOW);
    if (d && !d->ended && !d->visible) ShowWindow_c(dlg, 1);
    halopad_dialog_view *v = malloc(sizeof *v);
    uint32_t hwnds[HPD_MAX_ITEMS];
    while ((d = uget(dlg, H_WINDOW)) && !d->ended) {
        dialog_view(dlg, v, hwnds);
        if (!(d = uget(dlg, H_WINDOW)) || d->ended) break;
        int k = scripted_action(v);
        if (k == HPD_NO_SCREEN) k = halopad_host_dialog(v);
        if (k == HPD_NO_SCREEN) {
            print_dialog(v);
            hp_unsupported("DialogBox", "dialog \"%s\" needs the player, and this host has no screen to show it on "
                           "(the iPadOS app shows dialogs; HALOPAD_DIALOG_ACTIONS scripts them)", v->title);
        }
        if (k == HPD_CLOSE) { send(dlg, WM_SYSCOMMAND, SC_CLOSE, 0); continue; }
        if (k < 0 || k >= v->count) hp_unsupported("DialogBox", "host action %d", k);
        uint32_t c = hwnds[k];
        uobj *cw = uget(c, H_WINDOW);
        if (!cw || !cw->visible || cw->disabled) continue;          /* changed meanwhile */
        if (v->item[k].kind == HPD_LINK) {
            uint32_t pt = (uint32_t)(cw->w / 2 & 0xFFFF) | (uint32_t)(cw->h / 2) << 16;
            send(c, WM_LBUTTONDOWN, 1 /* MK_LBUTTON */, pt);
            if (is_window(c)) send(c, WM_LBUTTONUP, 0, pt);
        } else if (v->item[k].kind == HPD_BUTTON || v->item[k].kind == HPD_CHECKBOX) {
            send(c, BM_CLICK, 0, 0);
        }
    }
    free(v);
    halopad_host_dialog_done();
    uint32_t result = d ? d->dlgresult : 0;
    if (owner && is_window(owner)) {
        if (!owner_was_disabled) EnableWindow_c(owner, 1);
        uobj *o = uget(owner, H_WINDOW);
        if (o && o->visible) activate(owner);
    }
    if (d) DestroyWindow_c(dlg);
    return result;
}

uint32_t DialogBoxIndirectParamA_c(uint32_t instance, uint32_t tmpl, uint32_t owner, uint32_t proc, uint32_t param)
{
    if (owner && !is_window(owner)) { halopad_last_error = 1400; return 0xFFFFFFFFu; }
    uint32_t dlg = create_dialog(instance, tmpl, owner, proc, param);
    return modal_loop(dlg, owner);
}

uint32_t DialogBoxParamA_c(uint32_t instance, uint32_t name, uint32_t owner, uint32_t proc, uint32_t param)
{
    uint32_t r = FindResourceExA_c(instance, 5 /* RT_DIALOG */, name, 0);
    uint32_t t = r ? LoadResource_c(instance, r) : 0;
    if (!t) { halopad_last_error = 1814; return 0xFFFFFFFFu; }      /* ERROR_RESOURCE_NAME_NOT_FOUND */
    return DialogBoxIndirectParamA_c(instance, t, owner, proc, param);
}

static uobj *dialog_of(const char *service, uint32_t hwnd)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (w && w->ctl != CTL_DIALOG) hp_unsupported(service, "window 0x%x is not a dialog", hwnd);
    if (!w) halopad_last_error = 1400;
    return w;
}

uint32_t EndDialog_c(uint32_t hwnd, uint32_t result)
{
    uobj *w = dialog_of("EndDialog", hwnd);
    if (!w) return 0;
    w->dlgresult = result;
    w->ended = 1;
    return 1;
}

uint32_t GetDlgItem_c(uint32_t hwnd, uint32_t id)
{
    for (uint32_t i = 1; i < MAXU; i++) {
        uobj *c = &uobjs[i];
        if (c->kind == H_WINDOW && (c->style & WS_CHILD) && c->parent == hwnd && c->id == (id & 0xFFFF)) return UBASE + 4 * i;
    }
    halopad_last_error = 1421;                                      /* ERROR_CONTROL_ID_NOT_FOUND */
    return 0;
}

uint32_t SetDlgItemTextA_c(uint32_t hwnd, uint32_t id, uint32_t text)
{
    uint32_t c = GetDlgItem_c(hwnd, id);
    return c ? SetWindowTextA_c(c, text) : 0;
}

uint32_t IsDlgButtonChecked_c(uint32_t hwnd, uint32_t id)
{
    uint32_t c = GetDlgItem_c(hwnd, id);
    return c ? send(c, BM_GETCHECK, 0, 0) : 0;
}

uint32_t CheckDlgButton_c(uint32_t hwnd, uint32_t id, uint32_t check)
{
    uint32_t c = GetDlgItem_c(hwnd, id);
    if (!c) return 0;
    send(c, BM_SETCHECK, check, 0);
    return 1;
}

/* DefDlgProc: the dialog procedure first; its TRUE means handled (the result is DWL_MSGRESULT,
   or the procedure's own value for the messages that return one). */
uint32_t DefDlgProcA_c(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = dialog_of("DefDlgProcA", hwnd);
    if (!w) return 0;
    uint32_t r = 0;
    if (w->dlgproc) {
        uint32_t args[4] = {hwnd, msg, wp, lp};
        r = halopad_call_guest(w->dlgproc, 4, args);
        if (!(w = uget(hwnd, H_WINDOW))) return r;
    }
    if (r) {
        if (msg == WM_INITDIALOG || (msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC)) return r;
        return w->msgresult;
    }
    switch (msg) {
    case WM_SETFONT: w->font = wp; return 0;
    case WM_GETFONT: return w->font;
    case WM_INITDIALOG: return 0;
    case WM_COMMAND: return 0;
    case WM_CLOSE: {                                                /* IDCANCEL, if the dialog can cancel */
        uint32_t c = GetDlgItem_c(hwnd, 2);
        uobj *cw = c ? uget(c, H_WINDOW) : NULL;
        if (!cw || !cw->disabled) send(hwnd, WM_COMMAND, 2 /* IDCANCEL, BN_CLICKED */, c);
        return 0;
    }
    }
    if (msg >= WM_CTLCOLORMSGBOX && msg <= WM_CTLCOLORSTATIC) return 0;   /* the dialog face: the host draws the background */
    return DefWindowProcA_c(hwnd, msg, wp, lp);
}

static uobj *control_of(const char *service, uint32_t hwnd, int ctl)
{
    uobj *w = uget(hwnd, H_WINDOW);
    if (w && w->ctl != ctl) hp_unsupported(service, "window 0x%x is not a control of this class", hwnd);
    return w;
}

/* the Button class: push buttons and check boxes (auto or not) */
uint32_t HaloPadButtonWndProc_c(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = control_of("Button", hwnd, CTL_BUTTON);
    if (!w) return 0;
    switch (msg) {
    case WM_SETFONT: w->font = wp; return 0;
    case WM_GETFONT: return w->font;
    case BM_GETCHECK: return w->check;
    case BM_SETCHECK: if ((w->style & 0xF) >= 2) w->check = wp; return 0;
    case BM_GETSTATE: return w->check | (focus == hwnd ? 8u : 0);
    case BM_SETSTATE: return 0;
    case BM_CLICK:
        if (w->disabled || !w->visible) return 0;
        if ((w->style & 0xF) == 3) w->check = !w->check;             /* BS_AUTOCHECKBOX */
        send(w->parent, WM_COMMAND, w->id & 0xFFFF /* BN_CLICKED */, hwnd);
        return 0;
    }
    return DefWindowProcA_c(hwnd, msg, wp, lp);
}

/* the Static class: text (left, centred, right) and icons; SS_NOTIFY reports clicks */
uint32_t HaloPadStaticWndProc_c(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = control_of("Static", hwnd, CTL_STATIC);
    if (!w) return 0;
    switch (msg) {
    case WM_NCCREATE: {
        uint32_t t = rd32(lp + 36);
        if (t && g16(t) == 0xFFFF) { w->icon = g16(t + 2); w->text[0] = 0; return 1; }   /* an icon's resource ordinal */
        break;
    }
    case WM_SETFONT: w->font = wp; return 0;
    case WM_GETFONT: return w->font;
    case WM_LBUTTONUP:
        if (w->style & SS_NOTIFY) send(w->parent, WM_COMMAND, w->id & 0xFFFF /* STN_CLICKED */, hwnd);
        return 0;
    }
    return DefWindowProcA_c(hwnd, msg, wp, lp);
}

/* ---- ShellExecuteA (shell32): opening a URL or a document ---- */

uint32_t ShellExecuteA_c(uint32_t hwnd, uint32_t verb, uint32_t file, uint32_t params, uint32_t dir, uint32_t show)
{
    (void)hwnd; (void)dir; (void)show;
    const char *v = verb ? (const char *)G(verb) : "open";
    if (strcasecmp(v, "open")) hp_unsupported("ShellExecuteA", "verb \"%s\"", v);
    if (params && *(const char *)G(params)) hp_unsupported("ShellExecuteA", "parameters \"%s\"", (const char *)G(params));
    if (!file) return 2;                                            /* SE_ERR_FNF */
    const char *f = G(file);
    char url[1200];
    if (strstr(f, "://")) snprintf(url, sizeof url, "%s", f);
    else {
        char host[1024];
        if (!halopad_host_path(f, host, sizeof host)) return 2;     /* SE_ERR_FNF */
        FILE *probe = fopen(host, "rb");
        if (!probe) return 2;
        fclose(probe);
        snprintf(url, sizeof url, "file://%s", host);
    }
    int ok = halopad_shell_open_hook ? halopad_shell_open_hook(url) : halopad_host_open_url(url);
    fprintf(stderr, "HALOPAD: ShellExecuteA open %s: %s\n", url, ok ? "opened" : "no handler on this host");
    return ok ? 42 : 31;                                            /* above 32 succeeds; SE_ERR_NOASSOC */
}

/* TranslateAcceleratorA: Keystone passes a null table (KsTranslateAccelerator, from Halo's
   message loop); Windows translates nothing then (ERROR_INVALID_ACCEL_HANDLE). HaloPad loads
   no accelerator tables, so any other table stops. */
uint32_t TranslateAcceleratorA_c(uint32_t hwnd, uint32_t table, uint32_t msg)
{
    (void)hwnd; (void)msg;
    if (table) hp_unsupported("TranslateAcceleratorA", "accelerator table 0x%08x", table);
    halopad_last_error = 1403;
    return 0;
}

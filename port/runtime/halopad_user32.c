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
                 char text[256]; uint32_t userdata; int visible; } uobj;
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
    hp_unsupported("GetDesktopWindow", "desktop size on this platform");
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

static uint32_t send(uint32_t hwnd, uint32_t msg, uint32_t wp, uint32_t lp)
{
    uobj *w = uget(hwnd, H_WINDOW);
    wclass *c = &classes[w->cls];
    uint32_t args[4] = {hwnd, msg, wp, lp};
    return halopad_call_guest(c->wndproc, 4, args);
}

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
        if (wp) hp_unsupported("DefWindowProcA", "WM_NCCALCSIZE with NCCALCSIZE_PARAMS");
        wr32(lp, rd32(lp) + (uint32_t)l); wr32(lp + 4, rd32(lp + 4) + (uint32_t)t);
        wr32(lp + 8, rd32(lp + 8) - (uint32_t)r); wr32(lp + 12, rd32(lp + 12) - (uint32_t)b);
        return 0;
    }
    case WM_GETMINMAXINFO:
    case WM_CREATE:
        return 0;
    }
    hp_unsupported("DefWindowProcA", "message 0x%04x (wParam 0x%x, lParam 0x%x)", msg, wp, lp);
}

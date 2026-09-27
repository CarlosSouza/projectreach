/* HaloPad GDI (G3/G9): the bitmap, device-context and gamma services Halo uses.
 *
 * How Halo uses them (haloce.exe): the main window (0x5191d0) loads splash bitmap 0x86 with
 * LoadBitmapA, selects it into a memory DC compatible with the window's DC, and WM_PAINT
 * (0x545072) stretches it over the client area (GetObjectA, StretchBlt SRCCOPY) until
 * Direct3D presents; Direct3D start-up checks the desktop's BITSPIXEL (0x51a80e); the
 * brightness setting saves the display ramp (GetDeviceGammaRamp, 0x52597d) and sets a new
 * one on the window's DC with SetDeviceGammaRamp (0x525bb1) alongside Direct3D's.
 *
 * Bitmaps are device-dependent bitmaps as a 32-bit display holds them, decoded from the
 * resource's DIB (1/4/8-bit palettes, 16/24/32-bit, BI_RGB or BI_BITFIELDS). A window DC
 * draws on the window's host surface; the gamma ramp is the host window's presentation
 * table. Fonts, text, pens, brushes and raster operations other than SRCCOPY stop with
 * their names or values. */
#include "halopad_win32.h"
#include <pthread.h>

uint32_t FindResourceExA_c(uint32_t module, uint32_t type, uint32_t name, uint32_t lang);
uint32_t LoadResource_c(uint32_t module, uint32_t hrsrc);
uint32_t SizeofResource_c(uint32_t module, uint32_t hrsrc);
void *halopad_window_host_if_shown(uint32_t hwnd);
int halopad_window_client_size(uint32_t hwnd, int32_t *cw, int32_t *ch);
void halopad_desktop_size(int32_t *w, int32_t *h);
void halopad_host_window_blit(void *w, const uint32_t *src, uint32_t src_w, uint32_t src_h, int32_t dx, int32_t dy, int32_t dw, int32_t dh,
                              int32_t sx, int32_t sy, int32_t sw, int32_t sh);
void halopad_host_window_gamma(void *w, const uint16_t ramp[768]);
uint32_t IsWindow_c(uint32_t hwnd);

#define GBASE 0x06000000u
#define MAXG 1024
enum { G_FREE, G_BITMAP, G_WINDOW_DC, G_MEMORY_DC };
typedef struct { int kind; uint32_t w, h; uint32_t *px; uint32_t hwnd, bitmap, stock; int selected_in; } gobj;
static gobj objs[MAXG];
static pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER;
static uint16_t display_ramp[768];                                  /* what the display shows now */
static int ramp_set;

static uint32_t gnew(int kind)
{
    for (uint32_t i = 1; i < MAXG; i++) if (objs[i].kind == G_FREE) { memset(&objs[i], 0, sizeof objs[i]); objs[i].kind = kind; return GBASE + 4 * i; }
    hp_unsupported("GDI", "more than %d objects", MAXG);
}
static gobj *gget(uint32_t h, int kind)
{
    if (h < GBASE || (h - GBASE) % 4 || (h - GBASE) / 4 >= MAXG) return NULL;
    gobj *o = &objs[(h - GBASE) / 4];
    return o->kind != G_FREE && (kind < 0 || o->kind == kind) ? o : NULL;
}

/* ---- bitmaps from DIB resources ---- */

static uint32_t mask_channel(uint32_t v, uint32_t mask)
{
    if (!mask) return 0;
    int shift = __builtin_ctz(mask), bits = 32 - __builtin_clz(mask >> shift);
    uint32_t x = (v & mask) >> shift, max = bits >= 32 ? 0xFFFFFFFFu : (1u << bits) - 1;
    return max ? (x * 255 + max / 2) / max : 0;
}

static uint32_t bitmap_from_dib(const uint8_t *p, uint32_t size)
{
    uint32_t hs;
    memcpy(&hs, p, 4);
    if (hs < 40 || size < hs) hp_unsupported("LoadBitmapA", "a bitmap header of %u bytes", hs);
    int32_t w, h; uint16_t bits; uint32_t comp, used;
    memcpy(&w, p + 4, 4); memcpy(&h, p + 8, 4); memcpy(&bits, p + 14, 2); memcpy(&comp, p + 16, 4); memcpy(&used, p + 32, 4);
    if (comp != 0 && comp != 3) hp_unsupported("LoadBitmapA", "compression %u", comp);
    if (w <= 0 || h == 0 || w > 16384) hp_unsupported("LoadBitmapA", "a %dx%d bitmap", w, h);
    int top_down = h < 0;
    uint32_t H = (uint32_t)(top_down ? -h : h), W = (uint32_t)w;
    const uint8_t *table = p + hs;
    uint32_t colors = bits <= 8 ? (used ? used : 1u << bits) : 0;
    uint32_t masks[3] = {0x7C00, 0x03E0, 0x001F};
    if (bits == 32) { masks[0] = 0xFF0000; masks[1] = 0xFF00; masks[2] = 0xFF; }
    if (comp == 3) { memcpy(masks, table, 12); table += 12; }
    const uint8_t *px = table + 4 * colors;
    uint32_t stride = ((W * bits + 31) / 32) * 4;
    if (px + (size_t)stride * H > p + size) hp_unsupported("LoadBitmapA", "a truncated bitmap");
    uint32_t *out = malloc((size_t)W * H * 4);
    for (uint32_t y = 0; y < H; y++) {
        const uint8_t *row = px + (size_t)stride * (top_down ? y : H - 1 - y);
        for (uint32_t x = 0; x < W; x++) {
            uint32_t c;
            if (bits <= 8) {
                uint32_t idx;
                if (bits == 8) idx = row[x];
                else if (bits == 4) idx = (row[x / 2] >> (x & 1 ? 0 : 4)) & 15;
                else if (bits == 1) idx = (row[x / 8] >> (7 - x % 8)) & 1;
                else hp_unsupported("LoadBitmapA", "%u-bit pixels", bits);
                if (idx >= colors) idx = 0;
                memcpy(&c, table + 4 * idx, 4);                     /* RGBQUAD: B, G, R, reserved */
                c &= 0xFFFFFF;
            } else if (bits == 24) {
                c = row[3 * x] | (uint32_t)row[3 * x + 1] << 8 | (uint32_t)row[3 * x + 2] << 16;
            } else if (bits == 16 || bits == 32) {
                uint32_t v = 0;
                memcpy(&v, row + x * (bits / 8), bits / 8);
                c = mask_channel(v, masks[2]) | mask_channel(v, masks[1]) << 8 | mask_channel(v, masks[0]) << 16;
            } else hp_unsupported("LoadBitmapA", "%u-bit pixels", bits);
            out[(size_t)y * W + x] = c | 0xFF000000u;
        }
    }
    pthread_mutex_lock(&glock);
    uint32_t hb = gnew(G_BITMAP);
    gobj *b = gget(hb, G_BITMAP);
    b->w = W; b->h = H; b->px = out;
    pthread_mutex_unlock(&glock);
    return hb;
}

uint32_t LoadBitmapA_c(uint32_t instance, uint32_t name)
{
    if (!instance) hp_unsupported("LoadBitmapA", "system bitmap %u", name);
    uint32_t r = FindResourceExA_c(instance, 2 /* RT_BITMAP */, name, 0);
    if (!r) { halopad_last_error = 1814; return 0; }               /* ERROR_RESOURCE_NAME_NOT_FOUND */
    uint32_t data = LoadResource_c(instance, r), size = SizeofResource_c(instance, r);
    return bitmap_from_dib(G(data), size);
}

/* ---- device contexts ---- */

uint32_t GetDC_c(uint32_t hwnd)
{
    if (hwnd && !IsWindow_c(hwnd)) return 0;
    pthread_mutex_lock(&glock);
    uint32_t h = gnew(G_WINDOW_DC);
    gget(h, -1)->hwnd = hwnd;                                       /* 0: the screen */
    pthread_mutex_unlock(&glock);
    return h;
}

uint32_t ReleaseDC_c(uint32_t hwnd, uint32_t dc)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, G_WINDOW_DC);
    int ok = d && d->hwnd == hwnd;
    if (ok) d->kind = G_FREE;
    pthread_mutex_unlock(&glock);
    return (uint32_t)ok;
}

uint32_t CreateCompatibleDC_c(uint32_t dc)
{
    pthread_mutex_lock(&glock);
    if (dc && !gget(dc, -1)) { pthread_mutex_unlock(&glock); return 0; }
    uint32_t h = gnew(G_MEMORY_DC);
    uint32_t stock = gnew(G_BITMAP);                                /* a new memory DC holds a 1x1 monochrome bitmap */
    gobj *s = gget(stock, G_BITMAP);
    s->w = s->h = 1; s->px = calloc(1, 4); s->px[0] = 0xFF000000u;
    gobj *d = gget(h, G_MEMORY_DC);
    d->bitmap = d->stock = stock;
    s->selected_in = 1;
    pthread_mutex_unlock(&glock);
    return h;
}

uint32_t SelectObject_c(uint32_t dc, uint32_t obj)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1), *o = gget(obj, -1);
    if (!d || d->kind == G_BITMAP) { pthread_mutex_unlock(&glock); return 0; }
    if (!o) { pthread_mutex_unlock(&glock); hp_unsupported("SelectObject", "object 0x%x (fonts, pens and brushes are not modelled)", obj); }
    if (o->kind != G_BITMAP) { pthread_mutex_unlock(&glock); return 0; }
    if (d->kind != G_MEMORY_DC || (o->selected_in && d->bitmap != obj)) { pthread_mutex_unlock(&glock); return 0; }   /* one DC at a time */
    uint32_t old = d->bitmap;
    gobj *prev = gget(old, G_BITMAP);
    if (prev) prev->selected_in = 0;
    o->selected_in = 1;
    d->bitmap = obj;
    pthread_mutex_unlock(&glock);
    return old;
}

uint32_t DeleteObject_c(uint32_t obj)
{
    pthread_mutex_lock(&glock);
    gobj *o = gget(obj, -1);
    if (!o || o->kind != G_BITMAP || o->selected_in) { pthread_mutex_unlock(&glock); return 0; }   /* not while selected */
    free(o->px);
    o->kind = G_FREE;
    pthread_mutex_unlock(&glock);
    return 1;
}

uint32_t GetObjectA_c(uint32_t obj, uint32_t size, uint32_t out)
{
    pthread_mutex_lock(&glock);
    gobj *o = gget(obj, -1);
    if (!o) { pthread_mutex_unlock(&glock); return 0; }
    if (o->kind != G_BITMAP) { pthread_mutex_unlock(&glock); hp_unsupported("GetObjectA", "a non-bitmap object"); }
    uint32_t w = o->w, h = o->h;
    pthread_mutex_unlock(&glock);
    if (!out) return 24;
    if (size < 24) return 0;
    wr32(out, 0); wr32(out + 4, w); wr32(out + 8, h); wr32(out + 12, 4 * w);   /* BITMAP: a 32-bit DDB */
    wr16(out + 16, 1); wr16(out + 18, 32); wr32(out + 20, 0);
    return 24;
}

/* StretchBlt, SRCCOPY, from a memory DC's bitmap to a window or another memory DC */
uint32_t StretchBlt_c(uint32_t dst, uint32_t x, uint32_t y, uint32_t w, uint32_t h, uint32_t src, uint32_t sx, uint32_t sy, uint32_t sw,
                      uint32_t sh, uint32_t rop)
{
    if (rop != 0x00CC0020u) hp_unsupported("StretchBlt", "raster operation 0x%08x", rop);
    int32_t dx = (int32_t)x, dy = (int32_t)y, dw = (int32_t)w, dh = (int32_t)h, fx = (int32_t)sx, fy = (int32_t)sy, fw = (int32_t)sw, fh = (int32_t)sh;
    if (dw < 0 || dh < 0 || fw < 0 || fh < 0) hp_unsupported("StretchBlt", "mirroring (negative extents)");
    pthread_mutex_lock(&glock);
    gobj *d = gget(dst, -1), *s = gget(src, G_MEMORY_DC);
    if (!d || d->kind == G_BITMAP || !s) { pthread_mutex_unlock(&glock); return 0; }
    gobj *b = gget(s->bitmap, G_BITMAP);
    uint32_t bw = b->w, bh = b->h;
    uint32_t *copy = malloc((size_t)bw * bh * 4);
    memcpy(copy, b->px, (size_t)bw * bh * 4);
    uint32_t hwnd = d->hwnd;
    int to_window = d->kind == G_WINDOW_DC;
    gobj *tb = to_window ? NULL : gget(d->bitmap, G_BITMAP);
    if (tb) {                                                       /* into another bitmap */
        for (int32_t yy = 0; yy < dh; yy++)
            for (int32_t xx = 0; xx < dw; xx++) {
                int32_t tx = dx + xx, ty = dy + yy, ux = fx + (int32_t)(((int64_t)xx * fw) / (dw ? dw : 1)), uy = fy + (int32_t)(((int64_t)yy * fh) / (dh ? dh : 1));
                if (tx >= 0 && ty >= 0 && tx < (int32_t)tb->w && ty < (int32_t)tb->h && ux >= 0 && uy >= 0 && ux < (int32_t)bw && uy < (int32_t)bh)
                    tb->px[(size_t)ty * tb->w + (size_t)tx] = copy[(size_t)uy * bw + (size_t)ux];
            }
    }
    pthread_mutex_unlock(&glock);
    if (to_window) {
        if (!hwnd) hp_unsupported("StretchBlt", "drawing on the screen DC");
        void *host = halopad_window_host_if_shown(hwnd);
        if (host) halopad_host_window_blit(host, copy, bw, bh, dx, dy, dw, dh, fx, fy, fw, fh);   /* a hidden window shows nothing */
    }
    free(copy);
    return 1;
}

uint32_t GetDeviceCaps_c(uint32_t dc, uint32_t index)
{
    if (!gget(dc, -1)) return 0;
    int32_t w, h;
    halopad_desktop_size(&w, &h);
    switch (index) {
    case 2: return 1;                                               /* TECHNOLOGY: DT_RASDISPLAY */
    case 4: return (uint32_t)(w * 254 / 960);                       /* HORZSIZE in mm at 96 dpi */
    case 6: return (uint32_t)(h * 254 / 960);
    case 8: case 118: return (uint32_t)w;                           /* HORZRES, DESKTOPHORZRES */
    case 10: case 117: return (uint32_t)h;
    case 12: return 32;                                             /* BITSPIXEL */
    case 14: return 1;                                              /* PLANES */
    case 24: return 0xFFFFFFFFu;                                    /* NUMCOLORS: more than 256 */
    case 88: case 90: return 96;                                    /* LOGPIXELSX/Y */
    case 104: return 0;                                             /* SIZEPALETTE */
    case 108: return 24;                                            /* COLORRES */
    case 116: return 60;                                            /* VREFRESH */
    case 121: return 2;                                             /* COLORMGMTCAPS: CM_GAMMA_RAMP */
    }
    hp_unsupported("GetDeviceCaps", "index %u", index);
}

/* ---- the display's gamma ramp ---- */

uint32_t GetDeviceGammaRamp_c(uint32_t dc, uint32_t ramp)
{
    if (!gget(dc, -1) || !ramp) return 0;
    if (!ramp_set) { for (int c = 0; c < 3; c++) for (int i = 0; i < 256; i++) display_ramp[256 * c + i] = (uint16_t)(i * 257); ramp_set = 1; }
    memcpy(G(ramp), display_ramp, sizeof display_ramp);
    return 1;
}

uint32_t SetDeviceGammaRamp_c(uint32_t dc, uint32_t ramp)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    uint32_t hwnd = d ? d->hwnd : 0;
    pthread_mutex_unlock(&glock);
    if (!d || !ramp) return 0;
    memcpy(display_ramp, G(ramp), sizeof display_ramp);
    ramp_set = 1;
    void *host = hwnd ? halopad_window_host_if_shown(hwnd) : NULL;
    if (!hwnd) hp_unsupported("SetDeviceGammaRamp", "the whole screen (a window DC is applied to the game's window)");
    if (host) halopad_host_window_gamma(host, display_ramp);
    return 1;
}

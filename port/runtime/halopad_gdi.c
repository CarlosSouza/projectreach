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
#include <math.h>
#include <pthread.h>
#include <CoreText/CoreText.h>

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
enum { G_FREE, G_BITMAP, G_WINDOW_DC, G_MEMORY_DC, G_FONT };
struct font;
typedef struct {
    int kind;
    uint32_t w, h; uint32_t *px; uint32_t dib;                    /* bitmaps: top-down 32-bit rows; dib: guest bits of a DIB section */
    uint32_t hwnd, bitmap, stock; int selected_in;               /* DCs and bitmaps */
    uint32_t font, text_color, bk_color, bk_mode, align;         /* DC text state */
    struct font *f; int delete_pending, is_stock;                /* fonts: selected_in counts DCs */
} gobj;
static gobj objs[MAXG];
static pthread_mutex_t glock = PTHREAD_MUTEX_INITIALIZER;
static uint16_t display_ramp[768];                                  /* what the display shows now */
static int ramp_set;
static void resolve_font(gobj *o);                                  /* text section below */
static void free_font(gobj *o);
static uint32_t stock_font(void);
uint32_t VirtualFree_c(uint32_t address, uint32_t size, uint32_t type);

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

/* a new DC's text state: black text on white, opaque background, TA_TOP | TA_LEFT, the System font */
static void dc_defaults(gobj *d)
{
    d->text_color = 0; d->bk_color = 0xFFFFFF; d->bk_mode = 2; d->align = 0;
    d->font = stock_font();
}
static void dc_release_font(gobj *d)
{
    gobj *f = gget(d->font, G_FONT);
    if (f && !f->is_stock && f->selected_in && !--f->selected_in && f->delete_pending) free_font(f);
    d->font = 0;
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
    dc_defaults(gget(h, -1));
    pthread_mutex_unlock(&glock);
    return h;
}

uint32_t ReleaseDC_c(uint32_t hwnd, uint32_t dc)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, G_WINDOW_DC);
    int ok = d && d->hwnd == hwnd;
    if (ok) { dc_release_font(d); d->kind = G_FREE; }
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
    dc_defaults(d);
    pthread_mutex_unlock(&glock);
    return h;
}

/* DeleteDC: a memory DC; its selected bitmap is released and its stock bitmap freed. A font
   deleted while selected here goes with it. */
uint32_t DeleteDC_c(uint32_t dc)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, G_MEMORY_DC);
    if (!d) {
        int window = gget(dc, G_WINDOW_DC) != NULL;
        pthread_mutex_unlock(&glock);
        if (window) hp_unsupported("DeleteDC", "a window DC (ReleaseDC releases those)");
        return 0;
    }
    gobj *b = gget(d->bitmap, G_BITMAP);
    if (b) b->selected_in = 0;
    gobj *s = gget(d->stock, G_BITMAP);
    if (s) { free(s->px); s->kind = G_FREE; }
    dc_release_font(d);
    d->kind = G_FREE;
    pthread_mutex_unlock(&glock);
    return 1;
}

uint32_t SelectObject_c(uint32_t dc, uint32_t obj)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1), *o = gget(obj, -1);
    if (!d || (d->kind != G_WINDOW_DC && d->kind != G_MEMORY_DC)) { pthread_mutex_unlock(&glock); return 0; }
    if (!o) { pthread_mutex_unlock(&glock); hp_unsupported("SelectObject", "object 0x%x (pens, brushes and regions are not modelled)", obj); }
    if (o->kind == G_FONT) {                                        /* the font is realized (mapped to a face) here */
        if (!o->is_stock) resolve_font(o);
        uint32_t old = d->font;
        if (obj != old) {
            if (!o->is_stock) o->selected_in++;
            dc_release_font(d);
            d->font = obj;
        }
        pthread_mutex_unlock(&glock);
        return old;
    }
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
    if (o && o->kind == G_FONT) {                                   /* stock: no effect; selected: deleted when released */
        if (!o->is_stock) { if (o->selected_in) o->delete_pending = 1; else free_font(o); }
        pthread_mutex_unlock(&glock);
        return 1;
    }
    if (!o || o->kind != G_BITMAP || o->selected_in) { pthread_mutex_unlock(&glock); return 0; }   /* not while selected */
    if (o->dib) VirtualFree_c(o->dib, 0, 0x8000);                   /* MEM_RELEASE: a DIB section's bits */
    else free(o->px);
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

/* ---- text: fonts on CoreText, metrics as GDI reports them, DIB sections, ExtTextOutW ----
 *
 * How Keystone.dll uses it (0x1021a7b5, its glyph cache): a memory DC in MM_TEXT, white text on
 * black, CreateFontA(-MulDiv(points, 96, 72), ..., ANTIALIASED_QUALITY, VARIABLE_PITCH, face),
 * GetTextMetricsA and GetTextExtentPoint32W(L"?") for the cell, and a 32-bit top-down DIB
 * section. Each glyph is drawn with ExtTextOutW(ETO_OPAQUE) and its coverage read back from the
 * pixels.
 *
 * Fonts. The reference machine has Windows XP SP3's fonts. A face it lacks goes through GDI's
 * mapper: for a variable-pitch or default-pitch request with no particular family and an ANSI or
 * default character set, that is Arial. This is how Keystone's "Arial Narrow", an Office font,
 * renders on XP. Faces are drawn with the host's copy of the same font (Arial is ArialMT on macOS
 * and iOS), and the metrics come from that font's own tables, as GDI computes them:
 *  - the em height in pixels is -lfHeight. For a positive lfHeight, it is the largest size whose
 *    cell fits;
 *  - ascent and descent come from the VDMX table's hinted values where it has the size, and
 *    otherwise from the OS/2 usWinAscent/usWinDescent scaled and rounded;
 *  - external leading is max(0, lineGap - ((winAscent + winDescent) - (ascender - descender))),
 *    scaled;
 *  - the average width is xAvgCharWidth scaled, and the maximum width is advanceWidthMax scaled;
 *  - each advance comes from the hdmx table (GDI's hinted device widths) at that size, or else
 *    is the linear advance rounded.
 * Glyph shapes are CoreGraphics' grayscale antialiasing without hinting, so the coverage of a
 * pixel can differ slightly from GDI's. Positions and cell sizes follow GDI. Faces XP has that
 * are not provided here, other character sets, escapement, width scaling and
 * NONANTIALIASED_QUALITY's bitmap rendering stop with their values. */
typedef struct font {
    int32_t height, width, escapement, orientation, weight;
    uint8_t italic, underline, strikeout, charset, out_precision, clip_precision, quality, pitch_family;
    char face[32];
    CTFontRef ct;                                                  /* resolved face at the em size */
    const char *resolved;
    int ppem, upem, ascent, descent, ext_leading, avg_width, max_width, weight_out, family;
    int ul_pos, ul_size, st_pos, st_size;
    uint8_t *hdmx;                                                 /* device widths at ppem, or NULL */
    uint32_t nglyphs;
} font;

static uint32_t stock_font(void)
{
    static uint32_t h;
    if (!h) { h = gnew(G_FONT); gget(h, G_FONT)->is_stock = 1; }   /* SYSTEM_FONT, the default in every DC */
    return h;
}

static void free_font(gobj *o)
{
    if (o->f) {
        if (o->f->ct) CFRelease(o->f->ct);
        free(o->f->hdmx);
        free(o->f);
    }
    o->kind = G_FREE;
}

static uint32_t be16(const uint8_t *p) { return (uint32_t)p[0] << 8 | p[1]; }
static int32_t sbe16(const uint8_t *p) { return (int16_t)(uint16_t)be16(p); }

static CFDataRef table(CTFontRef f, uint32_t tag) { return CTFontCopyTable(f, tag, kCTFontTableOptionNoOptions); }

/* VDMX (ratio 1:1 or the default ratio): hinted yMax/yMin at a pixel size, or the largest size
   whose yMax - yMin fits 'cell' when ppem is 0 */
static int vdmx_lookup(CTFontRef f, int ppem, int cell, int *ymax, int *ymin, int *found_ppem)
{
    CFDataRef d = table(f, 'VDMX');
    if (!d) return 0;
    const uint8_t *v = CFDataGetBytePtr(d);
    size_t len = (size_t)CFDataGetLength(d);
    int ok = 0;
    uint32_t nratios = len >= 6 ? be16(v + 4) : 0;
    for (uint32_t r = 0; r < nratios && !ok; r++) {
        const uint8_t *rat = v + 6 + 4 * r;
        if (!(rat[0] == 0 && rat[1] == 0 && rat[2] == 0) && !(rat[1] == 1 && rat[2] == 1 && rat[3] == 1) && !(rat[1] == rat[2] && rat[1] == rat[3])) continue;
        uint32_t off = be16(v + 6 + 4 * nratios + 2 * r);
        if (off + 4 > len) break;
        const uint8_t *g = v + off;
        uint32_t recs = be16(g);
        int best = -1;
        for (uint32_t i = 0; i < recs && off + 4 + 6 * (i + 1) <= len; i++) {
            const uint8_t *e = g + 4 + 6 * i;
            int sz = (int)be16(e), hi = sbe16(e + 2), lo = sbe16(e + 4);
            if (ppem && sz == ppem) { *ymax = hi; *ymin = lo; *found_ppem = sz; ok = 1; break; }
            if (!ppem && hi - lo <= cell) { best = (int)i; }
        }
        if (!ppem && best >= 0) {
            const uint8_t *e = g + 4 + 6 * best;
            *found_ppem = (int)be16(e); *ymax = sbe16(e + 2); *ymin = sbe16(e + 4); ok = 1;
        }
        break;                                                     /* the first matching ratio decides */
    }
    CFRelease(d);
    return ok;
}

/* The faces the reference machine has (Windows XP SP3's Fonts folder). */
static const char *const xp_faces[] = {
    "Arial", "Arial Black", "Comic Sans MS", "Courier New", "Estrangelo Edessa", "Franklin Gothic Medium", "Gautami", "Georgia",
    "Impact", "Latha", "Lucida Console", "Lucida Sans Unicode", "Mangal", "Marlett", "Microsoft Sans Serif", "MV Boli",
    "Palatino Linotype", "Raavi", "Shruti", "Sylfaen", "Symbol", "Tahoma", "Times New Roman", "Trebuchet MS", "Tunga",
    "Verdana", "Webdings", "Wingdings", "MS Sans Serif", "MS Serif", "System", "Terminal", "Fixedsys", "Courier",
    "Small Fonts", "Modern", "Roman", "Script"};
/* faces provided, with their host PostScript names (regular, bold, italic, bold italic) and GDI family */
static const struct { const char *face; const char *ps[4]; int family; } provided[] = {
    {"Arial", {"ArialMT", "Arial-BoldMT", "Arial-ItalicMT", "Arial-BoldItalicMT"}, 0x20 /* FF_SWISS */},
};

static void resolve_font(gobj *o)
{
    font *f = o->f;
    if (f->ct) return;
    if (f->escapement || f->orientation) hp_unsupported("CreateFontA", "escapement %d / orientation %d", f->escapement, f->orientation);
    if (f->width) hp_unsupported("CreateFontA", "an explicit character width %d", f->width);
    if (f->charset != 0 && f->charset != 1) hp_unsupported("CreateFontA", "character set %u", f->charset);
    if (f->quality == 3) hp_unsupported("CreateFontA", "NONANTIALIASED_QUALITY (bitmap rendering)");
    const char *face = f->face;
    int installed = 0;
    for (size_t i = 0; i < sizeof xp_faces / sizeof xp_faces[0]; i++) if (!strcasecmp(face, xp_faces[i])) installed = 1;
    if (!installed) {
        int pitch = f->pitch_family & 3, family = f->pitch_family & 0xF0;
        if (pitch == 1 /* FIXED_PITCH */ || (family != 0 && family != 0x20))
            hp_unsupported("CreateFontA", "mapping the face \"%s\" (pitch and family 0x%02x), which the reference machine lacks", face, f->pitch_family);
        face = "Arial";                                            /* GDI's mapper for a variable-pitch sans request */
    }
    int k = -1;
    for (size_t i = 0; i < sizeof provided / sizeof provided[0]; i++) if (!strcasecmp(face, provided[i].face)) k = (int)i;
    if (k < 0) hp_unsupported("CreateFontA", "the face \"%s\" (on the reference machine, not provided yet)", face);
    int bold = f->weight > 550, style = (bold ? 1 : 0) | (f->italic ? 2 : 0);
    const char *ps = provided[k].ps[style];
    CFStringRef name = CFStringCreateWithCString(NULL, ps, kCFStringEncodingASCII);
    CTFontRef probe = CTFontCreateWithName(name, 12.0, NULL);
    CFStringRef got = CTFontCopyPostScriptName(probe);
    int same = CFStringCompare(got, name, 0) == kCFCompareEqualTo;
    CFRelease(got);
    if (!same) { CFRelease(probe); CFRelease(name); hp_unsupported("CreateFontA", "the host has no %s font", ps); }
    CFDataRef head = table(probe, 'head'), os2 = table(probe, 'OS/2'), hhea = table(probe, 'hhea'), post = table(probe, 'post');
    if (!head || !os2 || !hhea || !post) hp_unsupported("CreateFontA", "%s lacks a TrueType table GDI needs", ps);
    const uint8_t *hd = CFDataGetBytePtr(head), *o2 = CFDataGetBytePtr(os2), *hh = CFDataGetBytePtr(hhea), *pt = CFDataGetBytePtr(post);
    int upem = (int)be16(hd + 18);
    int win_a = (int)be16(o2 + 74), win_d = (int)be16(o2 + 76), avg = sbe16(o2 + 2), wclass = (int)be16(o2 + 4);
    int st_size = sbe16(o2 + 26), st_pos = sbe16(o2 + 28);
    int asc = sbe16(hh + 4), desc = sbe16(hh + 6), gap = sbe16(hh + 8), adv_max = (int)be16(hh + 10);
    int ul_pos = sbe16(pt + 8), ul_size = sbe16(pt + 10);
    int ppem, ymax, ymin, vppem;
    if (f->height < 0) ppem = -f->height;
    else if (f->height == 0) hp_unsupported("CreateFontA", "the default height (lfHeight 0)");
    else if (vdmx_lookup(probe, 0, f->height, &ymax, &ymin, &vppem)) ppem = vppem;
    else ppem = (int)(((int64_t)f->height * upem + (win_a + win_d) / 2) / (win_a + win_d));
    #define SCALE(v) ((int)floor((double)(v) * ppem / upem + 0.5))
    f->ppem = ppem; f->upem = upem;
    if (vdmx_lookup(probe, ppem, 0, &ymax, &ymin, &vppem)) { f->ascent = ymax; f->descent = -ymin; }
    else { f->ascent = SCALE(win_a); f->descent = SCALE(win_d); }
    int el = gap - ((win_a + win_d) - (asc - desc));
    f->ext_leading = el > 0 ? SCALE(el) : 0;
    f->avg_width = SCALE(avg);
    f->max_width = SCALE(adv_max);
    f->weight_out = wclass;
    f->ul_pos = SCALE(ul_pos); f->ul_size = SCALE(ul_size) > 0 ? SCALE(ul_size) : 1;
    f->st_pos = SCALE(st_pos); f->st_size = SCALE(st_size) > 0 ? SCALE(st_size) : 1;
    #undef SCALE
    f->family = provided[k].family;
    f->resolved = provided[k].face;
    f->nglyphs = (uint32_t)CTFontGetGlyphCount(probe);
    CFDataRef hdmx = table(probe, 'hdmx');
    if (hdmx) {
        const uint8_t *h = CFDataGetBytePtr(hdmx);
        uint32_t n = be16(h + 2), size = be16(h + 4) << 16 | be16(h + 6);
        for (uint32_t i = 0; i < n && 8 + (size_t)size * (i + 1) <= (size_t)CFDataGetLength(hdmx); i++)
            if (h[8 + size * i] == ppem && size >= 2 + f->nglyphs) {
                f->hdmx = malloc(f->nglyphs);
                memcpy(f->hdmx, h + 8 + size * i + 2, f->nglyphs);
            }
        CFRelease(hdmx);
    }
    CFRelease(head); CFRelease(os2); CFRelease(hhea); CFRelease(post);
    f->ct = CTFontCreateCopyWithAttributes(probe, (CGFloat)ppem, NULL, NULL);
    CFRelease(probe);
    CFRelease(name);
}

uint32_t CreateFontA_c(uint32_t args)                                /* the 14 stack arguments */
{
    font *f = calloc(1, sizeof *f);
    f->height = (int32_t)rd32(args); f->width = (int32_t)rd32(args + 4); f->escapement = (int32_t)rd32(args + 8);
    f->orientation = (int32_t)rd32(args + 12); f->weight = (int32_t)rd32(args + 16);
    f->italic = (uint8_t)rd32(args + 20); f->underline = (uint8_t)rd32(args + 24); f->strikeout = (uint8_t)rd32(args + 28);
    f->charset = (uint8_t)rd32(args + 32); f->out_precision = (uint8_t)rd32(args + 36); f->clip_precision = (uint8_t)rd32(args + 40);
    f->quality = (uint8_t)rd32(args + 44); f->pitch_family = (uint8_t)rd32(args + 48);
    uint32_t face = rd32(args + 52);
    if (face) snprintf(f->face, sizeof f->face, "%s", (const char *)G(face));   /* LF_FACESIZE: 31 characters */
    if (f->weight == 0) f->weight = 400;                             /* FW_DONTCARE */
    pthread_mutex_lock(&glock);
    uint32_t h = gnew(G_FONT);
    gget(h, G_FONT)->f = f;
    pthread_mutex_unlock(&glock);
    return h;
}

/* the DC's realized font, or a stop for the System font, which is not provided */
static font *dc_font(const char *service, gobj *d)
{
    gobj *o = gget(d->font, G_FONT);
    if (!o || o->is_stock) { pthread_mutex_unlock(&glock); hp_unsupported(service, "the System font (no font selected)"); }
    return o->f;
}

uint32_t SetMapMode_c(uint32_t dc, uint32_t mode)
{
    if (!gget(dc, -1)) return 0;
    if (mode != 1) hp_unsupported("SetMapMode", "mapping mode %u (MM_TEXT only)", mode);
    return 1;                                                        /* the previous mode: MM_TEXT */
}

static uint32_t set_color(const char *service, uint32_t dc, uint32_t color, int bk)
{
    if (color >> 24) hp_unsupported(service, "color 0x%08x (palette colors)", color);
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    if (!d || (d->kind != G_MEMORY_DC && d->kind != G_WINDOW_DC)) { pthread_mutex_unlock(&glock); return 0xFFFFFFFFu; }   /* CLR_INVALID */
    uint32_t *slot = bk ? &d->bk_color : &d->text_color, old = *slot;
    *slot = color;
    pthread_mutex_unlock(&glock);
    return old;
}
uint32_t SetTextColor_c(uint32_t dc, uint32_t color) { return set_color("SetTextColor", dc, color, 0); }
uint32_t SetBkColor_c(uint32_t dc, uint32_t color) { return set_color("SetBkColor", dc, color, 1); }

uint32_t SetBkMode_c(uint32_t dc, uint32_t mode)
{
    if (mode != 1 && mode != 2) return 0;
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    uint32_t old = d && (d->kind == G_MEMORY_DC || d->kind == G_WINDOW_DC) ? d->bk_mode : 0;
    if (old) d->bk_mode = mode;
    pthread_mutex_unlock(&glock);
    return old;
}

uint32_t SetTextAlign_c(uint32_t dc, uint32_t align)
{
    if (align & ~(0x2u | 0x6u | 0x8u | 0x18u)) hp_unsupported("SetTextAlign", "alignment 0x%x (TA_UPDATECP and right-to-left reading)", align);
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    if (!d || (d->kind != G_MEMORY_DC && d->kind != G_WINDOW_DC)) { pthread_mutex_unlock(&glock); return 0xFFFFFFFFu; }   /* GDI_ERROR */
    uint32_t old = d->align;
    d->align = align;
    pthread_mutex_unlock(&glock);
    return old;
}

uint32_t GetTextMetricsA_c(uint32_t dc, uint32_t tm)
{
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    if (!d || !tm || (d->kind != G_MEMORY_DC && d->kind != G_WINDOW_DC)) { pthread_mutex_unlock(&glock); return 0; }
    font *f = dc_font("GetTextMetricsA", d);
    uint8_t t[56] = {0};
    int32_t v[11] = {f->ascent + f->descent, f->ascent, f->descent, f->ascent + f->descent - f->ppem, f->ext_leading, f->avg_width,
                     f->max_width, f->weight_out, 0, 96, 96};
    memcpy(t, v, sizeof v);
    t[44] = 0x1E; t[45] = 0xFF; t[46] = 0x1F; t[47] = 0x20;         /* first, last, default and break characters (ANSI TrueType) */
    t[48] = f->italic ? 0xFF : 0; t[49] = f->underline ? 0xFF : 0; t[50] = f->strikeout ? 0xFF : 0;
    t[51] = (uint8_t)(0x1 | 0x2 | 0x4 | f->family);                  /* variable pitch, vector, TrueType, family */
    t[52] = 0;                                                       /* ANSI_CHARSET */
    pthread_mutex_unlock(&glock);
    memcpy(G(tm), t, 56);
    return 1;
}

/* glyphs and GDI advances for a UTF-16 run */
static void layout(const char *service, font *f, const uint16_t *s, uint32_t n, CGGlyph *g, int32_t *adv)
{
    for (uint32_t i = 0; i < n; i++)
        if (s[i] >= 0xD800 && s[i] < 0xE000) hp_unsupported(service, "surrogate U+%04X", s[i]);
    CTFontGetGlyphsForCharacters(f->ct, (const UniChar *)s, g, (CFIndex)n);   /* a missing character is glyph 0, the default glyph */
    for (uint32_t i = 0; i < n; i++) {
        if (f->hdmx && g[i] < f->nglyphs) { adv[i] = f->hdmx[g[i]]; continue; }
        CGSize a;
        CTFontGetAdvancesForGlyphs(f->ct, kCTFontOrientationHorizontal, &g[i], &a, 1);
        adv[i] = (int32_t)floor(a.width + 0.5);
    }
}

uint32_t GetTextExtentPoint32W_c(uint32_t dc, uint32_t str, uint32_t n, uint32_t size)
{
    if (!size || (int32_t)n < 0 || (n && !str)) return 0;
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    if (!d || (d->kind != G_MEMORY_DC && d->kind != G_WINDOW_DC)) { pthread_mutex_unlock(&glock); return 0; }
    font *f = dc_font("GetTextExtentPoint32W", d);
    uint16_t *s = malloc(2 * (n + 1));
    CGGlyph *g = malloc(sizeof *g * (n + 1));
    int32_t *adv = malloc(4 * (n + 1)), cx = 0;
    if (n) memcpy(s, G(str), 2 * n);
    layout("GetTextExtentPoint32W", f, s, n, g, adv);
    for (uint32_t i = 0; i < n; i++) cx += adv[i];
    int32_t cy = f->ascent + f->descent;
    pthread_mutex_unlock(&glock);
    free(s); free(g); free(adv);
    wr32(size, (uint32_t)cx);
    wr32(size + 4, (uint32_t)cy);
    return 1;
}

/* CreateDIBSection: 32-bit BI_RGB (or BI_BITFIELDS with the 8-8-8 masks), bits in guest memory from
   VirtualAlloc, zeroed as Windows commits them. File-mapping sections and other depths stop. */
uint32_t VirtualAlloc_c(uint32_t address, uint32_t size, uint32_t type, uint32_t prot);
uint32_t CreateDIBSection_c(uint32_t dc, uint32_t bmi, uint32_t usage, uint32_t bits_out, uint32_t section, uint32_t offset)
{
    (void)dc; (void)offset;
    if (!bmi) return 0;
    if (section) hp_unsupported("CreateDIBSection", "a file-mapping section 0x%x", section);
    if (usage != 0) hp_unsupported("CreateDIBSection", "usage %u (DIB_PAL_COLORS)", usage);
    uint32_t hs = rd32(bmi), comp = rd32(bmi + 16);
    int32_t w = (int32_t)rd32(bmi + 4), h = (int32_t)rd32(bmi + 8);
    uint16_t planes, bpp;
    memcpy(&planes, G(bmi + 12), 2); memcpy(&bpp, G(bmi + 14), 2);
    if (hs < 40 || planes != 1 || w <= 0 || h == 0) { halopad_last_error = 87; return 0; }
    if (bpp != 32) hp_unsupported("CreateDIBSection", "%u-bit pixels", bpp);
    if (comp == 3 && !(rd32(bmi + hs) == 0xFF0000 && rd32(bmi + hs + 4) == 0xFF00 && rd32(bmi + hs + 8) == 0xFF))
        hp_unsupported("CreateDIBSection", "BI_BITFIELDS masks other than 8-8-8");
    if (comp != 0 && comp != 3) hp_unsupported("CreateDIBSection", "compression %u", comp);
    if (h > 0) hp_unsupported("CreateDIBSection", "a bottom-up DIB (drawing is implemented for top-down ones)");
    uint32_t H = (uint32_t)-h, W = (uint32_t)w;
    uint64_t bytes = (uint64_t)W * H * 4;
    if (bytes > 0x10000000u) hp_unsupported("CreateDIBSection", "a %ux%u DIB", W, H);
    uint32_t bits = VirtualAlloc_c(0, (uint32_t)bytes, 0x3000, 0x04);
    if (!bits) return 0;
    pthread_mutex_lock(&glock);
    uint32_t hb = gnew(G_BITMAP);
    gobj *b = gget(hb, G_BITMAP);
    b->w = W; b->h = H; b->dib = bits; b->px = G(bits);
    pthread_mutex_unlock(&glock);
    if (bits_out) wr32(bits_out, bits);
    return hb;
}

/* ExtTextOutW into a memory DC's bitmap: ETO_OPAQUE fills the rectangle, an OPAQUE background mode
   fills the text cell, and glyphs are drawn with GDI's integer advances (or lpDx) from the
   alignment point. A DIB section's fourth byte is left 0, as GDI leaves it. */
uint32_t ExtTextOutW_c(uint32_t dc, uint32_t x, uint32_t y, uint32_t options, uint32_t rect, uint32_t str, uint32_t n, uint32_t dx)
{
    if (options & ~(0x2u | 0x4u | 0x1000u)) hp_unsupported("ExtTextOutW", "options 0x%x", options);
    if ((int32_t)n < 0 || (n && !str)) return 0;
    pthread_mutex_lock(&glock);
    gobj *d = gget(dc, -1);
    if (!d || (d->kind != G_MEMORY_DC && d->kind != G_WINDOW_DC)) { pthread_mutex_unlock(&glock); return 0; }
    if (d->kind == G_WINDOW_DC) { pthread_mutex_unlock(&glock); hp_unsupported("ExtTextOutW", "drawing on a window DC"); }
    font *f = dc_font("ExtTextOutW", d);
    gobj *b = gget(d->bitmap, G_BITMAP);
    uint16_t *s = malloc(2 * (n + 1));
    CGGlyph *g = malloc(sizeof *g * (n + 1));
    int32_t *adv = malloc(4 * (n + 1)), total = 0;
    if (n) memcpy(s, G(str), 2 * n);
    layout("ExtTextOutW", f, s, n, g, adv);
    for (uint32_t i = 0; i < n; i++) { if (dx) adv[i] = (int32_t)rd32(dx + 4 * i); total += adv[i]; }
    int32_t cell = f->ascent + f->descent, left = (int32_t)x, top = (int32_t)y;
    if ((d->align & 6) == 6) left -= total / 2; else if (d->align & 2) left -= total;
    if ((d->align & 0x18) == 0x18) top -= f->ascent; else if (d->align & 8) top -= cell;
    int32_t W = (int32_t)b->w, H = (int32_t)b->h;
    int32_t clip[4] = {0, 0, W, H};                                 /* left, top, right, bottom */
    int32_t r[4] = {0, 0, 0, 0};
    if (rect) for (int i = 0; i < 4; i++) r[i] = (int32_t)rd32(rect + 4 * i);
    if ((options & 4) && rect) {
        if (r[0] > clip[0]) clip[0] = r[0];
        if (r[1] > clip[1]) clip[1] = r[1];
        if (r[2] < clip[2]) clip[2] = r[2];
        if (r[3] < clip[3]) clip[3] = r[3];
    }
    uint32_t hi = b->dib ? 0 : 0xFF000000u;
    #define FILL(l, t, rr, bb, c) do { \
        int32_t l_ = (l) < 0 ? 0 : (l), t_ = (t) < 0 ? 0 : (t), r_ = (rr) > W ? W : (rr), b_ = (bb) > H ? H : (bb); \
        uint32_t px_ = hi | ((c) & 0xFF) << 16 | ((c) & 0xFF00) | ((c) >> 16 & 0xFF); \
        for (int32_t yy = t_; yy < b_; yy++) for (int32_t xx = l_; xx < r_; xx++) b->px[(size_t)yy * b->w + (size_t)xx] = px_; \
    } while (0)
    if ((options & 2) && rect) FILL(r[0], r[1], r[2], r[3], d->bk_color);
    if (d->bk_mode == 2 && n) {
        int32_t l = left, t = top, rr = left + total, bb = top + cell;
        if (l < clip[0]) l = clip[0];
        if (t < clip[1]) t = clip[1];
        if (rr > clip[2]) rr = clip[2];
        if (bb > clip[3]) bb = clip[3];
        FILL(l, t, rr, bb, d->bk_color);
    }
    if (n && clip[2] > clip[0] && clip[3] > clip[1]) {
        CGColorSpaceRef cs = CGColorSpaceCreateDeviceRGB();
        CGContextRef ctx = CGBitmapContextCreate(b->px, (size_t)W, (size_t)H, 8, (size_t)W * 4, cs,
                                                 (CGBitmapInfo)kCGImageAlphaNoneSkipFirst | kCGBitmapByteOrder32Little);
        CGColorSpaceRelease(cs);
        if (!ctx) { pthread_mutex_unlock(&glock); hp_unsupported("ExtTextOutW", "a %dx%d drawing surface", W, H); }
        CGContextClipToRect(ctx, CGRectMake(clip[0], H - clip[3], clip[2] - clip[0], clip[3] - clip[1]));
        CGContextSetShouldAntialias(ctx, true);
        CGContextSetShouldSmoothFonts(ctx, false);
        CGContextSetAllowsFontSubpixelPositioning(ctx, false);
        CGContextSetShouldSubpixelPositionFonts(ctx, false);
        CGContextSetAllowsFontSubpixelQuantization(ctx, false);
        CGContextSetTextMatrix(ctx, CGAffineTransformIdentity);
        uint32_t c = d->text_color;
        CGContextSetRGBFillColor(ctx, (c & 0xFF) / 255.0, (c >> 8 & 0xFF) / 255.0, (c >> 16 & 0xFF) / 255.0, 1.0);
        CGPoint *pos = malloc(sizeof *pos * n);
        int32_t pen = left;
        for (uint32_t i = 0; i < n; i++) { pos[i] = CGPointMake(pen, H - (top + f->ascent)); pen += adv[i]; }
        CTFontDrawGlyphs(f->ct, g, pos, (size_t)n, ctx);
        free(pos);
        if (f->underline) CGContextFillRect(ctx, CGRectMake(left, H - (top + f->ascent - f->ul_pos) - f->ul_size, total, f->ul_size));
        if (f->strikeout) CGContextFillRect(ctx, CGRectMake(left, H - (top + f->ascent - f->st_pos) - f->st_size, total, f->st_size));
        CGContextRelease(ctx);
        for (int32_t yy = clip[1]; yy < clip[3]; yy++)                /* the fourth byte as GDI leaves it */
            for (int32_t xx = clip[0]; xx < clip[2]; xx++) {
                uint32_t *p = &b->px[(size_t)yy * b->w + (size_t)xx];
                *p = (*p & 0xFFFFFFu) | hi;
            }
    }
    #undef FILL
    pthread_mutex_unlock(&glock);
    free(s); free(g); free(adv);
    return 1;
}

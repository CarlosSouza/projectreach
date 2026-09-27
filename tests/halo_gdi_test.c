/* GDI test (G3/G9): Halo's splash path (0x5191d0 and WM_PAINT at 0x545072) with its real
 * bitmap resource 0x86: LoadBitmapA, a memory DC, StretchBlt over a shown window's client
 * area onto the host window's GDI surface, compared with pixels decoded independently from
 * the resource; selection and deletion rules; BITSPIXEL; the gamma ramp.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_gdi_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

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
uint32_t FindResourceExA_c(uint32_t module, uint32_t type, uint32_t name, uint32_t lang);
uint32_t LoadResource_c(uint32_t module, uint32_t hrsrc);
void *halopad_window_host_if_shown(uint32_t hwnd);
uint32_t halopad_host_window_gdi_pixel(void *w, uint32_t x, uint32_t y);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t user32, gdi32, kernel32;
static uint32_t api(const char *name, uint32_t n, const uint32_t *args)
{
    static const char *const g[] = {"CreateCompatibleDC", "SelectObject", "DeleteObject", "GetObjectA", "StretchBlt", "GetDeviceCaps",
                                    "GetDeviceGammaRamp", "SetDeviceGammaRamp", "SetMapMode", "SetTextColor", "SetBkColor", "SetTextAlign",
                                    "CreateFontA", "GetTextMetricsA", "GetTextExtentPoint32W", "CreateDIBSection", "ExtTextOutW", "DeleteDC"};
    uint32_t mod = !strcmp(name, "MulDiv") ? kernel32 : user32;
    for (size_t i = 0; i < sizeof g / sizeof g[0]; i++) if (!strcmp(name, g[i])) mod = gdi32;
    uint32_t va = GetProcAddress_c(mod, str(name));
    if (!va) { printf("no export %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})

/* the resource's pixel (x, y), top-down, as 0xFFRRGGBB, decoded here from the DIB */
static const uint8_t *dib;
static int32_t dw, dh;
static uint16_t dbits;
static uint32_t dib_pixel(uint32_t x, uint32_t y)
{
    uint32_t hs; memcpy(&hs, dib, 4);
    uint32_t used; memcpy(&used, dib + 32, 4);
    uint32_t colors = dbits <= 8 ? (used ? used : 1u << dbits) : 0;
    uint32_t H = (uint32_t)(dh < 0 ? -dh : dh), stride = (((uint32_t)dw * dbits + 31) / 32) * 4;
    const uint8_t *row = dib + hs + 4 * colors + (size_t)stride * (dh < 0 ? y : H - 1 - y);
    uint32_t c = 0;
    if (dbits == 8) memcpy(&c, dib + hs + 4 * row[x], 4);
    else if (dbits == 24) c = row[3 * x] | (uint32_t)row[3 * x + 1] << 8 | (uint32_t)row[3 * x + 2] << 16;
    else if (dbits == 4) memcpy(&c, dib + hs + 4 * ((row[x / 2] >> (x & 1 ? 0 : 4)) & 15), 4);
    else { printf("test: %u-bit splash not decoded here\n", dbits); exit(2); }
    return (c & 0xFFFFFF) | 0xFF000000u;
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
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    halopad_host_input_off = 1;
    user32 = LoadLibraryA_c(str("user32.dll"));
    gdi32 = LoadLibraryA_c(str("gdi32.dll"));
    kernel32 = LoadLibraryA_c(str("kernel32.dll"));

    uint32_t wc = halopad_heap_alloc(48, 1);
    uint32_t wcv[12] = {48, 0, GetProcAddress_c(user32, str("DefWindowProcA")), 0, 0, 0x400000, 0, 0, 0, 0, str("HaloPadGdiTest"), 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    API("RegisterClassExA", wc);
    uint32_t hwnd = API("CreateWindowExA", 0, str("HaloPadGdiTest"), str("Halo"), 0x00CF0000, 0, 0, 648, 507, 0, 0, 0x400000, 0);
    check("a hidden window has no host window", halopad_window_host_if_shown(hwnd) == NULL, 1);
    API("ShowWindow", hwnd, 5);
    void *host = halopad_window_host_if_shown(hwnd);
    check("ShowWindow gives it a host window", host != NULL, 1);

    /* the splash bitmap, 0x86, in strings.dll (the handle Halo keeps at 0x6bde88) */
    uint32_t strings = LoadLibraryA_c(str("strings.dll"));
    check("LoadLibraryA(strings.dll)", strings != 0, 1);
    uint32_t r = FindResourceExA_c(strings, 2, 0x86, 0);
    dib = halopad_guest_ptr(LoadResource_c(strings, r));
    memcpy(&dw, dib + 4, 4); memcpy(&dh, dib + 8, 4); memcpy(&dbits, dib + 14, 2);
    printf("    (splash resource: %dx%d, %u bits)\n", dw, dh < 0 ? -dh : dh, dbits);
    uint32_t bmp = API("LoadBitmapA", strings, 0x86);
    check("LoadBitmapA(strings.dll, 0x86)", bmp != 0, 1);
    check("LoadBitmapA of a missing bitmap: NULL", API("LoadBitmapA", strings, 0x7777), 0);
    uint32_t bm = halopad_heap_alloc(24, 1);
    check("GetObjectA(BITMAP): 24 bytes", API("GetObjectA", bmp, 24, bm), 24);
    check("  its size is the resource's", rd(bm + 4) == (uint32_t)dw && rd(bm + 8) == (uint32_t)(dh < 0 ? -dh : dh), 1);
    check("  a 32-bit display bitmap", (rd(bm + 16) >> 16) == 32 && rd(bm + 12) == 4 * (uint32_t)dw, 1);

    /* as WM_PAINT draws it */
    uint32_t dc = API("GetDC", hwnd);
    uint32_t mem = API("CreateCompatibleDC", dc), mem2 = API("CreateCompatibleDC", dc);
    uint32_t stock = API("SelectObject", mem, bmp);
    check("SelectObject(bitmap) into a memory DC returns its stock bitmap", stock != 0 && stock != bmp, 1);
    check("  the same bitmap into a second DC: fails", API("SelectObject", mem2, bmp), 0);
    uint32_t bw = rd(bm + 4), bh = rd(bm + 8);
    check("StretchBlt(SRCCOPY) over the 640x480 client area", API("StretchBlt", dc, 0, 0, 640, 480, mem, 0, 0, bw, bh, 0x00CC0020), 1);
    check("  the window's top-left pixel is the bitmap's", halopad_host_window_gdi_pixel(host, 0, 0), dib_pixel(0, 0));
    check("  bottom-right is the bitmap's last", halopad_host_window_gdi_pixel(host, 639, 479), dib_pixel(639 * bw / 640, 479 * bh / 480));
    check("  and the middle, scaled nearest", halopad_host_window_gdi_pixel(host, 320, 240), dib_pixel(320 * bw / 640, 240 * bh / 480));
    check("DeleteObject of the selected bitmap: fails", API("DeleteObject", bmp), 0);
    check("SelectObject(stock) gives the bitmap back", API("SelectObject", mem, stock), bmp);
    check("  then DeleteObject", API("DeleteObject", bmp), 1);
    uint32_t sdc = API("GetDC", 0);
    check("GetDeviceCaps(screen DC, BITSPIXEL): 32, as Direct3D start-up checks", API("GetDeviceCaps", sdc, 12), 32);
    API("ReleaseDC", 0, sdc);

    /* the gamma ramp */
    uint32_t ramp = halopad_heap_alloc(1536, 1);
    check("GetDeviceGammaRamp", API("GetDeviceGammaRamp", dc, ramp), 1);
    check("  identity: entry 128 is 128 x 257", ((uint16_t *)halopad_guest_ptr(ramp))[128], 128 * 257);
    for (int i = 0; i < 768; i++) ((uint16_t *)halopad_guest_ptr(ramp))[i] = (uint16_t)((i % 256) * 128);   /* half brightness */
    check("SetDeviceGammaRamp on the window's DC", API("SetDeviceGammaRamp", dc, ramp), 1);
    memset(halopad_guest_ptr(ramp), 0, 1536);
    API("GetDeviceGammaRamp", dc, ramp);
    check("  reads back", ((uint16_t *)halopad_guest_ptr(ramp))[300], (300 % 256) * 128);

    /* text, as Keystone.dll's glyph cache draws it (0x1021a7b5): metrics from the font's own
       tables as GDI reports them (Windows values from Wine's gdi32 tests: Arial 12 is 12/9/3,
       Arial -34 is 39/32/7), and a glyph drawn into a 32-bit top-down DIB section */
    uint32_t tdc = API("CreateCompatibleDC", 0);
    check("CreateCompatibleDC(NULL)", tdc != 0, 1);
    check("SetMapMode(MM_TEXT) returns the previous mode", API("SetMapMode", tdc, 1), 1);
    check("SetTextColor(white) returns black", API("SetTextColor", tdc, 0xFFFFFF), 0);
    check("SetBkColor(black) returns white", API("SetBkColor", tdc, 0), 0xFFFFFF);
    check("SetTextAlign(TA_TOP | TA_LEFT) returns 0", API("SetTextAlign", tdc, 0), 0);
    check("GetDeviceCaps(LOGPIXELSY): 96", API("GetDeviceCaps", tdc, 90), 96);
    check("MulDiv(16, 96, 72) = 21 (rounded)", API("MulDiv", 16, 96, 72), 21);
    uint32_t tm = halopad_heap_alloc(56, 1);
    static const struct { int32_t h; uint32_t w; const char *face; int32_t th, ta, td; } fm[] = {
        {12, 400, "Arial", 12, 9, 3}, {-34, 400, "Arial", 39, 32, 7}, {-16, 400, "Arial", 18, 15, 3},
        {-21, 700, "Arial Narrow", 24, 19, 5}};
    uint32_t keep = 0;
    for (size_t i = 0; i < sizeof fm / sizeof fm[0]; i++) {
        uint32_t hf = API("CreateFontA", (uint32_t)fm[i].h, 0, 0, 0, fm[i].w, 0, 0, 0, 1, 0, 0, 4, 2, str(fm[i].face));
        uint32_t old = API("SelectObject", tdc, hf);
        API("GetTextMetricsA", tdc, tm);
        char what[96];
        snprintf(what, sizeof what, "%s %d%s: tmHeight/Ascent/Descent %d/%d/%d", fm[i].face, fm[i].h, fm[i].w > 400 ? " bold" : "", fm[i].th, fm[i].ta, fm[i].td);
        check(what, rd(tm) == (uint32_t)fm[i].th && rd(tm + 4) == (uint32_t)fm[i].ta && rd(tm + 8) == (uint32_t)fm[i].td, 1);
        if (i + 1 < sizeof fm / sizeof fm[0]) { API("SelectObject", tdc, old); check("  DeleteObject(font)", API("DeleteObject", hf), 1); }
        else keep = hf;
    }
    check("  Arial Narrow maps to Arial Bold (TMPF: variable, vector, TrueType, FF_SWISS; weight 700)",
          ((uint8_t *)halopad_guest_ptr(tm))[51] == 0x27 && rd(tm + 28) == 700, 1);
    check("  internal leading = height - em; external leading 1", rd(tm + 12) == 3 && rd(tm + 16) == 1, 1);
    uint32_t q = halopad_heap_alloc(4, 1), sz = halopad_heap_alloc(8, 1);
    ((uint16_t *)halopad_guest_ptr(q))[0] = '?';
    check("GetTextExtentPoint32W(L\"?\"): the hinted width from hdmx, 13 x 24", API("GetTextExtentPoint32W", tdc, q, 1, sz) && rd(sz) == 13 && rd(sz + 4) == 24, 1);
    uint32_t bmi = halopad_heap_alloc(44, 1), pbits = halopad_heap_alloc(4, 1);
    uint32_t dibh[3] = {40, 13, (uint32_t)-24};
    memcpy(halopad_guest_ptr(bmi), dibh, 12);
    ((uint16_t *)halopad_guest_ptr(bmi))[6] = 1; ((uint16_t *)halopad_guest_ptr(bmi))[7] = 32;
    uint32_t dib = API("CreateDIBSection", tdc, bmi, 0, pbits, 0, 0);
    check("CreateDIBSection(13 x -24, 32-bit)", dib != 0 && rd(pbits) != 0, 1);
    API("SelectObject", tdc, dib);
    uint32_t rc = halopad_heap_alloc(16, 1);
    uint32_t rcv[4] = {0, 0, 13, 24};
    memcpy(halopad_guest_ptr(rc), rcv, 16);
    memset(halopad_guest_ptr(rd(pbits)), 0x7F, 13 * 24 * 4);       /* ETO_OPAQUE must clear this */
    check("ExtTextOutW(0, 0, ETO_OPAQUE, cell, L\"?\")", API("ExtTextOutW", tdc, 0, 0, 2, rc, q, 1, 0), 1);
    const uint32_t *px = halopad_guest_ptr(rd(pbits));
    uint32_t ink = 0, ink_top = 99, ink_bottom = 0, fourth = 0, gray = 0;
    for (int y = 0; y < 24; y++) for (int x = 0; x < 13; x++) {
        uint32_t p = px[y * 13 + x], b = p & 0xFF;
        fourth |= p >> 24;
        if (b != ((p >> 8) & 0xFF) || b != ((p >> 16) & 0xFF)) gray = 1;   /* white on black: gray levels only */
        if (b > 0x80) { ink++; if (y < (int)ink_top) ink_top = (uint32_t)y; if (y > (int)ink_bottom) ink_bottom = (uint32_t)y; }
    }
    check("  ink drawn (pixels over half coverage)", ink > 10, 1);
    check("  the '?' lies between the cap height and the baseline (rows 4..18)", ink_top >= 3 && ink_top <= 6 && ink_bottom <= 18 && ink_bottom >= 15, 1);
    check("  antialiased gray levels only (no color fringes)", gray, 0);
    check("  the fourth byte stays 0, as GDI leaves a DIB section", fourth, 0);
    check("  corners are background", px[0] == 0 && px[13 * 24 - 1] == 0, 1);
    check("DeleteObject(font) while selected: TRUE, deferred", API("DeleteObject", keep), 1);
    check("DeleteDC", API("DeleteDC", tdc), 1);
    check("  then DeleteObject(DIB section)", API("DeleteObject", dib), 1);
    check("ReleaseDC with the wrong window: fails", API("ReleaseDC", 0, dc), 0);
    check("ReleaseDC", API("ReleaseDC", hwnd, dc), 1);
    API("DestroyWindow", hwnd);
    check("DestroyWindow removes the host window", halopad_window_host_if_shown(hwnd) == NULL, 1);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

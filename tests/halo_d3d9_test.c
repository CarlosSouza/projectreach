/* Direct3D 9 contract test (G3/G4): creates IDirect3D9 the way Halo does (LoadLibraryA,
 * GetProcAddress, Direct3DCreate9(0x1f) entered at its guest address) and calls methods
 * through the object's guest vtable with halopad_call_guest, so every call goes through
 * dispatch, the generated llasm wrapper and the stdcall convention check. Checks the
 * answers against docs/GRAPHICS-CONTRACT.md. Linked in place of the core's main by
 * scripts/run-core.py --main tests/halo_d3d9_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_d3d9_internal.h"
#include "../port/apple/halopad_metal.h"

extern void (*halopad_d3d9_native_draw_hook)(uint32_t, uint32_t, const hp_pipeline_desc *, const hp_draw_desc *);
static uint32_t native_draw_calls, native_draw_invalid;
static void inspect_mip_draw(uint32_t g, uint32_t request, const hp_pipeline_desc *p, const hp_draw_desc *d)
{
    (void)request;
    native_draw_calls++;
    native_draw_invalid += !g || !p->vs_msl || !p->ps_msl || p->nattr != 3 || p->stride[0] != 28 ||
        !d->pipeline || !d->vbuf[0] || !d->tex[0] || !d->smp[0] || d->ibuf ||
        d->prim != 4 || d->start || d->count != 4 || !d->vs_consts || d->vs_len != 4432 ||
        !d->ps_consts || d->ps_len != 3936;
}

void *halopad_com_state(const char *iface, uint32_t g);
void halopad_metal_read_texture(void *target, void *tex, uint32_t level, uint32_t x, uint32_t y, uint32_t w, uint32_t h, void *out, uint32_t pitch);

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

uint32_t halopad_d3d9_live_default(uint32_t dev);
uint32_t halopad_d3d9_live_stateblocks(uint32_t dev);
static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-58s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}

/* Call method 'index' of the COM object at guest address obj. */
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}

/* Direct3D's cube-map face and texel (2x2 face, point sampling) for a direction */
static void cube_texel(const float d[3], int *face, int *x, int *y)
{
    float ax = fabsf(d[0]), ay = fabsf(d[1]), az = fabsf(d[2]), sc, tc, ma;
    if (ax >= ay && ax >= az) { ma = ax; if (d[0] > 0) { *face = 0; sc = -d[2]; tc = -d[1]; } else { *face = 1; sc = d[2]; tc = -d[1]; } }
    else if (ay >= az) { ma = ay; if (d[1] > 0) { *face = 2; sc = d[0]; tc = d[2]; } else { *face = 3; sc = d[0]; tc = -d[2]; } }
    else { ma = az; if (d[2] > 0) { *face = 4; sc = d[0]; tc = -d[1]; } else { *face = 5; sc = -d[0]; tc = -d[1]; } }
    *x = (sc / ma + 1) / 2 >= 0.5f;
    *y = (tc / ma + 1) / 2 >= 0.5f;
}

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

    uint32_t d3d9 = LoadLibraryA_c(str("d3d9.dll"));
    uint32_t create = GetProcAddress_c(d3d9, str("Direct3DCreate9"));
    uint32_t sdk = 0x1f;
    uint32_t d3d = halopad_call_guest(create, 1, &sdk);
    check("Direct3DCreate9(0x1f) returns an object", d3d != 0, 1);

    enum { AddRef = 1, Release = 2, GetAdapterCount = 4, GetAdapterIdentifier = 5, GetAdapterModeCount = 6,
           EnumAdapterModes = 7, GetAdapterDisplayMode = 8, CheckDeviceType = 9, CheckDeviceFormat = 10,
           CheckDeviceMultiSampleType = 11, CheckDepthStencilMatch = 12, GetDeviceCaps = 14 };
    check("GetAdapterCount", method(d3d, GetAdapterCount, 0, NULL), 1);
    check("AddRef", method(d3d, AddRef, 0, NULL), 2);
    check("Release", method(d3d, Release, 0, NULL), 1);

    uint32_t id = halopad_heap_alloc(0x44C, 1);
    check("GetAdapterIdentifier", method(d3d, GetAdapterIdentifier, 3, (uint32_t[]){0, 2, id}), 0);
    check("  VendorId (ATI)", rd(id + 0x428), 0x1002);
    check("  DeviceId (Radeon 9700 PRO)", rd(id + 0x42C), 0x4E44);
    check("  DriverVersion high (6.14)", rd(id + 0x424), 0x6000E);
    check("  DriverVersion low (10.6467)", rd(id + 0x420), 0xA1943);

    uint32_t caps = halopad_heap_alloc(304, 1);
    check("GetDeviceCaps(HAL)", method(d3d, GetDeviceCaps, 3, (uint32_t[]){0, 1, caps}), 0);
    check("  PixelShaderVersion 2.0", rd(caps + 204), 0xFFFF0200);
    check("  VertexShaderVersion 2.0", rd(caps + 196), 0xFFFE0200);
    check("  RasterCaps depth bias bits", rd(caps + 36) & 0x06000000, 0x06000000);
    check("  MaxSimultaneousTextures", rd(caps + 152), 8);
    check("  TextureAddressCaps border", rd(caps + 76) & 8, 8);
    check("  TextureCaps has no POW2 restriction", rd(caps + 60) & 0x2, 0);
    check("GetDeviceCaps(REF) not available", method(d3d, GetDeviceCaps, 3, (uint32_t[]){0, 2, caps}), 0x8876086A);

    uint32_t mode = halopad_heap_alloc(16, 1);
    check("GetAdapterDisplayMode", method(d3d, GetAdapterDisplayMode, 2, (uint32_t[]){0, mode}), 0);
    uint32_t n = method(d3d, GetAdapterModeCount, 2, (uint32_t[]){0, 22});
    check("GetAdapterModeCount(X8R8G8B8) > 0", n > 0, 1);
    uint32_t last = 0, ordered = 1, has_desktop = 0;
    for (uint32_t i = 0; i < n; i++) {
        method(d3d, EnumAdapterModes, 4, (uint32_t[]){0, 22, i, mode + 0});
        uint32_t w = rd(mode), h = rd(mode + 4), key = (w << 16) | h;
        ordered &= key > last;
        last = key;
        uint32_t dm = halopad_heap_alloc(16, 1);
        method(d3d, GetAdapterDisplayMode, 2, (uint32_t[]){0, dm});
        has_desktop |= w == rd(dm) && h == rd(dm + 4);
    }
    check("  modes strictly ordered", ordered, 1);
    check("  modes include the desktop size", has_desktop, 1);
    check("EnumAdapterModes past the end", method(d3d, EnumAdapterModes, 4, (uint32_t[]){0, 22, n, mode}), 0x8876086C);

    check("CheckDeviceType X8R8G8B8/A8R8G8B8 fullscreen", method(d3d, CheckDeviceType, 5, (uint32_t[]){0, 1, 22, 21, 0}), 0);
    check("CheckDeviceType R5G6B5/X8R8G8B8 not available", method(d3d, CheckDeviceType, 5, (uint32_t[]){0, 1, 23, 22, 0}), 0x8876086A);
    check("CheckDeviceFormat DXT1 texture", method(d3d, CheckDeviceFormat, 6, (uint32_t[]){0, 1, 22, 0, 3, 0x31545844}), 0);
    check("CheckDeviceFormat D24S8 depth surface", method(d3d, CheckDeviceFormat, 6, (uint32_t[]){0, 1, 22, 2, 1, 75}), 0);
    check("CheckDeviceFormat A8R8G8B8 render-target texture", method(d3d, CheckDeviceFormat, 6, (uint32_t[]){0, 1, 22, 1, 3, 21}), 0);
    check("CheckDeviceFormat P8 texture not available", method(d3d, CheckDeviceFormat, 6, (uint32_t[]){0, 1, 22, 0, 3, 41}), 0x8876086A);
    check("CheckDeviceFormat DXT1 autogen mipmaps", method(d3d, CheckDeviceFormat, 6, (uint32_t[]){0, 1, 22, 0x400, 3, 0x31545844}), 0x0876019F);
    uint32_t q = halopad_heap_alloc(4, 1);
    check("CheckDeviceMultiSampleType NONE", method(d3d, CheckDeviceMultiSampleType, 6, (uint32_t[]){0, 1, 22, 1, 0, q}), 0);
    check("CheckDeviceMultiSampleType 4x not available", method(d3d, CheckDeviceMultiSampleType, 6, (uint32_t[]){0, 1, 22, 1, 4, q}), 0x8876086A);
    check("CheckDepthStencilMatch X8R8G8B8/D24S8", method(d3d, CheckDepthStencilMatch, 5, (uint32_t[]){0, 1, 22, 22, 75}), 0);
    /* Device on a real window: class and window created through their guest exports, with
       user32's own DefWindowProcA as the window procedure. */
    uint32_t user32 = LoadLibraryA_c(str("user32.dll"));
    uint32_t defproc = GetProcAddress_c(user32, str("DefWindowProcA"));
    uint32_t wc = halopad_heap_alloc(48, 1);
    uint32_t cls = str("HaloPadTest");
    uint32_t wcv[12] = {48, 0, defproc, 0, 0, 0x400000, 0, 0, 0, 0, cls, 0};
    memcpy(halopad_guest_ptr(wc), wcv, 48);
    check("RegisterClassExA", halopad_call_guest(GetProcAddress_c(user32, str("RegisterClassExA")), 1, &wc) != 0, 1);
    uint32_t cw[12] = {0, cls, str("HaloPad D3D9 test"), 0x00CF0000, 100, 100, 640 + 8, 480 + 27, 0, 0, 0x400000, 0};
    uint32_t hwnd = halopad_call_guest(GetProcAddress_c(user32, str("CreateWindowExA")), 12, cw);
    check("CreateWindowExA", hwnd != 0, 1);

    enum { CreateDevice = 16 };
    uint32_t pp = halopad_heap_alloc(56, 1);
    uint32_t ppv[14] = {0, 0, 0, 1, 0, 0, 1 /* DISCARD */, hwnd, 1 /* windowed */, 1, 75 /* D24S8 */, 0, 0, 0x80000000};
    memcpy(halopad_guest_ptr(pp), ppv, 56);
    uint32_t pdev = halopad_heap_alloc(4, 1);
    cpu._st_cw = 0x027F;
    check("CreateDevice (windowed, HW vertex processing)", method(d3d, CreateDevice, 6, (uint32_t[]){0, 1, hwnd, 0x40, pp, pdev}), 0);
    uint32_t device = rd(pdev);
    check("  back buffer width from the client area", rd(pp), 640);
    check("  back buffer height from the client area", rd(pp + 4), 480);
    check("  x87 set to single precision (no FPU_PRESERVE)", cpu._st_cw, 0x007F);
    enum { DRelease = 2, TestCooperativeLevel = 3, Present = 17, BeginScene = 41, EndScene = 42, Clear = 43,
           SetViewport = 47, GetViewport = 48, SetRenderState = 57, GetRenderState = 58, GetSamplerState = 68,
           GetTextureStageState = 66 };
    uint32_t v = halopad_heap_alloc(24, 1);
    check("TestCooperativeLevel", method(device, TestCooperativeLevel, 0, NULL), 0);
    method(device, GetRenderState, 2, (uint32_t[]){7, v});
    check("  default ZENABLE with auto depth", rd(v), 1);
    method(device, GetRenderState, 2, (uint32_t[]){58, v});
    check("  default STENCILMASK", rd(v), 0xFFFFFFFF);
    method(device, GetRenderState, 2, (uint32_t[]){22, v});
    check("  default CULLMODE (CCW)", rd(v), 3);
    method(device, GetTextureStageState, 3, (uint32_t[]){0, 1, v});
    check("  default stage 0 COLOROP (MODULATE)", rd(v), 4);
    method(device, GetTextureStageState, 3, (uint32_t[]){1, 1, v});
    check("  default stage 1 COLOROP (DISABLE)", rd(v), 1);
    method(device, GetSamplerState, 3, (uint32_t[]){0, 7, v});
    check("  default MIPFILTER (NONE)", rd(v), 0);
    check("SetRenderState ALPHABLENDENABLE", method(device, SetRenderState, 2, (uint32_t[]){27, 1}), 0);
    method(device, GetRenderState, 2, (uint32_t[]){27, v});
    check("  reads back", rd(v), 1);
    method(device, GetViewport, 1, (uint32_t[]){v});
    check("  default viewport covers the back buffer", rd(v + 8) == 640 && rd(v + 12) == 480, 1);
    check("BeginScene", method(device, BeginScene, 0, NULL), 0);
    check("BeginScene twice is invalid", method(device, BeginScene, 0, NULL), 0x8876086C);
    float one = 1.0f; uint32_t onebits; memcpy(&onebits, &one, 4);
    check("Clear target+z to 0xff336699", method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF336699, onebits, 0}), 0);
    check("EndScene", method(device, EndScene, 0, NULL), 0);
    check("Present", method(device, Present, 4, (uint32_t[]){0, 0, 0, 0}), 0);
    void *halopad_d3d9_device_target(uint32_t g);
    uint32_t halopad_metal_read_pixel(void *t, uint32_t x, uint32_t y);
    check("  presented pixel (0,0) from Metal", halopad_metal_read_pixel(halopad_d3d9_device_target(device), 0, 0), 0xFF336699);
    check("  presented pixel (639,479) from Metal", halopad_metal_read_pixel(halopad_d3d9_device_target(device), 639, 479), 0xFF336699);
    /* ---- resources ---- */
    enum { CreateTexture = 23, CreateVertexBuffer = 26, CreateIndexBuffer = 27, GetTexture = 64, SetTexture = 65,
           CreateVertexDeclaration = 86, SetVertexDeclaration = 87, GetVertexDeclaration = 88, CreateVertexShader = 91,
           SetVertexShader = 92, SetVertexShaderConstantF = 94, GetVertexShaderConstantF = 95, SetStreamSource = 100,
           GetStreamSource = 101, SetIndices = 104, CreatePixelShader = 106 };
    enum { TAddRef = 1, TRelease = 2, TGetLevelCount = 13, TGetLevelDesc = 17, TGetSurfaceLevel = 18, TLockRect = 19, TUnlockRect = 20 };
    uint32_t pt = halopad_heap_alloc(4, 1);
    check("CreateTexture 256x64 A8R8G8B8 managed, full chain",
          method(device, CreateTexture, 8, (uint32_t[]){256, 64, 0, 0, 21, 1, pt, 0}), 0);
    uint32_t tex = rd(pt);
    check("  GetLevelCount", method(tex, TGetLevelCount, 0, NULL), 9);
    uint32_t desc = halopad_heap_alloc(32, 1);
    method(tex, TGetLevelDesc, 2, (uint32_t[]){3, desc});
    check("  level 3 is 32x8", rd(desc + 24) == 32 && rd(desc + 28) == 8, 1);
    uint32_t lr = halopad_heap_alloc(8, 1);
    check("  LockRect level 0", method(tex, TLockRect, 4, (uint32_t[]){0, lr, 0, 0}), 0);
    check("  pitch", rd(lr), 1024);
    uint32_t bits = rd(lr + 4), px = 0x11223344;
    memcpy(halopad_guest_ptr(bits + 1024 * 2 + 4 * 5), &px, 4);
    check("  second LockRect of the same level is invalid", method(tex, TLockRect, 4, (uint32_t[]){0, lr, 0, 0}), 0x8876086C);
    check("  UnlockRect", method(tex, TUnlockRect, 1, (uint32_t[]){0}), 0);
    uint32_t rect = halopad_heap_alloc(16, 1);
    memcpy(halopad_guest_ptr(rect), (uint32_t[]){5, 2, 6, 3}, 16);
    method(tex, TLockRect, 4, (uint32_t[]){0, lr, rect, 0x10});
    check("  LockRect of a rectangle points at that texel", rd(rd(lr + 4)), 0x11223344);
    method(tex, TUnlockRect, 1, (uint32_t[]){0});
    check("  GetSurfaceLevel", method(tex, TGetSurfaceLevel, 2, (uint32_t[]){1, pt}), 0);
    uint32_t surf = rd(pt);
    check("  the surface shares the texture's count (AddRef -> 3)", method(surf, 1, 0, NULL), 3);
    uint32_t pc = halopad_heap_alloc(4, 1);
    method(surf, 11 /* GetContainer */, 2, (uint32_t[]){0, pc});
    check("  GetContainer returns the texture", rd(pc), tex);
    check("  release the container (-> 3)", method(tex, TRelease, 0, NULL), 3);
    check("  surface Release (-> 2)", method(surf, 2, 0, NULL), 2);
    check("  surface Release (-> 1)", method(surf, 2, 0, NULL), 1);
    check("  CreateTexture with 2 levels + AUTOGENMIPMAP is invalid",
          method(device, CreateTexture, 8, (uint32_t[]){64, 64, 2, 0x400, 21, 0, pt, 0}), 0x8876086C);
    check("  DXT1 64x64 texture", method(device, CreateTexture, 8, (uint32_t[]){64, 64, 1, 0, 0x31545844, 1, pt, 0}), 0);
    uint32_t dxt = rd(pt);
    method(dxt, TLockRect, 4, (uint32_t[]){0, lr, 0, 0});
    check("  DXT1 pitch is 16 blocks x 8 bytes", rd(lr), 128);
    method(dxt, TUnlockRect, 1, (uint32_t[]){0});
    method(dxt, TRelease, 0, NULL);

    check("SetTexture", method(device, SetTexture, 2, (uint32_t[]){0, tex}), 0);
    check("  Release the bound texture to 0", method(tex, TRelease, 0, NULL), 0);
    method(device, GetTexture, 2, (uint32_t[]){0, pt});
    check("  still bound and alive (GetTexture AddRefs -> 1)", rd(pt) == tex && method(tex, TGetLevelCount, 0, NULL) == 9, 1);
    method(tex, TRelease, 0, NULL);
    check("  unbinding frees it", method(device, SetTexture, 2, (uint32_t[]){0, 0}), 0);

    uint32_t pvb = halopad_heap_alloc(4, 1), pib = halopad_heap_alloc(4, 1);
    check("CreateVertexBuffer 1024 dynamic", method(device, CreateVertexBuffer, 6, (uint32_t[]){1024, 0x208, 0, 0, pvb, 0}), 0);
    uint32_t vb = rd(pvb), pdata = halopad_heap_alloc(4, 1);
    check("  Lock 16 bytes at 64", method(vb, 11, 4, (uint32_t[]){64, 16, pdata, 0x2000}), 0);
    uint32_t vbase = rd(pdata);
    check("  Lock past the end is invalid", method(vb, 11, 4, (uint32_t[]){1020, 8, pdata, 0}), 0x8876086C);
    check("  Unlock", method(vb, 12, 0, NULL), 0);
    method(vb, 13, 1, (uint32_t[]){desc});
    check("  GetDesc size and type", rd(desc + 16) == 1024 && rd(desc + 4) == 6, 1);
    check("  lock pointer is offset 64 into the buffer", method(vb, 11, 4, (uint32_t[]){0, 0, pdata, 0}) == 0 && rd(pdata) + 64 == vbase, 1);
    method(vb, 12, 0, NULL);
    check("CreateIndexBuffer INDEX16", method(device, CreateIndexBuffer, 6, (uint32_t[]){600, 8, 101, 1, pib, 0}), 0);
    check("  INDEX8 format is invalid", method(device, CreateIndexBuffer, 6, (uint32_t[]){600, 8, 50, 1, pib, 0}), 0x8876086C);
    check("SetStreamSource", method(device, SetStreamSource, 4, (uint32_t[]){0, vb, 0, 24}), 0);
    uint32_t po = halopad_heap_alloc(4, 1), pst = halopad_heap_alloc(4, 1);
    method(device, GetStreamSource, 4, (uint32_t[]){0, pvb, po, pst});
    check("  GetStreamSource stride", rd(pst), 24);
    method(vb, 2, 0, NULL);                                        /* GetStreamSource's reference */

    uint32_t el = halopad_heap_alloc(24, 1);
    uint8_t elems[24] = {0, 0, 0, 0, 2 /* FLOAT3 */, 0, 0 /* POSITION */, 0,  0, 0, 12, 0, 4 /* D3DCOLOR */, 0, 10 /* COLOR */, 0,
                         0xFF, 0, 0, 0, 17, 0, 0, 0};
    memcpy(halopad_guest_ptr(el), elems, 24);
    uint32_t pd = halopad_heap_alloc(4, 1);
    check("CreateVertexDeclaration (position, colour)", method(device, CreateVertexDeclaration, 2, (uint32_t[]){el, pd}), 0);
    uint32_t pn = halopad_heap_alloc(4, 1);
    method(rd(pd), 4 /* GetDeclaration */, 2, (uint32_t[]){0, pn});
    check("  GetDeclaration count including END", rd(pn), 3);
    check("SetVertexDeclaration", method(device, SetVertexDeclaration, 1, (uint32_t[]){rd(pd)}), 0);

    uint32_t vsbc[] = {0xFFFE0101, 0x0000001F, 0x80000000, 0x900F0000, 0x00000001, 0xC00F0000, 0x90E40000, 0x0000FFFF};
    uint32_t psbc[] = {0xFFFF0200, 0x02000001, 0x800F0800, 0xA0E40000, 0x0000FFFF};
    uint32_t vsg = halopad_heap_alloc(sizeof vsbc, 0), psg = halopad_heap_alloc(sizeof psbc, 0);
    memcpy(halopad_guest_ptr(vsg), vsbc, sizeof vsbc);
    memcpy(halopad_guest_ptr(psg), psbc, sizeof psbc);
    uint32_t pvs = halopad_heap_alloc(4, 1), pps = halopad_heap_alloc(4, 1), psz = halopad_heap_alloc(4, 1);
    check("CreateVertexShader vs_1_1 (dcl_position; mov oPos, v0)", method(device, CreateVertexShader, 2, (uint32_t[]){vsg, pvs}), 0);
    method(rd(pvs), 4 /* GetFunction */, 2, (uint32_t[]){0, psz});
    check("  GetFunction size", rd(psz), sizeof vsbc);
    check("CreatePixelShader ps_2_0 (mov oC0, c0)", method(device, CreatePixelShader, 2, (uint32_t[]){psg, pps}), 0);
    method(rd(pps), 4, 2, (uint32_t[]){0, psz});
    check("  GetFunction size", rd(psz), sizeof psbc);
    uint32_t bad = 0xFFFF0300;
    memcpy(halopad_guest_ptr(psg), &bad, 4);
    check("CreatePixelShader ps_3_0 is invalid (contract: 2.0)", method(device, CreatePixelShader, 2, (uint32_t[]){psg, pps}), 0x8876086C);
    check("SetVertexShader", method(device, SetVertexShader, 1, (uint32_t[]){rd(pvs)}), 0);
    uint32_t cf = halopad_heap_alloc(32, 1), cg = halopad_heap_alloc(32, 1);
    float consts[8] = {1, 2, 3, 4, 5, 6, 7, 8};
    memcpy(halopad_guest_ptr(cf), consts, 32);
    method(device, SetVertexShaderConstantF, 3, (uint32_t[]){254, cf, 2});
    method(device, GetVertexShaderConstantF, 3, (uint32_t[]){254, cg, 2});
    check("  vertex shader constants c254..c255 round trip", memcmp(halopad_guest_ptr(cf), halopad_guest_ptr(cg), 32) == 0, 1);
    check("  c255..c256 is out of range", method(device, SetVertexShaderConstantF, 3, (uint32_t[]){255, cf, 2}), 0x8876086C);

    /* ---- draws ---- */
    enum { SetPixelShader = 107, DrawPrimitive = 81, SetRenderStateX = 57, SetSamplerState = 69 };
    {
        uint32_t vsc[] = {0xFFFE0101, 0x1F, 0x80000000, 0x900F0000, 0x1F, 0x8000000A, 0x900F0001, 0x1F, 0x80000005, 0x900F0002,
                          0x1, 0xC00F0000, 0x90E40000, 0x1, 0xD00F0000, 0x90E40001, 0x1, 0xE00F0000, 0x90E40002, 0xFFFF};
        uint32_t psc[] = {0xFFFF0101, 0x1, 0x800F0000, 0x90E40000, 0xFFFF};                       /* mov r0, v0 */
        uint32_t ps2c[] = {0xFFFF0200, 0x0200001F, 0x80000000, 0xB00F0000, 0x0200001F, 0x90000000, 0xA00F0800,
                          0x03000042, 0x800F0000, 0xB0E40000, 0xA0E40800, 0x02000001, 0x800F0800, 0x80E40000, 0xFFFF};  /* texld r0, t0, s0 */
        uint32_t g1 = halopad_heap_alloc(sizeof vsc, 0), g2 = halopad_heap_alloc(sizeof psc, 0), g3 = halopad_heap_alloc(sizeof ps2c, 0);
        memcpy(halopad_guest_ptr(g1), vsc, sizeof vsc); memcpy(halopad_guest_ptr(g2), psc, sizeof psc); memcpy(halopad_guest_ptr(g3), ps2c, sizeof ps2c);
        uint32_t pv = halopad_heap_alloc(4, 1), pp1 = halopad_heap_alloc(4, 1), pp2 = halopad_heap_alloc(4, 1);
        check("draw: CreateVertexShader (position, colour, texcoord)", method(device, CreateVertexShader, 2, (uint32_t[]){g1, pv}), 0);
        check("draw: CreatePixelShader ps_1_1 (mov r0, v0)", method(device, CreatePixelShader, 2, (uint32_t[]){g2, pp1}), 0);
        check("draw: CreatePixelShader ps_2_0 (texld)", method(device, CreatePixelShader, 2, (uint32_t[]){g3, pp2}), 0);
        uint8_t el3[32] = {0, 0, 0, 0, 3 /* FLOAT4 */, 0, 0 /* POSITION */, 0,  0, 0, 16, 0, 4 /* D3DCOLOR */, 0, 10 /* COLOR */, 0,
                           0, 0, 20, 0, 1 /* FLOAT2 */, 0, 5 /* TEXCOORD */, 0,  0xFF, 0, 0, 0, 17, 0, 0, 0};
        uint32_t ge = halopad_heap_alloc(32, 0), pdl = halopad_heap_alloc(4, 1);
        memcpy(halopad_guest_ptr(ge), el3, 32);
        method(device, CreateVertexDeclaration, 2, (uint32_t[]){ge, pdl});
        /* vertices: x y z w, colour, u v (28 bytes) */
        struct { float x, y, z, w; uint32_t c; float u, v; } tri[6] = {
            {-1, 1, 0.5f, 1, 0xFF00FF00, 0, 0}, {1, 1, 0.5f, 1, 0xFF00FF00, 1, 0}, {-1, -1, 0.5f, 1, 0xFF00FF00, 0, 1},   /* clockwise on screen */
            {-1, 1, 0.5f, 1, 0xFFFF0000, 0, 0}, {-1, -1, 0.5f, 1, 0xFFFF0000, 0, 1}, {1, 1, 0.5f, 1, 0xFFFF0000, 1, 0}};   /* counter-clockwise */
        uint32_t pvb2 = halopad_heap_alloc(4, 1);
        method(device, CreateVertexBuffer, 6, (uint32_t[]){sizeof tri, 0, 0, 1, pvb2, 0});
        uint32_t vb2 = rd(pvb2);
        method(vb2, 11, 4, (uint32_t[]){0, 0, pdata, 0});
        memcpy(halopad_guest_ptr(rd(pdata)), tri, sizeof tri);
        method(vb2, 12, 0, NULL);
        method(device, SetVertexShader, 1, (uint32_t[]){rd(pv)});
        method(device, SetPixelShader, 1, (uint32_t[]){rd(pp1)});
        method(device, SetVertexDeclaration, 1, (uint32_t[]){rd(pdl)});
        method(device, SetStreamSource, 4, (uint32_t[]){0, vb2, 0, 28});
        check("draw: DrawPrimitive outside a scene is invalid", method(device, DrawPrimitive, 3, (uint32_t[]){4, 0, 1}), 0x8876086C);
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("draw: DrawPrimitive clockwise triangle", method(device, DrawPrimitive, 3, (uint32_t[]){4, 0, 1}), 0);
        check("draw: DrawPrimitive counter-clockwise triangle (culled by default CULL_CCW)", method(device, DrawPrimitive, 3, (uint32_t[]){4, 3, 1}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        void *tg = halopad_d3d9_device_target(device);
        check("  pixel (20,20) inside the triangle is green", halopad_metal_read_pixel(tg, 20, 20), 0xFF00FF00);
        check("  pixel (620,460) outside stays clear", halopad_metal_read_pixel(tg, 620, 460), 0xFF000000);
        method(device, SetRenderStateX, 2, (uint32_t[]){22, 1});   /* CULL_NONE */
        method(device, BeginScene, 0, NULL);
        method(device, DrawPrimitive, 3, (uint32_t[]){4, 3, 1});
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  with CULL_NONE the counter-clockwise triangle draws (red)", halopad_metal_read_pixel(tg, 20, 20), 0xFFFF0000);

        /* pixel centres: a quad covering Direct3D pixel [-0.5, 0.5] lights exactly pixel (0,0) */
        float x0 = -1.0f - 1.0f / 640, x1 = -1.0f + 1.0f / 640, y0 = 1.0f + 1.0f / 480, y1 = 1.0f - 1.0f / 480;
        struct { float x, y, z, w; uint32_t c; float u, v; } q[6] = {{x0, y0, 0, 1, 0xFFFFFFFF, 0, 0}, {x1, y0, 0, 1, 0xFFFFFFFF, 0, 0},
            {x0, y1, 0, 1, 0xFFFFFFFF, 0, 0}, {x1, y0, 0, 1, 0xFFFFFFFF, 0, 0}, {x1, y1, 0, 1, 0xFFFFFFFF, 0, 0}, {x0, y1, 0, 1, 0xFFFFFFFF, 0, 0}};
        uint32_t gq = halopad_heap_alloc(sizeof q, 0);
        memcpy(halopad_guest_ptr(gq), q, sizeof q);
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("draw: DrawPrimitiveUP quad at Direct3D pixel (0,0)", method(device, 83, 4, (uint32_t[]){4, 2, gq, 28}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  pixel (0,0) is lit (Direct3D pixel centres)", halopad_metal_read_pixel(tg, 0, 0), 0xFFFFFFFF);
        check("  pixel (1,0) is not", halopad_metal_read_pixel(tg, 1, 0), 0xFF000000);
        check("  pixel (0,1) is not", halopad_metal_read_pixel(tg, 0, 1), 0xFF000000);
        method(device, GetStreamSource, 4, (uint32_t[]){0, pvb, po, pst});
        check("  DrawPrimitiveUP reset stream 0", rd(pvb), 0);

        /* textured quad through ps_2_0 texld: a 2x2 texture, one colour per quadrant (point sampling) */
        method(device, CreateTexture, 8, (uint32_t[]){2, 2, 1, 0, 21, 1, pt, 0});
        uint32_t t2 = rd(pt);
        method(t2, TLockRect, 4, (uint32_t[]){0, lr, 0, 0});
        uint32_t texels[4] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00};
        for (int yy = 0; yy < 2; yy++) memcpy((uint8_t *)halopad_guest_ptr(rd(lr + 4)) + yy * rd(lr), texels + 2 * yy, 8);
        method(t2, TUnlockRect, 1, (uint32_t[]){0});
        struct { float x, y, z, w; uint32_t c; float u, v; } fq[4] = {{-1, 1, 0, 1, 0, 0, 0}, {1, 1, 0, 1, 0, 1, 0}, {-1, -1, 0, 1, 0, 0, 1}, {1, -1, 0, 1, 0, 1, 1}};
        uint32_t gfq = halopad_heap_alloc(sizeof fq, 0);
        memcpy(halopad_guest_ptr(gfq), fq, sizeof fq);
        method(device, SetPixelShader, 1, (uint32_t[]){rd(pp2)});
        method(device, SetTexture, 2, (uint32_t[]){0, t2});
        method(device, SetStreamSource, 4, (uint32_t[]){0, vb2, 0, 28});
        method(device, BeginScene, 0, NULL);
        check("draw: DrawPrimitiveUP textured strip", method(device, 83, 4, (uint32_t[]){5, 2, gfq, 28}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  top-left quadrant is texel (0,0) red", halopad_metal_read_pixel(tg, 100, 100), 0xFFFF0000);
        check("  top-right quadrant is texel (1,0) green", halopad_metal_read_pixel(tg, 540, 100), 0xFF00FF00);
        check("  bottom-left quadrant is texel (0,1) blue", halopad_metal_read_pixel(tg, 100, 380), 0xFF0000FF);
        check("  bottom-right quadrant is texel (1,1) yellow", halopad_metal_read_pixel(tg, 540, 380), 0xFFFFFF00);

        /* Real fragment derivatives, not the shader compute-test kernel's level(0).
           Solid colours per level make wrong mip choice/blending unambiguous. */
        check("mips: create 8x8 managed four-level texture",
              method(device, CreateTexture, 8, (uint32_t[]){8, 8, 4, 0, 21, 1, pt, 0}), 0);
        uint32_t tmip = rd(pt);
        const uint32_t mip_colors[] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFFFF};
        for (uint32_t level = 0; level < 4; level++) {
            check("mips: lock level", method(tmip, TLockRect, 4, (uint32_t[]){level, lr, 0, 0}), 0);
            uint32_t edge = 8 >> level;
            for (uint32_t yy = 0; yy < edge; yy++)
                for (uint32_t xx = 0; xx < edge; xx++)
                    memcpy((uint8_t *)halopad_guest_ptr(rd(lr + 4)) + yy * rd(lr) + 4 * xx, mip_colors + level, 4);
            check("mips: unlock level", method(tmip, TUnlockRect, 1, (uint32_t[]){level}), 0);
        }
        method(device, SetTexture, 2, (uint32_t[]){0, tmip});
        struct { const char *name; float lod; uint32_t filter, min_lod, want; } mip_cases[] = {
            {"mips: NONE keeps level zero under minification", 2, 0, 0, 0xFFFF0000},
            {"mips: POINT at LOD 1.25 selects green level one", 1.25f, 1, 0, 0xFF00FF00},
            {"mips: POINT at LOD 1.75 selects blue level two", 1.75f, 1, 0, 0xFF0000FF},
            {"mips: LINEAR at LOD 1 selects green", 1, 2, 0, 0xFF00FF00},
            {"mips: LINEAR at LOD 1.5 blends green and blue", 1.5f, 2, 0, 0xFF008080},
            {"mips: LINEAR at LOD 2 selects blue", 2, 2, 0, 0xFF0000FF},
            {"mips: MAXMIPLEVEL 2 clamps a magnified view to blue", -1, 2, 2, 0xFF0000FF},
        };
        for (size_t mi = 0; mi < sizeof mip_cases / sizeof mip_cases[0]; mi++) {
            float u = 640.0f / 8 * exp2f(mip_cases[mi].lod), vv = 480.0f / 8 * exp2f(mip_cases[mi].lod);
            fq[1].u = fq[3].u = u; fq[2].v = fq[3].v = vv;
            memcpy(halopad_guest_ptr(gfq), fq, sizeof fq);
            method(device, SetSamplerState, 3, (uint32_t[]){0, 7, mip_cases[mi].filter});
            method(device, SetSamplerState, 3, (uint32_t[]){0, 9, mip_cases[mi].min_lod});
            method(device, BeginScene, 0, NULL);
            method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
            halopad_d3d9_native_draw_hook = inspect_mip_draw;
            check("mips: draw textured strip", method(device, 83, 4, (uint32_t[]){5, 2, gfq, 28}), 0);
            halopad_d3d9_native_draw_hook = NULL;
            method(device, EndScene, 0, NULL);
            method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
            uint32_t got = halopad_metal_read_pixel(tg, 320, 240), want = mip_cases[mi].want;
            /* Allow one UNORM rounding unit per channel, including trilinear midpoint. */
            int close = (got >> 24) == (want >> 24);
            for (int shift = 0; shift <= 16; shift += 8)
                close &= abs((int)(got >> shift & 255) - (int)(want >> shift & 255)) <= 1;
            printf("    mip readback 0x%08x expected 0x%08x\n", got, want);
            check(mip_cases[mi].name, close, 1);
        }
        check("native draw hook: called once per encoded mip draw", native_draw_calls, 7);
        check("native draw hook: exact borrowed descriptors", native_draw_invalid, 0);
        /* Validate the readback independently of sampling. On this Simulator,
           Metal's short getBytes selector reads level zero for every mip. */
        res *mips = halopad_com_state("IDirect3DTexture9", tmip);
        for (uint32_t level=0; level<4; level++) {
            uint32_t edge=8>>level, pixels[80], sentinel=0xC0FFEE00;
            for (uint32_t i=0; i<80; i++) pixels[i]=sentinel;
            halopad_metal_read_texture(tg, mips->native, level, 0, 0, edge, edge, pixels, 4*(edge+1));
            int colors_ok=1, padding_ok=1;
            for (uint32_t y=0; y<edge; y++) {
                for (uint32_t x=0; x<edge; x++) colors_ok &= pixels[y*(edge+1)+x]==mip_colors[level];
                padding_ok &= pixels[y*(edge+1)+edge]==sentinel;
            }
            printf("    native mip %u first pixel %08x want %08x\n", level, pixels[0], mip_colors[level]);
            check("mips: native readback returns this level's texels", colors_ok, 1);
            check("mips: native readback respects padded row pitch", padding_ok, 1);
        }
        /* A texture rewritten before Present must not recolour an earlier draw.
           MAXMIPLEVEL isolates level 2; the other levels must survive the update. */
        fq[1].u = fq[3].u = fq[2].v = fq[3].v = 1;
        fq[1].x = fq[3].x = 0;
        memcpy(halopad_guest_ptr(gfq), fq, sizeof fq);
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("texture update: draw old level two on left", method(device, 83, 4, (uint32_t[]){5, 2, gfq, 28}), 0);
        check("texture update: lock level two before Present", method(tmip, TLockRect, 4, (uint32_t[]){2, lr, 0, 0}), 0);
        uint32_t yellow = 0xFFFFFF00;
        for (uint32_t yy = 0; yy < 2; yy++)
            for (uint32_t xx = 0; xx < 2; xx++)
                memcpy((uint8_t *)halopad_guest_ptr(rd(lr + 4)) + yy * rd(lr) + 4 * xx, &yellow, 4);
        method(tmip, TUnlockRect, 1, (uint32_t[]){2});
        fq[0].x = fq[2].x = 0; fq[1].x = fq[3].x = 1;
        memcpy(halopad_guest_ptr(gfq), fq, sizeof fq);
        check("texture update: draw new level two on right", method(device, 83, 4, (uint32_t[]){5, 2, gfq, 28}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("texture update: earlier draw stays blue", halopad_metal_read_pixel(tg, 160, 240), 0xFF0000FF);
        check("texture update: later draw is yellow", halopad_metal_read_pixel(tg, 480, 240), yellow);
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        for (uint32_t level = 0; level < 2; level++) {
            fq[0].x = fq[2].x = level ? 0 : -1;
            fq[1].x = fq[3].x = level ? 1 : 0;
            memcpy(halopad_guest_ptr(gfq), fq, sizeof fq);
            method(device, SetSamplerState, 3, (uint32_t[]){0, 9, level});
            method(device, 83, 4, (uint32_t[]){5, 2, gfq, 28});
        }
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("texture update: unchanged level zero stays red", halopad_metal_read_pixel(tg, 160, 240), mip_colors[0]);
        check("texture update: unchanged level one stays green", halopad_metal_read_pixel(tg, 480, 240), mip_colors[1]);
        method(device, SetSamplerState, 3, (uint32_t[]){0, 7, 0});
        method(device, SetSamplerState, 3, (uint32_t[]){0, 9, 0});
        method(device, SetTexture, 2, (uint32_t[]){0, t2});
        method(tmip, TRelease, 0, NULL);

        /* ---- fixed function ---- */
        enum { SetFVF = 89, SetTextureStageState = 67, SetTransform = 44, SetMaterial = 49, SetLight = 51, LightEnable = 53 };
        method(device, SetVertexShader, 1, (uint32_t[]){0});
        method(device, SetPixelShader, 1, (uint32_t[]){0});
        /* pretransformed UI quad (FVF 0x1c4: XYZRHW | DIFFUSE | SPECULAR | TEX1), stage 0 = texture x diffuse */
        struct { float x, y, z, rhw; uint32_t diffuse, specular; float u, v; } ui[4] = {
            {99.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF, 0, 0, 0}, {199.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF, 0, 1, 0},
            {99.5f, 199.5f, 0.5f, 1, 0xFF808080, 0, 0, 1}, {199.5f, 199.5f, 0.5f, 1, 0xFF808080, 0, 1, 1}};
        uint32_t gui = halopad_heap_alloc(sizeof ui, 0);
        memcpy(halopad_guest_ptr(gui), ui, sizeof ui);
        method(device, SetFVF, 1, (uint32_t[]){0x1C4});
        method(device, SetTexture, 2, (uint32_t[]){0, t2});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 1, 4});   /* COLOROP MODULATE (texture, current=diffuse) */
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("ff: pretransformed quad (FVF 0x1c4), texture x diffuse", method(device, 83, 4, (uint32_t[]){5, 2, gui, sizeof ui[0]}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        uint32_t p100 = halopad_metal_read_pixel(tg, 100, 100);           /* diffuse interpolates from white: 254-255 */
        check("  pixel (100,100): red texel x ~white", (p100 & 0xFF00FFFF) == 0xFF000000 && ((p100 >> 16) & 0xFF) >= 0xFE, 1);
        check("  pixel (99,99) outside the quad", halopad_metal_read_pixel(tg, 99, 99), 0xFF000000);
        uint32_t p199 = halopad_metal_read_pixel(tg, 199, 100);
        check("  pixel (199,100): green texel x ~white", (p199 & 0xFFFF00FF) == 0xFF000000 && ((p199 >> 8) & 0xFF) >= 0xFE, 1);
        check("  pixel (200,100) outside the quad", halopad_metal_read_pixel(tg, 200, 100), 0xFF000000);
        uint32_t lowmid = halopad_metal_read_pixel(tg, 100, 199);            /* blue texel x ~0.5 grey (interpolated) */
        check("  pixel (100,199): blue x interpolated diffuse (~0x80)", (lowmid & 0xFFFF00) == 0 && (lowmid & 0xFF) >= 0x7C && (lowmid & 0xFF) <= 0x84, 1);

        /* transformed, unlit triangle (FVF XYZ | DIFFUSE), identity matrices */
        method(device, SetTexture, 2, (uint32_t[]){0, 0});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 1, 2});   /* SELECTARG1 */
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 0});   /* ARG1 = DIFFUSE */
        method(device, SetRenderStateX, 2, (uint32_t[]){137, 0});          /* LIGHTING off */
        struct { float x, y, z; uint32_t c; } tri2[3] = {{-1, 1, 0.5f, 0xFF0000FF}, {1, 1, 0.5f, 0xFF0000FF}, {-1, -1, 0.5f, 0xFF0000FF}};
        uint32_t gt2 = halopad_heap_alloc(sizeof tri2, 0);
        memcpy(halopad_guest_ptr(gt2), tri2, sizeof tri2);
        method(device, SetFVF, 1, (uint32_t[]){0x42});
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("ff: transformed unlit triangle (FVF 0x42; default alpha op reads the unbound texture as white)", method(device, 83, 4, (uint32_t[]){4, 1, gt2, 16}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  pixel (20,20) is the vertex colour", halopad_metal_read_pixel(tg, 20, 20), 0xFF0000FF);

        /* lit triangle: directional light along +z onto normals facing -z; material diffuse (0.5, 0.25, 1) */
        struct { float x, y, z, nx, ny, nz; } tri3[3] = {{-1, 1, 0.5f, 0, 0, -1}, {1, 1, 0.5f, 0, 0, -1}, {-1, -1, 0.5f, 0, 0, -1}};
        uint32_t gt3 = halopad_heap_alloc(sizeof tri3, 0);
        memcpy(halopad_guest_ptr(gt3), tri3, sizeof tri3);
        float mat[17] = {0.5f, 0.25f, 1.0f, 1.0f, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        uint32_t gm = halopad_heap_alloc(68, 0);
        memcpy(halopad_guest_ptr(gm), mat, 68);
        float light[26] = {0};
        uint32_t three = 3;
        memcpy(&light[0], &three, 4);
        light[1] = light[2] = light[3] = light[4] = 1.0f;                 /* white diffuse */
        light[18] = 1.0f;                                                  /* direction +z */
        uint32_t gl = halopad_heap_alloc(104, 0);
        memcpy(halopad_guest_ptr(gl), light, 104);
        method(device, SetMaterial, 1, (uint32_t[]){gm});
        method(device, SetLight, 2, (uint32_t[]){0, gl});
        method(device, LightEnable, 2, (uint32_t[]){0, 1});
        method(device, SetRenderStateX, 2, (uint32_t[]){137, 1});          /* LIGHTING on */
        method(device, SetFVF, 1, (uint32_t[]){0x12});                     /* XYZ | NORMAL */
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("ff: lit triangle (directional light, material)", method(device, 83, 4, (uint32_t[]){4, 1, gt3, 24}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        uint32_t lit = halopad_metal_read_pixel(tg, 20, 20);
        check("  lit colour = material diffuse x light (0x80, 0x40, 0xff)", ((lit >> 16) & 0xFF) >= 0x7F && ((lit >> 16) & 0xFF) <= 0x81
              && ((lit >> 8) & 0xFF) >= 0x3F && ((lit >> 8) & 0xFF) <= 0x41 && (lit & 0xFF) == 0xFF, 1);

        /* texture factor through SELECTARG1(TFACTOR) */
        method(device, SetRenderStateX, 2, (uint32_t[]){137, 0});
        method(device, SetRenderStateX, 2, (uint32_t[]){60, 0xFF123456});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 3});   /* ARG1 = TFACTOR */
        method(device, SetFVF, 1, (uint32_t[]){0x42});
        method(device, BeginScene, 0, NULL);
        check("ff: SELECTARG1(TFACTOR)", method(device, 83, 4, (uint32_t[]){4, 1, gt2, 16}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  pixel is the texture factor", halopad_metal_read_pixel(tg, 20, 20), 0xFF123456);
        /* ---- render targets and StretchRect ---- */
        enum { GetBackBuffer = 18, StretchRect = 34, CreateOffscreenPlainSurface = 36, SetRenderTarget = 37, GetRenderTarget = 38,
               GetDepthStencilSurface = 40, SGetDesc = 12, SLockRect = 13, SUnlockRect = 14, SRelease = 2 };
        uint32_t pbb = halopad_heap_alloc(4, 1), prt = halopad_heap_alloc(4, 1), desc = halopad_heap_alloc(32, 1);
        check("rt: GetBackBuffer(0, 0, MONO)", method(device, GetBackBuffer, 4, (uint32_t[]){0, 0, 0, pbb}), 0);
        uint32_t bb = rd(pbb);
        method(bb, SGetDesc, 1, (uint32_t[]){desc});
        check("  back buffer desc: X8R8G8B8 render target 640x480",
              rd(desc) == 22 && rd(desc + 8) == 1 && rd(desc + 24) == 640 && rd(desc + 28) == 480, 1);
        check("rt: GetRenderTarget(0) is the back buffer", method(device, GetRenderTarget, 2, (uint32_t[]){0, prt}) == 0 && rd(prt) == bb, 1);
        method(bb, SRelease, 0, NULL);
        check("rt: GetBackBuffer(0, 1) is invalid", method(device, GetBackBuffer, 4, (uint32_t[]){0, 1, 0, pbb}), 0x8876086C);
        check("rt: GetDepthStencilSurface", method(device, GetDepthStencilSurface, 1, (uint32_t[]){prt}), 0);
        method(rd(prt), SRelease, 0, NULL);

        /* a 64x32 render-target texture, cleared green with its left half drawn red */
        check("rt: CreateTexture 64x32 RENDERTARGET", method(device, CreateTexture, 8, (uint32_t[]){64, 32, 1, 1, 21, 0, pt, 0}), 0);
        uint32_t rtt = rd(pt);
        method(rtt, TGetSurfaceLevel, 2, (uint32_t[]){0, prt});
        uint32_t rts = rd(prt);
        check("rt: LockRect on a render-target texture is invalid", method(rtt, TLockRect, 4, (uint32_t[]){0, lr, 0, 0}), 0x8876086C);
        check("rt: SetRenderTarget(1, ...) is invalid (one target)", method(device, SetRenderTarget, 2, (uint32_t[]){1, rts}), 0x8876086C);
        check("rt: SetRenderTarget(0, NULL) is invalid", method(device, SetRenderTarget, 2, (uint32_t[]){0, 0}), 0x8876086C);
        check("rt: SetRenderTarget(0, texture level 0)", method(device, SetRenderTarget, 2, (uint32_t[]){0, rts}), 0);
        method(device, GetViewport, 1, (uint32_t[]){v});
        check("  viewport reset to 64x32", rd(v) == 0 && rd(v + 8) == 64 && rd(v + 12) == 32, 1);
        struct { float x, y, z, rhw; uint32_t c; } half[4] = {
            {-0.5f, -0.5f, 0.5f, 1, 0xFFFF0000}, {31.5f, -0.5f, 0.5f, 1, 0xFFFF0000},
            {-0.5f, 31.5f, 0.5f, 1, 0xFFFF0000}, {31.5f, 31.5f, 0.5f, 1, 0xFFFF0000}};
        uint32_t ghalf = halopad_heap_alloc(sizeof half, 0);
        memcpy(halopad_guest_ptr(ghalf), half, sizeof half);
        method(device, SetFVF, 1, (uint32_t[]){0x44});                     /* XYZRHW | DIFFUSE */
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 0});     /* ARG1 = DIFFUSE */
        method(device, BeginScene, 0, NULL);
        check("rt: Clear the render target green", method(device, Clear, 6, (uint32_t[]){0, 0, 1, 0xFF00FF00, onebits, 0}), 0);
        check("rt: draw its left half red (depth buffer larger)", method(device, 83, 4, (uint32_t[]){5, 2, ghalf, sizeof half[0]}), 0);
        method(device, EndScene, 0, NULL);

        /* back on the back buffer, sample the texture across the screen */
        check("rt: SetRenderTarget(0, back buffer)", method(device, SetRenderTarget, 2, (uint32_t[]){0, bb}), 0);
        method(device, GetViewport, 1, (uint32_t[]){v});
        check("  viewport reset to 640x480", rd(v + 8) == 640 && rd(v + 12) == 480, 1);
        struct { float x, y, z, rhw, u, v; } full[4] = {
            {-0.5f, -0.5f, 0.5f, 1, 0, 0}, {639.5f, -0.5f, 0.5f, 1, 1, 0}, {-0.5f, 479.5f, 0.5f, 1, 0, 1}, {639.5f, 479.5f, 0.5f, 1, 1, 1}};
        uint32_t gfull = halopad_heap_alloc(sizeof full, 0);
        memcpy(halopad_guest_ptr(gfull), full, sizeof full);
        method(device, SetFVF, 1, (uint32_t[]){0x104});                    /* XYZRHW | TEX1 */
        method(device, SetTexture, 2, (uint32_t[]){0, rtt});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 2});     /* ARG1 = TEXTURE */
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        check("rt: draw the screen with the render-target texture", method(device, 83, 4, (uint32_t[]){5, 2, gfull, sizeof full[0]}), 0);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  left half of the screen samples red", halopad_metal_read_pixel(tg, 100, 240), 0xFFFF0000);
        check("  right half samples green", halopad_metal_read_pixel(tg, 540, 240), 0xFF00FF00);
        method(device, SetTexture, 2, (uint32_t[]){0, 0});

        /* StretchRect: the texture onto the back buffer at twice the size */
        uint32_t rc = halopad_heap_alloc(16, 0);
        memcpy(halopad_guest_ptr(rc), (int32_t[]){0, 0, 128, 64}, 16);
        method(device, Clear, 6, (uint32_t[]){0, 0, 1, 0xFF000000, onebits, 0});
        check("rt: StretchRect texture -> back buffer 128x64 (point)", method(device, StretchRect, 5, (uint32_t[]){rts, 0, bb, rc, 1}), 0);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  (10,10) red", halopad_metal_read_pixel(tg, 10, 10), 0xFFFF0000);
        check("  (63,63) red, (64,63) green (scaled edge)", halopad_metal_read_pixel(tg, 63, 63) == 0xFFFF0000
              && halopad_metal_read_pixel(tg, 64, 63) == 0xFF00FF00, 1);
        check("  (130,10) outside the rectangle untouched", halopad_metal_read_pixel(tg, 130, 10), 0xFF000000);
        check("rt: StretchRect onto the same surface is invalid", method(device, StretchRect, 5, (uint32_t[]){bb, 0, bb, 0, 0}), 0x8876086C);
        memcpy(halopad_guest_ptr(rc), (int32_t[]){0, 0, 65, 32}, 16);
        check("rt: StretchRect with a source rectangle outside is invalid", method(device, StretchRect, 5, (uint32_t[]){rts, rc, bb, 0, 0}), 0x8876086C);

        /* back buffer -> texture (same size: a copy), then the texture back at (200, 200) */
        memcpy(halopad_guest_ptr(rc), (int32_t[]){32, 0, 96, 32}, 16);
        check("rt: StretchRect back buffer -> texture (copy)", method(device, StretchRect, 5, (uint32_t[]){bb, rc, rts, 0, 0}), 0);
        memcpy(halopad_guest_ptr(rc), (int32_t[]){200, 200, 264, 232}, 16);
        method(device, StretchRect, 5, (uint32_t[]){rts, 0, bb, rc, 0});
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  red (back buffer x 32-63) lands at (231,210)", halopad_metal_read_pixel(tg, 231, 210), 0xFFFF0000);
        check("  green (back buffer x 64-95) at (232,210) and (263,231)", halopad_metal_read_pixel(tg, 232, 210) == 0xFF00FF00
              && halopad_metal_read_pixel(tg, 263, 231) == 0xFF00FF00, 1);
        check("  (264,210) outside untouched", halopad_metal_read_pixel(tg, 264, 210), 0xFF000000);

        /* Halo's loading screen: an offscreen plain X8R8G8B8 surface, filled, stretched onto the target */
        uint32_t poff = halopad_heap_alloc(4, 1);
        check("rt: CreateOffscreenPlainSurface 4x4 X8R8G8B8 default pool",
              method(device, CreateOffscreenPlainSurface, 6, (uint32_t[]){4, 4, 22, 0, poff, 0}), 0);
        uint32_t off = rd(poff);
        method(off, SGetDesc, 1, (uint32_t[]){desc});
        check("  desc: X8R8G8B8 4x4, no usage", rd(desc) == 22 && rd(desc + 8) == 0 && rd(desc + 24) == 4 && rd(desc + 28) == 4, 1);
        check("  LockRect", method(off, SLockRect, 3, (uint32_t[]){lr, 0, 0}), 0);
        for (int yy = 0; yy < 4; yy++)
            for (int xx = 0; xx < 4; xx++) {
                uint32_t px = xx < 2 ? 0x00FF00FFu : 0x000000FFu;           /* X byte 0: must still read as opaque */
                memcpy((uint8_t *)halopad_guest_ptr(rd(lr + 4)) + yy * rd(lr) + 4 * xx, &px, 4);
            }
        check("  UnlockRect", method(off, SUnlockRect, 0, NULL), 0);
        check("rt: StretchRect offscreen -> back buffer (filter NONE)", method(device, StretchRect, 5, (uint32_t[]){off, 0, bb, 0, 0}), 0);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        check("  (100,240) magenta, opaque", halopad_metal_read_pixel(tg, 100, 240), 0xFFFF00FF);
        check("  (540,240) blue, opaque", halopad_metal_read_pixel(tg, 540, 240), 0xFF0000FF);
        check("rt: CreateOffscreenPlainSurface in the managed pool is invalid",
              method(device, CreateOffscreenPlainSurface, 6, (uint32_t[]){4, 4, 22, 1, poff, 0}), 0x8876086C);
        check("  Release the offscreen surface", method(off, SRelease, 0, NULL), 0);
        /* ---- occlusion queries ---- */
        enum { CreateQuery = 118, QRelease = 2, QGetType = 4, QGetDataSize = 5, QIssue = 6, QGetData = 7 };
        uint32_t pq = halopad_heap_alloc(4, 1), qd = halopad_heap_alloc(4, 1);
        check("query: CreateQuery(OCCLUSION, NULL) support check", method(device, CreateQuery, 2, (uint32_t[]){9, 0}), 0);
        check("query: CreateQuery(OCCLUSION)", method(device, CreateQuery, 2, (uint32_t[]){9, pq}), 0);
        uint32_t oq = rd(pq);
        *(uint32_t *)halopad_guest_ptr(qd) = 0xFFFFFFFFu;
        check("query: GetData before Issue is invalid", method(oq, QGetData, 3, (uint32_t[]){qd, 4, 1}), 0x8876086C);
        check("  unissued query preserves conservative sample count", rd(qd), 0xFFFFFFFFu);
        check("  GetType 9, GetDataSize 4", method(oq, QGetType, 0, NULL) == 9 && method(oq, QGetDataSize, 0, NULL) == 4, 1);
        struct { float x, y, z, rhw; uint32_t c; } sq[4] = {
            {99.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF}, {115.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF}, {99.5f, 115.5f, 0.5f, 1, 0xFFFFFFFF}, {115.5f, 115.5f, 0.5f, 1, 0xFFFFFFFF}};
        uint32_t gsq = halopad_heap_alloc(sizeof sq, 0);
        memcpy(halopad_guest_ptr(gsq), sq, sizeof sq);
        method(device, SetFVF, 1, (uint32_t[]){0x44});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 0});     /* ARG1 = DIFFUSE */
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
        method(oq, QIssue, 1, (uint32_t[]){2});
        method(device, 83, 4, (uint32_t[]){5, 2, gsq, sizeof sq[0]});
        check("query: GetData while building is invalid", method(oq, QGetData, 3, (uint32_t[]){qd, 4, 1}), 0x8876086C);
        method(oq, QIssue, 1, (uint32_t[]){1});
        check("query: GetData(FLUSH) after a 16x16 quad", method(oq, QGetData, 3, (uint32_t[]){qd, 4, 1}), 0);
        check("  256 samples passed", rd(qd), 256);
        method(device, SetRenderStateX, 2, (uint32_t[]){23, 1});            /* ZFUNC NEVER */
        method(oq, QIssue, 1, (uint32_t[]){2});
        method(device, 83, 4, (uint32_t[]){5, 2, gsq, sizeof sq[0]});
        method(oq, QIssue, 1, (uint32_t[]){1});
        method(oq, QGetData, 3, (uint32_t[]){qd, 4, 1});
        check("  with ZFUNC NEVER: 0 samples", rd(qd), 0);
        method(device, SetRenderStateX, 2, (uint32_t[]){23, 4});            /* LESSEQUAL */
        /* one query across two render passes (the StretchRect in between ends the first) */
        uint32_t poff2 = halopad_heap_alloc(4, 1);
        method(device, CreateOffscreenPlainSurface, 6, (uint32_t[]){4, 4, 22, 0, poff2, 0});
        method(oq, QIssue, 1, (uint32_t[]){2});
        method(device, 83, 4, (uint32_t[]){5, 2, gsq, sizeof sq[0]});
        memcpy(halopad_guest_ptr(rc), (int32_t[]){300, 300, 304, 304}, 16);
        method(device, StretchRect, 5, (uint32_t[]){rd(poff2), 0, bb, rc, 0});
        struct { float x, y, z, rhw; uint32_t c; } sq8[4] = {
            {199.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF}, {207.5f, 99.5f, 0.5f, 1, 0xFFFFFFFF}, {199.5f, 107.5f, 0.5f, 1, 0xFFFFFFFF}, {207.5f, 107.5f, 0.5f, 1, 0xFFFFFFFF}};
        memcpy(halopad_guest_ptr(gsq), sq8, sizeof sq8);
        method(device, 83, 4, (uint32_t[]){5, 2, gsq, sizeof sq8[0]});
        method(oq, QIssue, 1, (uint32_t[]){1});
        method(oq, QGetData, 3, (uint32_t[]){qd, 4, 1});
        check("  across two passes: 256 + 64 samples", rd(qd), 320);
        method(device, EndScene, 0, NULL);
        method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
        method(oq, QIssue, 1, (uint32_t[]){1});
        check("query: END without BEGIN", method(oq, QGetData, 3, (uint32_t[]){qd, 4, 0}), 0);
        check("  counts 0", rd(qd), 0);
        check("query: GetData with a 2-byte buffer is invalid", method(oq, QGetData, 3, (uint32_t[]){qd, 2, 0}), 0x8876086C);
        check("  Release the query", method(oq, QRelease, 0, NULL), 0);
        method(rd(poff2), SRelease, 0, NULL);
        method(rts, SRelease, 0, NULL);
        check("  Release the render-target texture", method(rtt, TRelease, 0, NULL), 0);
        method(bb, SRelease, 0, NULL);
        /* ---- cube and volume textures (fixed function, 3-component texture coordinates) ---- */
        enum { CreateVolumeTexture = 24, CreateCubeTexture = 25, BGetType = 10, CLockRect = 19, CUnlockRect = 20,
               VGetLevelDesc = 17, VLockBox = 19, VUnlockBox = 20, CGetCubeMapSurface = 18 };
        uint32_t pcube = halopad_heap_alloc(4, 1);
        check("tex: CreateCubeTexture 2x2 A8R8G8B8 managed", method(device, CreateCubeTexture, 7, (uint32_t[]){2, 1, 0, 21, 1, pcube, 0}), 0);
        uint32_t cube = rd(pcube);
        check("  GetType is CUBETEXTURE", method(cube, BGetType, 0, NULL), 5);
        static const uint32_t face_colour[6] = {0xFFFF0000, 0xFF00FF00, 0xFF0000FF, 0xFFFFFF00, 0xFF00FFFF, 0xFFFF00FF};
        static const float dirs[6][3] = {{1, 0.2f, 0.1f}, {-1, 0.2f, 0.1f}, {0.1f, 1, 0.2f}, {0.1f, -1, 0.2f}, {0.2f, 0.1f, 1}, {0.2f, 0.1f, -1}};
        uint32_t cube_ok = 1;
        for (uint32_t f = 0; f < 6; f++) {                              /* the face's colour only where dirs[f] lands */
            int cf, cx, cy;
            cube_texel(dirs[f], &cf, &cx, &cy);
            cube_ok &= cf == (int)f;
            cube_ok &= method(cube, CLockRect, 5, (uint32_t[]){f, 0, lr, 0, 0}) == 0;
            for (int yy = 0; yy < 2; yy++)
                for (int xx = 0; xx < 2; xx++) {
                    uint32_t c = xx == cx && yy == cy ? face_colour[f] : 0xFF808080u;
                    memcpy((uint8_t *)halopad_guest_ptr(rd(lr + 4)) + yy * rd(lr) + 4 * xx, &c, 4);
                }
            cube_ok &= method(cube, CUnlockRect, 2, (uint32_t[]){f, 0}) == 0;
        }
        check("  LockRect/UnlockRect on all six faces", cube_ok, 1);
        check("  LockRect face 6 is invalid", method(cube, CLockRect, 5, (uint32_t[]){6, 0, lr, 0, 0}), 0x8876086C);
        check("  GetCubeMapSurface(NEGATIVE_Z, 0)", method(cube, CGetCubeMapSurface, 3, (uint32_t[]){5, 0, prt}), 0);
        method(rd(prt), SGetDesc, 1, (uint32_t[]){desc});
        check("  face surface desc 2x2 A8R8G8B8", rd(desc) == 21 && rd(desc + 24) == 2 && rd(desc + 28) == 2, 1);
        method(rd(prt), SRelease, 0, NULL);
        /* a quad with one direction per draw: XYZRHW | TEX1 | TEXCOORDSIZE3(0) */
        struct { float x, y, z, rhw, s, t, r; } cq[4] = {
            {-0.5f, -0.5f, 0.5f, 1, 0, 0, 0}, {15.5f, -0.5f, 0.5f, 1, 0, 0, 0}, {-0.5f, 15.5f, 0.5f, 1, 0, 0, 0}, {15.5f, 15.5f, 0.5f, 1, 0, 0, 0}};
        uint32_t gcq = halopad_heap_alloc(sizeof cq, 0);
        method(device, SetFVF, 1, (uint32_t[]){0x104 | 0x10000});
        method(device, SetTexture, 2, (uint32_t[]){0, cube});
        method(device, SetTextureStageState, 3, (uint32_t[]){0, 2, 2});     /* ARG1 = TEXTURE */
        uint32_t faces_ok = 1;
        for (int f = 0; f < 6; f++) {
            for (int k = 0; k < 4; k++) { cq[k].s = dirs[f][0]; cq[k].t = dirs[f][1]; cq[k].r = dirs[f][2]; }
            memcpy(halopad_guest_ptr(gcq), cq, sizeof cq);
            method(device, BeginScene, 0, NULL);
            method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
            faces_ok &= method(device, 83, 4, (uint32_t[]){5, 2, gcq, sizeof cq[0]}) == 0;
            method(device, EndScene, 0, NULL);
            method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
            uint32_t got = halopad_metal_read_pixel(tg, 8, 8);
            if (got != face_colour[f]) { printf("    face %d: got 0x%08x want 0x%08x\n", f, got, face_colour[f]); faces_ok = 0; }
        }
        check("  each direction samples its face and the texel Direct3D's face table gives", faces_ok, 1);
        method(device, SetTexture, 2, (uint32_t[]){0, 0});
        check("  Release the cube texture", method(cube, TRelease, 0, NULL), 0);

        uint32_t pvol = halopad_heap_alloc(4, 1), lbox = halopad_heap_alloc(12, 1), bx = halopad_heap_alloc(24, 0);
        check("tex: CreateVolumeTexture 2x2x2 A8R8G8B8 managed",
              method(device, CreateVolumeTexture, 9, (uint32_t[]){2, 2, 2, 1, 0, 21, 1, pvol, 0}), 0);
        uint32_t vol = rd(pvol);
        check("  GetType is VOLUMETEXTURE", method(vol, BGetType, 0, NULL), 4);
        method(vol, VGetLevelDesc, 2, (uint32_t[]){0, desc});
        check("  level desc 2x2x2 (D3DVOLUME_DESC)", rd(desc + 4) == 2 && rd(desc + 16) == 2 && rd(desc + 20) == 2 && rd(desc + 24) == 2, 1);
        check("  LockBox whole level", method(vol, VLockBox, 4, (uint32_t[]){0, lbox, 0, 0}), 0);
        check("  row pitch 8, slice pitch 16", rd(lbox) == 8 && rd(lbox + 4) == 16, 1);
        for (int zz = 0; zz < 2; zz++)
            for (int i = 0; i < 4; i++) { uint32_t c = zz ? 0xFF0000FFu : 0xFFFF0000u; memcpy((uint8_t *)halopad_guest_ptr(rd(lbox + 8)) + 16 * zz + 4 * i, &c, 4); }
        check("  a second LockBox while locked is invalid", method(vol, VLockBox, 4, (uint32_t[]){0, lbox, 0, 0}), 0x8876086C);
        check("  UnlockBox", method(vol, VUnlockBox, 1, (uint32_t[]){0}), 0);
        memcpy(halopad_guest_ptr(bx), (uint32_t[]){1, 1, 2, 2, 1, 2}, 24);
        method(vol, VLockBox, 4, (uint32_t[]){0, lbox, bx, 0});
        uint32_t green = 0xFF00FF00;
        memcpy(halopad_guest_ptr(rd(lbox + 8)), &green, 4);                /* texel (1,1,1) */
        method(vol, VUnlockBox, 1, (uint32_t[]){0});
        method(device, SetTexture, 2, (uint32_t[]){0, vol});
        static const float probe[3][3] = {{0.25f, 0.25f, 0.25f}, {0.25f, 0.25f, 0.75f}, {0.75f, 0.75f, 0.75f}};
        static const uint32_t want_vol[3] = {0xFFFF0000, 0xFF0000FF, 0xFF00FF00};
        uint32_t vol_ok = 1;
        for (int k = 0; k < 3; k++) {
            for (int j = 0; j < 4; j++) { cq[j].s = probe[k][0]; cq[j].t = probe[k][1]; cq[j].r = probe[k][2]; }
            memcpy(halopad_guest_ptr(gcq), cq, sizeof cq);
            method(device, BeginScene, 0, NULL);
            method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF000000, onebits, 0});
            vol_ok &= method(device, 83, 4, (uint32_t[]){5, 2, gcq, sizeof cq[0]}) == 0;
            method(device, EndScene, 0, NULL);
            method(device, Present, 4, (uint32_t[]){0, 0, 0, 0});
            uint32_t got = halopad_metal_read_pixel(tg, 8, 8);
            if (got != want_vol[k]) { printf("    probe %d: got 0x%08x want 0x%08x\n", k, got, want_vol[k]); vol_ok = 0; }
        }
        check("  slice 0 red, slice 1 blue, boxed texel (1,1,1) green", vol_ok, 1);
        method(device, SetTexture, 2, (uint32_t[]){0, 0});
        check("  Release the volume texture", method(vol, TRelease, 0, NULL), 0);
        method(t2, TRelease, 0, NULL);
    }

    {   /* state blocks: Keystone.dll saves and restores Halo's state around its drawing with them */
        enum { SB_Create = 59, SB_Begin = 60, SB_End = 61, SB_GetTexture = 64, SB_SetTexture = 65, SB_CreateTexture = 23,
               SB_Release = 2, SB_Capture = 4, SB_Apply = 5, SB_GetDevice = 3 };
        uint32_t out = halopad_heap_alloc(4, 1), val = halopad_heap_alloc(24, 1);
        method(device, SB_CreateTexture, 8, (uint32_t[]){2, 2, 1, 0, 21, 1, out, 0});
        uint32_t tx = rd(out);
        method(device, SetRenderState, 2, (uint32_t[]){27, 0});
        check("EndStateBlock without BeginStateBlock: invalid", method(device, SB_End, 1, (uint32_t[]){out}), 0x8876086C);
        check("BeginStateBlock", method(device, SB_Begin, 0, NULL), 0);
        check("  BeginStateBlock while recording: invalid", method(device, SB_Begin, 0, NULL), 0x8876086C);
        check("  SetRenderState(ALPHABLENDENABLE, 1) is recorded", method(device, SetRenderState, 2, (uint32_t[]){27, 1}), 0);
        method(device, GetRenderState, 2, (uint32_t[]){27, val});
        check("  ... and the device keeps 0 while recording", rd(val), 0);
        check("  SetTexture(0, texture) is recorded", method(device, SB_SetTexture, 2, (uint32_t[]){0, tx}), 0);
        method(device, SB_GetTexture, 2, (uint32_t[]){0, out});
        check("  ... and stage 0 stays empty on the device", rd(out), 0);
        check("EndStateBlock", method(device, SB_End, 1, (uint32_t[]){out}), 0);
        uint32_t sb = rd(out);
        check("  gives a block", sb != 0, 1);
        method(sb, SB_GetDevice, 1, (uint32_t[]){out});
        check("  GetDevice", rd(out), device);
        method(device, DRelease, 0, NULL);
        check("Apply", method(sb, SB_Apply, 0, NULL), 0);
        method(device, GetRenderState, 2, (uint32_t[]){27, val});
        check("  ALPHABLENDENABLE now 1", rd(val), 1);
        method(device, SB_GetTexture, 2, (uint32_t[]){0, out});
        check("  stage 0 now holds the texture", rd(out), tx);
        method(tx, 2, 0, NULL);                                          /* GetTexture's reference */
        method(device, SetRenderState, 2, (uint32_t[]){27, 0});
        check("Capture (the block's states only)", method(sb, SB_Capture, 0, NULL), 0);
        method(device, SetRenderState, 2, (uint32_t[]){27, 1});
        method(sb, SB_Apply, 0, NULL);
        method(device, GetRenderState, 2, (uint32_t[]){27, val});
        check("  Apply restores the captured 0", rd(val), 0);
        method(device, SB_SetTexture, 2, (uint32_t[]){0, 0});
        check("  the texture stays alive while the block holds it (Release -> 0 references left to the app)", method(tx, 2, 0, NULL), 0);
        method(device, SB_CreateTexture, 8, (uint32_t[]){2, 2, 1, 0, 21, 1, out, 0});
        check("Release the recorded block", method(sb, SB_Release, 0, NULL), 0);
        uint32_t tx2 = rd(out);
        method(tx2, 2, 0, NULL);
        /* CreateStateBlock(D3DSBT_ALL) */
        method(device, GetViewport, 1, (uint32_t[]){val});
        uint32_t vp0[6]; memcpy(vp0, halopad_guest_ptr(val), 24);
        method(device, SetRenderState, 2, (uint32_t[]){22, 2});
        check("CreateStateBlock(ALL)", method(device, SB_Create, 2, (uint32_t[]){1, out}), 0);
        uint32_t all = rd(out);
        uint32_t vp1[6] = {10, 10, 100, 100, 0, 0x3F800000};
        memcpy(halopad_guest_ptr(val), vp1, 24);
        method(device, SetViewport, 1, (uint32_t[]){val});
        method(device, SetRenderState, 2, (uint32_t[]){22, 3});
        method(all, SB_Apply, 0, NULL);
        method(device, GetViewport, 1, (uint32_t[]){val});
        check("  Apply restores the viewport", !memcmp(halopad_guest_ptr(val), vp0, 24), 1);
        method(device, GetRenderState, 2, (uint32_t[]){22, val});
        check("  ... and CULLMODE", rd(val), 2);
        method(all, SB_Release, 0, NULL);
        /* CreateStateBlock(D3DSBT_PIXELSTATE): SRCBLEND is pixel state, CULLMODE is not */
        method(device, SetRenderState, 2, (uint32_t[]){19, 2});
        check("CreateStateBlock(PIXELSTATE)", method(device, SB_Create, 2, (uint32_t[]){2, out}), 0);
        uint32_t px = rd(out);
        method(device, SetRenderState, 2, (uint32_t[]){19, 5});
        method(device, SetRenderState, 2, (uint32_t[]){22, 3});
        method(px, SB_Apply, 0, NULL);
        method(device, GetRenderState, 2, (uint32_t[]){19, val});
        check("  Apply restores SRCBLEND", rd(val), 2);
        method(device, GetRenderState, 2, (uint32_t[]){22, val});
        check("  ... and leaves CULLMODE (vertex state) alone", rd(val), 3);
        method(px, SB_Release, 0, NULL);
        method(device, SetRenderState, 2, (uint32_t[]){22, 2});
        method(device, SetRenderState, 2, (uint32_t[]){19, 2});
    }


    /* Reset, as Halo calls it after releasing its default-pool objects (0x519751) */
    {
        enum { Reset = 16, CreateVB = 26, VRelease = 2, GetBackBufferX = 18, SRel = 2, SGetDesc = 12 };
        method(device, SetStreamSource, 4, (uint32_t[]){0, 0, 0, 0});
        method(vb, 2, 0, NULL);                                       /* the dynamic vertex buffer from above */
        uint32_t live = halopad_d3d9_live_default(device), blocks = halopad_d3d9_live_stateblocks(device);
        if (live || blocks) printf("    (left alive by earlier checks: %u default-pool resources, %u state blocks)\n", live, blocks);
        uint32_t pvb = halopad_heap_alloc(4, 1), npp = halopad_heap_alloc(56, 1);
        uint32_t nppv[14] = {800, 600, 22, 1, 0, 0, 1, hwnd, 1, 1, 75, 0, 0, 0x80000000};
        memcpy(halopad_guest_ptr(npp), nppv, 56);
        method(device, CreateVB, 6, (uint32_t[]){64, 0x8, 0, 0, pvb, 0});   /* a default-pool vertex buffer */
        check("Reset with a default-pool vertex buffer alive: D3DERR_INVALIDCALL", method(device, Reset, 1, (uint32_t[]){npp}), 0x8876086C);
        method(rd(pvb), VRelease, 0, NULL);
        uint32_t pbb = halopad_heap_alloc(4, 1);
        method(device, GetBackBufferX, 4, (uint32_t[]){0, 0, 0, pbb});
        check("Reset while the application holds the back buffer: D3DERR_INVALIDCALL", method(device, Reset, 1, (uint32_t[]){npp}), 0x8876086C);
        method(rd(pbb), SRel, 0, NULL);
        method(device, SetRenderState, 2, (uint32_t[]){22, 1});        /* CULLMODE NONE, to see it reset */
        check("Reset to 800x600 once they are released", method(device, Reset, 1, (uint32_t[]){npp}), 0);
        method(device, GetRenderState, 2, (uint32_t[]){22, v});
        check("  render states back to their defaults (CULLMODE CCW)", rd(v), 3);
        method(device, GetViewport, 1, (uint32_t[]){v});
        check("  viewport: the new 800x600 back buffer", rd(v + 8) == 800 && rd(v + 12) == 600, 1);
        method(device, GetBackBufferX, 4, (uint32_t[]){0, 0, 0, pbb});
        uint32_t desc = halopad_heap_alloc(32, 1);
        method(rd(pbb), SGetDesc, 1, (uint32_t[]){desc});
        check("  back buffer 800x600", rd(desc + 24) == 800 && rd(desc + 28) == 600, 1);
        method(rd(pbb), SRel, 0, NULL);
        float one = 1.0f; uint32_t ob; memcpy(&ob, &one, 4);
        method(device, BeginScene, 0, NULL);
        method(device, Clear, 6, (uint32_t[]){0, 0, 3, 0xFF00FF00, ob, 0});
        method(device, EndScene, 0, NULL);
        check("  Present after Reset", method(device, Present, 4, (uint32_t[]){0, 0, 0, 0}), 0);
        check("  pixel (799,599) of the new back buffer is the clear colour",
              halopad_metal_read_pixel(halopad_d3d9_device_target(device), 799, 599), 0xFF00FF00);
    }
    check("Device Release to zero", method(device, DRelease, 0, NULL), 0);
    check("Release to zero", method(d3d, Release, 0, NULL), 0);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

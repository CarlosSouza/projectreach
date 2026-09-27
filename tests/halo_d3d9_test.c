/* Direct3D 9 contract test (G3/G4): creates IDirect3D9 the way Halo does (LoadLibraryA,
 * GetProcAddress, Direct3DCreate9(0x1f) entered at its guest address) and calls methods
 * through the object's guest vtable with halopad_call_guest, so every call goes through
 * dispatch, the generated llasm wrapper and the stdcall convention check. Checks the
 * answers against docs/GRAPHICS-CONTRACT.md. Linked in place of the core's main by
 * scripts/run-core.py --main tests/halo_d3d9_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

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
    check("Device Release to zero", method(device, DRelease, 0, NULL), 0);
    check("Release to zero", method(d3d, Release, 0, NULL), 0);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

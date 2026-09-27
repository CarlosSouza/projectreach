/* HaloPad Direct3D 9, part 2 (G3/G4): IDirect3DDevice9 on Metal.
 *
 * Implemented so far: device creation on Halo's window, reference counting, cooperative
 * level, caps/display/creation queries, scenes, Clear (whole targets) and Present (whole
 * back buffer), and the full fixed state: render states, texture-stage states, sampler
 * states, transforms and viewport, starting from Direct3D 9's documented defaults.
 * Every other method traps with its name (generated stubs). Unknown state numbers,
 * partial clears/presents, multisampling and pure devices stop with their values. */
#include "halopad_win32.h"
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);
void *halopad_com_state(const char *iface, uint32_t g);
void halopad_d3d9_fill_caps(uint32_t c);
void halopad_desktop_size(int32_t *w, int32_t *h);
int halopad_window_client_size(uint32_t hwnd, int32_t *w, int32_t *h);
const char *halopad_window_text(uint32_t hwnd);
void *halopad_metal_target_create(uint32_t width, uint32_t height, int depth_stencil, const char *title);
void halopad_metal_target_destroy(void *t);
void halopad_metal_clear(void *t, int color, int depth, int stencil, const float rgba[4], float z, uint32_t s);
int halopad_metal_present(void *t);
extern _Thread_local _cpu *halopad_cpu;

#define D3D_OK 0u
#define D3DERR_INVALIDCALL 0x8876086Cu
#define NRS 210
#define NTSS 33
#define NSS 14
#define NSAMPLERS 16

static uint8_t rs_valid[NRS];   /* D3DRENDERSTATETYPE values that exist (set from the default tables) */

typedef struct {
    uint32_t guest, d3d, window, behavior, pp[14];
    uint32_t rs[NRS], tss[8][NTSS], ss[NSAMPLERS][NSS];
    float transform[512][16];
    uint32_t viewport[6];
    int in_scene, software_vp, cursor_shown;
    uint16_t gamma[3][256];
    void *target;
} device;

static device *dev(uint32_t g) { return halopad_com_state("IDirect3DDevice9", g); }
static uint32_t fbits(float f) { uint32_t u; memcpy(&u, &f, 4); return u; }

/* Direct3D 9 render-state defaults (d3d9types.h / SDK documentation); 0 = not a state. */
static void default_states(device *d)
{
    static const struct { uint16_t s; uint32_t v; } rs[] = {
        {8, 3}, {9, 2}, {14, 1}, {15, 0}, {16, 1}, {19, 2}, {20, 1}, {22, 3}, {23, 4}, {24, 0}, {25, 8}, {26, 0},
        {27, 0}, {28, 0}, {29, 0}, {34, 0}, {35, 0}, {48, 0}, {52, 0}, {53, 1}, {54, 1}, {55, 1}, {56, 8}, {57, 0},
        {58, 0xFFFFFFFF}, {59, 0xFFFFFFFF}, {60, 0xFFFFFFFF}, {128, 0}, {129, 0}, {130, 0}, {131, 0}, {132, 0}, {133, 0},
        {134, 0}, {135, 0}, {136, 1}, {137, 1}, {139, 0}, {140, 0}, {141, 1}, {142, 1}, {143, 0}, {145, 1}, {146, 2},
        {147, 0}, {148, 0}, {151, 0}, {152, 0}, {156, 0}, {157, 0}, {161, 1}, {162, 0xFFFFFFFF}, {163, 0}, {165, 0},
        {167, 0}, {168, 0xF}, {171, 1}, {172, 3}, {173, 1}, {174, 0}, {175, 0}, {176, 0}, {180, 0}, {181, 0}, {182, 0},
        {184, 0}, {185, 0}, {186, 1}, {187, 1}, {188, 1}, {189, 8}, {190, 0xF}, {191, 0xF}, {192, 0xF},
        {193, 0xFFFFFFFF}, {194, 0}, {195, 0}, {198, 0}, {199, 0}, {200, 0}, {201, 0}, {202, 0}, {203, 0}, {204, 0},
        {205, 0}, {206, 0}, {207, 2}, {208, 1}, {209, 1},
    };
    memset(d->rs, 0, sizeof d->rs);
    for (size_t i = 0; i < sizeof rs / sizeof rs[0]; i++) { d->rs[rs[i].s] = rs[i].v; rs_valid[rs[i].s] = 1; }
    d->rs[7] = d->pp[9] ? 1u : 0u;                                   /* ZENABLE: TRUE with an automatic depth buffer */
    rs_valid[7] = 1;
    static const struct { uint16_t s; float v; } fs[] = {
        {36, 0.0f}, {37, 1.0f}, {38, 1.0f}, {154, 1.0f}, {155, 1.0f}, {158, 1.0f}, {159, 0.0f}, {160, 0.0f},
        {166, 64.0f}, {170, 0.0f}, {178, 1.0f}, {179, 1.0f}, {183, 1.0f},
    };
    for (size_t i = 0; i < sizeof fs / sizeof fs[0]; i++) { d->rs[fs[i].s] = fbits(fs[i].v); rs_valid[fs[i].s] = 1; }
    for (int t = 0; t < 8; t++) {
        memset(d->tss[t], 0xFF, sizeof d->tss[t]);
        d->tss[t][1] = t ? 1 : 4;  d->tss[t][2] = 2; d->tss[t][3] = 1;   /* COLOROP, COLORARG1/2 */
        d->tss[t][4] = t ? 1 : 2;  d->tss[t][5] = 2; d->tss[t][6] = 1;   /* ALPHAOP, ALPHAARG1/2 */
        for (int k = 7; k <= 10; k++) d->tss[t][k] = 0;                  /* BUMPENVMAT */
        d->tss[t][11] = (uint32_t)t;                                     /* TEXCOORDINDEX */
        d->tss[t][22] = 0; d->tss[t][23] = 0; d->tss[t][24] = 0;         /* BUMPENVLSCALE/LOFFSET, TEXTURETRANSFORMFLAGS */
        d->tss[t][26] = 1; d->tss[t][27] = 1; d->tss[t][28] = 1;         /* COLORARG0, ALPHAARG0, RESULTARG */
        d->tss[t][32] = 0;                                               /* CONSTANT */
    }
    for (int s = 0; s < NSAMPLERS; s++) {
        uint32_t *v = d->ss[s];
        v[0] = 0xFFFFFFFF;
        v[1] = v[2] = v[3] = 1;                                          /* ADDRESSU/V/W: WRAP */
        v[4] = 0; v[5] = 1; v[6] = 1; v[7] = 0;                          /* BORDERCOLOR, MAG/MIN POINT, MIP NONE */
        v[8] = 0; v[9] = 0; v[10] = 1; v[11] = 0; v[12] = 0; v[13] = 0;  /* LODBIAS, MAXMIPLEVEL, MAXANISOTROPY, SRGB, ELEMENTINDEX, DMAPOFFSET */
    }
    for (int i = 0; i < 512; i++) {
        memset(d->transform[i], 0, sizeof d->transform[i]);
        d->transform[i][0] = d->transform[i][5] = d->transform[i][10] = d->transform[i][15] = 1.0f;
    }
    d->viewport[0] = 0; d->viewport[1] = 0; d->viewport[2] = d->pp[0]; d->viewport[3] = d->pp[1];
    d->viewport[4] = fbits(0.0f); d->viewport[5] = fbits(1.0f);
    for (int c = 0; c < 3; c++) for (int i = 0; i < 256; i++) d->gamma[c][i] = (uint16_t)(i * 257);
    d->cursor_shown = 0;
}

static void destroy(void *p)
{
    device *d = p;
    if (d->target) halopad_metal_target_destroy(d->target);
    free(d);
}

uint32_t halopad_d3d9_create_device(uint32_t d3d, uint32_t adapter, uint32_t type, uint32_t focus, uint32_t behavior,
                                    uint32_t pp, uint32_t out)
{
    if (adapter || type != 1) return D3DERR_INVALIDCALL;
    if (behavior & 0x10) return D3DERR_INVALIDCALL;                  /* PUREDEVICE: caps do not offer it */
    uint32_t known = 0x2 | 0x4 | 0x20 | 0x40 | 0x80;                 /* FPU_PRESERVE, MULTITHREADED, SW/HW/MIXED VP */
    if (behavior & ~known) hp_unsupported("IDirect3D9::CreateDevice", "behavior flags 0x%x", behavior);
    int vp = !!(behavior & 0x20) + !!(behavior & 0x40) + !!(behavior & 0x80);
    if (vp != 1) return D3DERR_INVALIDCALL;
    device *d = calloc(1, sizeof *d);
    for (int i = 0; i < 14; i++) d->pp[i] = rd32(pp + 4 * i);
    uint32_t window = d->pp[7] ? d->pp[7] : focus;
    if (d->pp[4] || d->pp[5]) hp_unsupported("IDirect3D9::CreateDevice", "multisampling (type %u, quality %u)", d->pp[4], d->pp[5]);
    if (d->pp[3] > 1) hp_unsupported("IDirect3D9::CreateDevice", "%u back buffers", d->pp[3]);
    if (d->pp[6] < 1 || d->pp[6] > 3) return D3DERR_INVALIDCALL;
    if (d->pp[11] & ~0x1u) hp_unsupported("IDirect3D9::CreateDevice", "presentation flags 0x%x", d->pp[11]);
    int32_t w = (int32_t)d->pp[0], h = (int32_t)d->pp[1];
    if (d->pp[8] && (!w || !h)) {                                   /* windowed with 0: the window's client area */
        int32_t cw, ch;
        if (!halopad_window_client_size(window, &cw, &ch)) return D3DERR_INVALIDCALL;
        if (!w) w = cw;
        if (!h) h = ch;
    }
    if (w <= 0 || h <= 0) return D3DERR_INVALIDCALL;
    if (d->pp[2] == 0 && d->pp[8]) d->pp[2] = 22;                    /* windowed UNKNOWN: the display format */
    if (d->pp[2] != 21 && d->pp[2] != 22) hp_unsupported("IDirect3D9::CreateDevice", "back buffer format %u", d->pp[2]);
    if (d->pp[9] && d->pp[10] != 75 && d->pp[10] != 77 && d->pp[10] != 80)
        hp_unsupported("IDirect3D9::CreateDevice", "depth format %u", d->pp[10]);
    d->pp[0] = (uint32_t)w; d->pp[1] = (uint32_t)h;
    if (!d->pp[3]) d->pp[3] = 1;
    wr32(pp, (uint32_t)w); wr32(pp + 4, (uint32_t)h); wr32(pp + 8, d->pp[2]); wr32(pp + 12, d->pp[3]);   /* as Direct3D fills them in */
    d->d3d = d3d; d->window = window; d->behavior = behavior; d->software_vp = !!(behavior & 0x20);
    default_states(d);
    d->target = halopad_metal_target_create((uint32_t)w, (uint32_t)h, d->pp[9] != 0, halopad_window_text(window));
    /* Without D3DCREATE_FPU_PRESERVE, Direct3D 9 sets the x87 unit to single precision and
       round-to-nearest for the calling thread. */
    if (!(behavior & 0x2) && halopad_cpu) halopad_cpu->_st_cw &= ~0x0F00u;
    d->guest = halopad_com_new("IDirect3DDevice9", 4, d, destroy);
    halopad_com_addref(d3d);
    wr32(out, d->guest);
    return D3D_OK;
}

uint32_t hpcom_IDirect3DDevice9_AddRef_c(uint32_t g) { dev(g); return halopad_com_addref(g); }
uint32_t hpcom_IDirect3DDevice9_Release_c(uint32_t g)
{
    device *d = dev(g);
    uint32_t d3d = d->d3d, n = halopad_com_release(g);
    if (!n) halopad_com_release(d3d);
    return n;
}
uint32_t hpcom_IDirect3DDevice9_TestCooperativeLevel_c(uint32_t g) { dev(g); return D3D_OK; }
uint32_t hpcom_IDirect3DDevice9_GetAvailableTextureMem_c(uint32_t g) { dev(g); return 128u << 20; }   /* the card class: 128 MB */
uint32_t hpcom_IDirect3DDevice9_GetDirect3D_c(uint32_t g, uint32_t out)
{
    device *d = dev(g);
    halopad_com_addref(d->d3d);
    wr32(out, d->d3d);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetDeviceCaps_c(uint32_t g, uint32_t caps) { dev(g); halopad_d3d9_fill_caps(caps); return D3D_OK; }
uint32_t hpcom_IDirect3DDevice9_GetDisplayMode_c(uint32_t g, uint32_t swap, uint32_t mode)
{
    device *d = dev(g);
    if (swap) return D3DERR_INVALIDCALL;
    if (d->pp[8]) {
        int32_t w, h;
        halopad_desktop_size(&w, &h);
        wr32(mode, (uint32_t)w); wr32(mode + 4, (uint32_t)h); wr32(mode + 8, 60); wr32(mode + 12, 22);
    } else {
        wr32(mode, d->pp[0]); wr32(mode + 4, d->pp[1]); wr32(mode + 8, d->pp[12] ? d->pp[12] : 60); wr32(mode + 12, d->pp[2]);
    }
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetCreationParameters_c(uint32_t g, uint32_t out)
{
    device *d = dev(g);
    wr32(out, 0); wr32(out + 4, 1); wr32(out + 8, d->window); wr32(out + 12, d->behavior);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_ShowCursor_c(uint32_t g, uint32_t show)
{
    device *d = dev(g);
    int old = d->cursor_shown;
    d->cursor_shown = show != 0;
    return (uint32_t)old;
}
uint32_t hpcom_IDirect3DDevice9_SetGammaRamp_c(uint32_t g, uint32_t swap, uint32_t flags, uint32_t ramp)
{
    device *d = dev(g);
    (void)flags;
    if (swap) return D3DERR_INVALIDCALL;
    memcpy(d->gamma, G(ramp), sizeof d->gamma);
    for (int c = 0; c < 3; c++) for (int i = 0; i < 256; i++)
        if (d->gamma[c][i] != (uint16_t)(i * 257))
            hp_unsupported("IDirect3DDevice9::SetGammaRamp", "a non-identity ramp (applying gamma at presentation)");
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetGammaRamp_c(uint32_t g, uint32_t swap, uint32_t ramp)
{
    device *d = dev(g);
    if (swap) return D3DERR_INVALIDCALL;
    memcpy(G(ramp), d->gamma, sizeof d->gamma);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_BeginScene_c(uint32_t g)
{
    device *d = dev(g);
    if (d->in_scene) return D3DERR_INVALIDCALL;
    d->in_scene = 1;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_EndScene_c(uint32_t g)
{
    device *d = dev(g);
    if (!d->in_scene) return D3DERR_INVALIDCALL;
    d->in_scene = 0;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_Clear_c(uint32_t g, uint32_t count, uint32_t rects, uint32_t flags, uint32_t color, uint32_t z,
                                        uint32_t stencil)
{
    device *d = dev(g);
    if (count || rects) hp_unsupported("IDirect3DDevice9::Clear", "%u rectangles", count);
    if (flags & ~0x7u) return D3DERR_INVALIDCALL;
    if ((flags & 0x6) && !d->pp[9]) return D3DERR_INVALIDCALL;       /* no depth/stencil buffer */
    uint32_t v[6];
    memcpy(v, d->viewport, sizeof v);
    if (v[0] || v[1] || v[2] != d->pp[0] || v[3] != d->pp[1])
        hp_unsupported("IDirect3DDevice9::Clear", "a clear limited to viewport %ux%u at %u,%u", v[2], v[3], v[0], v[1]);
    float rgba[4] = {((color >> 16) & 0xFF) / 255.0f, ((color >> 8) & 0xFF) / 255.0f, (color & 0xFF) / 255.0f, (color >> 24) / 255.0f};
    float zf;
    memcpy(&zf, &z, 4);
    halopad_metal_clear(d->target, flags & 1, (flags >> 1) & 1, (flags >> 2) & 1, rgba, zf, stencil);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_Present_c(uint32_t g, uint32_t src, uint32_t dst, uint32_t window, uint32_t dirty)
{
    device *d = dev(g);
    if (src || dst || (window && window != d->window) || dirty)
        hp_unsupported("IDirect3DDevice9::Present", "source/destination rectangles, another window or a dirty region");
    halopad_metal_present(d->target);
    return D3D_OK;
}

uint32_t hpcom_IDirect3DDevice9_SetRenderState_c(uint32_t g, uint32_t s, uint32_t v)
{
    device *d = dev(g);
    if (s >= NRS || !rs_valid[s]) hp_unsupported("IDirect3DDevice9::SetRenderState", "state %u (value 0x%x)", s, v);
    d->rs[s] = v;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetRenderState_c(uint32_t g, uint32_t s, uint32_t out)
{
    device *d = dev(g);
    if (s >= NRS || !rs_valid[s]) return D3DERR_INVALIDCALL;
    wr32(out, d->rs[s]);
    return D3D_OK;
}
static int valid_tss(uint32_t s) { return (s >= 1 && s <= 11) || s == 22 || s == 23 || s == 24 || s == 26 || s == 27 || s == 28 || s == 32; }
uint32_t hpcom_IDirect3DDevice9_SetTextureStageState_c(uint32_t g, uint32_t stage, uint32_t s, uint32_t v)
{
    device *d = dev(g);
    if (stage >= 8 || !valid_tss(s)) hp_unsupported("IDirect3DDevice9::SetTextureStageState", "stage %u state %u (value 0x%x)", stage, s, v);
    d->tss[stage][s] = v;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetTextureStageState_c(uint32_t g, uint32_t stage, uint32_t s, uint32_t out)
{
    device *d = dev(g);
    if (stage >= 8 || !valid_tss(s)) return D3DERR_INVALIDCALL;
    wr32(out, d->tss[stage][s]);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_SetSamplerState_c(uint32_t g, uint32_t sampler, uint32_t s, uint32_t v)
{
    device *d = dev(g);
    if (sampler >= NSAMPLERS || s < 1 || s >= NSS)
        hp_unsupported("IDirect3DDevice9::SetSamplerState", "sampler %u state %u (value 0x%x)", sampler, s, v);
    d->ss[sampler][s] = v;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetSamplerState_c(uint32_t g, uint32_t sampler, uint32_t s, uint32_t out)
{
    device *d = dev(g);
    if (sampler >= NSAMPLERS || s < 1 || s >= NSS) return D3DERR_INVALIDCALL;
    wr32(out, d->ss[sampler][s]);
    return D3D_OK;
}
static int transform_index(uint32_t s) { return (s == 2 || s == 3 || (s >= 16 && s <= 23) || (s >= 256 && s < 512)) ? (int)s : -1; }
uint32_t hpcom_IDirect3DDevice9_SetTransform_c(uint32_t g, uint32_t s, uint32_t m)
{
    device *d = dev(g);
    int i = transform_index(s);
    if (i < 0) hp_unsupported("IDirect3DDevice9::SetTransform", "transform state %u", s);
    memcpy(d->transform[i], G(m), 64);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetTransform_c(uint32_t g, uint32_t s, uint32_t m)
{
    device *d = dev(g);
    int i = transform_index(s);
    if (i < 0) return D3DERR_INVALIDCALL;
    memcpy(G(m), d->transform[i], 64);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_SetViewport_c(uint32_t g, uint32_t vp)
{
    device *d = dev(g);
    uint32_t v[6];
    memcpy(v, G(vp), sizeof v);
    if (v[0] + v[2] > d->pp[0] || v[1] + v[3] > d->pp[1] || !v[2] || !v[3]) return D3DERR_INVALIDCALL;
    memcpy(d->viewport, v, sizeof v);
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetViewport_c(uint32_t g, uint32_t vp) { device *d = dev(g); memcpy(G(vp), d->viewport, 24); return D3D_OK; }
uint32_t hpcom_IDirect3DDevice9_SetSoftwareVertexProcessing_c(uint32_t g, uint32_t sw)
{
    device *d = dev(g);
    if (!(d->behavior & 0x80)) return D3DERR_INVALIDCALL;            /* only mixed-mode devices switch */
    d->software_vp = sw != 0;
    return D3D_OK;
}
uint32_t hpcom_IDirect3DDevice9_GetSoftwareVertexProcessing_c(uint32_t g) { return (uint32_t)dev(g)->software_vp; }

/* Test support: the device's Metal target. */
void *halopad_d3d9_device_target(uint32_t g) { return dev(g)->target; }

/* HaloPad DirectDraw 7 (G3): what Halo asks DirectDraw before Direct3D starts.
 *
 * Halo's machine check (0x580e70, after the CPU and memory measurement) enumerates the display
 * devices with DirectDrawEnumerateExA(DDENUM_ATTACHEDSECONDARYDEVICES), creates an
 * IDirectDraw7 for each with DirectDrawCreateEx, sets DDSCL_NORMAL and asks GetAvailableVidMem
 * for four kinds of surface (primary, 3D textures, textures, off-screen plain in local video
 * memory); the smallest total, rounded, is the card's memory, which the hardware check
 * (0x580a00) and its machine info use. Any failure is a fatal error in Halo (string 0x79).
 *
 * HaloPad answers as the reference machine's single-monitor Windows XP does: one device, the
 * "Primary Display Driver" with a NULL GUID, whose video memory is the Radeon 9700 PRO's 128 MB
 * (the same card class IDirect3DDevice9::GetAvailableTextureMem reports). DirectDraw drawing
 * (surfaces, clippers, palettes, display modes) is not provided; those methods stop with their
 * names. */
#include "halopad_win32.h"

uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);

#define DD_OK 0u
#define DDERR_INVALIDPARAMS 0x80070057u
#define DDERR_INVALIDDIRECTDRAWGUID 0x88760231u
#define CLASS_E_NOAGGREGATION 0x80040110u
#define E_NOINTERFACE 0x80004002u
#define VIDEO_MEMORY (128u << 20)

/* IID_IDirectDraw7 {15e65ec0-3b9c-11d2-b92f-00609797ea5b} and IUnknown */
static const uint8_t iid_dd7[16] = {0xC0, 0x5E, 0xE6, 0x15, 0x9C, 0x3B, 0xD2, 0x11, 0xB9, 0x2F, 0x00, 0x60, 0x97, 0x97, 0xEA, 0x5B};
static const uint8_t iid_unknown[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46};

typedef struct { uint32_t coop; } ddraw;
static ddraw *DD(uint32_t g) { return halopad_com_state("IDirectDraw7", g); }

static uint32_t guest_string(const char *s)
{
    uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0);
    memcpy(G(g), s, strlen(s) + 1);
    return g;
}

uint32_t DirectDrawEnumerateExA_c(uint32_t callback, uint32_t context, uint32_t flags)
{
    if (!callback) return DDERR_INVALIDPARAMS;
    if (flags & ~0x7u) return DDERR_INVALIDPARAMS;                  /* ATTACHED/DETACHEDSECONDARYDEVICES, NONDISPLAYDEVICES */
    uint32_t desc = guest_string("Primary Display Driver"), name = guest_string("display");
    uint32_t args[5] = {0 /* the primary: NULL GUID */, desc, name, context, 0 /* hMonitor */};
    halopad_call_guest(callback, 5, args);                          /* its answer only decides whether to go on */
    halopad_heap_free(desc);
    halopad_heap_free(name);
    return DD_OK;
}

uint32_t DirectDrawCreateEx_c(uint32_t guid, uint32_t out, uint32_t iid, uint32_t outer)
{
    if (!out) return DDERR_INVALIDPARAMS;
    wr32(out, 0);
    if (outer) return CLASS_E_NOAGGREGATION;
    if (!iid || memcmp(G(iid), iid_dd7, 16)) return DDERR_INVALIDPARAMS;   /* DirectDrawCreateEx makes only IDirectDraw7 */
    if (guid > 2) {                                                  /* DDCREATE_EMULATIONONLY (1), HARDWAREONLY (2), or a GUID */
        static const uint8_t zero[16];
        if (memcmp(G(guid), zero, 16)) return DDERR_INVALIDDIRECTDRAWGUID;   /* no device but the primary */
    }
    ddraw *d = calloc(1, sizeof *d);
    wr32(out, halopad_com_new("IDirectDraw7", 4, d, free));
    return DD_OK;
}

uint32_t hpcom_IDirectDraw7_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    DD(g);
    if (!out) return DDERR_INVALIDPARAMS;
    if (!memcmp(G(iid), iid_dd7, 16) || !memcmp(G(iid), iid_unknown, 16)) { halopad_com_addref(g); wr32(out, g); return DD_OK; }
    wr32(out, 0);
    hp_unsupported("IDirectDraw7::QueryInterface", "interface %08x-... (older DirectDraw interfaces)", rd32(iid));
}
uint32_t hpcom_IDirectDraw7_AddRef_c(uint32_t g) { DD(g); return halopad_com_addref(g); }
uint32_t hpcom_IDirectDraw7_Release_c(uint32_t g) { DD(g); return halopad_com_release(g); }

uint32_t hpcom_IDirectDraw7_SetCooperativeLevel_c(uint32_t g, uint32_t hwnd, uint32_t flags)
{
    ddraw *d = DD(g);
    (void)hwnd;
    if (flags != 0x8u) hp_unsupported("IDirectDraw7::SetCooperativeLevel", "flags 0x%x (DDSCL_NORMAL only)", flags);
    d->coop = flags;
    return DD_OK;
}

/* DDSCAPS2 (16 bytes); video memory kinds answer with the card's memory, system memory with none */
uint32_t hpcom_IDirectDraw7_GetAvailableVidMem_c(uint32_t g, uint32_t caps, uint32_t total, uint32_t free_out)
{
    DD(g);
    if (!caps) return DDERR_INVALIDPARAMS;
    uint32_t c = rd32(caps);
    if (c & 0x800u) hp_unsupported("IDirectDraw7::GetAvailableVidMem", "system memory (caps 0x%x)", c);   /* DDSCAPS_SYSTEMMEMORY */
    if (c & 0x08000000u) hp_unsupported("IDirectDraw7::GetAvailableVidMem", "non-local (AGP) video memory (caps 0x%x)", c);
    if (total) wr32(total, VIDEO_MEMORY);
    if (free_out) wr32(free_out, VIDEO_MEMORY);
    return DD_OK;
}

/* HaloPad OLE and DxDiag (G3/G4): ole32/oleaut32 as Halo uses them, and a DxDiag provider.
 *
 * How Halo uses them (haloce.exe, 0x580e70): CoInitialize; GetDeviceID(DSDEVID_DefaultPlayback)
 * from dsound; CoCreateInstance(CLSID_DxDiagProvider, IID_IDxDiagProvider); Initialize;
 * GetRootContainer; GetChildContainer("DxDiag_DirectSound.DxDiag_SoundDevices"); for each
 * device, szDescription, szGuidDeviceID, szDriverVersion and szHardwareID as BSTR variants
 * (VariantInit/VariantClear), matched against the default device's GUID; CoUninitialize.
 *
 * The DxDiag tree describes what HaloPad provides: one sound device, the Core Audio
 * output DirectSound plays through, whose GUID is also the one GetDeviceID reports.
 * Containers and properties outside that description, and other COM classes, stop with
 * their names. */
#include "halopad_win32.h"

uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t LocalAlloc_c(uint32_t flags, uint32_t size);
uint32_t LocalFree_c(uint32_t a);

#define S_OK 0u
#define S_FALSE 1u
#define E_INVALIDARG 0x80070057u
#define E_POINTER 0x80004003u
#define E_NOINTERFACE 0x80004002u
#define CO_E_NOTINITIALIZED 0x800401F0u
#define CLASS_E_NOAGGREGATION 0x80040110u

/* The HaloPad audio output's device GUID (dsound GetDeviceID, DxDiag szGuidDeviceID). */
const uint8_t halopad_audio_guid[16] = {0xA2, 0xC3, 0xF4, 0xB6, 0x1D, 0x9E, 0x57, 0x4F, 0x8A, 0x61, 0x48, 0x41, 0x4C, 0x4F, 0x50, 0x41};

static void guid_text(const uint8_t *g, char out[39])
{
    uint32_t d1; uint16_t d2, d3;
    memcpy(&d1, g, 4); memcpy(&d2, g + 4, 2); memcpy(&d3, g + 6, 2);
    snprintf(out, 39, "{%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X}", d1, d2, d3, g[8], g[9], g[10], g[11], g[12], g[13], g[14], g[15]);
}

/* ---- ole32 ---- */

static _Thread_local int com_inits;

uint32_t CoInitialize_c(uint32_t reserved)
{
    if (reserved) return E_INVALIDARG;
    return com_inits++ ? S_FALSE : S_OK;
}
uint32_t CoUninitialize_c(void) { if (com_inits) com_inits--; return 0; }

uint32_t StringFromGUID2_c(uint32_t guid, uint32_t out, uint32_t cch)
{
    if (cch < 39) return 0;
    char t[39];
    guid_text(G(guid), t);
    for (int i = 0; i < 39; i++) wr16(out + 2 * (uint32_t)i, (uint8_t)t[i]);
    return 39;
}

static int hexval(uint16_t c) { return c >= '0' && c <= '9' ? c - '0' : (c | 0x20) >= 'a' && (c | 0x20) <= 'f' ? (c | 0x20) - 'a' + 10 : -1; }
uint32_t CLSIDFromString_c(uint32_t s, uint32_t out)
{
    static const int pos[16] = {1, 3, 5, 7, 10, 12, 15, 17, 20, 22, 25, 27, 29, 31, 33, 35};   /* hex pairs, in text order */
    uint16_t w[39];
    for (int i = 0; i < 39; i++) w[i] = ((uint16_t *)G(s))[i];
    if (w[0] != '{' || w[9] != '-' || w[14] != '-' || w[19] != '-' || w[24] != '-' || w[37] != '}' || w[38])
        hp_unsupported("CLSIDFromString", "a ProgID or malformed CLSID");
    uint8_t b[16];
    for (int i = 0; i < 16; i++) {
        int h = hexval(w[pos[i]]), l = hexval(w[pos[i] + 1]);
        if (h < 0 || l < 0) return 0x800401F3u;                     /* CO_E_CLASSSTRING */
        b[i] = (uint8_t)(h << 4 | l);
    }
    uint8_t g[16] = {b[3], b[2], b[1], b[0], b[5], b[4], b[7], b[6], b[8], b[9], b[10], b[11], b[12], b[13], b[14], b[15]};
    memcpy(G(out), g, 16);
    return S_OK;
}

static int guid_is(uint32_t g, const char *text)
{
    char t[39];
    guid_text(G(g), t);
    return !strcmp(t, text);
}

/* ---- oleaut32: VARIANTs with BSTRs ---- */

static uint32_t bstr_of(const char *s)
{
    uint32_t n = (uint32_t)strlen(s);
    uint32_t m = LocalAlloc_c(0, 4 + 2 * n + 2);
    wr32(m, 2 * n);
    for (uint32_t i = 0; i <= n; i++) wr16(m + 4 + 2 * i, (uint8_t)s[i]);
    return m + 4;
}

uint32_t VariantInit_c(uint32_t v) { memset(G(v), 0, 16); return 0; }   /* VT_EMPTY */

uint32_t VariantClear_c(uint32_t v)
{
    if (!v) return E_INVALIDARG;
    uint16_t vt;
    memcpy(&vt, G(v), 2);
    switch (vt) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 11: case 18: case 19: break;   /* EMPTY, NULL, numbers, BOOL */
    case 8: { uint32_t b = rd32(v + 8); if (b) LocalFree_c(b - 4); break; }             /* VT_BSTR */
    default: hp_unsupported("VariantClear", "variant type %u", vt);
    }
    memset(G(v), 0, 16);
    return S_OK;
}

/* ---- DxDiag ---- */

typedef struct node { const char *name; const struct node *children; int nchildren; const char *const (*props)[2]; int nprops; } node;

static char dev_guid[39];
static const char *const sound_props[][2] = {
    {"szDescription", "HaloPad Audio (Core Audio)"}, {"szGuidDeviceID", dev_guid}, {"szDriverName", "halopad-coreaudio"},
    {"szDriverVersion", "1.0.0.0"}, {"szHardwareID", "HALOPAD\\COREAUDIO"}, {"szManufacturerID", "0"}, {"szProductID", "0"},
};
static const node sound_device[] = {{"0", NULL, 0, sound_props, (int)(sizeof sound_props / sizeof sound_props[0])}};
static const node dsound_children[] = {{"DxDiag_SoundDevices", sound_device, 1, NULL, 0}, {"DxDiag_SoundCaptureDevices", NULL, 0, NULL, 0}};
static const node root_children[] = {{"DxDiag_DirectSound", dsound_children, 2, NULL, 0}};
static const node root = {"", root_children, 1, NULL, 0};

typedef struct { const node *n; char path[128]; } container;
typedef struct { int initialized; } provider;

static void wtext(uint32_t w, char *out, size_t size)
{
    size_t i = 0;
    for (; i + 1 < size; i++) { uint16_t c = ((uint16_t *)G(w))[i]; if (!c) break; if (c > 0x7F) hp_unsupported("DxDiag", "a non-ASCII name"); out[i] = (char)c; }
    out[i] = 0;
}

uint32_t halopad_ole_create(uint32_t clsid, uint32_t iid, uint32_t out);

uint32_t CoCreateInstance_c(uint32_t clsid, uint32_t outer, uint32_t ctx, uint32_t iid, uint32_t out)
{
    if (!out) return E_POINTER;
    wr32(out, 0);
    if (!com_inits) return CO_E_NOTINITIALIZED;
    if (outer) return CLASS_E_NOAGGREGATION;
    if (!(ctx & 0x1)) hp_unsupported("CoCreateInstance", "class context 0x%x", ctx);   /* CLSCTX_INPROC_SERVER */
    if (guid_is(clsid, "{A65B8071-3BFE-4213-9A5B-491DA4461CA7}")) {                    /* CLSID_DxDiagProvider */
        if (!guid_is(iid, "{9C6B4CB0-23F8-49CC-A3ED-45A55000A6D2}")) return E_NOINTERFACE;
        provider *p = calloc(1, sizeof *p);
        wr32(out, halopad_com_new("IDxDiagProvider", 4, p, free));
        return S_OK;
    }
    char t[39];
    guid_text(G(clsid), t);
    hp_unsupported("CoCreateInstance", "class %s", t);
}

uint32_t hpcom_IDxDiagProvider_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    halopad_com_state("IDxDiagProvider", g);
    if (guid_is(iid, "{9C6B4CB0-23F8-49CC-A3ED-45A55000A6D2}") || guid_is(iid, "{00000000-0000-0000-C000-000000000046}")) {
        halopad_com_addref(g); wr32(out, g); return S_OK;
    }
    wr32(out, 0);
    return E_NOINTERFACE;
}
uint32_t hpcom_IDxDiagProvider_AddRef_c(uint32_t g) { halopad_com_state("IDxDiagProvider", g); return halopad_com_addref(g); }
uint32_t hpcom_IDxDiagProvider_Release_c(uint32_t g) { halopad_com_state("IDxDiagProvider", g); return halopad_com_release(g); }
uint32_t hpcom_IDxDiagProvider_Initialize_c(uint32_t g, uint32_t params)
{
    provider *p = halopad_com_state("IDxDiagProvider", g);
    if (!params || rd32(params) != 16) return E_INVALIDARG;
    if (rd32(params + 4) != 111) return 0x80070057u;                /* DXDIAG_DX9_SDK_VERSION */
    p->initialized = 1;
    guid_text(halopad_audio_guid, dev_guid);
    return S_OK;
}

static uint32_t new_container(const node *n, const char *path)
{
    container *c = calloc(1, sizeof *c);
    c->n = n;
    snprintf(c->path, sizeof c->path, "%s", path);
    return halopad_com_new("IDxDiagContainer", 4, c, free);
}

uint32_t hpcom_IDxDiagProvider_GetRootContainer_c(uint32_t g, uint32_t out)
{
    provider *p = halopad_com_state("IDxDiagProvider", g);
    if (!out) return E_POINTER;
    if (!p->initialized) return 0x80004005u;                        /* E_FAIL: not initialized */
    wr32(out, new_container(&root, ""));
    return S_OK;
}

static container *C(uint32_t g) { return halopad_com_state("IDxDiagContainer", g); }
uint32_t hpcom_IDxDiagContainer_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    C(g);
    if (guid_is(iid, "{7D0F462F-4064-4862-BC7F-933E5058C10F}") || guid_is(iid, "{00000000-0000-0000-C000-000000000046}")) {
        halopad_com_addref(g); wr32(out, g); return S_OK;
    }
    wr32(out, 0);
    return E_NOINTERFACE;
}
uint32_t hpcom_IDxDiagContainer_AddRef_c(uint32_t g) { C(g); return halopad_com_addref(g); }
uint32_t hpcom_IDxDiagContainer_Release_c(uint32_t g) { C(g); return halopad_com_release(g); }

uint32_t hpcom_IDxDiagContainer_GetNumberOfChildContainers_c(uint32_t g, uint32_t out)
{
    container *c = C(g);
    if (!out) return E_INVALIDARG;
    if (!c->n->children && c->path[0] == 0) hp_unsupported("DxDiag", "the root's containers");
    wr32(out, (uint32_t)c->n->nchildren);
    return S_OK;
}

static uint32_t put_wname(const char *s, uint32_t out, uint32_t cch)
{
    uint32_t n = (uint32_t)strlen(s);
    if (!out) return E_INVALIDARG;
    if (cch < n + 1) return 0x8007007Au;                            /* DXDIAG_E_INSUFFICIENT_BUFFER */
    for (uint32_t i = 0; i <= n; i++) wr16(out + 2 * i, (uint8_t)s[i]);
    return S_OK;
}

uint32_t hpcom_IDxDiagContainer_EnumChildContainerNames_c(uint32_t g, uint32_t i, uint32_t out, uint32_t cch)
{
    container *c = C(g);
    if (c->path[0] == 0) hp_unsupported("DxDiag", "enumerating the root's containers");
    if (i >= (uint32_t)c->n->nchildren) return E_INVALIDARG;
    return put_wname(c->n->children[i].name, out, cch);
}

uint32_t hpcom_IDxDiagContainer_GetChildContainer_c(uint32_t g, uint32_t wname, uint32_t out)
{
    container *c = C(g);
    if (!wname || !out) return E_INVALIDARG;
    char name[128], path[160];
    wtext(wname, name, sizeof name);
    const node *n = c->n;
    char *save = NULL;
    snprintf(path, sizeof path, "%s", c->path);
    for (char *part = strtok_r(name, ".", &save); part; part = strtok_r(NULL, ".", &save)) {
        const node *next = NULL;
        for (int k = 0; k < n->nchildren; k++) if (!strcmp(n->children[k].name, part)) next = &n->children[k];
        size_t l = strlen(path);
        snprintf(path + l, sizeof path - l, "%s%s", l ? "." : "", part);
        if (!next) {
            if (n == &root) hp_unsupported("DxDiag", "container \"%s\"", path);
            return E_INVALIDARG;                                    /* e.g. a device index past the last */
        }
        n = next;
    }
    wr32(out, new_container(n, path));
    return S_OK;
}

uint32_t hpcom_IDxDiagContainer_GetNumberOfProps_c(uint32_t g, uint32_t out)
{
    container *c = C(g);
    if (!out) return E_INVALIDARG;
    wr32(out, (uint32_t)c->n->nprops);
    return S_OK;
}
uint32_t hpcom_IDxDiagContainer_EnumPropNames_c(uint32_t g, uint32_t i, uint32_t out, uint32_t cch)
{
    container *c = C(g);
    if (i >= (uint32_t)c->n->nprops) return E_INVALIDARG;
    return put_wname(c->n->props[i][0], out, cch);
}
uint32_t hpcom_IDxDiagContainer_GetProp_c(uint32_t g, uint32_t wname, uint32_t v)
{
    container *c = C(g);
    if (!wname || !v) return E_INVALIDARG;
    char name[64];
    wtext(wname, name, sizeof name);
    for (int i = 0; i < c->n->nprops; i++)
        if (!strcmp(c->n->props[i][0], name)) {
            VariantClear_c(v);
            uint16_t vt = 8;
            memcpy(G(v), &vt, 2);
            wr32(v + 8, bstr_of(c->n->props[i][1]));
            return S_OK;
        }
    hp_unsupported("DxDiag", "property \"%s\" of \"%s\"", name, c->path);
}

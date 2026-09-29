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
#include <strings.h>

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
#define REGDB_E_CLASSNOTREG 0x80040154u

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
int halopad_guid_is(uint32_t g, const char *text) { return guid_is(g, text); }
void halopad_guid_text(uint32_t g, char out[39]) { guid_text(G(g), out); }
int halopad_urlmon_create(uint32_t clsid, uint32_t iid, uint32_t out, uint32_t *hr);   /* halopad_urlmon.c */

/* ---- oleaut32: VARIANTs with BSTRs ---- */

static uint16_t rd16g(uint32_t a) { uint16_t v; memcpy(&v, G(a), 2); return v; }
static uint32_t bstr_of(const char *s)
{
    uint32_t n = (uint32_t)strlen(s);
    uint32_t m = LocalAlloc_c(0, 4 + 2 * n + 2);
    wr32(m, 2 * n);
    for (uint32_t i = 0; i <= n; i++) wr16(m + 4 + 2 * i, (uint8_t)s[i]);
    return m + 4;
}

/* ---- BSTRs: a 4-byte byte-length prefix, the characters, a terminating L'\0'; the BSTR
 * points past the prefix. LocalAlloc'd (VariantClear frees VT_BSTR the same way). ---- */
static uint32_t bstr_new(uint32_t bytes)
{
    uint32_t m = LocalAlloc_c(0, 4 + bytes + 2);
    if (!m) return 0;
    wr32(m, bytes);
    wr16(m + 4 + bytes, 0);
    if (bytes & 1) wr16(m + 4 + bytes - 1 + 2, 0);                  /* an odd byte count: still two zero bytes after */
    return m + 4;
}
static uint32_t wlen(uint32_t s) { uint32_t n = 0; while (rd16g(s + 2 * n)) n++; return n; }
uint32_t SysAllocStringLen_c(uint32_t src, uint32_t len)
{
    uint32_t b = bstr_new(2 * len);
    if (b && src) memmove(G(b), G(src), 2 * len);                  /* NULL source: the text is left as it is */
    static int trace = -1;                                          /* HALOPAD_TRACE_BSTR=1: diagnostics only */
    if (trace < 0) trace = getenv("HALOPAD_TRACE_BSTR") != NULL;
    if (trace && b && src && len > 1) {
        fprintf(stderr, "HALOPAD BSTR: \"");
        for (uint32_t i = 0; i < len && i < 400; i++) { uint16_t c = rd16g(b + 2 * i); fputc(c >= 0x20 && c < 0x7F ? (int)c : '.', stderr); }
        fprintf(stderr, "%s\"\n", len > 400 ? "..." : "");
    }
    return b;
}
uint32_t SysAllocString_c(uint32_t src) { return src ? SysAllocStringLen_c(src, wlen(src)) : 0; }
uint32_t SysAllocStringByteLen_c(uint32_t src, uint32_t len)
{
    uint32_t b = bstr_new(len);
    if (b && src) memmove(G(b), G(src), len);
    return b;
}
uint32_t SysFreeString_c(uint32_t b) { if (b) LocalFree_c(b - 4); return 0; }
uint32_t SysStringLen_c(uint32_t b) { return b ? rd32(b - 4) / 2 : 0; }
uint32_t SysStringByteLen_c(uint32_t b) { return b ? rd32(b - 4) : 0; }

uint32_t VariantInit_c(uint32_t v) { memset(G(v), 0, 16); return 0; }   /* VT_EMPTY */

/* VariantChangeType(Ex): the conversions reached are implemented below; any other pair
 * stops with its types. */
uint32_t VariantClear_c(uint32_t v);
static int plain_type(uint16_t vt) { return vt <= 5 || vt == 11 || vt == 18 || vt == 19 || vt == 8; }   /* EMPTY..R8, BOOL, UI2, UI4, BSTR */
uint32_t VariantChangeTypeEx_c(uint32_t dst, uint32_t src, uint32_t lcid, uint32_t flags, uint32_t vt)
{
    uint16_t from;
    memcpy(&from, G(src), 2);
    if (from == vt && plain_type(from)) {                           /* the same type: VariantCopy */
        if (dst == src) return S_OK;
        uint8_t copy[16];
        memcpy(copy, G(src), 16);
        VariantClear_c(dst);
        memcpy(G(dst), copy, 16);
        if (vt == 8) {                                              /* VT_BSTR: a byte-exact duplicate */
            uint32_t b;
            memcpy(&b, copy + 8, 4);
            uint32_t nb = b ? SysAllocStringByteLen_c(b, SysStringByteLen_c(b)) : 0;
            if (b && !nb) { memset(G(dst), 0, 16); return 0x8007000Eu; }   /* E_OUTOFMEMORY */
            wr32(dst + 8, nb);
        }
        return S_OK;
    }
    hp_unsupported("VariantChangeTypeEx", "variant type %u to %u (flags 0x%x, locale 0x%x)", from, vt, flags, lcid);
}

uint32_t VariantClear_c(uint32_t v)
{
    uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
    if (!v) return E_INVALIDARG;
    uint16_t vt;
    memcpy(&vt, G(v), 2);
    switch (vt) {
    case 0: case 1: case 2: case 3: case 4: case 5: case 11: case 18: case 19: break;   /* EMPTY, NULL, numbers, BOOL */
    case 8: { uint32_t b = rd32(v + 8); if (b) LocalFree_c(b - 4); break; }             /* VT_BSTR */
    case 9: case 13: {                                                                  /* VT_DISPATCH, VT_UNKNOWN: Release */
        uint32_t p = rd32(v + 8);
        if (p) { uint32_t a = p; halopad_call_guest(rd32(rd32(p) + 8), 1, &a); }
        break;
    }
    default:
        if ((vt & 0x4000) && (vt & 0xFFF) <= 19 && !(vt & 0x2000)) break;             /* VT_BYREF: nothing is owned */
        hp_unsupported("VariantClear", "variant type %u", vt);
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

/* In-process servers of the reference machine (config/runtime/com-servers.txt, compiled into
 * dispatch.ll by scripts/va-model.py): the server is a translated DLL. As ole32 does, it is
 * loaded (and stays loaded), DllGetClassObject gives the class factory for IID_IClassFactory,
 * IClassFactory::CreateInstance(outer, iid) the object, and the factory is released. */
extern const uint32_t halopad_com_server_count;
extern const char *const halopad_com_server_clsids[], *const halopad_com_server_dlls[];
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void halopad_heap_free(uint32_t p);

static uint32_t guest_string(const char *s)
{
    uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0);
    memcpy(G(g), s, strlen(s) + 1);
    return g;
}

static int inproc_server(uint32_t clsid, uint32_t outer, uint32_t iid, uint32_t out, uint32_t *hr)
{
    char t[39];
    guid_text(G(clsid), t);
    for (uint32_t i = 0; i < halopad_com_server_count; i++) {
        if (strcasecmp(halopad_com_server_clsids[i], t)) continue;
        uint32_t name = guest_string(halopad_com_server_dlls[i]);
        uint32_t module = LoadLibraryA_c(name);
        halopad_heap_free(name);
        if (!module) { *hr = 0x8007007Eu; return 1; }                /* HRESULT_FROM_WIN32(ERROR_MOD_NOT_FOUND) */
        uint32_t fn = guest_string("DllGetClassObject");
        uint32_t dgco = GetProcAddress_c(module, fn);
        halopad_heap_free(fn);
        if (!dgco) { *hr = 0x8004014Fu; return 1; }                 /* CO_E_ERRORINDLL */
        static const uint8_t iid_factory[16] = {1, 0, 0, 0, 0, 0, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46};
        uint32_t giid = halopad_heap_alloc(16, 0), pfactory = halopad_heap_alloc(4, 1);
        memcpy(G(giid), iid_factory, 16);
        uint32_t a[3] = {clsid, giid, pfactory};
        *hr = halopad_call_guest(dgco, 3, a);
        uint32_t factory = rd32(pfactory);
        halopad_heap_free(giid);
        halopad_heap_free(pfactory);
        if ((int32_t)*hr < 0) return 1;
        uint32_t c[4] = {factory, outer, iid, out};
        *hr = halopad_call_guest(rd32(rd32(factory) + 12), 4, c);   /* IClassFactory::CreateInstance */
        halopad_call_guest(rd32(rd32(factory) + 8), 1, &factory);  /* Release */
        return 1;
    }
    return 0;
}

/* OleRun, as ole32 does it: an object that has IRunnableObject is Run(NULL); any other is
 * already running (S_OK). Keystone.dll calls it on its MSXML document. */
uint32_t OleRun_c(uint32_t unk)
{
    static const uint8_t iid_runnable[16] = {0x26, 1, 0, 0, 0, 0, 0, 0, 0xC0, 0, 0, 0, 0, 0, 0, 0x46};
    uint32_t giid = halopad_heap_alloc(16, 0), pout = halopad_heap_alloc(4, 1);
    memcpy(G(giid), iid_runnable, 16);
    uint32_t q[3] = {unk, giid, pout};
    uint32_t hr = halopad_call_guest(rd32(rd32(unk)), 3, q);        /* QueryInterface */
    uint32_t runnable = rd32(pout);
    halopad_heap_free(giid);
    halopad_heap_free(pout);
    if ((int32_t)hr < 0 || !runnable) return S_OK;
    uint32_t run[2] = {runnable, 0};
    hr = halopad_call_guest(rd32(rd32(runnable) + 16), 2, run);     /* IRunnableObject::Run(NULL) */
    halopad_call_guest(rd32(rd32(runnable) + 8), 1, &runnable);    /* Release */
    return hr;
}

uint32_t CoCreateInstance_c(uint32_t clsid, uint32_t outer, uint32_t ctx, uint32_t iid, uint32_t out)
{
    if (!out) return E_POINTER;
    wr32(out, 0);
    if (!com_inits) return CO_E_NOTINITIALIZED;
    if (!(ctx & 0x1)) hp_unsupported("CoCreateInstance", "class context 0x%x", ctx);   /* CLSCTX_INPROC_SERVER */
    uint32_t hr;
    if (inproc_server(clsid, outer, iid, out, &hr)) return hr;
    if (outer) return CLASS_E_NOAGGREGATION;
    /* Halo asks for Windows' Text Services Framework language bar while resetting
       video mode. iOS has no registered TSF server; let Halo handle the normal
       COM failure and keep the output pointer NULL. */
    if (guid_is(clsid, "{EBB08C45-6C4A-4FDC-AE53-4EB8C4C7DB8E}")) return REGDB_E_CLASSNOTREG;
    if (halopad_urlmon_create(clsid, iid, out, &hr)) return hr;
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

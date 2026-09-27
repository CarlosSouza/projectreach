/* HaloPad urlmon (G3): the Internet security manager msxml4.dll creates for every document it
 * loads (CoCreateInstance(CLSID_InternetSecurityManager)). Only local files are handled: on
 * Windows XP they are in the Local Machine zone (URLZONE_LOCAL_MACHINE). Other URL schemes, zone
 * mappings and custom policies stop until something reaches them. */
#include "halopad_win32.h"

#define S_OK 0u
#define E_NOINTERFACE 0x80004002u
#define E_INVALIDARG 0x80070057u
#define URLZONE_LOCAL_MACHINE 0u

int halopad_guid_is(uint32_t g, const char *text);
void halopad_guid_text(uint32_t g, char out[39]);
uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t halopad_com_addref(uint32_t g);
uint32_t halopad_com_release(uint32_t g);

#define IID_IUnknown "{00000000-0000-0000-C000-000000000046}"
#define IID_IInternetSecurityManager "{79EAC9EE-BAF9-11CE-8C82-00AA004BA90B}"

typedef struct { uint32_t site; } secmgr;

static void destroy(void *p)
{
    secmgr *s = p;
    (void)s;                                    /* the site is not reference counted here (see SetSecuritySite) */
    free(p);
}

int halopad_urlmon_create(uint32_t clsid, uint32_t iid, uint32_t out, uint32_t *hr)
{
    if (!halopad_guid_is(clsid, "{7B8A2D94-0AC9-11D1-896C-00C04FB6BFC4}")) return 0;   /* CLSID_InternetSecurityManager */
    if (!halopad_guid_is(iid, IID_IInternetSecurityManager) && !halopad_guid_is(iid, IID_IUnknown)) {
        char t[39];
        halopad_guid_text(iid, t);
        hp_unsupported("CoCreateInstance(CLSID_InternetSecurityManager)", "interface %s", t);
    }
    wr32(out, halopad_com_new("IInternetSecurityManager", 4, calloc(1, sizeof(secmgr)), destroy));
    *hr = S_OK;
    return 1;
}

static uint16_t wat(uint32_t s, uint32_t i) { uint16_t c; memcpy(&c, G(s + 2 * i), 2); return c; }
static int prefix_ci(uint32_t s, const char *p)
{
    for (uint32_t i = 0; p[i]; i++) {
        uint16_t c = wat(s, i);
        if (c >= 'A' && c <= 'Z') c += 32;
        if (c != (uint8_t)p[i]) return 0;
    }
    return 1;
}
static void url_text(uint32_t url, char *out, size_t n)
{
    size_t i = 0;
    for (; i + 1 < n && wat(url, (uint32_t)i); i++) { uint16_t c = wat(url, (uint32_t)i); out[i] = c < 0x80 ? (char)c : '?'; }
    out[i] = 0;
}

/* A local file: a "file:" URL naming a drive path, or a drive path itself */
static int local_file(uint32_t url)
{
    uint32_t i = 0;
    if (prefix_ci(url, "file:")) { i = 5; while (wat(url, i) == '/' || wat(url, i) == '\\') i++; }
    uint16_t c = wat(url, i);
    return c < 0x80 && ((c | 0x20) >= 'a' && (c | 0x20) <= 'z') && (wat(url, i + 1) == ':' || wat(url, i + 1) == '|');
}

uint32_t hpcom_IInternetSecurityManager_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out)
{
    halopad_com_state("IInternetSecurityManager", g);
    if (halopad_guid_is(iid, IID_IInternetSecurityManager) || halopad_guid_is(iid, IID_IUnknown)) {
        halopad_com_addref(g);
        wr32(out, g);
        return S_OK;
    }
    wr32(out, 0);
    return E_NOINTERFACE;
}
uint32_t hpcom_IInternetSecurityManager_AddRef_c(uint32_t g) { halopad_com_state("IInternetSecurityManager", g); return halopad_com_addref(g); }
uint32_t hpcom_IInternetSecurityManager_Release_c(uint32_t g) { halopad_com_state("IInternetSecurityManager", g); return halopad_com_release(g); }

/* The site supplies an IInternetSecurityMgrSite (a window for prompts). The Local Machine zone
   never prompts for what is handled here, so only a NULL site is accepted for now. */
uint32_t hpcom_IInternetSecurityManager_SetSecuritySite_c(uint32_t g, uint32_t site)
{
    secmgr *s = halopad_com_state("IInternetSecurityManager", g);
    if (site) hp_unsupported("IInternetSecurityManager::SetSecuritySite", "a site object 0x%08x", site);
    s->site = 0;
    return S_OK;
}
uint32_t hpcom_IInternetSecurityManager_GetSecuritySite_c(uint32_t g, uint32_t out)
{
    secmgr *s = halopad_com_state("IInternetSecurityManager", g);
    if (!out) return E_INVALIDARG;
    wr32(out, s->site);
    return S_OK;
}

uint32_t hpcom_IInternetSecurityManager_MapUrlToZone_c(uint32_t g, uint32_t url, uint32_t zone, uint32_t flags)
{
    halopad_com_state("IInternetSecurityManager", g);
    (void)flags;
    if (!url || !zone) return E_INVALIDARG;
    char t[300];
    url_text(url, t, sizeof t);
    if (!local_file(url)) hp_unsupported("IInternetSecurityManager::MapUrlToZone", "URL \"%s\"", t);
    wr32(zone, URLZONE_LOCAL_MACHINE);
    return S_OK;
}

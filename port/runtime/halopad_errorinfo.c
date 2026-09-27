/* HaloPad OLE Automation error objects (G3): CreateErrorInfo, SetErrorInfo and GetErrorInfo.
 * msxml4.dll describes a failed call this way (its message text comes from msxml4r.dll), and a
 * caller such as Keystone.dll reads it back through IErrorInfo. As in oleaut32, one error object
 * answers both ICreateErrorInfo and IErrorInfo with one reference count (its IUnknown is the
 * IErrorInfo pointer), and each thread holds one current error object, which GetErrorInfo hands
 * over and clears. */
#include "halopad_win32.h"
#include <pthread.h>

#define S_OK 0u
#define S_FALSE 1u
#define E_NOINTERFACE 0x80004002u
#define E_INVALIDARG 0x80070057u

int halopad_guid_is(uint32_t g, const char *text);
uint32_t halopad_com_new(const char *iface, uint32_t size, void *state, void (*destroy)(void *));
void *halopad_com_state(const char *iface, uint32_t g);
uint32_t halopad_com_release(uint32_t g);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t SysAllocString_c(uint32_t src);
uint32_t SysFreeString_c(uint32_t b);

#define IID_IUnknown "{00000000-0000-0000-C000-000000000046}"
#define IID_IErrorInfo "{1CF2B120-547D-101B-8E65-08002B2BD119}"
#define IID_ICreateErrorInfo "{22F03340-547D-101B-8E65-08002B2BD119}"

typedef struct {
    uint32_t refs, as_info, as_create;          /* the object's two interface pointers */
    uint8_t guid[16];
    uint32_t source, description, helpfile;     /* guest BSTRs the object owns */
    uint32_t context;
} errinfo;

static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static _Thread_local uint32_t current;          /* this thread's error object (an IErrorInfo pointer) */

static errinfo *info(uint32_t g) { return halopad_com_state("IErrorInfo", g); }
static errinfo *creator(uint32_t g) { return halopad_com_state("ICreateErrorInfo", g); }

uint32_t CreateErrorInfo_c(uint32_t out)
{
    if (!out) return E_INVALIDARG;
    errinfo *e = calloc(1, sizeof *e);
    e->refs = 1;
    e->as_info = halopad_com_new("IErrorInfo", 4, e, NULL);
    e->as_create = halopad_com_new("ICreateErrorInfo", 4, e, NULL);
    wr32(out, e->as_create);
    return S_OK;
}

static uint32_t addref(errinfo *e) { pthread_mutex_lock(&lock); uint32_t n = ++e->refs; pthread_mutex_unlock(&lock); return n; }
static uint32_t release(errinfo *e)
{
    pthread_mutex_lock(&lock);
    uint32_t n = --e->refs;
    pthread_mutex_unlock(&lock);
    if (n) return n;
    SysFreeString_c(e->source);
    SysFreeString_c(e->description);
    SysFreeString_c(e->helpfile);
    halopad_com_release(e->as_info);
    halopad_com_release(e->as_create);
    free(e);
    return 0;
}
static uint32_t query(errinfo *e, uint32_t iid, uint32_t out)
{
    if (!out) return E_INVALIDARG;
    uint32_t p = 0;
    if (halopad_guid_is(iid, IID_IUnknown) || halopad_guid_is(iid, IID_IErrorInfo)) p = e->as_info;
    else if (halopad_guid_is(iid, IID_ICreateErrorInfo)) p = e->as_create;
    wr32(out, p);
    if (!p) return E_NOINTERFACE;
    addref(e);
    return S_OK;
}
static uint32_t get_string(uint32_t s, uint32_t out)
{
    if (!out) return E_INVALIDARG;
    wr32(out, s ? SysAllocString_c(s) : 0);
    return S_OK;
}
static uint32_t set_string(uint32_t *s, uint32_t text)
{
    SysFreeString_c(*s);
    *s = text ? SysAllocString_c(text) : 0;
    return S_OK;
}

uint32_t hpcom_ICreateErrorInfo_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out) { return query(creator(g), iid, out); }
uint32_t hpcom_ICreateErrorInfo_AddRef_c(uint32_t g) { return addref(creator(g)); }
uint32_t hpcom_ICreateErrorInfo_Release_c(uint32_t g) { return release(creator(g)); }
uint32_t hpcom_ICreateErrorInfo_SetGUID_c(uint32_t g, uint32_t guid) { memcpy(creator(g)->guid, G(guid), 16); return S_OK; }
uint32_t hpcom_ICreateErrorInfo_SetSource_c(uint32_t g, uint32_t s) { return set_string(&creator(g)->source, s); }
uint32_t hpcom_ICreateErrorInfo_SetDescription_c(uint32_t g, uint32_t s) { return set_string(&creator(g)->description, s); }
uint32_t hpcom_ICreateErrorInfo_SetHelpFile_c(uint32_t g, uint32_t s) { return set_string(&creator(g)->helpfile, s); }
uint32_t hpcom_ICreateErrorInfo_SetHelpContext_c(uint32_t g, uint32_t c) { creator(g)->context = c; return S_OK; }

uint32_t hpcom_IErrorInfo_QueryInterface_c(uint32_t g, uint32_t iid, uint32_t out) { return query(info(g), iid, out); }
uint32_t hpcom_IErrorInfo_AddRef_c(uint32_t g) { return addref(info(g)); }
uint32_t hpcom_IErrorInfo_Release_c(uint32_t g) { return release(info(g)); }
uint32_t hpcom_IErrorInfo_GetGUID_c(uint32_t g, uint32_t out) { if (!out) return E_INVALIDARG; memcpy(G(out), info(g)->guid, 16); return S_OK; }
uint32_t hpcom_IErrorInfo_GetSource_c(uint32_t g, uint32_t out) { return get_string(info(g)->source, out); }
uint32_t hpcom_IErrorInfo_GetDescription_c(uint32_t g, uint32_t out) { return get_string(info(g)->description, out); }
uint32_t hpcom_IErrorInfo_GetHelpFile_c(uint32_t g, uint32_t out) { return get_string(info(g)->helpfile, out); }
uint32_t hpcom_IErrorInfo_GetHelpContext_c(uint32_t g, uint32_t out) { if (!out) return E_INVALIDARG; wr32(out, info(g)->context); return S_OK; }

/* The current error object may be any IErrorInfo, so references go through its vtable. */
static void com_call(uint32_t obj, uint32_t index) { uint32_t a = obj; halopad_call_guest(rd32(rd32(obj) + 4 * index), 1, &a); }

uint32_t SetErrorInfo_c(uint32_t reserved, uint32_t e)
{
    if (reserved) return E_INVALIDARG;
    if (e) com_call(e, 1);                       /* AddRef */
    uint32_t old = current;
    current = e;
    if (old) com_call(old, 2);                   /* Release */
    return S_OK;
}

uint32_t GetErrorInfo_c(uint32_t reserved, uint32_t out)
{
    if (reserved || !out) return E_INVALIDARG;
    wr32(out, current);
    uint32_t had = current != 0;
    current = 0;                                 /* the caller now owns the reference */
    return had ? S_OK : S_FALSE;
}

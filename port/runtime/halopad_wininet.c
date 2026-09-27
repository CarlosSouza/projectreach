/* HaloPad WinInet and WinHTTP (G5): the proxy discovery Halo's version check does before it
 * contacts Bungie (0x57a620, on its own thread).
 *
 * Halo asks WinInet for the system proxy (InternetQueryOptionA(NULL, INTERNET_OPTION_PROXY))
 * and uses the proxy string only when one is set. It then loads winhttp.dll and tries automatic
 * discovery (WinHttpOpen, WinHttpGetProxyForUrl, WinHttpCloseHandle, all by GetProcAddress).
 * The reference machine (Windows XP SP3 with WinHTTP 5.1) has no proxy configured and no WPAD
 * server on its network: WinInet reports direct access, and automatic discovery fails with
 * ERROR_WINHTTP_AUTODETECTION_FAILED. Other options and uses stop with their values. */
#include "halopad_win32.h"
#include <pthread.h>

#define INTERNET_OPTION_PROXY 38
#define INTERNET_OPEN_TYPE_DIRECT 1
#define ERROR_INSUFFICIENT_BUFFER 122
#define ERROR_INVALID_HANDLE 6
#define ERROR_WINHTTP_AUTODETECTION_FAILED 12180

uint32_t InternetQueryOptionA_c(uint32_t internet, uint32_t option, uint32_t buffer, uint32_t lenp)
{
    if (internet || option != INTERNET_OPTION_PROXY)
        hp_unsupported("InternetQueryOptionA", "option %u on handle 0x%x", option, internet);
    if (!lenp) { halopad_last_error = 87; return 0; }                 /* ERROR_INVALID_PARAMETER */
    /* INTERNET_PROXY_INFO: dwAccessType, lpszProxy, lpszProxyBypass (no strings when direct) */
    if (!buffer || rd32(lenp) < 12) { wr32(lenp, 12); halopad_last_error = ERROR_INSUFFICIENT_BUFFER; return 0; }
    wr32(buffer, INTERNET_OPEN_TYPE_DIRECT);
    wr32(buffer + 4, 0);
    wr32(buffer + 8, 0);
    wr32(lenp, 12);
    return 1;
}

/* WinHTTP sessions: handles only, since nothing is ever sent */
static uint32_t sessions[16];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

uint32_t WinHttpOpen_c(uint32_t agent, uint32_t access, uint32_t proxy, uint32_t bypass, uint32_t flags)
{
    (void)agent; (void)proxy; (void)bypass;
    if (flags) hp_unsupported("WinHttpOpen", "flags 0x%x (access type %u)", flags, access);   /* WINHTTP_FLAG_ASYNC */
    pthread_mutex_lock(&lock);
    uint32_t h = 0;
    for (uint32_t i = 0; i < 16 && !h; i++) if (!sessions[i]) { sessions[i] = 1; h = 0x00CC0000u + 4 * (i + 1); }
    pthread_mutex_unlock(&lock);
    if (!h) hp_unsupported("WinHttpOpen", "more than 16 sessions");
    return h;
}

static int session_index(uint32_t h)
{
    uint32_t i = (h - 0x00CC0000u) / 4;
    return (h >= 0x00CC0004u && (h & 3) == 0 && i >= 1 && i <= 16 && sessions[i - 1]) ? (int)i - 1 : -1;
}

uint32_t WinHttpGetProxyForUrl_c(uint32_t session, uint32_t url, uint32_t options, uint32_t info)
{
    (void)url; (void)options; (void)info;
    if (session_index(session) < 0) { halopad_last_error = ERROR_INVALID_HANDLE; return 0; }
    halopad_last_error = ERROR_WINHTTP_AUTODETECTION_FAILED;         /* no WPAD server on the reference network */
    return 0;
}

uint32_t WinHttpCloseHandle_c(uint32_t h)
{
    pthread_mutex_lock(&lock);
    int i = session_index(h);
    if (i >= 0) sessions[i] = 0;
    pthread_mutex_unlock(&lock);
    if (i < 0) { halopad_last_error = ERROR_INVALID_HANDLE; return 0; }
    return 1;
}

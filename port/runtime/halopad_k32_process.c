/* HaloPad kernel32 process, thread, locale and time services (G3).
 *
 * The runtime presents one fixed Windows environment: Windows XP SP3 (5.1.2600), one
 * plain x86 CPU (see docs/G2D-SIMD-SCOPE.md), code page 1252, US English, an empty
 * environment block, no console (a GUI process started from Explorer), and the game
 * installed at HP_INSTALL_DIR. Each choice is deliberate and documented in
 * docs/G3-RUNTIME.md; nothing here returns a guessed value. */
#include "halopad_win32.h"
#include <pthread.h>
#include <time.h>
#include <unistd.h>

extern uint32_t halopad_last_error;   /* defined in halopad_kernel32.c */
uint32_t GetLastError_c(void) { return halopad_last_error; }
void SetLastError_c(uint32_t e) { halopad_last_error = e; }

/* ---- version and machine ---- */

uint32_t GetVersionExA_c(uint32_t info)
{
    uint32_t size = rd32(info);
    if (size != 148 && size != 156) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    memset(G(info) + 4, 0, size - 4);
    wr32(info + 4, 5); wr32(info + 8, 1); wr32(info + 12, 2600); wr32(info + 16, 2 /* VER_PLATFORM_WIN32_NT */);
    strcpy((char *)G(info) + 20, "Service Pack 3");
    if (size == 156) {
        wr16(info + 148, 3); wr16(info + 150, 0);          /* service pack 3.0 */
        wr16(info + 152, 0x100);                            /* VER_SUITE_SINGLEUSERTS */
        ((uint8_t *)G(info))[154] = 1;                      /* VER_NT_WORKSTATION */
    }
    return 1;
}

void GetSystemInfo_c(uint32_t info)
{
    memset(G(info), 0, 36);
    wr16(info + 0, 0);                  /* PROCESSOR_ARCHITECTURE_INTEL */
    wr32(info + 4, 0x1000);             /* page size */
    wr32(info + 8, 0x00010000);
    wr32(info + 12, 0x7FFEFFFF);
    wr32(info + 16, 1);                 /* active processor mask */
    wr32(info + 20, 1);                 /* one processor */
    wr32(info + 24, 586);               /* PROCESSOR_INTEL_PENTIUM */
    wr32(info + 28, 0x10000);           /* allocation granularity */
    wr16(info + 32, 6); wr16(info + 34, 0x0303);   /* family 6, model 3 stepping 3: matches cpuid */
}

uint32_t IsProcessorFeaturePresent_c(uint32_t feature)
{
    switch (feature) {
    case 0: case 1: return 0;           /* FP errata, FP emulated */
    case 2: return 1;                   /* CMPXCHG8B */
    case 3: case 6: case 7: case 10: return 0;   /* MMX, SSE, 3DNow!, SSE2: plain-CPU contract */
    }
    hp_unsupported("IsProcessorFeaturePresent", "feature %u", feature);
}

/* ---- process and module ---- */

static uint32_t guest_string(const char *s)
{
    uint32_t n = (uint32_t)strlen(s) + 1, a = halopad_heap_alloc(n, 0);
    memcpy(G(a), s, n);
    return a;
}

static uint32_t command_line;
uint32_t GetCommandLineA_c(void)
{
    if (!command_line) {
        const char *extra = getenv("HALOPAD_ARGS");
        char buf[1024];
        snprintf(buf, sizeof buf, "\"%s\\haloce.exe\"%s%s", HP_INSTALL_DIR, extra && *extra ? " " : "", extra ? extra : "");
        command_line = guest_string(buf);
    }
    return command_line;
}

void GetStartupInfoA_c(uint32_t info)
{
    memset(G(info), 0, 68);
    wr32(info, 68);
    wr32(info + 44, 1);                 /* STARTF_USESHOWWINDOW */
    wr16(info + 48, 1);                 /* SW_SHOWNORMAL, as Explorer starts it */
}

/* A GUI process started from Explorer has no standard handles. */
uint32_t GetStdHandle_c(uint32_t which)
{
    if (which == 0xFFFFFFF6u || which == 0xFFFFFFF5u || which == 0xFFFFFFF4u) return 0;
    halopad_last_error = HP_ERROR_INVALID_HANDLE;
    return 0xFFFFFFFFu;
}

uint32_t halopad_file_handle_valid(uint32_t handle);
uint32_t GetFileType_c(uint32_t handle)
{
    if (halopad_file_handle_valid(handle)) return 1;   /* FILE_TYPE_DISK */
    halopad_last_error = HP_ERROR_INVALID_HANDLE;
    return 0;                                           /* FILE_TYPE_UNKNOWN */
}

uint32_t SetHandleCount_c(uint32_t n) { return n; }    /* no effect on Windows NT */

uint32_t GetModuleFileNameA_c(uint32_t module, uint32_t buf, uint32_t size)
{
    if (module && module != HP_IMAGE_BASE) hp_unsupported("GetModuleFileNameA", "module 0x%08x", module);
    const char *path = HP_INSTALL_DIR "\\haloce.exe";
    uint32_t n = (uint32_t)strlen(path);
    if (size == 0) { halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER; return 0; }
    if (n >= size) {                    /* Windows XP: truncated, not terminated */
        memcpy(G(buf), path, size);
        halopad_last_error = HP_ERROR_INSUFFICIENT_BUFFER;
        return size;
    }
    memcpy(G(buf), path, n + 1);
    return n;
}

/* Empty environment: a double NUL in each form. */
uint32_t GetEnvironmentStringsW_c(void) { return halopad_heap_alloc(4, 1); }
uint32_t GetEnvironmentStrings_c(void) { return halopad_heap_alloc(2, 1); }
uint32_t FreeEnvironmentStringsW_c(uint32_t p) { halopad_heap_free(p); return 1; }
uint32_t FreeEnvironmentStringsA_c(uint32_t p) { halopad_heap_free(p); return 1; }

uint32_t GetCurrentProcess_c(void) { return 0xFFFFFFFFu; }
uint32_t GetCurrentThread_c(void) { return 0xFFFFFFFEu; }
uint32_t GetCurrentProcessId_c(void) { return 0x1000; }
uint32_t GetCurrentThreadId_c(void) { return 0x1004; }   /* main thread; more with CreateThread */

void ExitProcess_c(uint32_t code)
{
    fflush(stdout);
    fprintf(stderr, "HALOPAD: ExitProcess(%u)\n", code);
    exit((int)(code & 0xFF));
}

static uint32_t unhandled_filter;
uint32_t SetUnhandledExceptionFilter_c(uint32_t filter)
{
    uint32_t old = unhandled_filter;
    unhandled_filter = filter;          /* guest faults stop the program; see docs/G3-RUNTIME.md */
    return old;
}

/* ---- thread-local storage (TEB TlsSlots at +0xE10, 64 slots) ---- */

#define TEB_VA 0x7FFDE000u
static uint64_t tls_used;

uint32_t TlsAlloc_c(void)
{
    for (uint32_t i = 0; i < 64; i++)
        if (!(tls_used >> i & 1)) { tls_used |= 1ull << i; wr32(TEB_VA + 0xE10 + 4 * i, 0); return i; }
    halopad_last_error = 259;           /* ERROR_NO_MORE_ITEMS */
    return 0xFFFFFFFFu;
}

uint32_t TlsGetValue_c(uint32_t i)
{
    if (i >= 64 || !(tls_used >> i & 1)) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    halopad_last_error = 0;
    return rd32(TEB_VA + 0xE10 + 4 * i);
}

uint32_t TlsSetValue_c(uint32_t i, uint32_t v)
{
    if (i >= 64 || !(tls_used >> i & 1)) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    wr32(TEB_VA + 0xE10 + 4 * i, v);
    return 1;
}

uint32_t TlsFree_c(uint32_t i)
{
    if (i >= 64 || !(tls_used >> i & 1)) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    tls_used &= ~(1ull << i);
    return 1;
}

/* ---- synchronization: critical sections are host recursive mutexes keyed by the
 * guest CRITICAL_SECTION address ---- */

typedef struct { uint32_t addr; pthread_mutex_t *m; } cs_entry;
static cs_entry cs_table[1024];

static cs_entry *cs_find(uint32_t addr, int create)
{
    for (uint32_t i = (addr >> 3) & 1023, n = 0; n < 1024; i = (i + 1) & 1023, n++) {
        if (cs_table[i].addr == addr) return &cs_table[i];
        if (!cs_table[i].addr) {
            if (!create) return NULL;
            pthread_mutexattr_t at;
            pthread_mutexattr_init(&at);
            pthread_mutexattr_settype(&at, PTHREAD_MUTEX_RECURSIVE);
            cs_table[i].addr = addr;
            cs_table[i].m = malloc(sizeof(pthread_mutex_t));
            pthread_mutex_init(cs_table[i].m, &at);
            return &cs_table[i];
        }
    }
    hp_unsupported("InitializeCriticalSection", "more than 1024 critical sections");
}

void InitializeCriticalSection_c(uint32_t cs)
{
    memset(G(cs), 0, 24);
    wr32(cs + 4, 0xFFFFFFFFu);          /* LockCount = -1, as Windows initializes it */
    cs_find(cs, 1);
}

void EnterCriticalSection_c(uint32_t cs)
{
    cs_entry *e = cs_find(cs, 0);
    if (!e) hp_unsupported("EnterCriticalSection", "uninitialized critical section 0x%08x", cs);
    pthread_mutex_lock(e->m);
}

void LeaveCriticalSection_c(uint32_t cs)
{
    cs_entry *e = cs_find(cs, 0);
    if (!e) hp_unsupported("LeaveCriticalSection", "uninitialized critical section 0x%08x", cs);
    pthread_mutex_unlock(e->m);
}

void DeleteCriticalSection_c(uint32_t cs)
{
    cs_entry *e = cs_find(cs, 0);
    if (!e) return;
    pthread_mutex_destroy(e->m);
    free(e->m);
    e->m = NULL;                        /* keep the slot as a tombstone for probing */
    e->addr = 0xFFFFFFFFu;
}

uint32_t InterlockedExchange_c(uint32_t target, uint32_t value)
{
    return __atomic_exchange_n((uint32_t *)G(target), value, __ATOMIC_SEQ_CST);
}

/* ---- time ---- */

static uint64_t mono_ns(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return (uint64_t)t.tv_sec * 1000000000u + (uint64_t)t.tv_nsec;
}

void GetSystemTimeAsFileTime_c(uint32_t ft)
{
    struct timespec t;
    clock_gettime(CLOCK_REALTIME, &t);
    uint64_t v = (uint64_t)t.tv_sec * 10000000u + (uint64_t)t.tv_nsec / 100u + 116444736000000000ull;
    wr32(ft, (uint32_t)v); wr32(ft + 4, (uint32_t)(v >> 32));
}

uint32_t GetTickCount_c(void) { return (uint32_t)(mono_ns() / 1000000u); }

/* Performance counter: 10 MHz, from the monotonic clock. */
uint32_t QueryPerformanceFrequency_c(uint32_t out) { wr32(out, 10000000u); wr32(out + 4, 0); return 1; }
uint32_t QueryPerformanceCounter_c(uint32_t out)
{
    uint64_t v = mono_ns() / 100u;
    wr32(out, (uint32_t)v); wr32(out + 4, (uint32_t)(v >> 32));
    return 1;
}

/* ---- code pages: 1252 (ANSI) and 437 (OEM) ---- */

uint32_t GetACP_c(void) { return 1252; }
uint32_t GetOEMCP_c(void) { return 437; }
uint32_t IsValidCodePage_c(uint32_t cp) { return cp == 1252 || cp == 437; }

uint32_t GetCPInfo_c(uint32_t cp, uint32_t info)
{
    if (cp == 0) cp = 1252;
    if (cp == 1) cp = 437;
    if (cp != 1252 && cp != 437) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    memset(G(info), 0, 20);
    wr32(info, 1);                      /* MaxCharSize */
    ((uint8_t *)G(info))[4] = '?';      /* DefaultChar */
    return 1;
}

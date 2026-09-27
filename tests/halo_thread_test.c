/* Thread test (G3): CreateThread and the thread services, called the way Halo calls them.
 * Thread procedures are real one-argument KERNEL32 services entered through their guest
 * addresses (SetEvent, ExitThread, TlsGetValue, GlobalFree, EnterCriticalSection,
 * SetLastError, Sleep), so every thread runs through dispatch and the stdcall check as a
 * Halo thread would. A host-thread stress run checks the heap under contention.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_thread_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
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
uint32_t GetProcessHeap_c(void);
uint32_t HeapAlloc_c(uint32_t handle, uint32_t flags, uint32_t size);
uint32_t HeapFree_c(uint32_t handle, uint32_t flags, uint32_t a);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-64s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t k32;
static uint32_t proc(const char *n) { uint32_t va = GetProcAddress_c(k32, str(n)); if (!va) { printf("no %s\n", n); exit(2); } return va; }
static uint32_t api(const char *name, uint32_t n, const uint32_t *args) { return halopad_call_guest(proc(name), n, args); }
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})

static uint32_t tidp;
static uint32_t thread(uint32_t start, uint32_t param, uint32_t flags) { return API("CreateThread", 0, 0x4000, start, param, flags, tidp); }
static uint32_t exit_code(uint32_t h) { uint32_t c = halopad_heap_alloc(4, 1); API("GetExitCodeThread", h, c); return rd(c); }

/* heap stress from host threads */
static int stress_bad;
static void *stress(void *arg)
{
    uint32_t seed = (uint32_t)(uintptr_t)arg, heap = GetProcessHeap_c(), live[64] = {0}, len[64] = {0};
    for (int i = 0; i < 20000; i++) {
        seed = seed * 1103515245u + 12345u;
        int s = (seed >> 8) & 63;
        if (live[s]) {
            uint8_t *p = halopad_guest_ptr(live[s]);
            for (uint32_t k = 0; k < len[s]; k++) if (p[k] != (uint8_t)(live[s] + k)) { stress_bad = 1; break; }
            HeapFree_c(heap, 0, live[s]);
            live[s] = 0;
        } else {
            len[s] = 1 + (seed >> 16) % 3000;
            live[s] = HeapAlloc_c(heap, 0, len[s]);
            uint8_t *p = halopad_guest_ptr(live[s]);
            for (uint32_t k = 0; k < len[s]; k++) p[k] = (uint8_t)(live[s] + k);
        }
    }
    for (int s = 0; s < 64; s++) if (live[s]) HeapFree_c(heap, 0, live[s]);
    return NULL;
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
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    k32 = LoadLibraryA_c(str("kernel32.dll"));
    tidp = halopad_heap_alloc(4, 1);

    /* suspended start, resume, exit code, handle signaled */
    uint32_t ev = API("CreateEventA", 0, 1, 0, 0);
    uint32_t h = thread(proc("SetEvent"), ev, 4);
    check("CreateThread(SetEvent, CREATE_SUSPENDED)", h != 0, 1);
    check("  a new thread ID (not the main thread's 0x1004)", rd(tidp) != 0x1004 && rd(tidp) != 0, 1);
    check("  suspended: the event stays unset", API("WaitForSingleObject", ev, 50), 0x102);
    check("  GetExitCodeThread: STILL_ACTIVE", exit_code(h), 259);
    check("  ResumeThread returns the previous count 1", API("ResumeThread", h), 1);
    check("  the thread handle is signaled when it ends", API("WaitForSingleObject", h, 5000), 0);
    check("  exit code is SetEvent's TRUE", exit_code(h), 1);
    check("  and the event is set", API("WaitForSingleObject", ev, 0), 0);
    check("  a signaled thread stays signaled", API("WaitForSingleObject", h, 0), 0);
    check("  CloseHandle", API("CloseHandle", h), 1);

    /* ExitThread unwinds to the thread's root */
    h = thread(proc("ExitThread"), 42, 0);
    API("WaitForSingleObject", h, 5000);
    check("ExitThread(42) as the thread procedure: exit code 42", exit_code(h), 42);
    API("CloseHandle", h);

    /* TLS slots are per thread */
    uint32_t idx = api("TlsAlloc", 0, NULL);
    API("TlsSetValue", idx, 1234);
    h = thread(proc("TlsGetValue"), idx, 0);
    API("WaitForSingleObject", h, 5000);
    check("TlsGetValue on a new thread reads 0", exit_code(h), 0);
    check("  the main thread still reads 1234", API("TlsGetValue", idx), 1234);
    API("CloseHandle", h);

    /* last error is per thread */
    h = thread(proc("SetLastError"), 77, 4);
    API("SetLastError", 5);
    API("ResumeThread", h);
    API("WaitForSingleObject", h, 5000);
    check("SetLastError on another thread leaves this thread's value", api("GetLastError", 0, NULL), 5);
    API("CloseHandle", h);

    /* sixteen threads at once, each on its own event, then sixteen freeing heap blocks */
    uint32_t evs[16], hs[16], ok = 1;
    for (int i = 0; i < 16; i++) evs[i] = API("CreateEventA", 0, 1, 0, 0);
    for (int i = 0; i < 16; i++) hs[i] = thread(proc("SetEvent"), evs[i], 0);
    for (int i = 0; i < 16; i++) {
        ok &= API("WaitForSingleObject", hs[i], 5000) == 0 && exit_code(hs[i]) == 1 && API("WaitForSingleObject", evs[i], 0) == 0;
        API("CloseHandle", hs[i]);
    }
    check("16 threads at once: each set its event and ended with TRUE", ok, 1);
    uint32_t blocks[16];
    ok = 1;
    for (int i = 0; i < 16; i++) blocks[i] = API("GlobalAlloc", 0, 1000 + 16 * i);
    for (int i = 0; i < 16; i++) hs[i] = thread(proc("GlobalFree"), blocks[i], 0);
    for (int i = 0; i < 16; i++) { ok &= API("WaitForSingleObject", hs[i], 5000) == 0 && exit_code(hs[i]) == 0; API("CloseHandle", hs[i]); }
    check("16 threads freeing heap blocks at once: all succeed", ok, 1);

    /* a critical section held by the main thread blocks a worker */
    uint32_t cs = halopad_heap_alloc(24, 1);
    API("InitializeCriticalSection", cs);
    API("EnterCriticalSection", cs);
    h = thread(proc("EnterCriticalSection"), cs, 0);
    check("a worker waits for a critical section the main thread holds", API("WaitForSingleObject", h, 100), 0x102);
    API("LeaveCriticalSection", cs);
    check("  and gets it once released", API("WaitForSingleObject", h, 5000), 0);
    API("CloseHandle", h);

    /* priorities, a running thread, termination before it runs */
    h = thread(proc("Sleep"), 150, 0);
    check("a sleeping thread is STILL_ACTIVE", exit_code(h), 259);
    check("SetThreadPriority(ABOVE_NORMAL)", API("SetThreadPriority", h, 1), 1);
    check("  GetThreadPriority reads it back", API("GetThreadPriority", h), 1);
    check("  an invalid priority is refused", API("SetThreadPriority", h, 7), 0);
    check("  it ends", API("WaitForSingleObject", h, 5000), 0);
    API("CloseHandle", h);
    ev = API("CreateEventA", 0, 1, 0, 0);
    h = thread(proc("SetEvent"), ev, 4);
    check("TerminateThread on a thread that never started", API("TerminateThread", h, 9), 1);
    check("  ends it with that code, without running it", API("WaitForSingleObject", h, 5000) == 0 && exit_code(h) == 9
          && API("WaitForSingleObject", ev, 0) == 0x102, 1);
    API("CloseHandle", h);
    check("SleepEx(10, TRUE) returns 0", API("SleepEx", 10, 1), 0);

    /* the heap under contention */
    pthread_t st[4];
    for (int i = 0; i < 4; i++) pthread_create(&st[i], NULL, stress, (void *)(uintptr_t)(i + 1));
    for (int i = 0; i < 4; i++) pthread_join(st[i], NULL);
    check("heap: 4 host threads x 20,000 allocations, contents intact", stress_bad, 0);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

/* HaloPad threads (G3): CreateThread and the thread services Halo calls.
 *
 * How Halo uses them (haloce.exe): ten CreateThread sites (0x4404b0 ... 0x57a409) with
 * stacks of 0x4000 or 0x10400 bytes, some CREATE_SUSPENDED then SetThreadPriority and
 * ResumeThread; the C runtime's _beginthreadex/_endthreadex (ResumeThread 0x5cb649,
 * ExitThread 0x5cb54f); GetExitCodeThread polling (8 sites); TerminateThread at two
 * shutdown sites; SleepEx.
 *
 * Each guest thread is a host thread with its own guest CPU state, a guest stack (the
 * image's reserve size unless asked for more, all committed), a TEB and a copy of the
 * static TLS block (halopad_thread.c). The thread procedure is entered with
 * halopad_call_guest, so the stdcall contract is checked when it returns. ExitThread
 * unwinds to the thread's root. The handle is a kernel object (halopad_sync.c) that is
 * signaled, with the exit code, when the thread ends; the thread holds its own reference
 * so the handle may be closed while it runs. Priorities are recorded and reported back;
 * the host schedules all threads at its normal priority. TerminateThread works on a
 * thread that has ended or never started; stopping a running one stops the program with
 * the thread named, since it cannot be done safely in the middle of translated code. */
#include "halopad_win32.h"
#include <pthread.h>
#include <setjmp.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern _Thread_local _cpu *halopad_cpu;
extern _Thread_local uint32_t halopad_current_tid;
extern uint64_t halopad_guest_base;
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_thread_attach(uint32_t stack_base, uint32_t stack_limit);
void halopad_thread_detach(void);
uint32_t halopad_object_thread_new(void);
void halopad_object_addref(uint32_t h);
void halopad_object_thread_exit(uint32_t h, uint32_t code);
int halopad_object_thread_query(uint32_t h, uint32_t *code);
uint32_t halopad_object_close(uint32_t h);
void Sleep_c(uint32_t ms);

#define IMAGE_BASE 0x400000u
#define CREATE_SUSPENDED 0x4u
#define STACK_SIZE_PARAM_IS_A_RESERVATION 0x10000u
#define CURRENT_THREAD 0xFFFFFFFEu
#define STILL_ACTIVE 259u

typedef struct hthread {
    uint32_t handle, start, param, tid, stack, reserve;
    int suspend, started, cancelled;
    int32_t priority;
    uint32_t code;
    jmp_buf exit;
    struct hthread *next;
} hthread;

static hthread *threads;
static pthread_mutex_t tlock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t tcond = PTHREAD_COND_INITIALIZER;
static _Thread_local hthread *self;
static uint32_t next_tid = 0x1008;
static int32_t main_priority;

static hthread *find(uint32_t h)
{
    for (hthread *t = threads; t; t = t->next) if (t->handle == h) return t;
    return NULL;
}

static uint32_t image_stack_reserve(void) { return rd32(IMAGE_BASE + rd32(IMAGE_BASE + 0x3C) + 0x18 + 0x48); }

static void *run(void *p)
{
    hthread *t = p;
    self = t;
    halopad_current_tid = t->tid;
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = t->stack + t->reserve;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x027F;                                            /* x87 control word of a new Windows thread */
    halopad_cpu = &cpu;
    halopad_thread_attach(t->stack + t->reserve, t->stack);
    pthread_mutex_lock(&tlock);
    while (t->suspend && !t->cancelled) pthread_cond_wait(&tcond, &tlock);
    t->started = !t->cancelled;
    pthread_mutex_unlock(&tlock);
    if (t->started && !setjmp(t->exit)) t->code = halopad_call_guest(t->start, 1, &t->param);
    halopad_thread_detach();
    halopad_vm_release(t->stack);
    pthread_mutex_lock(&tlock);
    for (hthread **q = &threads; *q; q = &(*q)->next) if (*q == t) { *q = t->next; break; }
    pthread_mutex_unlock(&tlock);
    halopad_object_thread_exit(t->handle, t->code);
    halopad_object_close(t->handle);                                /* the thread's own reference */
    halopad_cpu = NULL;
    self = NULL;
    free(t);
    return NULL;
}

uint32_t CreateThread_c(uint32_t security, uint32_t stack_size, uint32_t start, uint32_t param, uint32_t flags, uint32_t id_out)
{
    if (security) hp_unsupported("CreateThread", "security attributes 0x%08x", security);
    if (flags & ~(CREATE_SUSPENDED | STACK_SIZE_PARAM_IS_A_RESERVATION)) hp_unsupported("CreateThread", "creation flags 0x%x", flags);
    if (!start) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    uint32_t reserve = (stack_size + 0xFFFFu) & ~0xFFFFu;
    if (!(flags & STACK_SIZE_PARAM_IS_A_RESERVATION) && reserve < image_stack_reserve()) reserve = image_stack_reserve();
    if (!reserve) reserve = 0x100000;
    hthread *t = calloc(1, sizeof *t);
    t->stack = halopad_vm_reserve(0, reserve, 1);
    if (!t->stack) { free(t); halopad_last_error = HP_ERROR_NOT_ENOUGH_MEMORY; return 0; }
    t->reserve = reserve; t->start = start; t->param = param; t->suspend = (flags & CREATE_SUSPENDED) ? 1 : 0;
    t->handle = halopad_object_thread_new();
    halopad_object_addref(t->handle);                               /* held by the thread until it ends */
    pthread_mutex_lock(&tlock);
    t->tid = next_tid;
    next_tid += 4;
    t->next = threads;
    threads = t;
    pthread_mutex_unlock(&tlock);
    uint32_t h = t->handle, tid = t->tid;
    pthread_attr_t a;
    pthread_attr_init(&a);
    pthread_attr_setstacksize(&a, 16u << 20);                       /* translated frames run on the host stack */
    pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);
    pthread_t host;
    if (pthread_create(&host, &a, run, t)) hp_unsupported("CreateThread", "a host thread (pthread_create failed)");
    pthread_attr_destroy(&a);
    if (id_out) wr32(id_out, tid);
    return h;
}

uint32_t ExitThread_c(uint32_t code)
{
    if (!self) hp_unsupported("ExitThread", "on the main thread (exit code %u)", code);
    self->code = code;
    longjmp(self->exit, 1);
}

uint32_t ResumeThread_c(uint32_t h)
{
    if (h == CURRENT_THREAD) return 0;
    pthread_mutex_lock(&tlock);
    hthread *t = find(h);
    uint32_t prev = t ? (uint32_t)t->suspend : 0;
    if (t && t->suspend) { t->suspend--; if (!t->suspend) pthread_cond_broadcast(&tcond); }
    pthread_mutex_unlock(&tlock);
    uint32_t code;
    if (!t && !halopad_object_thread_query(h, &code)) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0xFFFFFFFFu; }
    return prev;
}

uint32_t GetExitCodeThread_c(uint32_t h, uint32_t out)
{
    uint32_t code = STILL_ACTIVE;
    if (h != CURRENT_THREAD && !halopad_object_thread_query(h, &code)) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    if (!out) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    wr32(out, code);
    return 1;
}

static int valid_priority(int32_t p) { return p == -15 || p == 15 || (p >= -2 && p <= 2); }

uint32_t SetThreadPriority_c(uint32_t h, uint32_t p)
{
    int32_t pr = (int32_t)p;
    if (!valid_priority(pr)) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    if (h == CURRENT_THREAD) { if (self) self->priority = pr; else main_priority = pr; return 1; }
    pthread_mutex_lock(&tlock);
    hthread *t = find(h);
    if (t) t->priority = pr;
    pthread_mutex_unlock(&tlock);
    uint32_t code;
    if (!t && !halopad_object_thread_query(h, &code)) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    return 1;
}

uint32_t GetThreadPriority_c(uint32_t h)
{
    if (h == CURRENT_THREAD) return (uint32_t)(self ? self->priority : main_priority);
    pthread_mutex_lock(&tlock);
    hthread *t = find(h);
    int32_t p = t ? t->priority : 0;
    pthread_mutex_unlock(&tlock);
    uint32_t code;
    if (!t && !halopad_object_thread_query(h, &code)) { halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0x7FFFFFFFu; }   /* THREAD_PRIORITY_ERROR_RETURN */
    return (uint32_t)p;
}

uint32_t TerminateThread_c(uint32_t h, uint32_t code)
{
    pthread_mutex_lock(&tlock);
    hthread *t = find(h);
    if (t && !t->started && t->suspend) {                           /* never ran: it ends without running */
        t->cancelled = 1; t->code = code;
        pthread_cond_broadcast(&tcond);
        pthread_mutex_unlock(&tlock);
        return 1;
    }
    pthread_mutex_unlock(&tlock);
    uint32_t c;
    if (!t) {
        if (halopad_object_thread_query(h, &c)) return 1;           /* already ended */
        halopad_last_error = HP_ERROR_INVALID_HANDLE;
        return 0;
    }
    hp_unsupported("TerminateThread", "stopping running thread 0x%x (start 0x%08x) in the middle of its code", t->tid, t->start);
}

int halopad_apc_pending(void);
int halopad_apc_deliver(void);

/* An alertable sleep runs the thread's queued completion routines (ReadFileEx) and
   returns WAIT_IO_COMPLETION at once; only this thread queues APCs to itself, so none can
   arrive while it sleeps. */
uint32_t SleepEx_c(uint32_t ms, uint32_t alertable)
{
    if (alertable && halopad_apc_pending()) { halopad_apc_deliver(); return 0xC0; }
    Sleep_c(ms);
    return 0;
}

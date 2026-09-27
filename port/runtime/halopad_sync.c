/* HaloPad kernel objects and waits (G3): CreateMutexA, ReleaseMutex, CreateEventA,
 * SetEvent, WaitForSingleObject(Ex), Sleep, and CloseHandle for these objects.
 *
 * All objects share one host lock and condition variable, which keeps waits simple and
 * correct once Halo's own threads run. Named objects are process-wide: creating an
 * existing name returns a handle to it with ERROR_ALREADY_EXISTS, as on Windows.
 * Handles are 0x1000 + 4*i (file handles live below 0x1000). Alertable waits that would
 * need to run queued APCs stop the program until APC delivery exists. */
#include "halopad_win32.h"
#include <errno.h>
#include <pthread.h>
#include <strings.h>
#include <time.h>

#define OBJ_BASE 0x1000u
#define MAX_OBJ 512
#define WAIT_OBJECT_0 0u
#define WAIT_TIMEOUT 0x102u
#define INFINITE 0xFFFFFFFFu
#define ERROR_ALREADY_EXISTS 183

enum { FREE, MUTEX, EVENT, THREAD, TOKEN, SEMAPHORE };   /* a thread object is signaled, for good, when the thread ends;
                                                          a semaphore keeps its count in count and its maximum in owner */
typedef struct { int kind; uint32_t refs; char *name; int manual, signaled; uint32_t owner, count; } object;

static object objs[MAX_OBJ];
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t changed = PTHREAD_COND_INITIALIZER;
extern _Thread_local uint32_t halopad_current_tid;

static object *obj(uint32_t h)
{
    if (h < OBJ_BASE || (h - OBJ_BASE) % 4 || (h - OBJ_BASE) / 4 >= MAX_OBJ) return NULL;
    object *o = &objs[(h - OBJ_BASE) / 4];
    return o->kind == FREE ? NULL : o;
}

int halopad_is_object_handle(uint32_t h) { return obj(h) != NULL; }

static uint32_t create(int kind, uint32_t name, int manual, int initial)
{
    pthread_mutex_lock(&lock);
    const char *n = name ? (const char *)G(name) : NULL;
    if (n) {
        for (int i = 0; i < MAX_OBJ; i++)
            if (objs[i].kind != FREE && objs[i].name && !strcmp(objs[i].name, n)) {
                if (objs[i].kind != kind) { pthread_mutex_unlock(&lock); halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
                objs[i].refs++;
                pthread_mutex_unlock(&lock);
                halopad_last_error = ERROR_ALREADY_EXISTS;
                return OBJ_BASE + 4 * (uint32_t)i;
            }
    }
    for (int i = 0; i < MAX_OBJ; i++) {
        if (objs[i].kind != FREE) continue;
        objs[i] = (object){kind, 1, n ? strdup(n) : NULL, manual, 0, 0, 0};
        if (kind == MUTEX && initial) { objs[i].owner = halopad_current_tid; objs[i].count = 1; }
        if (kind == EVENT) objs[i].signaled = initial;
        pthread_mutex_unlock(&lock);
        halopad_last_error = 0;
        return OBJ_BASE + 4 * (uint32_t)i;
    }
    pthread_mutex_unlock(&lock);
    hp_unsupported("CreateObject", "more than %d kernel objects", MAX_OBJ);
}

uint32_t CreateMutexA_c(uint32_t security, uint32_t initial_owner, uint32_t name)
{
    if (security) hp_unsupported("CreateMutexA", "security attributes 0x%08x", security);
    return create(MUTEX, name, 0, initial_owner != 0);
}

uint32_t CreateEventA_c(uint32_t security, uint32_t manual, uint32_t initial, uint32_t name)
{
    if (security) hp_unsupported("CreateEventA", "security attributes 0x%08x", security);
    return create(EVENT, name, manual != 0, initial != 0);
}

uint32_t ReleaseMutex_c(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o || o->kind != MUTEX) { pthread_mutex_unlock(&lock); halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    if (o->owner != halopad_current_tid || !o->count) {
        pthread_mutex_unlock(&lock);
        halopad_last_error = 288;                       /* ERROR_NOT_OWNER */
        return 0;
    }
    if (--o->count == 0) { o->owner = 0; pthread_cond_broadcast(&changed); }
    pthread_mutex_unlock(&lock);
    return 1;
}

uint32_t CreateSemaphoreA_c(uint32_t security, uint32_t initial, uint32_t maximum, uint32_t name)
{
    if (security) hp_unsupported("CreateSemaphoreA", "security attributes 0x%08x", security);
    if ((int32_t)maximum <= 0 || (int32_t)initial < 0 || initial > maximum) { halopad_last_error = HP_ERROR_INVALID_PARAMETER; return 0; }
    uint32_t h = create(SEMAPHORE, name, 0, 0);
    if (h && halopad_last_error != ERROR_ALREADY_EXISTS) {
        pthread_mutex_lock(&lock);
        object *o = obj(h);
        o->count = initial;
        o->owner = maximum;
        pthread_mutex_unlock(&lock);
    }
    return h;
}

uint32_t ReleaseSemaphore_c(uint32_t h, uint32_t n, uint32_t previous)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o || o->kind != SEMAPHORE) { pthread_mutex_unlock(&lock); halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    if ((int32_t)n <= 0 || n > o->owner - o->count) {
        pthread_mutex_unlock(&lock);
        halopad_last_error = (int32_t)n <= 0 ? HP_ERROR_INVALID_PARAMETER : 298;   /* ERROR_TOO_MANY_POSTS */
        return 0;
    }
    if (previous) wr32(previous, o->count);
    o->count += n;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    return 1;
}

uint32_t ResetEvent_c(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o || o->kind != EVENT) { pthread_mutex_unlock(&lock); halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    o->signaled = 0;
    pthread_mutex_unlock(&lock);
    return 1;
}

uint32_t SetEvent_c(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o || o->kind != EVENT) { pthread_mutex_unlock(&lock); halopad_last_error = HP_ERROR_INVALID_HANDLE; return 0; }
    o->signaled = 1;
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
    return 1;
}

/* Called with the lock held: take the object if it is signaled for this thread. */
static int try_acquire(object *o)
{
    if (o->kind == SEMAPHORE) {
        if (!o->count) return 0;
        o->count--;
        return 1;
    }
    if (o->kind == MUTEX) {
        if (o->count && o->owner != halopad_current_tid) return 0;
        o->owner = halopad_current_tid;
        o->count++;
        return 1;
    }
    if (!o->signaled) return 0;
    if (!o->manual) o->signaled = 0;
    return 1;
}

int halopad_apc_pending(void);
int halopad_apc_deliver(void);

uint32_t WaitForSingleObjectEx_c(uint32_t h, uint32_t timeout, uint32_t alertable)
{
    if (alertable && halopad_apc_pending()) { halopad_apc_deliver(); return 0xC0; }   /* WAIT_IO_COMPLETION */
    struct timespec deadline;
    if (timeout != INFINITE) {
        clock_gettime(CLOCK_REALTIME, &deadline);
        deadline.tv_sec += timeout / 1000;
        deadline.tv_nsec += (long)(timeout % 1000) * 1000000L;
        if (deadline.tv_nsec >= 1000000000L) { deadline.tv_sec++; deadline.tv_nsec -= 1000000000L; }
    }
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o) {
        pthread_mutex_unlock(&lock);
        hp_unsupported("WaitForSingleObject", "handle 0x%08x (not a mutex or event)", h);
    }
    for (;;) {
        if (try_acquire(o)) { pthread_mutex_unlock(&lock); return WAIT_OBJECT_0; }
        if (timeout == 0) { pthread_mutex_unlock(&lock); return WAIT_TIMEOUT; }
        int rc = timeout == INFINITE ? pthread_cond_wait(&changed, &lock) : pthread_cond_timedwait(&changed, &lock, &deadline);
        if (rc == ETIMEDOUT) {
            if (try_acquire(o)) { pthread_mutex_unlock(&lock); return WAIT_OBJECT_0; }
            pthread_mutex_unlock(&lock);
            return WAIT_TIMEOUT;
        }
    }
}

uint32_t WaitForSingleObject_c(uint32_t h, uint32_t timeout) { return WaitForSingleObjectEx_c(h, timeout, 0); }

uint32_t halopad_object_close(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (!o) { pthread_mutex_unlock(&lock); return 0; }
    if (--o->refs == 0) { free(o->name); *o = (object){0}; }
    pthread_mutex_unlock(&lock);
    return 1;
}

void Sleep_c(uint32_t ms)
{
    struct timespec t = {ms / 1000, (long)(ms % 1000) * 1000000L};
    if (ms == 0) { sched_yield(); return; }
    while (nanosleep(&t, &t) != 0 && errno == EINTR) {}
}

/* MsgWaitForMultipleObjects: take the first of n handles (a guest array) that is signaled
   now; -1 if none is. */
int halopad_wait_poll(uint32_t n, uint32_t handles)
{
    pthread_mutex_lock(&lock);
    for (uint32_t i = 0; i < n; i++) {
        uint32_t h = rd32(handles + 4 * i);
        object *o = obj(h);
        if (!o) { pthread_mutex_unlock(&lock); hp_unsupported("MsgWaitForMultipleObjects", "handle 0x%08x (not a mutex or event)", h); }
        if (try_acquire(o)) { pthread_mutex_unlock(&lock); return (int)i; }
    }
    pthread_mutex_unlock(&lock);
    return -1;
}

/* ---- thread objects (halopad_k32_thread.c): count holds the exit code ---- */

uint32_t halopad_object_thread_new(void)
{
    uint32_t e = halopad_last_error;                                /* CreateThread leaves the caller's last error */
    uint32_t h = create(THREAD, 0, 1, 0);
    halopad_last_error = e;
    return h;
}

void halopad_object_addref(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (o) o->refs++;
    pthread_mutex_unlock(&lock);
}

void halopad_object_thread_exit(uint32_t h, uint32_t code)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    if (o && o->kind == THREAD) { o->signaled = 1; o->count = code; }
    pthread_cond_broadcast(&changed);
    pthread_mutex_unlock(&lock);
}

/* 1 for a thread handle, with its exit code (STILL_ACTIVE, 259, while it runs) */
int halopad_object_thread_query(uint32_t h, uint32_t *code)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    int is = o && o->kind == THREAD;
    if (is) *code = o->signaled ? o->count : 259;
    pthread_mutex_unlock(&lock);
    return is;
}

/* ---- access tokens (halopad_misc.c): manual holds "impersonation" ---- */

uint32_t halopad_object_token_new(int impersonation)
{
    uint32_t e = halopad_last_error;
    uint32_t h = create(TOKEN, 0, impersonation, 0);
    halopad_last_error = e;
    return h;
}

/* 1 impersonation, 0 primary, -1 not a token */
int halopad_object_token_impersonation(uint32_t h)
{
    pthread_mutex_lock(&lock);
    object *o = obj(h);
    int r = o && o->kind == TOKEN ? o->manual : -1;
    pthread_mutex_unlock(&lock);
    return r;
}

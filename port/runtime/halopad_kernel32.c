/* HaloPad Windows runtime: kernel32 files and directories (G3).
 *
 * A virtual Windows C: drive. The install directory
 * (C:\Program Files\Microsoft Games\Halo Custom Edition, also the current directory) is
 * two layers: the game files (HALOPAD_GAME_ROOT, never written) under a writable layer in
 * HALOPAD_STATE_ROOT/install; a game file opened for writing is first copied up. Every
 * other C: path lives in HALOPAD_STATE_ROOT/C, where the user profile (My Documents, Local
 * Settings\Temp) exists from the start as it does on Windows. Other drives do not exist.
 * Names are case-insensitive, as on Windows, whatever the host file system does.
 *
 * How Halo uses it (haloce.exe): 17 CreateFileA sites, most with FILE_FLAG_OVERLAPPED for
 * streaming (ReadFileEx with completion routines, SleepEx(0, TRUE) to run them; one
 * ReadFile + GetOverlappedResult), others plain reads, read/write OPEN_ALWAYS and
 * CREATE_ALWAYS writes; SetFilePointer (18), WriteFile (10), FindFirstFileA (13),
 * directories, DeleteFileA/CopyFileA, attributes and times; SHGetFolderPathA(CSIDL_PERSONAL)
 * from shfolder.dll for "My Documents\My Games\Halo CE".
 *
 * Overlapped reads complete when issued (Windows may do the same); ReadFileEx's routine is
 * queued as an APC on the calling thread and runs at its next alertable wait, as on
 * Windows. Deleting or removing an installed game file or directory stops the program. */
#include "halopad_win32.h"
#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <strings.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/attr.h>
#endif

uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
int halopad_is_object_handle(uint32_t handle);
uint32_t halopad_object_close(uint32_t handle);
uint32_t SetEvent_c(uint32_t h);

_Thread_local uint32_t halopad_last_error;

#define INVALID 0xFFFFFFFFu
#define FILE_BASE 0x100u
#define MAXFILES 256
#define FIND_BASE 0x3000u
#define MAXFIND 64
#define INSTALL "C:\\Program Files\\Microsoft Games\\Halo Custom Edition"
#define PROFILE "C:\\Documents and Settings\\Player"
#define MYDOCS PROFILE "\\My Documents"
#define TEMP PROFILE "\\Local Settings\\Temp"
enum { E_FILE_NOT_FOUND = 2, E_PATH_NOT_FOUND = 3, E_ACCESS_DENIED = 5, E_INVALID_HANDLE = 6, E_NO_MORE_FILES = 18,
       E_HANDLE_EOF = 38, E_FILE_EXISTS = 80, E_INVALID_PARAMETER = 87, E_DISK_FULL = 112, E_INVALID_NAME = 123,
       E_NEGATIVE_SEEK = 131, E_DIR_NOT_EMPTY = 145, E_ALREADY_EXISTS = 183 };

static uint32_t err(uint32_t e) { halopad_last_error = e; return 0; }
static uint32_t err_errno(void)
{
    switch (errno) {
    case ENOENT: return err(E_FILE_NOT_FOUND);
    case ENOTDIR: return err(E_PATH_NOT_FOUND);
    case EACCES: case EPERM: case EISDIR: case EROFS: return err(E_ACCESS_DENIED);
    case EEXIST: return err(E_ALREADY_EXISTS);
    case ENOTEMPTY: return err(E_DIR_NOT_EMPTY);
    case ENOSPC: return err(E_DISK_FULL);
    case ENAMETOOLONG: return err(206);
    }
    return err(E_ACCESS_DENIED);
}

/* ---- the virtual drive ---- */

static char game_root[1024], state_root[1024];
static pthread_once_t once = PTHREAD_ONCE_INIT;

static int mkdirs(const char *path)
{
    char p[1024];
    snprintf(p, sizeof p, "%s", path);
    for (char *s = p + 1; *s; s++) if (*s == '/') { *s = 0; mkdir(p, 0755); *s = '/'; }
    return mkdir(p, 0755) == 0 || errno == EEXIST;
}

static void init(void)
{
    const char *g = getenv("HALOPAD_GAME_ROOT"), *s = getenv("HALOPAD_STATE_ROOT");
    if (!g) { fprintf(stderr, "HALOPAD TRAP: HALOPAD_GAME_ROOT is not set\n"); abort(); }
    if (!s) { fprintf(stderr, "HALOPAD TRAP: HALOPAD_STATE_ROOT is not set\n"); abort(); }
    snprintf(game_root, sizeof game_root, "%s", g);
    snprintf(state_root, sizeof state_root, "%s", s);
    static const char *const folders[] = {"install", "C/Documents and Settings/Player/My Documents",
                                          "C/Documents and Settings/Player/Local Settings/Temp",
                                          "C/Documents and Settings/Player/Application Data",
                                          "C/Documents and Settings/All Users/Application Data", "C/WINDOWS/system32",
                                          /* the install directory's own entry in its parent, as on an installed
                                             machine (its contents come from the install layer, not from here) */
                                          "C/Program Files/Microsoft Games/Halo Custom Edition"};
    for (size_t i = 0; i < sizeof folders / sizeof folders[0]; i++) {
        char p[1200];
        snprintf(p, sizeof p, "%s/%s", state_root, folders[i]);
        if (!mkdirs(p)) { fprintf(stderr, "HALOPAD TRAP: cannot create %s\n", p); abort(); }
    }
}

/* Normalize a guest path to "C:\a\b" form (absolute, no . or .., single separators).
   Returns 0 for names Windows would reject or HaloPad does not model (UNC, devices). */
static int normalize(const char *in, char *out, size_t size)
{
    char tmp[1200];
    if (!in || !*in) return 0;
    if ((in[0] == '\\' || in[0] == '/') && (in[1] == '\\' || in[1] == '/'))
        hp_unsupported("file path", "UNC or device path \"%s\"", in);
    if (in[0] && in[1] == ':') snprintf(tmp, sizeof tmp, "%c:%s%s", in[0] & ~0x20, (in[2] == '\\' || in[2] == '/') ? "" : "\\", in + 2);
    else if (in[0] == '\\' || in[0] == '/') snprintf(tmp, sizeof tmp, "C:%s", in);
    else snprintf(tmp, sizeof tmp, "%s\\%s", INSTALL, in);
    char *parts[128];
    int n = 0;
    for (char *p = tmp + 2; *p; p++) if (*p == '/') *p = '\\';
    char *save = NULL;
    for (char *c = strtok_r(tmp + 2, "\\", &save); c; c = strtok_r(NULL, "\\", &save)) {
        if (!strcmp(c, ".")) continue;
        if (!strcmp(c, "..")) { if (n) n--; continue; }
        size_t l = strlen(c);
        while (l && (c[l - 1] == ' ' || c[l - 1] == '.')) c[--l] = 0;   /* Windows drops trailing dots and spaces */
        if (!l) continue;
        if (strpbrk(c, "<>:\"|")) return 0;
        static const char *const dev[] = {"CON", "PRN", "AUX", "NUL", "COM1", "COM2", "LPT1", "LPT2"};
        for (size_t d = 0; d < sizeof dev / sizeof dev[0]; d++)
            if (!strncasecmp(c, dev[d], strlen(dev[d])) && (c[strlen(dev[d])] == 0 || c[strlen(dev[d])] == '.'))
                hp_unsupported("file path", "device name \"%s\"", in);
        if (n == 128) return 0;
        parts[n++] = c;
    }
    size_t o = (size_t)snprintf(out, size, "%c:", tmp[0]);
    for (int i = 0; i < n; i++) o += (size_t)snprintf(out + o, size > o ? size - o : 0, "\\%s", parts[i]);
    if (!n) snprintf(out + o, size > o ? size - o : 0, "\\");
    return o < size;
}

/* Find 'rel' ("a\b\c") under host directory 'root', matching each component without
   regard to case. Returns 1 and the host path if it exists; else 0, with *parent set
   when everything but the last component exists (host path then ends in the name as given). */
static int find_ci(const char *root, const char *rel, char *out, size_t size, int *parent)
{
    snprintf(out, size, "%s", root);
    *parent = 0;
    if (!*rel) return 1;
    char buf[1024];
    snprintf(buf, sizeof buf, "%s", rel);
    char *save = NULL, *c = strtok_r(buf, "\\", &save);
    while (c) {
        char *next = strtok_r(NULL, "\\", &save);
        size_t len = strlen(out);
        snprintf(out + len, size - len, "/%s", c);
        struct stat st;
        if (lstat(out, &st) != 0) {
            out[len] = 0;
            DIR *d = opendir(out);
            int found = 0;
            if (d) {
                struct dirent *e;
                while ((e = readdir(d)))
                    if (!strcasecmp(e->d_name, c)) { snprintf(out + len, size - len, "/%s", e->d_name); found = 1; break; }
                closedir(d);
            }
            if (!found) {
                snprintf(out + len, size - len, "/%s", c);
                *parent = next == NULL && d != NULL;
                return 0;
            }
        }
        c = next;
    }
    return 1;
}

enum { NONE, UPPER, LOWER };
typedef struct { int where, parent_exists, isdir, in_install; char host[1024]; char upper[1024]; } node;

/* Where a normalized path lives: the state layer, or for the install directory the
   writable layer over the game files. upper is where a new or copied-up file goes. */
static int lookup(const char *guest, node *n)
{
    pthread_once(&once, init);
    char norm[1024];
    memset(n, 0, sizeof *n);
    if (!normalize(guest, norm, sizeof norm)) return 0;
    if (norm[0] != 'C') { n->where = NONE; return 1; }            /* no other drives */
    size_t il = strlen(INSTALL);
    char up[1200], rel[1024];
    int pu, pl = 0, lower_found = 0;
    char lower[1024];
    if (!strncasecmp(norm, INSTALL, il) && (norm[il] == '\\' || norm[il] == 0)) {
        n->in_install = 1;
        snprintf(rel, sizeof rel, "%s", norm[il] ? norm + il + 1 : "");
        snprintf(up, sizeof up, "%s/install", state_root);
        lower_found = find_ci(game_root, rel, lower, sizeof lower, &pl);
    } else {
        snprintf(rel, sizeof rel, "%s", norm + 3);
        snprintf(up, sizeof up, "%s/C", state_root);
    }
    int upper_found = find_ci(up, rel, n->upper, sizeof n->upper, &pu);
    struct stat st;
    if (upper_found) { n->where = UPPER; snprintf(n->host, sizeof n->host, "%s", n->upper); }
    else if (lower_found) { n->where = LOWER; snprintf(n->host, sizeof n->host, "%s", lower); }
    else n->where = NONE;
    if (n->where != NONE && stat(n->host, &st) == 0) n->isdir = S_ISDIR(st.st_mode);
    n->parent_exists = pu || pl || upper_found || lower_found;
    if (n->where == NONE && !n->parent_exists) {
        /* the parent may exist only in the other layer: then it can be created in the upper one */
        char parent_guest[1024];
        snprintf(parent_guest, sizeof parent_guest, "%s", norm);
        char *slash = strrchr(parent_guest, '\\');
        if (slash && slash > parent_guest + 2) {
            *slash = 0;
            node p;
            if (lookup(parent_guest, &p) && p.where != NONE && p.isdir) n->parent_exists = 1;
        } else if (slash) n->parent_exists = 1;
    }
    return 1;
}

/* Make n->upper usable for writing: its directories exist in the upper layer. */
static void prepare_upper(node *n)
{
    char dir[1024];
    snprintf(dir, sizeof dir, "%s", n->upper);
    char *s = strrchr(dir, '/');
    if (s) { *s = 0; mkdirs(dir); }
}

static int copy_file(const char *from, const char *to)
{
    int a = open(from, O_RDONLY), b = -1;
    if (a < 0) return 0;
    struct stat st;
    fstat(a, &st);
    b = open(to, O_WRONLY | O_CREAT | O_TRUNC, (st.st_mode & 0777) | S_IWUSR);
    if (b < 0) { close(a); return 0; }
    char buf[65536];
    ssize_t r;
    int ok = 1;
    while ((r = read(a, buf, sizeof buf)) > 0) if (write(b, buf, (size_t)r) != r) { ok = 0; break; }
    struct timespec t[2] = {st.st_atimespec, st.st_mtimespec};
    futimens(b, t);
    close(a); close(b);
    return ok && r == 0;
}

/* Copy a game file up into the writable layer before it changes. */
static int copy_up(node *n)
{
    if (n->where != LOWER) return 1;
    prepare_upper(n);
    if (n->isdir) { mkdirs(n->upper); }
    else if (!copy_file(n->host, n->upper)) return 0;
    snprintf(n->host, sizeof n->host, "%s", n->upper);
    n->where = UPPER;
    return 1;
}

/* For eula.c and the resource loader: a guest path as a host path to read. */
int halopad_host_path(const char *guest, char *out, size_t size)
{
    node n;
    if (!lookup(guest, &n) || n.where == NONE) return 0;
    snprintf(out, size, "%s", n.host);
    return 1;
}

/* ---- time conversion ---- */

static uint64_t filetime_of(struct timespec ts) { return (uint64_t)(ts.tv_sec + 11644473600LL) * 10000000ull + (uint64_t)ts.tv_nsec / 100; }
static void put_ft(uint32_t g, uint64_t ft) { if (g) { wr32(g, (uint32_t)ft); wr32(g + 4, (uint32_t)(ft >> 32)); } }
static struct timespec ts_of(uint64_t ft) { struct timespec t = {(time_t)(ft / 10000000ull) - 11644473600LL, (long)(ft % 10000000ull) * 100}; return t; }

static int in_game_root(const char *host) { size_t l = strlen(game_root); return !strncmp(host, game_root, l) && (host[l] == '/' || !host[l]); }
static int writable(const char *host, const struct stat *st) { return in_game_root(host) || (st->st_mode & S_IWUSR); }

static uint32_t attributes_of(const char *host, const struct stat *st)
{
    if (in_game_root(host)) return S_ISDIR(st->st_mode) ? 0x10u : 0x20u;   /* installed files: as Windows installed them */
    if (S_ISDIR(st->st_mode)) return 0x10u | (st->st_mode & S_IWUSR ? 0 : 0x1u);
    return 0x20u | (st->st_mode & S_IWUSR ? 0 : 0x1u);               /* ARCHIVE, READONLY */
}
#if defined(__APPLE__)
static struct timespec birth(const struct stat *st) { return st->st_birthtimespec; }
#else
static struct timespec birth(const struct stat *st) { return st->st_ctim; }
#endif

/* ---- file handles ---- */

typedef struct { int used, fd, overlapped, delete_on_close; uint32_t access; char host[1024]; } file;
static file files[MAXFILES];
static pthread_mutex_t files_lock = PTHREAD_MUTEX_INITIALIZER;

static file *F(uint32_t h)
{
    if (h < FILE_BASE || (h - FILE_BASE) % 4 || (h - FILE_BASE) / 4 >= MAXFILES) return NULL;
    file *f = &files[(h - FILE_BASE) / 4];
    return f->used ? f : NULL;
}
uint32_t halopad_file_handle_valid(uint32_t h) { return F(h) != NULL; }

uint32_t CreateFileA_c(uint32_t name, uint32_t access, uint32_t share, uint32_t security, uint32_t disp, uint32_t flags, uint32_t tmpl)
{
    (void)share;                                                    /* one process: sharing never conflicts */
    if (security) hp_unsupported("CreateFileA", "security attributes 0x%08x", security);
    if (tmpl) hp_unsupported("CreateFileA", "a template file");
    if (access & ~0xC0000000u) hp_unsupported("CreateFileA", "access 0x%08x", access);
    if (flags & ~(0x1u | 0x2u | 0x4u | 0x20u | 0x80u | 0x100u | 0x02000000u | 0x04000000u | 0x08000000u | 0x10000000u | 0x20000000u
                  | 0x40000000u | 0x80000000u))
        hp_unsupported("CreateFileA", "flags and attributes 0x%08x", flags);
    if (disp < 1 || disp > 5) return err(E_INVALID_PARAMETER), INVALID;
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME), INVALID;
    int write_access = (access & 0x40000000u) != 0;
    if (n.where != NONE && n.isdir) {
        if (!(flags & 0x02000000u)) return err(E_ACCESS_DENIED), INVALID;
        hp_unsupported("CreateFileA", "opening a directory (\"%s\")", (const char *)G(name));
    }
    int exists = n.where != NONE;
    if (!exists && !n.parent_exists) return err(E_PATH_NOT_FOUND), INVALID;
    if (disp == 1 && exists) return err(E_FILE_EXISTS), INVALID;                     /* CREATE_NEW */
    if ((disp == 3 || disp == 5) && !exists) return err(E_FILE_NOT_FOUND), INVALID;  /* OPEN_EXISTING, TRUNCATE_EXISTING */
    int truncate = disp == 2 || disp == 5;
    if (exists) {
        struct stat st;
        stat(n.host, &st);
        if ((write_access || truncate) && !writable(n.host, &st)) return err(E_ACCESS_DENIED), INVALID;   /* read-only file */
    }
    if (write_access || truncate || !exists) {
        if (exists && !truncate && !copy_up(&n)) return err_errno(), INVALID;
        if (!exists || truncate) { prepare_upper(&n); snprintf(n.host, sizeof n.host, "%s", n.upper); }
    }
    int oflags = (write_access ? ((access & 0x80000000u) ? O_RDWR : O_WRONLY) : O_RDONLY);
    if (!exists || truncate) oflags |= O_CREAT | (truncate ? O_TRUNC : 0);
    if (!write_access && (!exists || truncate)) oflags = (oflags & ~O_ACCMODE) | O_RDWR;   /* creating needs write; access checks stay ours */
    int fd = open(n.host, oflags, (flags & 0x1u) ? 0444 : 0644);
    if (fd < 0) return err_errno(), INVALID;
    pthread_mutex_lock(&files_lock);
    for (uint32_t i = 0; i < MAXFILES; i++)
        if (!files[i].used) {
            files[i] = (file){1, fd, (flags & 0x40000000u) != 0, (flags & 0x04000000u) != 0, access, {0}};
            snprintf(files[i].host, sizeof files[i].host, "%s", n.host);
            pthread_mutex_unlock(&files_lock);
            halopad_last_error = (exists && (disp == 2 || disp == 4)) ? E_ALREADY_EXISTS : 0;
            return FILE_BASE + 4 * i;
        }
    pthread_mutex_unlock(&files_lock);
    close(fd);
    hp_unsupported("CreateFileA", "more than %d open files", MAXFILES);
}

uint32_t CloseHandle_c(uint32_t h)
{
    if (halopad_is_object_handle(h)) return halopad_object_close(h);
    pthread_mutex_lock(&files_lock);
    file *f = F(h);
    if (!f) { pthread_mutex_unlock(&files_lock); return err(E_INVALID_HANDLE); }
    close(f->fd);
    if (f->delete_on_close) unlink(f->host);
    f->used = 0;
    pthread_mutex_unlock(&files_lock);
    return 1;
}

/* APCs: completion routines queued to this thread, run at its next alertable wait. */
typedef struct { uint32_t routine, error, bytes, ov; } apc;
static _Thread_local apc apcs[64];
static _Thread_local uint32_t napc;

int halopad_apc_pending(void) { return napc != 0; }
int halopad_apc_deliver(void)
{
    int n = 0;
    while (napc) {
        apc a = apcs[0];
        memmove(apcs, apcs + 1, (--napc) * sizeof apcs[0]);
        uint32_t args[3] = {a.error, a.bytes, a.ov};
        halopad_call_guest(a.routine, 3, args);
        n++;
    }
    return n;
}

static uint64_t ov_offset(uint32_t ov) { return rd32(ov + 8) | (uint64_t)rd32(ov + 12) << 32; }

/* positioned or sequential read/write; returns bytes or -1 */
static ssize_t io(file *f, int write_op, uint32_t buf, uint32_t n, uint32_t ov)
{
    if (ov) {
        off_t off = (off_t)ov_offset(ov);
        ssize_t r = write_op ? pwrite(f->fd, G(buf), n, off) : pread(f->fd, G(buf), n, off);
        if (r >= 0 && !f->overlapped) lseek(f->fd, off + r, SEEK_SET);   /* a synchronous handle's pointer moves */
        return r;
    }
    return write_op ? write(f->fd, n ? G(buf) : "", n) : read(f->fd, n ? G(buf) : NULL, n);
}

static uint32_t rw(uint32_t h, int write_op, uint32_t buf, uint32_t n, uint32_t done, uint32_t ov)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (!(f->access & (write_op ? 0x40000000u : 0x80000000u))) return err(E_ACCESS_DENIED);
    if (f->overlapped && !ov) return err(E_INVALID_PARAMETER);
    if (done) wr32(done, 0);
    if (ov && !write_op) {
        struct stat st;
        fstat(f->fd, &st);
        if (ov_offset(ov) >= (uint64_t)st.st_size && n) {
            wr32(ov, 0xC0000011u); wr32(ov + 4, 0);                 /* STATUS_END_OF_FILE */
            return err(E_HANDLE_EOF);
        }
    }
    ssize_t r = io(f, write_op, buf, n, ov);
    if (r < 0) return err_errno();
    if (done) wr32(done, (uint32_t)r);
    if (ov) {
        wr32(ov, 0); wr32(ov + 4, (uint32_t)r);                     /* Internal = STATUS_SUCCESS, InternalHigh = bytes */
        if (rd32(ov + 16)) SetEvent_c(rd32(ov + 16));
    }
    return 1;
}

uint32_t ReadFile_c(uint32_t h, uint32_t buf, uint32_t n, uint32_t done, uint32_t ov) { return rw(h, 0, buf, n, done, ov); }
uint32_t WriteFile_c(uint32_t h, uint32_t buf, uint32_t n, uint32_t done, uint32_t ov) { return rw(h, 1, buf, n, done, ov); }

uint32_t ReadFileEx_c(uint32_t h, uint32_t buf, uint32_t n, uint32_t ov, uint32_t routine)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (!f->overlapped || !ov || !routine) return err(E_INVALID_PARAMETER);
    if (!(f->access & 0x80000000u)) return err(E_ACCESS_DENIED);
    struct stat st;
    fstat(f->fd, &st);
    if (ov_offset(ov) >= (uint64_t)st.st_size) return err(E_HANDLE_EOF);
    ssize_t r = pread(f->fd, n ? G(buf) : NULL, n, (off_t)ov_offset(ov));
    if (r < 0) return err_errno();
    wr32(ov, 0); wr32(ov + 4, (uint32_t)r);
    if (napc == 64) hp_unsupported("ReadFileEx", "more than 64 completions waiting on one thread");
    apcs[napc++] = (apc){routine, 0, (uint32_t)r, ov};
    halopad_last_error = 0;
    return 1;
}

uint32_t GetOverlappedResult_c(uint32_t h, uint32_t ov, uint32_t done, uint32_t wait)
{
    (void)wait;                                                     /* operations complete when issued */
    if (!F(h)) return err(E_INVALID_HANDLE);
    if (!ov) return err(E_INVALID_PARAMETER);
    if (done) wr32(done, rd32(ov + 4));
    uint32_t status = rd32(ov);
    if (status == 0) return 1;
    return err(status == 0xC0000011u ? E_HANDLE_EOF : E_INVALID_PARAMETER);
}

uint32_t SetFilePointer_c(uint32_t h, uint32_t lo, uint32_t highp, uint32_t method)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE), INVALID;
    if (method > 2) return err(E_INVALID_PARAMETER), INVALID;
    int64_t d = highp ? (int64_t)((uint64_t)rd32(highp) << 32 | lo) : (int64_t)(int32_t)lo;
    off_t base = method == 0 ? 0 : method == 1 ? lseek(f->fd, 0, SEEK_CUR) : lseek(f->fd, 0, SEEK_END);
    if (base + d < 0) return err(E_NEGATIVE_SEEK), INVALID;
    off_t p = lseek(f->fd, base + d, SEEK_SET);
    if (p < 0) return err_errno(), INVALID;
    if (highp) wr32(highp, (uint32_t)((uint64_t)p >> 32));
    else if ((uint64_t)p > 0xFFFFFFFFull) return err(E_INVALID_PARAMETER), INVALID;
    halopad_last_error = 0;                                         /* 0xFFFFFFFF can be a real position */
    return (uint32_t)p;
}

uint32_t SetEndOfFile_c(uint32_t h)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (!(f->access & 0x40000000u)) return err(E_ACCESS_DENIED);
    if (ftruncate(f->fd, lseek(f->fd, 0, SEEK_CUR))) return err_errno();
    return 1;
}

uint32_t FlushFileBuffers_c(uint32_t h)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (!(f->access & 0x40000000u)) return err(E_ACCESS_DENIED);
    fsync(f->fd);
    return 1;
}

uint32_t GetFileSize_c(uint32_t h, uint32_t high)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE), INVALID;
    struct stat st;
    fstat(f->fd, &st);
    if (high) wr32(high, (uint32_t)((uint64_t)st.st_size >> 32));
    halopad_last_error = 0;
    return (uint32_t)st.st_size;
}

uint32_t GetFileTime_c(uint32_t h, uint32_t created, uint32_t accessed, uint32_t written)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    struct stat st;
    fstat(f->fd, &st);
    put_ft(created, filetime_of(birth(&st))); put_ft(accessed, filetime_of(st.st_atimespec)); put_ft(written, filetime_of(st.st_mtimespec));
    return 1;
}

uint32_t SetFileTime_c(uint32_t h, uint32_t created, uint32_t accessed, uint32_t written)
{
    file *f = F(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (!(f->access & 0x40000000u)) return err(E_ACCESS_DENIED);
    struct timespec t[2] = {{0, UTIME_OMIT}, {0, UTIME_OMIT}};
    if (accessed) t[0] = ts_of(rd32(accessed) | (uint64_t)rd32(accessed + 4) << 32);
    if (written) t[1] = ts_of(rd32(written) | (uint64_t)rd32(written + 4) << 32);
    if (futimens(f->fd, t)) return err_errno();
#if defined(__APPLE__)
    if (created) {
        struct attrlist al = {ATTR_BIT_MAP_COUNT, 0, ATTR_CMN_CRTIME, 0, 0, 0, 0};
        struct timespec c = ts_of(rd32(created) | (uint64_t)rd32(created + 4) << 32);
        if (fsetattrlist(f->fd, &al, &c, sizeof c, 0)) return err_errno();
    }
#else
    if (created) hp_unsupported("SetFileTime", "creation time on this platform");
#endif
    return 1;
}

/* ---- names: attributes, directories, deletion, copies ---- */

uint32_t GetFileAttributesA_c(uint32_t name)
{
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME), INVALID;
    if (n.where == NONE) return err(n.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND), INVALID;
    struct stat st;
    stat(n.host, &st);
    return attributes_of(n.host, &st);
}

uint32_t GetFileAttributesExA_c(uint32_t name, uint32_t level, uint32_t data)
{
    if (level != 0) hp_unsupported("GetFileAttributesExA", "information level %u", level);
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME);
    if (n.where == NONE) return err(n.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND);
    struct stat st;
    stat(n.host, &st);
    wr32(data, attributes_of(n.host, &st));
    put_ft(data + 4, filetime_of(birth(&st))); put_ft(data + 12, filetime_of(st.st_atimespec)); put_ft(data + 20, filetime_of(st.st_mtimespec));
    uint64_t size = S_ISDIR(st.st_mode) ? 0 : (uint64_t)st.st_size;
    wr32(data + 28, (uint32_t)(size >> 32)); wr32(data + 32, (uint32_t)size);
    return 1;
}

uint32_t SetFileAttributesA_c(uint32_t name, uint32_t attrs)
{
    if (attrs & ~(0x1u | 0x2u | 0x4u | 0x20u | 0x80u | 0x100u)) hp_unsupported("SetFileAttributesA", "attributes 0x%x", attrs);
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME);
    if (n.where == NONE) return err(n.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND);
    if (!copy_up(&n)) return err_errno();
    struct stat st;
    stat(n.host, &st);
    mode_t m = (attrs & 0x1u) ? (st.st_mode & ~0222u) : (st.st_mode | S_IWUSR);   /* READONLY; the others are not kept */
    if (chmod(n.host, m & 07777)) return err_errno();
    return 1;
}

uint32_t CreateDirectoryA_c(uint32_t name, uint32_t security)
{
    if (security) hp_unsupported("CreateDirectoryA", "security attributes 0x%08x", security);
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME);
    if (n.where != NONE) return err(E_ALREADY_EXISTS);
    if (!n.parent_exists) return err(E_PATH_NOT_FOUND);
    prepare_upper(&n);
    if (mkdir(n.upper, 0755)) return err_errno();
    return 1;
}

uint32_t RemoveDirectoryA_c(uint32_t name)
{
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME);
    if (n.where == NONE) return err(n.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND);
    if (!n.isdir) return err(267);                                  /* ERROR_DIRECTORY */
    if (n.where == LOWER) hp_unsupported("RemoveDirectoryA", "removing an installed game directory (\"%s\")", (const char *)G(name));
    if (rmdir(n.host)) return err_errno();
    return 1;
}

uint32_t DeleteFileA_c(uint32_t name)
{
    node n;
    if (!name || !lookup(G(name), &n)) return err(E_INVALID_NAME);
    if (n.where == NONE) return err(n.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND);
    if (n.isdir) return err(E_ACCESS_DENIED);
    if (n.where == LOWER) hp_unsupported("DeleteFileA", "deleting an installed game file (\"%s\")", (const char *)G(name));
    struct stat st;
    stat(n.host, &st);
    if (!writable(n.host, &st)) return err(E_ACCESS_DENIED);         /* read-only files are not deleted */
    char lower[1024];
    int pl;
    if (n.in_install) {                                             /* a copied-up game file would reappear */
        char norm[1024];
        normalize(G(name), norm, sizeof norm);
        size_t il = strlen(INSTALL);
        if (norm[il] && find_ci(game_root, norm + il + 1, lower, sizeof lower, &pl))
            hp_unsupported("DeleteFileA", "deleting an installed game file (\"%s\")", (const char *)G(name));
    }
    if (unlink(n.host)) return err_errno();
    return 1;
}

uint32_t CopyFileA_c(uint32_t from, uint32_t to, uint32_t fail_if_exists)
{
    node a, b;
    if (!from || !to || !lookup(G(from), &a) || !lookup(G(to), &b)) return err(E_INVALID_NAME);
    if (a.where == NONE) return err(a.parent_exists ? E_FILE_NOT_FOUND : E_PATH_NOT_FOUND);
    if (a.isdir) return err(E_ACCESS_DENIED);
    if (b.where != NONE && fail_if_exists) return err(E_FILE_EXISTS);
    if (b.where == NONE && !b.parent_exists) return err(E_PATH_NOT_FOUND);
    if (b.where != NONE) {
        struct stat st;
        stat(b.host, &st);
        if (b.isdir || !writable(b.host, &st)) return err(E_ACCESS_DENIED);
    }
    prepare_upper(&b);
    if (!copy_file(a.host, b.upper)) return err_errno();
    return 1;
}

/* ---- enumeration ---- */

typedef struct { char name[260]; uint32_t attrs; uint64_t c, a, w, size; } entry;
typedef struct { int used; entry *e; uint32_t n, next; } finder;
static finder finders[MAXFIND];

/* Windows wildcard match: * and ?, without regard to case; "*.*" matches every name */
static int wild(const char *p, const char *s)
{
    if (!strcmp(p, "*.*") || !strcmp(p, "*")) return 1;
    if (*p == 0) return *s == 0;
    if (*p == '*') { for (;; s++) { if (wild(p + 1, s)) return 1; if (!*s) return 0; } }
    if (!*s) return !strcmp(p, ".") || (*p == '.' && wild(p + 1, s));   /* "name.*" matches "name" */
    if (*p == '?' || tolower((unsigned char)*p) == tolower((unsigned char)*s)) return wild(p + 1, s + 1);
    return 0;
}

static int by_name(const void *x, const void *y) { return strcasecmp(((const entry *)x)->name, ((const entry *)y)->name); }

static void add_dir(const char *host, const char *pattern, entry **e, uint32_t *n, uint32_t *cap)
{
    DIR *d = opendir(host);
    if (!d) return;
    struct dirent *de;
    while ((de = readdir(d))) {
        if (!wild(pattern, de->d_name)) continue;
        int dup = 0;
        for (uint32_t i = 0; i < *n; i++) if (!strcasecmp((*e)[i].name, de->d_name)) dup = 1;
        if (dup) continue;                                          /* the upper layer's copy wins */
        char p[2048];
        snprintf(p, sizeof p, "%s/%s", host, de->d_name);
        struct stat st;
        if (stat(p, &st)) continue;
        if (*n == *cap) { *cap = *cap ? *cap * 2 : 64; *e = realloc(*e, *cap * sizeof **e); }
        entry *x = &(*e)[(*n)++];
        snprintf(x->name, sizeof x->name, "%s", de->d_name);
        x->attrs = attributes_of(p, &st);
        x->c = filetime_of(birth(&st)); x->a = filetime_of(st.st_atimespec); x->w = filetime_of(st.st_mtimespec);
        x->size = S_ISDIR(st.st_mode) ? 0 : (uint64_t)st.st_size;
    }
    closedir(d);
}

static void put_find(uint32_t data, const entry *x)
{
    memset(G(data), 0, 320);
    wr32(data, x->attrs);
    put_ft(data + 4, x->c); put_ft(data + 12, x->a); put_ft(data + 20, x->w);
    wr32(data + 28, (uint32_t)(x->size >> 32)); wr32(data + 32, (uint32_t)x->size);
    snprintf((char *)G(data + 44), 260, "%s", x->name);
}

uint32_t FindFirstFileA_c(uint32_t pattern, uint32_t data)
{
    pthread_once(&once, init);
    if (!pattern || !data) return err(E_INVALID_PARAMETER), INVALID;
    char norm[1024];
    if (!normalize(G(pattern), norm, sizeof norm)) return err(E_INVALID_NAME), INVALID;
    char *slash = strrchr(norm, '\\');
    char pat[260], dirg[1024];
    snprintf(pat, sizeof pat, "%s", slash + 1);
    *slash = 0;
    snprintf(dirg, sizeof dirg, "%s%s", norm, strlen(norm) == 2 ? "\\" : "");
    node dn;
    lookup(dirg, &dn);
    if (dn.where == NONE || !dn.isdir) return err(E_PATH_NOT_FOUND), INVALID;
    entry *e = NULL;
    uint32_t n = 0, cap = 0;
    /* the writable layer first, then the game files below it */
    if (dn.in_install) {
        char norm2[1024], upath[1200], uhost[1024], lower[1024];
        int pl;
        normalize(dirg, norm2, sizeof norm2);
        size_t il = strlen(INSTALL);
        const char *rel = norm2[il] ? norm2 + il + 1 : "";
        snprintf(upath, sizeof upath, "%s/install", state_root);
        if (find_ci(upath, rel, uhost, sizeof uhost, &pl)) add_dir(uhost, pat, &e, &n, &cap);
        if (find_ci(game_root, rel, lower, sizeof lower, &pl)) add_dir(lower, pat, &e, &n, &cap);
    } else {
        add_dir(dn.host, pat, &e, &n, &cap);
    }
    if (strlen(dirg) == 3) {                                        /* a drive root has no . or .. */
        uint32_t k = 0;
        for (uint32_t i = 0; i < n; i++) if (strcmp(e[i].name, ".") && strcmp(e[i].name, "..")) e[k++] = e[i];
        n = k;
    }
    if (!n) { free(e); return err(E_FILE_NOT_FOUND), INVALID; }
    qsort(e, n, sizeof *e, by_name);                                /* NTFS returns names in order */
    pthread_mutex_lock(&files_lock);
    for (uint32_t i = 0; i < MAXFIND; i++)
        if (!finders[i].used) {
            finders[i] = (finder){1, e, n, 1};
            pthread_mutex_unlock(&files_lock);
            put_find(data, &e[0]);
            halopad_last_error = 0;
            return FIND_BASE + 4 * i;
        }
    pthread_mutex_unlock(&files_lock);
    hp_unsupported("FindFirstFileA", "more than %d open searches", MAXFIND);
}

static finder *FD(uint32_t h)
{
    if (h < FIND_BASE || (h - FIND_BASE) % 4 || (h - FIND_BASE) / 4 >= MAXFIND) return NULL;
    finder *f = &finders[(h - FIND_BASE) / 4];
    return f->used ? f : NULL;
}

uint32_t FindNextFileA_c(uint32_t h, uint32_t data)
{
    finder *f = FD(h);
    if (!f) return err(E_INVALID_HANDLE);
    if (f->next >= f->n) return err(E_NO_MORE_FILES);
    put_find(data, &f->e[f->next++]);
    return 1;
}

uint32_t FindClose_c(uint32_t h)
{
    pthread_mutex_lock(&files_lock);
    finder *f = FD(h);
    if (!f) { pthread_mutex_unlock(&files_lock); return err(E_INVALID_HANDLE); }
    free(f->e);
    f->used = 0;
    pthread_mutex_unlock(&files_lock);
    return 1;
}

/* ---- directories Windows names ---- */

static uint32_t put_string(const char *s, uint32_t buf, uint32_t len)
{
    uint32_t n = (uint32_t)strlen(s);
    if (!buf || len <= n) return n + 1;                             /* the size needed, with the terminator */
    memcpy(G(buf), s, n + 1);
    return n;
}

uint32_t GetCurrentDirectoryA_c(uint32_t len, uint32_t buf) { return put_string(INSTALL, buf, len); }
uint32_t GetTempPathA_c(uint32_t len, uint32_t buf) { pthread_once(&once, init); return put_string(TEMP "\\", buf, len); }

uint32_t GetDiskFreeSpaceExA_c(uint32_t dir, uint32_t caller_free, uint32_t total, uint32_t total_free)
{
    pthread_once(&once, init);
    if (dir) {
        node n;
        if (!lookup(G(dir), &n) || n.where == NONE) return err(E_PATH_NOT_FOUND);
    }
    struct statfs s;
    if (statfs(state_root, &s)) return err_errno();
    uint64_t f = (uint64_t)s.f_bavail * s.f_bsize, t = (uint64_t)s.f_blocks * s.f_bsize, tf = (uint64_t)s.f_bfree * s.f_bsize;
    put_ft(caller_free, f); put_ft(total, t); put_ft(total_free, tf);
    return 1;
}

/* shfolder.dll SHGetFolderPathA */
uint32_t SHGetFolderPathA_c(uint32_t hwnd, uint32_t csidl, uint32_t token, uint32_t flags, uint32_t out)
{
    (void)hwnd;
    pthread_once(&once, init);
    if (token && token != 0xFFFFFFFFu) hp_unsupported("SHGetFolderPathA", "an access token");
    if (flags > 1) return 0x80070057u;                              /* E_INVALIDARG */
    const char *p;
    switch (csidl & 0xFF) {
    case 0x05: p = MYDOCS; break;                                   /* CSIDL_PERSONAL */
    case 0x1A: p = PROFILE "\\Application Data"; break;             /* CSIDL_APPDATA */
    case 0x1C: p = PROFILE "\\Local Settings\\Application Data"; break;   /* CSIDL_LOCAL_APPDATA */
    case 0x23: p = "C:\\Documents and Settings\\All Users\\Application Data"; break;   /* CSIDL_COMMON_APPDATA */
    case 0x24: p = "C:\\WINDOWS"; break;                            /* CSIDL_WINDOWS */
    case 0x25: p = "C:\\WINDOWS\\system32"; break;                  /* CSIDL_SYSTEM */
    case 0x26: p = "C:\\Program Files"; break;                      /* CSIDL_PROGRAM_FILES */
    case 0x28: p = PROFILE; break;                                  /* CSIDL_PROFILE */
    default: hp_unsupported("SHGetFolderPathA", "folder CSIDL 0x%x", csidl);
    }
    if (csidl & 0x8000) {                                           /* CSIDL_FLAG_CREATE */
        node n;
        lookup(p, &n);
        if (n.where == NONE) { prepare_upper(&n); mkdirs(n.upper); }
    }
    snprintf(G(out), 260, "%s", p);
    return 0;
}

/* ---- INI files (GetPrivateProfileStringA), as Windows XP reads them ----
 * Controls.dll (Keystone's controls) reads its defaults from controls\controls.ini this way.
 * A name without a drive or leading backslash is in the Windows directory. Section and key
 * names match without regard to case, the first match wins, names and values lose leading
 * and trailing blanks, and a value quoted with the same quote at both ends loses them.
 * NULL section: every section name; NULL key: every key of the section; each NUL-terminated
 * with a final second NUL (a list that does not fit is cut and double-terminated, returning
 * size - 2). A value that does not fit is cut and terminated, returning size - 1. A missing
 * file, section or key gives the default (NULL: empty) without its trailing blanks. */
static void ini_trim(const char **s, const char **e)
{
    while (*s < *e && (**s == ' ' || **s == '\t')) (*s)++;
    while (*e > *s && ((*e)[-1] == ' ' || (*e)[-1] == '\t' || (*e)[-1] == '\r')) (*e)--;
}
static int ini_eq(const char *s, const char *e, const char *want)
{
    size_t n = (size_t)(e - s);
    return strlen(want) == n && !strncasecmp(s, want, n);
}
static uint32_t ini_copy_list(const char *list, size_t len, uint32_t buf, uint32_t size)
{
    char *out = G(buf);
    if (size < 2) { if (size) out[0] = 0; return 0; }
    if (len + 1 <= size) { memcpy(out, list, len); out[len] = 0; return len ? (uint32_t)len - 1 : 0; }
    memcpy(out, list, size - 2);
    out[size - 2] = 0; out[size - 1] = 0;
    return size - 2;
}
uint32_t GetPrivateProfileStringA_c(uint32_t app, uint32_t key, uint32_t def, uint32_t buf, uint32_t size, uint32_t file)
{
    pthread_once(&once, init);
    if (!buf || !size) return 0;
    char path[1024];
    const char *f = file ? (const char *)G(file) : "win.ini";
    int absolute = (f[0] && f[1] == ':') || f[0] == '\\' || f[0] == '/';
    snprintf(path, sizeof path, absolute ? "%s" : "C:\\WINDOWS\\%s", f);
    char *text = NULL;
    size_t tlen = 0;
    node n;
    if (lookup(path, &n) && n.where != NONE && !n.isdir) {
        FILE *fp = fopen(n.host, "rb");
        if (fp) {
            fseek(fp, 0, SEEK_END); long sz = ftell(fp); fseek(fp, 0, SEEK_SET);
            text = malloc((size_t)sz + 1);
            tlen = fread(text, 1, (size_t)sz, fp);
            text[tlen] = 0;
            fclose(fp);
        }
    }
    const char *want_app = app ? (const char *)G(app) : NULL, *want_key = key ? (const char *)G(key) : NULL;
    char *list = malloc(tlen + 2);
    size_t llen = 0;
    int in_section = 0, section_seen = 0, found = 0;
    const char *vs = NULL, *ve = NULL;
    for (const char *p = text; p && *p && !found; ) {
        const char *le = strchr(p, '\n');
        if (!le) le = p + strlen(p);
        const char *s = p, *e = le;
        ini_trim(&s, &e);
        if (s < e && *s == '[') {
            const char *ns = s + 1, *ne = memchr(ns, ']', (size_t)(e - ns));
            if (!ne) ne = e;
            ini_trim(&ns, &ne);
            if (!want_app) { memcpy(list + llen, ns, (size_t)(ne - ns)); llen += (size_t)(ne - ns); list[llen++] = 0; in_section = 0; }
            else {
                if (in_section) break;                              /* the first matching section only */
                in_section = !section_seen && ini_eq(ns, ne, want_app);
                section_seen |= in_section;
            }
        } else if (in_section && s < e) {
            const char *eq = memchr(s, '=', (size_t)(e - s));
            const char *ks = s, *ke = eq ? eq : e;
            ini_trim(&ks, &ke);
            if (!want_key) { memcpy(list + llen, ks, (size_t)(ke - ks)); llen += (size_t)(ke - ks); list[llen++] = 0; }
            else if (ini_eq(ks, ke, want_key)) {
                found = 1;
                vs = eq ? eq + 1 : e; ve = e;
                ini_trim(&vs, &ve);
                if (ve - vs >= 2 && (*vs == '"' || *vs == '\'') && ve[-1] == *vs) { vs++; ve--; }
            }
        }
        p = *le ? le + 1 : le;
    }
    uint32_t r;
    if (!want_app || !want_key) {
        list[llen++] = 0;                                           /* the list's final NUL */
        r = ini_copy_list(list, llen, buf, size);
    } else {
        char *out = G(buf);
        const char *s = vs, *e = ve;
        if (!found) {
            s = def ? (const char *)G(def) : "";
            e = s + strlen(s);
            while (e > s && e[-1] == ' ') e--;
        }
        size_t len = (size_t)(e - s);
        if (len > size - 1) len = size - 1;
        memcpy(out, s, len);
        out[len] = 0;
        r = (uint32_t)len;
    }
    free(list);
    free(text);
    halopad_last_error = 0;
    return r;
}

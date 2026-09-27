/* Files, time and locale test (G3): the virtual C: drive over the real game files, with a
 * fresh writable state directory; overlapped reads with a ReadFileEx completion routine
 * run at an alertable SleepEx; time, date and locale formatting; system messages. Calls go
 * through the guest addresses GetProcAddress gives.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_files_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
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

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static char *P(uint32_t g) { return halopad_guest_ptr(g); }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-70s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static void checks(const char *what, const char *got, const char *want)
{
    int ok = !strcmp(got, want);
    printf("%-70s %s (got \"%s\", want \"%s\")\n", what, ok ? "PASS" : "FAIL", got, want);
    failures += !ok;
}
static uint32_t k32, shf;
static uint32_t proc(const char *n) { uint32_t va = GetProcAddress_c(strcmp(n, "SHGetFolderPathA") ? k32 : shf, str(n)); if (!va) { printf("no %s\n", n); exit(2); } return va; }
static uint32_t api(const char *name, uint32_t n, const uint32_t *args) { return halopad_call_guest(proc(name), n, args); }
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define LE() api("GetLastError", 0, NULL)
#define GR 0x80000000u
#define GW 0x40000000u
static const char *mygames = "C:\\Documents and Settings\\Player\\My Documents\\My Games\\Halo CE";
static char path_buf[512];
static uint32_t in_mg(const char *name) { snprintf(path_buf, sizeof path_buf, "%s\\%s", mygames, name); return str(path_buf); }
static uint32_t buf, n4;

static uint32_t st(int y, int m, int d, int h, int mi, int s)
{
    uint32_t g = halopad_heap_alloc(16, 1);
    uint16_t v[8] = {(uint16_t)y, (uint16_t)m, 0, (uint16_t)d, (uint16_t)h, (uint16_t)mi, (uint16_t)s, 0};
    memcpy(P(g), v, 16);
    return g;
}

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    char state[] = "/tmp/halopad-files-XXXXXX";
    if (!mkdtemp(state)) return 2;
    setenv("HALOPAD_STATE_ROOT", state, 1);
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
    shf = LoadLibraryA_c(str("shfolder.dll"));
    buf = halopad_heap_alloc(4096, 1);
    n4 = halopad_heap_alloc(4, 1);
    const char *game = getenv("HALOPAD_GAME_ROOT");

    /* folders */
    check("SHGetFolderPathA(CSIDL_PERSONAL)", API("SHGetFolderPathA", 0, 5, 0, 0, buf), 0);
    checks("  My Documents", P(buf), "C:\\Documents and Settings\\Player\\My Documents");
    check("CreateDirectoryA My Games\\Halo CE with My Games missing: ERROR_PATH_NOT_FOUND",
          API("CreateDirectoryA", str(mygames), 0) == 0 && LE() == 3, 1);
    check("CreateDirectoryA My Games, then Halo CE",
          API("CreateDirectoryA", str("C:\\Documents and Settings\\Player\\My Documents\\My Games"), 0) && API("CreateDirectoryA", str(mygames), 0), 1);
    check("  again: ERROR_ALREADY_EXISTS", API("CreateDirectoryA", str(mygames), 0) == 0 && LE() == 183, 1);
    API("GetCurrentDirectoryA", 260, buf);
    checks("GetCurrentDirectoryA: the install directory", P(buf), "C:\\Program Files\\Microsoft Games\\Halo Custom Edition");
    uint32_t tl = API("GetTempPathA", 260, buf);
    check("GetTempPathA ends with a backslash", tl > 3 && P(buf)[tl - 1] == '\\', 1);
    check("GetTempPathA with a small buffer: the size needed", API("GetTempPathA", 4, buf), tl + 1);

    /* reading the game files, without regard to case */
    check("GetFileAttributesA(\"MAPS\\\\BLOODGULCH.MAP\") (relative, other case): a file",
          API("GetFileAttributesA", str("MAPS\\BLOODGULCH.MAP")), 0x20);
    check("GetFileAttributesA(\"maps\"): a directory", API("GetFileAttributesA", str("maps")) & 0x10, 0x10);
    check("GetFileAttributesA of a missing file: ERROR_FILE_NOT_FOUND", API("GetFileAttributesA", str("maps\\nothing.map")) == 0xFFFFFFFF && LE() == 2, 1);
    check("  in a missing directory: ERROR_PATH_NOT_FOUND", API("GetFileAttributesA", str("nodir\\nothing.map")) == 0xFFFFFFFF && LE() == 3, 1);
    uint32_t h = API("CreateFileA", str("maps\\bloodgulch.map"), GR, 1, 0, 3, 0x08000000, 0);
    check("CreateFileA(maps\\bloodgulch.map, OPEN_EXISTING, SEQUENTIAL_SCAN)", h != 0xFFFFFFFF, 1);
    char hp[1024]; snprintf(hp, sizeof hp, "%s/maps/bloodgulch.map", game);
    struct stat hs; stat(hp, &hs);
    check("  GetFileSize matches the host file", API("GetFileSize", h, 0), (uint32_t)hs.st_size);
    check("  ReadFile 4 bytes: the map header tag 'daeh'", API("ReadFile", h, buf, 4, n4, 0) && rd(n4) == 4 && !memcmp(P(buf), "daeh", 4), 1);
    check("  WriteFile on a read handle: ERROR_ACCESS_DENIED", API("WriteFile", h, buf, 4, n4, 0) == 0 && LE() == 5, 1);
    check("  CloseHandle", API("CloseHandle", h), 1);

    /* writing in My Games */
    h = API("CreateFileA", in_mg("test.txt"), GW, 0, 0, 2, 0x80, 0);
    check("CreateFileA(CREATE_ALWAYS) a new file: last error 0", h != 0xFFFFFFFF && LE() == 0, 1);
    check("  WriteFile \"hello world\"", API("WriteFile", h, str("hello world"), 11, n4, 0) && rd(n4) == 11, 1);
    check("  SetFilePointer(6, FILE_BEGIN)", API("SetFilePointer", h, 6, 0, 0), 6);
    API("WriteFile", h, str("HALO!"), 5, n4, 0);
    check("  SetFilePointer(-3, FILE_END)", API("SetFilePointer", h, (uint32_t)-3, 0, 2), 8);
    check("  SetFilePointer before the start: ERROR_NEGATIVE_SEEK", API("SetFilePointer", h, (uint32_t)-100, 0, 1) == 0xFFFFFFFF && LE() == 131, 1);
    API("CloseHandle", h);
    h = API("CreateFileA", in_mg("TEST.TXT"), GR, 1, 0, 3, 0x80, 0);
    memset(P(buf), 0, 64);
    API("ReadFile", h, buf, 64, n4, 0);
    checks("reopened (other case): \"hello HALO!\"", P(buf), "hello HALO!");
    check("  ReadFile at the end: TRUE, 0 bytes", API("ReadFile", h, buf, 64, n4, 0) == 1 && rd(n4) == 0, 1);
    API("CloseHandle", h);
    check("CREATE_NEW on an existing file: ERROR_FILE_EXISTS", API("CreateFileA", in_mg("test.txt"), GW, 0, 0, 1, 0x80, 0) == 0xFFFFFFFF && LE() == 80, 1);
    h = API("CreateFileA", in_mg("test.txt"), GR | GW, 0, 0, 4, 0x80, 0);
    check("OPEN_ALWAYS on an existing file: ERROR_ALREADY_EXISTS, a handle", h != 0xFFFFFFFF && LE() == 183, 1);
    API("SetFilePointer", h, 5, 0, 0);
    check("  SetEndOfFile at 5", API("SetEndOfFile", h) == 1 && API("GetFileSize", h, 0) == 5, 1);
    uint32_t ftw = halopad_heap_alloc(8, 1);
    check("  GetFileTime", API("GetFileTime", h, 0, 0, ftw) == 1 && rd(ftw + 4) > 0x01D00000, 1);
    API("CloseHandle", h);
    h = API("CreateFileA", in_mg("test.txt"), GW, 0, 0, 5, 0x80, 0);
    check("TRUNCATE_EXISTING: size 0", API("GetFileSize", h, 0), 0);
    API("WriteFile", h, str("abc"), 3, n4, 0);
    API("CloseHandle", h);
    check("CopyFileA(test.txt -> copy.txt, fail if exists)", API("CopyFileA", in_mg("test.txt"), in_mg("copy.txt"), 1), 1);
    check("  again: ERROR_FILE_EXISTS", API("CopyFileA", in_mg("test.txt"), in_mg("copy.txt"), 1) == 0 && LE() == 80, 1);
    uint32_t fad = halopad_heap_alloc(36, 1);
    check("GetFileAttributesExA(copy.txt): 3 bytes", API("GetFileAttributesExA", in_mg("copy.txt"), 0, fad) == 1 && rd(fad + 32) == 3, 1);

    /* enumeration */
    uint32_t fd = halopad_heap_alloc(320, 1);
    snprintf(path_buf, sizeof path_buf, "%s\\*.*", mygames);
    uint32_t fh = API("FindFirstFileA", str(path_buf), fd);
    char names[512] = "";
    if (fh != 0xFFFFFFFF) { do { strcat(names, P(fd + 44)); strcat(names, "|"); } while (API("FindNextFileA", fh, fd)); }
    checks("FindFirstFileA(\"*.*\"): ., .., then names in order", names, ".|..|copy.txt|test.txt|");
    check("  FindNextFileA at the end: ERROR_NO_MORE_FILES", LE(), 18);
    API("FindClose", fh);
    snprintf(path_buf, sizeof path_buf, "%s\\*.TXT", mygames);
    fh = API("FindFirstFileA", str(path_buf), fd);
    check("FindFirstFileA(\"*.TXT\") matches without regard to case", fh != 0xFFFFFFFF && !strcmp(P(fd + 44), "copy.txt"), 1);
    API("FindClose", fh);
    fh = API("FindFirstFileA", str("maps\\*.map"), fd);
    int maps = 0, has_bg = 0;
    if (fh != 0xFFFFFFFF) { do { maps++; has_bg |= !strcmp(P(fd + 44), "bloodgulch.map"); } while (API("FindNextFileA", fh, fd)); API("FindClose", fh); }
    check("FindFirstFileA(\"maps\\\\*.map\") lists the game's maps, bloodgulch.map among them", maps > 10 && has_bg, 1);
    /* Keystone.dll checks its content directory this way (KeystoneCreate) */
    fh = API("FindFirstFileA", str("C:\\Program Files\\Microsoft Games\\Halo Custom Edition"), fd);
    check("FindFirstFileA(install directory, no wildcard): its own entry, a directory",
          fh != 0xFFFFFFFF && !strcmp(P(fd + 44), "Halo Custom Edition") && (rd(fd) & 0x10), 1);
    if (fh != 0xFFFFFFFF) API("FindClose", fh);
    snprintf(path_buf, sizeof path_buf, "%s\\*.none", mygames);
    check("FindFirstFileA with no match: ERROR_FILE_NOT_FOUND", API("FindFirstFileA", str(path_buf), fd) == 0xFFFFFFFF && LE() == 2, 1);

    /* INI files (Controls.dll reads controls\controls.ini this way) */
    {
        static const char ini[] = "[a]\r\nfont=font://Verdana-10-underline\r\ncolor = #FF3399FF \r\n[Body]\r\nx=1\r\nx=2\r\nq=\"quoted value\"\r\nbare\r\n[a]\r\nlate=1\r\n";
        uint32_t h = API("CreateFileA", in_mg("test.ini"), GW, 0, 0, 2, 0x80, 0);
        API("WriteFile", h, str(ini), (uint32_t)strlen(ini), n4, 0);
        API("CloseHandle", h);
        char inipath[512]; snprintf(inipath, sizeof inipath, "%s\\test.ini", mygames);
        uint32_t fp = str(inipath), out = halopad_heap_alloc(64, 1);
        #define GPS(app, key, def, size) API("GetPrivateProfileStringA", app, key, def, out, size, fp)
        check("GetPrivateProfileStringA: a value", GPS(str("a"), str("font"), 0, 64) == 27 && !strcmp(P(out), "font://Verdana-10-underline"), 1);
        check("  blanks around key and value are dropped", GPS(str("A"), str("COLOR"), 0, 64) == 9 && !strcmp(P(out), "#FF3399FF"), 1);
        check("  the first of two equal keys", GPS(str("body"), str("x"), 0, 64) == 1 && !strcmp(P(out), "1"), 1);
        check("  surrounding quotes are dropped", GPS(str("body"), str("q"), 0, 64) == 12 && !strcmp(P(out), "quoted value"), 1);
        check("  a key in a repeated section is not seen (first section only)", GPS(str("a"), str("late"), str("dflt  "), 64) == 4 && !strcmp(P(out), "dflt"), 1);
        check("  missing key, NULL default: empty", GPS(str("a"), str("none"), 0, 64) == 0 && P(out)[0] == 0, 1);
        check("  a value that does not fit: cut, size - 1", GPS(str("a"), str("font"), 0, 5) == 4 && !strcmp(P(out), "font"), 1);
        memset(P(out), 'x', 64);
        uint32_t nsec = GPS(0, str("x"), 0, 64);
        check("  NULL section: every section name, double-NUL terminated", nsec == 9 && !memcmp(P(out), "a\0Body\0a\0\0", 10), 1);
        uint32_t nkey = GPS(str("body"), 0, 0, 64);
        check("  NULL key: every key of the section", nkey == 11 && !memcmp(P(out), "x\0x\0q\0bare\0\0", 12), 1);
        check("  a list that does not fit: size - 2", GPS(str("body"), 0, 0, 5) == 3 && !memcmp(P(out), "x\0x\0\0", 5), 1);
        check("  missing file: the default", API("GetPrivateProfileStringA", str("a"), str("b"), str("d"), out, 64, str("C:\\none.ini")) == 1 && !strcmp(P(out), "d"), 1);
        #undef GPS
        API("DeleteFileA", in_mg("test.ini"));
    }

    /* read-only, deletion, directories */
    check("SetFileAttributesA(READONLY)", API("SetFileAttributesA", in_mg("copy.txt"), 1) == 1 && (API("GetFileAttributesA", in_mg("copy.txt")) & 1), 1);
    check("  opening it for writing: ERROR_ACCESS_DENIED", API("CreateFileA", in_mg("copy.txt"), GW, 0, 0, 3, 0x80, 0) == 0xFFFFFFFF && LE() == 5, 1);
    check("  DeleteFileA: ERROR_ACCESS_DENIED", API("DeleteFileA", in_mg("copy.txt")) == 0 && LE() == 5, 1);
    API("SetFileAttributesA", in_mg("copy.txt"), 0x80);
    check("  once writable again, DeleteFileA", API("DeleteFileA", in_mg("copy.txt")), 1);
    check("DeleteFileA of a missing file: ERROR_FILE_NOT_FOUND", API("DeleteFileA", in_mg("copy.txt")) == 0 && LE() == 2, 1);
    check("RemoveDirectoryA of a non-empty directory: ERROR_DIR_NOT_EMPTY", API("RemoveDirectoryA", str(mygames)) == 0 && LE() == 145, 1);
    API("DeleteFileA", in_mg("test.txt"));
    check("  once empty, RemoveDirectoryA", API("RemoveDirectoryA", str(mygames)), 1);

    /* the install directory: new files and copied-up game files go to the writable layer */
    h = API("CreateFileA", str("debug.txt"), GW, 1, 0, 2, 0x80, 0);
    API("WriteFile", h, str("log"), 3, n4, 0);
    API("CloseHandle", h);
    char gp[1024], sp[1024]; struct stat x;
    snprintf(gp, sizeof gp, "%s/debug.txt", game); snprintf(sp, sizeof sp, "%s/install/debug.txt", state);
    check("a new file in the install directory lands in the writable layer, not the game files", stat(sp, &x) == 0 && stat(gp, &x) != 0, 1);
    /* a small installed file to change */
    char small[512] = "";
    DIR *d = opendir(game);
    struct dirent *de;
    while (d && (de = readdir(d))) {
        char q[1024]; snprintf(q, sizeof q, "%s/%s", game, de->d_name);
        if (de->d_name[0] != '.' && stat(q, &x) == 0 && S_ISREG(x.st_mode) && x.st_size > 16 && x.st_size < 200000) { snprintf(small, sizeof small, "%s", de->d_name); break; }
    }
    if (d) closedir(d);
    snprintf(gp, sizeof gp, "%s/%s", game, small);
    FILE *gf = fopen(gp, "rb"); unsigned char orig[4] = {0}; fread(orig, 1, 4, gf); fclose(gf);
    h = API("CreateFileA", str(small), GR | GW, 1, 0, 3, 0x80, 0);
    API("WriteFile", h, str("ZZZZ"), 4, n4, 0);
    API("CloseHandle", h);
    gf = fopen(gp, "rb"); unsigned char after[4] = {0}; fread(after, 1, 4, gf); fclose(gf);
    h = API("CreateFileA", str(small), GR, 1, 0, 3, 0x80, 0);
    API("ReadFile", h, buf, 4, n4, 0);
    API("CloseHandle", h);
    printf("    (installed file used: %s)\n", small);
    check("changing an installed file: the game sees the change", memcmp(P(buf), "ZZZZ", 4), 0);
    check("  and the game files on disk are untouched", memcmp(orig, after, 4), 0);

    /* overlapped reads: ReadFileEx completes through an APC at an alertable wait */
    h = API("CreateFileA", str("maps\\bloodgulch.map"), GR, 1, 0, 3, 0x40000000, 0);
    uint32_t ov = halopad_heap_alloc(20, 1);
    check("overlapped handle: ReadFile without an OVERLAPPED is invalid", API("ReadFile", h, buf, 4, n4, 0) == 0 && LE() == 87, 1);
    memcpy(P(ov + 8), (uint32_t[]){0}, 4);
    API("SetLastError", 0);
    check("ReadFileEx(offset 0, 4 bytes, completion routine)", API("ReadFileEx", h, buf, 4, ov, proc("CopyFileA")), 1);
    check("  the data is there, OVERLAPPED.InternalHigh = 4", !memcmp(P(buf), "daeh", 4) && rd(ov + 4) == 4, 1);
    check("  the routine has not run yet", LE(), 0);
    check("  SleepEx(0, TRUE) runs it: WAIT_IO_COMPLETION", API("SleepEx", 0, 1), 0xC0);
    check("  (the routine, CopyFileA(0, 4, ov), left ERROR_INVALID_NAME)", LE(), 123);
    check("  nothing more queued: SleepEx(0, TRUE) returns 0", API("SleepEx", 0, 1), 0);
    memcpy(P(ov + 8), (uint32_t[]){(uint32_t)hs.st_size + 10}, 4);
    check("ReadFileEx past the end: ERROR_HANDLE_EOF", API("ReadFileEx", h, buf, 4, ov, proc("CopyFileA")) == 0 && LE() == 38, 1);
    uint32_t ev = API("CreateEventA", 0, 1, 0, 0);
    memset(P(ov), 0, 20);
    memcpy(P(ov + 8), (uint32_t[]){2048}, 4);
    memcpy(P(ov + 16), &ev, 4);
    check("ReadFile with an OVERLAPPED at offset 2048", API("ReadFile", h, buf, 16, 0, ov), 1);
    check("  its event is set", API("WaitForSingleObject", ev, 0), 0);
    check("  GetOverlappedResult: 16 bytes", API("GetOverlappedResult", h, ov, n4, 1) == 1 && rd(n4) == 16, 1);
    API("CloseHandle", h);

    /* time */
    uint32_t s = halopad_heap_alloc(16, 1), ft = halopad_heap_alloc(8, 1);
    API("GetSystemTime", s);
    check("GetSystemTime: this year", ((uint16_t *)P(s))[0] >= 2026, 1);
    check("SystemTimeToFileTime(2000-01-01)", API("SystemTimeToFileTime", st(2000, 1, 1, 0, 0, 0), ft) == 1 && rd(ft) == 0x256D4000 && rd(ft + 4) == 0x01BF53EB, 1);
    check("SystemTimeToFileTime(February 30): invalid", API("SystemTimeToFileTime", st(2004, 2, 30, 0, 0, 0), ft), 0);
    uint32_t tz = halopad_heap_alloc(172, 1);
    uint32_t tzr = API("GetTimeZoneInformation", tz);
    time_t now = time(NULL); struct tm lt; localtime_r(&now, &lt);
    check("GetTimeZoneInformation: bias + daylight bias gives the host's offset now",
          tzr <= 2 && (int32_t)(rd(tz) + (tzr == 2 ? rd(tz + 168) : 0)) == (int32_t)(-lt.tm_gmtoff / 60), 1);
    API("GetDateFormatA", 0x400, 1, st(2004, 6, 1, 13, 5, 9), 0, buf, 32);
    checks("GetDateFormatA(DATE_SHORTDATE)", P(buf), "6/1/2004");
    API("GetDateFormatA", 0x400, 2, st(2004, 6, 1, 13, 5, 9), 0, buf, 64);
    checks("GetDateFormatA(DATE_LONGDATE)", P(buf), "Tuesday, June 01, 2004");
    API("GetTimeFormatA", 0x400, 0, st(2004, 6, 1, 13, 5, 9), 0, buf, 32);
    checks("GetTimeFormatA", P(buf), "1:05:09 PM");
    API("GetTimeFormatA", 0x400, 0xC, st(2004, 6, 1, 13, 5, 9), 0, buf, 32);
    checks("GetTimeFormatA(NOTIMEMARKER | FORCE24HOURFORMAT), as Halo asks", P(buf), "13:05:09");
    API("GetTimeFormatA", 0x400, 0, st(2004, 6, 1, 13, 5, 9), str("HH'h'mm"), buf, 32);
    checks("GetTimeFormatA with a picture and a quoted literal", P(buf), "13h05");
    check("GetDateFormatA with a 4-byte buffer: ERROR_INSUFFICIENT_BUFFER", API("GetDateFormatA", 0x400, 1, st(2004, 6, 1, 0, 0, 0), 0, buf, 4) == 0 && LE() == 122, 1);

    /* locale */
    check("GetLocaleInfoA(LOCALE_IDEFAULTANSICODEPAGE)", API("GetLocaleInfoA", 0x400, 0x1004, buf, 16) == 5 && !strcmp(P(buf), "1252"), 1);
    check("  with LOCALE_RETURN_NUMBER", API("GetLocaleInfoA", 0x409, 0x20001004, buf, 16) == 4 && rd(buf) == 1252, 1);
    check("  size query", API("GetLocaleInfoA", 0x409, 0x2A, 0, 0), 7);
    API("GetLocaleInfoW", 0x409, 0x38, buf, 16);
    check("GetLocaleInfoW(LOCALE_SMONTHNAME1)", ((uint16_t *)P(buf))[0] == 'J' && ((uint16_t *)P(buf))[6] == 'y', 1);
    check("GetUserDefaultLCID", api("GetUserDefaultLCID", 0, NULL), 0x409);
    check("CompareStringA(IGNORECASE, \"abc\", \"ABD\"): less", API("CompareStringA", 0x409, 1, str("abc"), (uint32_t)-1, str("ABD"), (uint32_t)-1), 1);
    check("CompareStringA(\"a\", \"A\"): lowercase first", API("CompareStringA", 0x409, 0, str("a"), (uint32_t)-1, str("A"), (uint32_t)-1), 1);
    check("CompareStringA(IGNORECASE, \"Halo\", \"hALO\"): equal", API("CompareStringA", 0x409, 1, str("Halo"), 4, str("hALO"), 4), 2);

    /* messages and memory */
    check("FormatMessageA(FROM_SYSTEM|IGNORE_INSERTS|MAX_WIDTH, ERROR_FILE_NOT_FOUND)", API("FormatMessageA", 0x12FF, 0, 2, 0, buf, 0x800, 0), 43);
    checks("  text on one line", P(buf), "The system cannot find the file specified. ");
    check("FormatMessageA of a Direct3D code: ERROR_MR_MID_NOT_FOUND", API("FormatMessageA", 0x12FF, 0, 0x8876086C, 0, buf, 0x800, 0) == 0 && LE() == 317, 1);
    check("FormatMessageA(ALLOCATE_BUFFER)", API("FormatMessageA", 0x1300, 0, 5, 0x400, buf, 0, 0) == 19 && !strcmp(P(rd(buf)), "Access is denied.\r\n"), 1);
    uint32_t ms = halopad_heap_alloc(32, 1);
    API("GlobalMemoryStatus", ms);
    check("GlobalMemoryStatus: 32 bytes, a 2 GB address space", rd(ms) == 32 && rd(ms + 24) == 0x7FFE0000 && rd(ms + 8) > 0, 1);
    check("IsBadReadPtr(NULL, 4)", API("IsBadReadPtr", 0, 4), 1);
    check("IsBadReadPtr(a heap block, 16)", API("IsBadReadPtr", buf, 16), 0);

    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

/* Console, version, security and clipboard test (G3): the console calls fail as in a GUI
 * process; the version resource of haloce.exe through GetFileVersionInfoA/VerQueryValueA
 * (1.0.10.621); the security API step by step and then Halo's own "is the player a local
 * administrator" function (0x545c50) run from translated code; clipboard open/close rules.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_misc_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t bytes(const void *p, uint32_t n) { uint32_t g = halopad_heap_alloc(n, 0); memcpy(halopad_guest_ptr(g), p, n); return g; }
static char *P(uint32_t g) { return halopad_guest_ptr(g); }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static uint32_t mods[5];
static uint32_t api(const char *name, uint32_t n, const uint32_t *args)
{
    uint32_t va = 0;
    for (int i = 0; i < 5 && !va; i++) {
        static const char *const in[5][12] = {
            {"GetStdHandle", "GetNumberOfConsoleInputEvents", "GetModuleFileNameA", "GetLastError", "SetLastError", "CloseHandle"},
            {"GetFileVersionInfoSizeA", "GetFileVersionInfoA", "VerQueryValueA"},
            {"OpenThreadToken", "OpenProcessToken", "DuplicateToken", "AllocateAndInitializeSid", "FreeSid", "GetLengthSid",
             "InitializeSecurityDescriptor", "IsValidSecurityDescriptor", "SetSecurityDescriptorDacl", "InitializeAcl", "AddAccessAllowedAce", "AccessCheck"},
            {"OpenClipboard", "CloseClipboard", "IsClipboardFormatAvailable"}, {0}};
        for (int k = 0; k < 12 && in[i][k]; k++) if (!strcmp(in[i][k], name)) va = GetProcAddress_c(mods[i], str(name));
    }
    if (!va) { printf("no export %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define API(name, ...) api(name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define API0(name) api(name, 0, NULL)

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
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
    mods[0] = LoadLibraryA_c(str("kernel32.dll")); mods[1] = LoadLibraryA_c(str("version.dll"));
    mods[2] = LoadLibraryA_c(str("advapi32.dll")); mods[3] = LoadLibraryA_c(str("user32.dll"));
    uint32_t n = halopad_heap_alloc(4, 1);

    /* no console */
    uint32_t in = API("GetStdHandle", 0xFFFFFFF6);
    check("GetNumberOfConsoleInputEvents on the input handle: ERROR_INVALID_HANDLE",
          API("GetNumberOfConsoleInputEvents", in, n) == 0 && API0("GetLastError") == 6, 1);

    /* the version resource of haloce.exe */
    uint32_t path = halopad_heap_alloc(260, 1);
    API("GetModuleFileNameA", 0, path, 260);
    printf("    (module: %s)\n", P(path));
    uint32_t size = API("GetFileVersionInfoSizeA", path, n);
    check("GetFileVersionInfoSizeA(haloce.exe) > 0", size > 0, 1);
    uint32_t vi = halopad_heap_alloc(size, 1), pv = halopad_heap_alloc(4, 1), lv = halopad_heap_alloc(4, 1);
    check("GetFileVersionInfoA", API("GetFileVersionInfoA", path, 0, size, vi), 1);
    check("VerQueryValueA(\"\\\\\"): VS_FIXEDFILEINFO", API("VerQueryValueA", vi, str("\\"), pv, lv) == 1 && rd(lv) == 52 && rd(rd(pv)) == 0xFEEF04BD, 1);
    check("  file version 1.0.10.621", rd(rd(pv) + 8) == 0x00010000 && rd(rd(pv) + 12) == 0x000A026D, 1);
    check("VerQueryValueA(\"\\\\VarFileInfo\\\\Translation\")", API("VerQueryValueA", vi, str("\\VarFileInfo\\Translation"), pv, lv) == 1 && rd(lv) >= 4, 1);
    uint32_t tr = rd(rd(pv));
    char sub[80];
    snprintf(sub, sizeof sub, "\\StringFileInfo\\%04x%04x\\FileVersion", tr & 0xFFFF, tr >> 16);
    check("VerQueryValueA(FileVersion) as ANSI text", API("VerQueryValueA", vi, str(sub), pv, lv) == 1 && strlen(P(rd(pv))) + 1 == rd(lv), 1);
    printf("    (FileVersion: \"%s\")\n", P(rd(pv)));
    check("  a missing value: fails", API("VerQueryValueA", vi, str("\\StringFileInfo\\nothing"), pv, lv), 0);

    /* the security API step by step */
    uint32_t tok = halopad_heap_alloc(4, 1), imp = halopad_heap_alloc(4, 1), sid = halopad_heap_alloc(4, 1);
    check("OpenThreadToken when not impersonating: ERROR_NO_TOKEN", API("OpenThreadToken", 0xFFFFFFFE, 0xA, 1, tok) == 0 && API0("GetLastError") == 1008, 1);
    check("OpenProcessToken", API("OpenProcessToken", 0xFFFFFFFF, 0xA, tok), 1);
    check("DuplicateToken(SecurityImpersonation)", API("DuplicateToken", rd(tok), 2, imp), 1);
    static const uint8_t nt[6] = {0, 0, 0, 0, 0, 5};
    check("AllocateAndInitializeSid(S-1-5-32-544)", API("AllocateAndInitializeSid", bytes(nt, 6), 2, 32, 544, 0, 0, 0, 0, 0, 0, sid), 1);
    check("  GetLengthSid: 16", API("GetLengthSid", rd(sid)), 16);
    uint32_t sd = halopad_heap_alloc(20, 1), acl = halopad_heap_alloc(64, 1);
    API("InitializeSecurityDescriptor", sd, 1);
    API("InitializeAcl", acl, 64, 2);
    check("AddAccessAllowedAce(Administrators, 1|2)", API("AddAccessAllowedAce", acl, 2, 3, rd(sid)), 1);
    API("SetSecurityDescriptorDacl", sd, 1, acl, 0);
    check("IsValidSecurityDescriptor", API("IsValidSecurityDescriptor", sd), 1);
    uint32_t map = bytes((uint32_t[]){1, 2, 0, 3}, 16), ps = halopad_heap_alloc(20, 1), pl = bytes((uint32_t[]){20}, 4);
    uint32_t granted = halopad_heap_alloc(4, 1), status = halopad_heap_alloc(4, 1);
    check("AccessCheck with the primary token: ERROR_NO_IMPERSONATION_TOKEN",
          API("AccessCheck", sd, rd(tok), 1, map, ps, pl, granted, status) == 0 && API0("GetLastError") == 1309, 1);
    check("AccessCheck(read) as the player: granted", API("AccessCheck", sd, rd(imp), 1, map, ps, pl, granted, status) == 1 && rd(status) == 1 && rd(granted) == 1, 1);
    uint32_t sid2 = halopad_heap_alloc(4, 1), acl2 = halopad_heap_alloc(64, 1), sd2 = halopad_heap_alloc(20, 1);
    API("AllocateAndInitializeSid", bytes(nt, 6), 2, 32, 547, 0, 0, 0, 0, 0, 0, sid2);   /* Power Users: not a member */
    API("InitializeSecurityDescriptor", sd2, 1);
    API("InitializeAcl", acl2, 64, 2);
    API("AddAccessAllowedAce", acl2, 2, 3, rd(sid2));
    API("SetSecurityDescriptorDacl", sd2, 1, acl2, 0);
    check("  for a group the player is not in: denied", API("AccessCheck", sd2, rd(imp), 1, map, ps, pl, granted, status) == 1 && rd(status) == 0, 1);
    API("CloseHandle", rd(imp)); API("CloseHandle", rd(tok));

    /* Halo's own check, translated */
    check("Halo's 0x545c50 (is the player a local administrator) returns TRUE", halopad_call_guest_ex(0x545c50, 0, NULL, 0, 0) & 0xFF, 1);
    check("  and records it at 0x6399a8", rd(0x6399a8), 1);

    /* clipboard rules */
    check("CloseClipboard when not open: ERROR_CLIPBOARD_NOT_OPEN", API0("CloseClipboard") == 0 && API0("GetLastError") == 1418, 1);
    check("OpenClipboard", API("OpenClipboard", 0), 1);
    check("  OpenClipboard again: fails", API("OpenClipboard", 0), 0);
    check("  CloseClipboard", API0("CloseClipboard"), 1);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

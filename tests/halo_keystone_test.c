/* Translated DLL loader test (G3): Keystone.dll and ksimeui.dll, the chat UI library Halo
 * loads at start-up, run as translated code at their preferred bases. LoadLibraryA maps
 * Keystone, loads its static import ksimeui.dll first, binds both import tables, runs both
 * DllMains (ksimeui first); GetProcAddress reads the export directory; FreeLibrary runs
 * DLL_PROCESS_DETACH and unmaps (so the range reads as free again); a second load
 * starts from a fresh image. Also the exact name list and order Halo's 0x545f9d uses.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_keystone_test.c. */
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
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);
uint32_t GetModuleHandleA_c(uint32_t name);
uint32_t FreeLibrary_c(uint32_t module);
uint32_t GetModuleFileNameA_c(uint32_t module, uint32_t buf, uint32_t size);
uint32_t VirtualQuery_c(uint32_t address, uint32_t info, uint32_t length);
uint32_t GetLastError_c(void);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static char *P(uint32_t g) { return halopad_guest_ptr(g); }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-72s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}

/* the PE's own export table, read from the file (independent of the loader) */
static uint32_t file_export(const char *path, const char *name)
{
    FILE *f = fopen(path, "rb");
    if (!f) { perror(path); exit(2); }
    fseek(f, 0, SEEK_END); long n = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *b = malloc(n); fread(b, 1, n, f); fclose(f);
    uint32_t pe, base, dir, nsec, opt;
    memcpy(&pe, b + 0x3C, 4); memcpy(&base, b + pe + 0x34, 4); memcpy(&dir, b + pe + 0x78, 4);
    nsec = b[pe + 6] | b[pe + 7] << 8; opt = b[pe + 0x14] | b[pe + 0x15] << 8;
    #define OFF(rva) ({ uint32_t _r = (rva), _o = 0; for (uint32_t _i = 0; _i < nsec; _i++) { const unsigned char *_s = b + pe + 0x18 + opt + 40 * _i; uint32_t _va, _sz, _raw; memcpy(&_va, _s + 12, 4); memcpy(&_sz, _s + 16, 4); memcpy(&_raw, _s + 20, 4); if (_r >= _va && _r < _va + _sz) _o = _raw + _r - _va; } _o; })
    uint32_t e = OFF(dir), nnames, funcs, names, ords;
    memcpy(&nnames, b + e + 0x18, 4); memcpy(&funcs, b + e + 0x1C, 4); memcpy(&names, b + e + 0x20, 4); memcpy(&ords, b + e + 0x24, 4);
    uint32_t out = 0;
    for (uint32_t k = 0; k < nnames; k++) {
        uint32_t nr; memcpy(&nr, b + OFF(names) + 4 * k, 4);
        if (!strcmp((char *)b + OFF(nr), name)) {
            uint16_t o; memcpy(&o, b + OFF(ords) + 2 * k, 2);
            uint32_t rva; memcpy(&rva, b + OFF(funcs) + 4 * o, 4);
            out = base + rva;
        }
    }
    free(b);
    return out;
}

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE"), *root = getenv("HALOPAD_GAME_ROOT");
    if (!image || !root) return 2;
    setvbuf(stdout, NULL, _IONBF, 0);
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    halopad_protect_image(0x400000);
    halopad_modules_init();
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    char ks_path[1024];
    snprintf(ks_path, sizeof ks_path, "%s/Keystone.dll", root);
    uint32_t mbi = halopad_heap_alloc(28, 1);

    VirtualQuery_c(0x10200000, mbi, 28);
    check("before loading: Keystone's range reads as free (MEM_FREE)", rd(mbi + 16), 0x10000);
    check("before loading: GetModuleHandleA(\"keystone.dll\") is NULL", GetModuleHandleA_c(str("keystone.dll")), 0);

    uint32_t h = LoadLibraryA_c(str("keystone.dll"));        /* Halo's own spelling (0x61063c) */
    check("LoadLibraryA(\"keystone.dll\") -> preferred base", h, 0x10200000);
    check("ksimeui.dll loaded as Keystone's static import", GetModuleHandleA_c(str("KSIMEUI.DLL")), 0x10000000);
    check("MZ at the module base", rd(0x10200000) & 0xFFFF, 0x5A4D);
    uint32_t buf = halopad_heap_alloc(260, 1);
    GetModuleFileNameA_c(h, buf, 260);
    check("GetModuleFileNameA names Keystone.dll in the install directory",
          !strcmp(P(buf), "C:\\Program Files\\Microsoft Games\\Halo Custom Edition\\Keystone.dll"), 1);
    /* Keystone's import table now holds bound addresses: a ksimeui export and kernel32 service */
    static const char *const halo_names[] = {"KeystoneCreate", "Call_KsTranslateAccelerator", "Call_KsCreateWindow",
        "Call_KsGetWindow", "Call_KsUpdate", "Call_KsDispatchMessage", "Call_KW_Release", "Call_KsSetFocusWindow",
        "Call_KW_GetControlByID", "Call_KC_GetAttribute", "Call_KC_SetAttribute", "Call_KC_SendMessage",
        "Call_KW_ReLayout", "Call_KW_SetFocusControl", "Call_KW_AddDirtyControl", "Call_KsRelease", "Call_KW_ShowWindow"};
    int same = 0;
    for (int i = 0; i < 17; i++) same += GetProcAddress_c(h, str(halo_names[i])) == file_export(ks_path, halo_names[i]);
    check("GetProcAddress: all 17 names Halo resolves match the file's export table", same, 17);
    check("GetProcAddress by ordinal 19 (KeystoneCreate)", GetProcAddress_c(h, 19), file_export(ks_path, "KeystoneCreate"));
    check("GetProcAddress of a missing name: NULL", GetProcAddress_c(h, str("KsNoSuchExport")), 0);
    check("  ... with ERROR_PROC_NOT_FOUND", GetLastError_c(), 127);
    check("a second LoadLibraryA returns the same handle", LoadLibraryA_c(str("C:\\Program Files\\Microsoft Games\\Halo Custom Edition\\Keystone.dll")), h);
    check("FreeLibrary (reference 2 -> 1) keeps it loaded", FreeLibrary_c(h) && GetModuleHandleA_c(str("keystone.dll")) == h, 1);
    check("FreeLibrary (1 -> 0) unloads Keystone", FreeLibrary_c(h) && GetModuleHandleA_c(str("keystone.dll")) == 0, 1);
    check("  ... and its import ksimeui.dll", GetModuleHandleA_c(str("ksimeui.dll")), 0);
    VirtualQuery_c(0x10200000, mbi, 28);
    check("  ... and the range is free again", rd(mbi + 16), 0x10000);
    check("reload after unload: same base", LoadLibraryA_c(str("keystone.dll")), 0x10200000);
    check("  ... exports resolve again", GetProcAddress_c(0x10200000, str("Call_KsUpdate")), file_export(ks_path, "Call_KsUpdate"));
    /* MSXML 4.0's message DLL, loaded as msxml4.dll does: LoadLibraryExA(AS_DATAFILE) */
    uint32_t LoadLibraryExA_c(uint32_t name, uint32_t file, uint32_t flags);
    uint32_t FormatMessageW_c(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint32_t buf, uint32_t size, uint32_t args);
    uint32_t FormatMessageA_c(uint32_t flags, uint32_t source, uint32_t id, uint32_t lang, uint32_t buf, uint32_t size, uint32_t args);
    uint32_t r4 = LoadLibraryExA_c(str("C:\\WINDOWS\\system32\\msxml4r.dll"), 0, 0xA);
    check("LoadLibraryExA(msxml4r.dll, AS_DATAFILE): the base with the data-file bit", r4, 0x78AE0001);
    check("  GetModuleHandleA does not see a data-file module", GetModuleHandleA_c(str("msxml4r.dll")), 0);
    uint32_t wbuf = halopad_heap_alloc(512, 1), arg = halopad_heap_alloc(64, 1), argv = halopad_heap_alloc(4, 1);
    static const char ins[] = "Out of memory";
    for (unsigned i = 0; i < sizeof ins; i++) ((uint16_t *)halopad_guest_ptr(arg))[i] = (uint8_t)ins[i];
    memcpy(halopad_guest_ptr(argv), &arg, 4);
    uint32_t n = FormatMessageW_c(0x2800, r4, 0xC00CE30A, 0x400, wbuf, 256, argv);
    if (!n) printf("    FormatMessageW failed, error %u\n", GetLastError_c());
    char got[128] = {0};
    for (uint32_t i = 0; i < n && i < 127; i++) got[i] = (char)((uint16_t *)halopad_guest_ptr(wbuf))[i];
    check("  FormatMessageW(FROM_HMODULE|ARGUMENT_ARRAY) from its message table, %1 inserted",
          !strncmp(got, "System error: Out of memory.", 28), 1);
    uint32_t an = FormatMessageA_c(0x2A00, r4, 0xC00CE30A, 0x400, wbuf, 256, 0);
    check("  FormatMessageA with IGNORE_INSERTS keeps %1", an >= 17 && !strncmp(P(wbuf), "System error: %1.", 17), 1);
    check("  FreeLibrary of the data-file handle", FreeLibrary_c(r4), 1);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

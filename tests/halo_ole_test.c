/* OLE, DxDiag, CryptoAPI and timer test (G3/G4): Halo's sequence at 0x580e70 (dsound
 * GetDeviceID by ordinal, CoInitialize, the DxDiag provider and its sound devices), GUID text,
 * SHA-1/MD5 through CryptoAPI, and winmm timer periods.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_ole_test.c. */
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
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static uint32_t wstr(const char *s) { uint32_t n = (uint32_t)strlen(s), g = halopad_heap_alloc(2 * n + 2, 1); for (uint32_t i = 0; i < n; i++) ((uint16_t *)halopad_guest_ptr(g))[i] = (uint8_t)s[i]; return g; }
static void narrow(uint32_t w, char *out, int n) { int i = 0; for (; i < n - 1; i++) { uint16_t c = ((uint16_t *)halopad_guest_ptr(w))[i]; if (!c) break; out[i] = (char)c; } out[i] = 0; }
static char *P(uint32_t g) { return halopad_guest_ptr(g); }
static void check(const char *what, uint32_t got, uint32_t want)
{
    printf("%-66s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}
static void checks(const char *what, const char *got, const char *want)
{
    int ok = !strcmp(got, want);
    printf("%-66s %s (got \"%s\", want \"%s\")\n", what, ok ? "PASS" : "FAIL", got, want);
    failures += !ok;
}
static uint32_t method(uint32_t obj, uint32_t index, uint32_t n, const uint32_t *args)
{
    uint32_t a[12] = {obj};
    memcpy(a + 1, args, 4 * n);
    return halopad_call_guest(rd(rd(obj) + 4 * index), n + 1, a);
}
#define M(obj, index, ...) method(obj, index, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
#define M0(obj, index) method(obj, index, 0, NULL)
static uint32_t call(uint32_t mod, const char *name, uint32_t n, const uint32_t *args)
{
    uint32_t va = GetProcAddress_c(mod, str(name));
    if (!va) { printf("no %s\n", name); exit(2); }
    return halopad_call_guest(va, n, args);
}
#define C(mod, name, ...) call(mod, name, sizeof((uint32_t[]){__VA_ARGS__}) / 4, (uint32_t[]){__VA_ARGS__})
static uint32_t guid(const char *hex16)                                   /* raw bytes as hex */
{
    uint32_t g = halopad_heap_alloc(16, 0);
    for (int i = 0; i < 16; i++) { unsigned v; sscanf(hex16 + 2 * i, "%2x", &v); P(g)[i] = (char)v; }
    return g;
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
    uint32_t ole = LoadLibraryA_c(str("ole32.dll")), oa = LoadLibraryA_c(str("oleaut32.dll")), adv = LoadLibraryA_c(str("advapi32.dll"));
    uint32_t ds = LoadLibraryA_c(str("dsound.dll")), mm = LoadLibraryA_c(str("winmm.dll"));

    /* dsound GetDeviceID by ordinal, as Halo imports it */
    uint32_t gid = GetProcAddress_c(ds, 9);
    check("GetProcAddress(dsound, ordinal 9)", gid != 0, 1);
    uint32_t dev = halopad_heap_alloc(16, 1);
    uint32_t defplay = guid("0000f0de6d9ced47aaf14dda8f2b5c03");   /* DSDEVID_DefaultPlayback */
    check("GetDeviceID(DSDEVID_DefaultPlayback)", halopad_call_guest(gid, 2, (uint32_t[]){defplay, dev}), 0);
    uint32_t wg = halopad_heap_alloc(80, 1);
    check("StringFromGUID2 of it: 39 characters", C(ole, "StringFromGUID2", dev, wg, 40), 39);
    char devtext[64]; narrow(wg, devtext, 64);
    uint32_t back = halopad_heap_alloc(16, 1);
    check("CLSIDFromString round trip", C(ole, "CLSIDFromString", wg, back) == 0 && !memcmp(P(back), P(dev), 16), 1);

    /* DxDiag, as Halo reads it */
    uint32_t pobj = halopad_heap_alloc(4, 1);
    uint32_t clsid = guid("71805ba6fe3b13429a5b491da4461ca7"), iid = guid("b04c6b9cf823cc49a3ed45a55000a6d2");
    check("CoCreateInstance before CoInitialize: CO_E_NOTINITIALIZED", C(ole, "CoCreateInstance", clsid, 0, 1, iid, pobj), 0x800401F0);
    check("CoInitialize(NULL)", C(ole, "CoInitialize", 0), 0);
    check("  again: S_FALSE", C(ole, "CoInitialize", 0), 1);
    check("CoCreateInstance(CLSID_DxDiagProvider, IID_IDxDiagProvider)", C(ole, "CoCreateInstance", clsid, 0, 1, iid, pobj), 0);
    uint32_t prov = rd(pobj);
    uint32_t params = halopad_heap_alloc(16, 1);
    memcpy(P(params), (uint32_t[]){16, 111, 0, 0}, 16);
    check("  Initialize({16, DXDIAG_DX9_SDK_VERSION})", M(prov, 3, params), 0);
    check("  GetRootContainer", M(prov, 4, pobj), 0);
    uint32_t rootc = rd(pobj);
    check("  GetChildContainer(\"DxDiag_DirectSound.DxDiag_SoundDevices\")", M(rootc, 5, wstr("DxDiag_DirectSound.DxDiag_SoundDevices"), pobj), 0);
    uint32_t devs = rd(pobj), n = halopad_heap_alloc(4, 1);
    check("  one sound device", M(devs, 3, n) == 0 && rd(n) == 1, 1);
    uint32_t nm = halopad_heap_alloc(64, 1);
    char name[32];
    M(devs, 4, 0, nm, 32); narrow(nm, name, 32);
    checks("  EnumChildContainerNames(0)", name, "0");
    check("  EnumChildContainerNames(1): E_INVALIDARG", M(devs, 4, 1, nm, 32), 0x80070057);
    check("  GetChildContainer(\"0\")", M(devs, 5, wstr("0"), pobj), 0);
    uint32_t d0 = rd(pobj), v = halopad_heap_alloc(16, 1);
    C(oa, "VariantInit", v);
    check("  GetProp(\"szGuidDeviceID\"): a BSTR", M(d0, 8, wstr("szGuidDeviceID"), v) == 0 && (rd(v) & 0xFFFF) == 8, 1);
    char text[64]; narrow(rd(v + 8), text, 64);
    checks("  the same GUID GetDeviceID reported", text, devtext);
    check("  its BSTR length prefix is in bytes", rd(rd(v + 8) - 4), 2 * (uint32_t)strlen(devtext));
    check("  VariantClear: VT_EMPTY", C(oa, "VariantClear", v) == 0 && rd(v) == 0, 1);
    M(d0, 8, wstr("szDescription"), v); narrow(rd(v + 8), text, 64);
    checks("  szDescription", text, "HaloPad Audio (Core Audio)");
    C(oa, "VariantClear", v);
    check("  a device index past the last: E_INVALIDARG", M(devs, 5, wstr("1"), pobj), 0x80070057);
    /* BSTRs (Keystone.dll and msxml4.dll) */
    uint32_t bs = C(oa, "SysAllocString", wstr("chat"));
    check("SysAllocString: byte-length prefix 8, 4 characters, terminated", rd(bs - 4) == 8 && C(oa, "SysStringLen", bs) == 4 && ((uint16_t *)P(bs))[4] == 0, 1);
    check("  SysStringByteLen", C(oa, "SysStringByteLen", bs), 8);
    check("SysAllocString(NULL): NULL", C(oa, "SysAllocString", 0), 0);
    uint32_t bl = C(oa, "SysAllocStringLen", wstr("abcdef"), 3);
    char t3[8]; narrow(bl, t3, 8);
    checks("SysAllocStringLen(\"abcdef\", 3)", t3, "abc");
    uint32_t bb = C(oa, "SysAllocStringByteLen", str("xyz"), 3);
    check("SysAllocStringByteLen(\"xyz\", 3): bytes kept, length 3", rd(bb - 4) == 3 && !memcmp(P(bb), "xyz", 3), 1);
    C(oa, "SysFreeString", bs); C(oa, "SysFreeString", bl); C(oa, "SysFreeString", bb); C(oa, "SysFreeString", 0);
    M0(d0, 2); M0(devs, 2); M0(rootc, 2);
    check("  Release the provider", M0(prov, 2), 0);
    call(ole, "CoUninitialize", 0, NULL); call(ole, "CoUninitialize", 0, NULL);

    /* CryptoAPI hashes */
    uint32_t hp = halopad_heap_alloc(4, 1), hh = halopad_heap_alloc(4, 1), out = halopad_heap_alloc(32, 1), len = halopad_heap_alloc(4, 1);
    check("CryptAcquireContextA(PROV_RSA_FULL, CRYPT_VERIFYCONTEXT)", C(adv, "CryptAcquireContextA", hp, 0, 0, 1, 0xF0000000), 1);
    check("CryptCreateHash(CALG_SHA1)", C(adv, "CryptCreateHash", rd(hp), 0x8004, 0, 0, hh), 1);
    C(adv, "CryptHashData", rd(hh), str("abc"), 3, 0);
    check("CryptGetHashParam(HP_HASHVAL, NULL): size 20", C(adv, "CryptGetHashParam", rd(hh), 2, 0, len, 0) == 1 && rd(len) == 20, 1);
    memcpy(P(len), (uint32_t[]){10}, 4);
    check("  a 10-byte buffer: ERROR_MORE_DATA", C(adv, "CryptGetHashParam", rd(hh), 2, out, len, 0) == 0 && rd(len) == 20, 1);
    memcpy(P(len), (uint32_t[]){20}, 4);
    C(adv, "CryptGetHashParam", rd(hh), 2, out, len, 0);
    static const unsigned char sha_abc[20] = {0xa9, 0x99, 0x3e, 0x36, 0x47, 0x06, 0x81, 0x6a, 0xba, 0x3e, 0x25, 0x71, 0x78, 0x50, 0xc2, 0x6c, 0x9c, 0xd0, 0xd8, 0x9d};
    check("  SHA-1(\"abc\")", memcmp(P(out), sha_abc, 20), 0);
    check("  CryptHashData after the value is read: NTE_BAD_HASH_STATE", C(adv, "CryptHashData", rd(hh), str("x"), 1, 0) == 0 && call(LoadLibraryA_c(str("kernel32.dll")), "GetLastError", 0, NULL) == 0x8009000C, 1);
    C(adv, "CryptDestroyHash", rd(hh));
    C(adv, "CryptCreateHash", rd(hp), 0x8003, 0, 0, hh);
    C(adv, "CryptHashData", rd(hh), str("abc"), 3, 0);
    memcpy(P(len), (uint32_t[]){32}, 4);
    C(adv, "CryptGetHashParam", rd(hh), 2, out, len, 0);
    static const unsigned char md5_abc[16] = {0x90, 0x01, 0x50, 0x98, 0x3c, 0xd2, 0x4f, 0xb0, 0xd6, 0x96, 0x3f, 0x7d, 0x28, 0xe1, 0x7f, 0x72};
    check("CALG_MD5: MD5(\"abc\"), 16 bytes", rd(len) == 16 && !memcmp(P(out), md5_abc, 16), 1);
    C(adv, "CryptDestroyHash", rd(hh));
    check("CryptReleaseContext", C(adv, "CryptReleaseContext", rd(hp), 0), 1);

    /* winmm */
    check("timeBeginPeriod(1)", C(mm, "timeBeginPeriod", 1), 0);
    check("timeEndPeriod(1)", C(mm, "timeEndPeriod", 1), 0);
    check("timeEndPeriod(1) without a matching begin: TIMERR_NOCANDO", C(mm, "timeEndPeriod", 1), 97);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures ? 1 : 0;
}

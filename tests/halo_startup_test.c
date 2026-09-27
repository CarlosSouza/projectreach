/* Start-up checks test (G3): what Halo's start-up concludes about a HaloPad machine.
 * The C runtime is started as Halo's entry point 0x5ccac7 starts it, up to WinMain.
 *
 * After the license, WinMain (0x5449c7-0x544c38) checks the machine and reports each problem
 * with its warning and error dialog (0x582060). This test runs the same checks from Halo's own
 * code, in WinMain's order, on HaloPad's services, with a host that records any dialog:
 *   - Direct3D 9: d3d9.dll and Direct3DCreate9, then 0x580a00(adapter 0, IDirect3D9), which
 *     identifies the adapter, reads the game's config.txt (Halo's card database) and applies
 *     the properties for this card; it returns an error text or 0;
 *   - DirectSound, DirectInput, shfolder.dll: LoadLibraryA and GetProcAddress, as WinMain;
 *   - an unclean last exit (0x581e40: HKCU ExitFlag);
 *   - first, memory and CPU speed (0x580e70: GlobalMemoryStatus, rdtsc against the performance
 *     counter) and each display device's video memory (DirectDraw 7), checked against the
 *     minimums 0x580a00 sets (0x68af8c MB, 0x68afa0 MHz);
 *   - free space in the temporary folder against 0x68af94 MB;
 *   - the product ID (0x5829e0): expected missing on HaloPad, which never writes one.
 * Linked in place of the core's main by scripts/run-core.py --main tests/halo_startup_test.c. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"
#include "../port/runtime/halopad_dialog.h"

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
extern _Thread_local _cpu *halopad_cpu;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);
uint32_t GetProcAddress_c(uint32_t module, uint32_t name);
uint32_t GetTempPathA_c(uint32_t len, uint32_t buf);
uint32_t GetDiskFreeSpaceExA_c(uint32_t dir, uint32_t caller_free, uint32_t total, uint32_t total_free);

static int failures;
static uint32_t rd(uint32_t g) { uint32_t v; memcpy(&v, halopad_guest_ptr(g), 4); return v; }
static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }
static uint32_t str(const char *s) { uint32_t g = halopad_heap_alloc((uint32_t)strlen(s) + 1, 0); strcpy(halopad_guest_ptr(g), s); return g; }
static const char *gs(uint32_t g) { return g ? (const char *)halopad_guest_ptr(g) : "(null)"; }
static void check(const char *what, uint32_t got, uint32_t want)
{
    fprintf(stderr, "HALOPAD TEST: %s\n", what);
    printf("%-72s %s (got 0x%x, want 0x%x)\n", what, got == want ? "PASS" : "FAIL", got, want);
    failures += got != want;
}

static int dialogs;
int halopad_host_dialog(const halopad_dialog_view *v)
{
    dialogs++;
    printf("    DIALOG \"%s\":", v->title);
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1006) printf(" %s", v->item[i].text);
    printf("\n");
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1004 && v->item[i].enabled) return i;   /* Continue Anyway */
    for (int i = 0; i < v->count; i++) if (v->item[i].id == 1005) return i;                        /* else Exit */
    return HPD_CLOSE;
}

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
    halopad_protect_image(0x400000);
    halopad_modules_init();
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    uint32_t crt_arg = 1;
    check("CRT _heap_init (0x5d6ba6)", halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0) != 0, 1);
    check("CRT _mtinit (0x5cf966)", halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0) != 0, 1);
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    check("CRT _ioinit (0x5cf3f2)", (int32_t)halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0) >= 0, 1);
    /* the rest of the entry point before WinMain: command line, environment, _setargv, _setenvp,
       and _cinit (the floating-point support and C++ initializers config.txt's parsing needs) */
    uint32_t k32 = LoadLibraryA_c(str("kernel32.dll"));
    wr(0x6bece8, halopad_call_guest(GetProcAddress_c(k32, str("GetCommandLineA")), 0, NULL));
    wr(0x63e2f4, halopad_call_guest_ex(0x5d6a6a, 0, NULL, 0, 0));
    check("CRT _setargv (0x5d69c8)", (int32_t)halopad_call_guest_ex(0x5d69c8, 0, NULL, 0, 0) >= 0, 1);
    check("CRT _setenvp (0x5d6795)", (int32_t)halopad_call_guest_ex(0x5d6795, 0, NULL, 0, 0) >= 0, 1);
    uint32_t one = 1;
    check("CRT _cinit (0x5caa87)", halopad_call_guest_ex(0x5caa87, 1, &one, 0, 0), 0);
    wr(0x6bde88, LoadLibraryA_c(str("strings.dll")));             /* 0x582590 */
    wr(0x6e1480, 0x400000);                                       /* WinMain's hInstance */

    /* the machine measurement, as WinMain 0x54494c: memory, CPU speed, and each display
       device's video memory through DirectDraw 7 */
    halopad_call_guest_ex(0x580e70, 0, NULL, 0, 0);
    printf("    display devices: %u; first: \"%s\", %u MB\n", rd(0x6bde84), (const char *)halopad_guest_ptr(0x68ad90), rd(0x68adb0) >> 20);
    check("0x580e70: one display device (DirectDrawEnumerateExA)", rd(0x6bde84), 1);
    check("  its video memory, 128 MB (GetAvailableVidMem)", rd(0x68adb0), 128u << 20);

    /* Direct3D 9, as WinMain 0x5449c7 */
    uint32_t d3d9 = LoadLibraryA_c(str("d3d9.dll")), create = d3d9 ? GetProcAddress_c(d3d9, str("Direct3DCreate9")) : 0;
    check("d3d9.dll and Direct3DCreate9 (else string 0x6b, fatal)", d3d9 && create, 1);
    uint32_t sdk = 0x1f, d3d = halopad_call_guest(create, 1, &sdk);
    check("Direct3DCreate9(D3D_SDK_VERSION) (else string 0x81)", d3d != 0, 1);
    cpu._edx = d3d;
    uint32_t err = halopad_call_guest_ex(0x580a00, 0, NULL, 0 /* ecx: adapter 0 */, 0);
    if (err) printf("    0x580a00 says: %s\n", gs(err));
    check("0x580a00: adapter and config.txt accepted (no error text)", err, 0);
    printf("    config.txt card: \"%s\" \"%s\"; card memory %u MB\n", gs(rd(0x6bde60)), gs(rd(0x6bde64)), rd(0x6bde80) >> 20);
    check("  the card's memory, from the display device (0x6bde80)", rd(0x6bde80), 128u << 20);

    /* the other libraries */
    static const char *libs[][2] = {{"dsound.dll", "DirectSoundCreate8"}, {"dinput8.dll", "DirectInput8Create"}, {"shfolder.dll", "SHGetFolderPathA"}};
    for (int i = 0; i < 3; i++) {
        uint32_t h = LoadLibraryA_c(str(libs[i][0])), f = h ? GetProcAddress_c(h, str(libs[i][1])) : 0;
        char what[96];
        snprintf(what, sizeof what, "%s and %s", libs[i][0], libs[i][1]);
        check(what, h && f, 1);
    }

    /* an unclean last exit */
    check("0x581e40: no unclean last exit recorded (else string 0x6a, safe mode)", halopad_call_guest_ex(0x581e40, 0, NULL, 0, 0), 0);

    /* memory and CPU speed (measured first, at 0x54494c) against 0x580a00's minimums */
    uint32_t mb = rd(0x6bde78), mhz = rd(0x6bde7c);
    printf("    measured: %u MB, %u MHz; minimums %u MB (less 16), %u MHz, %u MB free\n", mb, mhz, rd(0x68af8c), rd(0x68afa0), rd(0x68af94));
    check("memory at least the minimum (else string 0x65)", mb >= rd(0x68af8c) - 16, 1);
    check("CPU speed at least the minimum (else string 0x66)", mhz >= rd(0x68afa0), 1);

    /* temporary folder space */
    uint32_t tmp = halopad_heap_alloc(260, 1), fr = halopad_heap_alloc(8, 1);
    GetTempPathA_c(260, tmp);
    check("GetDiskFreeSpaceExA on the temporary folder", GetDiskFreeSpaceExA_c(tmp, fr, 0, 0), 1);
    uint64_t free_bytes = (uint64_t)rd(fr) | (uint64_t)rd(fr + 4) << 32;
    printf("    %s: %llu MB free\n", gs(tmp), (unsigned long long)(free_bytes >> 20));
    check("free space at least the minimum (else string 0x6d)", free_bytes >= (uint64_t)rd(0x68af94) << 20, 1);

    /* the product ID */
    uint32_t pid = halopad_call_guest_ex(0x5829e0, 0, NULL, 0, 0);
    check("0x5829e0: no product ID on HaloPad (string 0xa0, fatal, follows)", *(const char *)halopad_guest_ptr(pid) == 0, 1);
    check("no dialog before the product-ID check", (uint32_t)dialogs, 0);
    printf("%s: %d failure(s)\n", failures ? "FAIL" : "PASS", failures);
    return failures != 0;
}

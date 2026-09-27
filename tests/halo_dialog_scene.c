/* iPadOS app scene (G3 evidence, never shipped): Halo's own warning dialog on the Simulator.
 *
 * Linked into the app by scripts/build-ios-app.py --scene tests/halo_dialog_scene.c in place of
 * the core's start (halopad_app_entry). It prepares what Halo's start-up would have (the C
 * runtime, hInstance, strings.dll, the machine info) and calls Halo's 0x582060 for the Ctrl-key
 * warning (text 0x87, link 0x7e, not fatal), so the app shows Halo's dialog through
 * halopad_host_dialog. Nothing is tapped: the screenshot shows the dialog waiting for the player. */
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
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
uint32_t halopad_heap_alloc(uint32_t size, int zero);
void *halopad_guest_ptr(uint32_t guest);
uint32_t LoadLibraryA_c(uint32_t name);

static void wr(uint32_t g, uint32_t v) { memcpy(halopad_guest_ptr(g), &v, 4); }

int halopad_app_entry(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) return 2;
    halopad_guest_harness_heap = 0;
    uint32_t top = halopad_guest_init(image, 0x400000);
    halopad_thread_init(0x00300000, 0x00100000, 0x400000);
    halopad_vm_mark(0x00100000, 0x00200000);
    halopad_vm_mark(0x400000, 0x42C000);
    halopad_vm_mark(0x7FFD0000, 0x00030000);
    halopad_protect_image(0x400000);
    halopad_modules_init();
    static _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x27F;
    halopad_cpu = &cpu;
    uint32_t crt_arg = 1;
    halopad_call_guest_ex(0x5d6ba6, 1, &crt_arg, 0, 0);          /* _heap_init */
    halopad_call_guest_ex(0x5cf966, 0, NULL, 0, 0);               /* _mtinit */
    halopad_call_guest_ex(0x5d3ac3, 0, NULL, 0, 0);
    halopad_call_guest_ex(0x5cf3f2, 0, NULL, 0, 0);               /* _ioinit */
    uint32_t name = halopad_heap_alloc(16, 1);
    strcpy(halopad_guest_ptr(name), "strings.dll");
    wr(0x6bde88, LoadLibraryA_c(name));
    wr(0x6e1480, 0x400000);
    wr(0x6bde7c, 2400);
    wr(0x6bde78, 512);
    uint32_t args[3] = {0x87, 0x7e, 0};
    fprintf(stderr, "HALOPAD SCENE: Halo's Ctrl-key warning (0x582060)\n");
    halopad_call_guest_ex(0x582060, 3, args, 0, 0);
    fprintf(stderr, "HALOPAD SCENE: the dialog returned\n");
    return 0;
}

/* HaloPad native core (G3): runs Halo Custom Edition's translated code from its PE
 * entry point on the host. Guest memory, dispatch and the thread environment come from
 * port/runtime; every Windows service Halo reaches is either a HaloPad implementation
 * or a trap that names it.
 *
 * Environment: HALOPAD_IMAGE (prepared image), HALOPAD_GAME_ROOT (game files). */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

#define IMAGE_BASE 0x400000u
#define ENTRY_VA 0x5CCAC7u

extern uint64_t halopad_guest_base;
extern int halopad_guest_harness_heap;
uint32_t halopad_guest_init(const char *image_path, uint32_t image_base);
void halopad_thread_init(uint32_t stack_base, uint32_t stack_limit, uint32_t image_base);
void halopad_vm_mark(uint32_t base, uint32_t size);
void halopad_protect_image(uint32_t image_base);
void halopad_modules_init(void);
void halopad_enter(_cpu *cpu, uint32_t va);
extern _Thread_local _cpu *halopad_cpu;

int main(void)
{
    const char *image = getenv("HALOPAD_IMAGE");
    if (!image) { fprintf(stderr, "HALOPAD_IMAGE not set\n"); return 2; }
    halopad_guest_harness_heap = 0;
    uint32_t stack_top = halopad_guest_init(image, IMAGE_BASE);
    halopad_thread_init(0x00300000, 0x00100000, IMAGE_BASE);
    /* ranges the memory services must never hand out */
    halopad_vm_mark(0x00100000, 0x00200000);           /* main-thread stack */
    halopad_vm_mark(IMAGE_BASE, 0x0042C000);           /* image (SizeOfImage) */
    halopad_vm_mark(0x7FFD0000, 0x00030000);           /* TLS block, TEB, and the page above */
    halopad_protect_image(IMAGE_BASE);
    halopad_modules_init();                            /* translated DLLs' preferred ranges */
    _cpu cpu;
    memset(&cpu, 0, sizeof cpu);
    cpu._esp = stack_top;
    cpu._pointer_offset = halopad_guest_base;
    cpu._st_cw = 0x027F;   /* x87 control word at Windows process start */
    halopad_cpu = &cpu;
    fprintf(stderr, "HALOPAD: entering Halo at 0x%08x\n", ENTRY_VA);
    halopad_enter(&cpu, ENTRY_VA);
    fprintf(stderr, "HALOPAD: entry point returned, eax=0x%08x\n", cpu._eax);
    return (int)cpu._eax;
}

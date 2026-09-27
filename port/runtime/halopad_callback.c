/* HaloPad guest callbacks (G3): runtime services calling translated Halo code, e.g. a
 * window procedure during CreateWindowExA or DispatchMessageA.
 *
 * The callback runs nested on the same guest stack, below the service's own arguments:
 * arguments are pushed right to left, halopad_enter pushes the host-return sentinel and
 * runs the procedure until it returns there. Windows' callback convention (stdcall) is
 * then checked: the callee pops its arguments and preserves ebx, esi, edi and ebp, with the
 * direction flag clear. A violation stops the program with the callback's address. */
#include <setjmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

void halopad_enter(_cpu *cpu, uint32_t va);
void halopad_enter_with(_cpu *cpu, uint32_t va, uint32_t sentinel);
void *halopad_guest_ptr(uint32_t guest);
extern void *halopad_return_to_host_address(void);

_Thread_local _cpu *halopad_cpu;   /* this thread's guest CPU state, set before entering guest code */

/* Entry levels. Translated code runs in continuation style, so the host stack grows only when
   a runtime service calls back into guest code. Level 0 is the thread's first entry
   (halopad_enter, sentinel 0xFFFFF000); each callback gets the next level and its own sentinel
   0xFFFFF000 + 16 * level. A guest return to the innermost sentinel returns to its host caller.
   A return to an outer sentinel happens when guest code abandons the inner entries, as an SEH
   handler called from RaiseException does when it jumps to an __except block: the inner host
   frames are then discarded with longjmp, and the outer entry returns as if normally. */
#define HOST_RETURN_VA 0xFFFFF000u
#define MAX_LEVELS 64
static _Thread_local jmp_buf levels[MAX_LEVELS + 1];
static _Thread_local uint32_t depth;

void *halopad_host_return(uint32_t level)
{
    if (level == depth) return halopad_return_to_host_address();
    if (level > depth || level == 0) {
        fprintf(stderr, "HALOPAD TRAP: guest return to host entry level %u while %u levels are active\n", level, depth);
        abort();
    }
    depth = level;
    longjmp(levels[level], 1);
}

uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);

uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args)
{
    return halopad_call_guest_ex(va, nargs, args, halopad_cpu ? halopad_cpu->_ecx : 0, 1);
}

/* General form: ecx is set on entry (thiscall/fastcall), and callee_pops selects stdcall
   (callee pops the arguments) or cdecl (the caller, here, pops them). */
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops)
{
    _cpu *cpu = halopad_cpu;
    if (!cpu) { fprintf(stderr, "HALOPAD TRAP: guest callback 0x%08x with no guest CPU on this thread\n", va); abort(); }
    uint32_t esp0 = cpu->_esp, ebx0 = cpu->_ebx, esi0 = cpu->_esi, edi0 = cpu->_edi, ebp0 = cpu->_ebp;
    for (uint32_t i = nargs; i-- > 0;) {
        cpu->_esp -= 4;
        memcpy(halopad_guest_ptr(cpu->_esp), &args[i], 4);
    }
    cpu->_ecx = entry_ecx;
    if (depth == MAX_LEVELS) { fprintf(stderr, "HALOPAD TRAP: guest callback 0x%08x nested more than %d deep\n", va, MAX_LEVELS); abort(); }
    uint32_t level = ++depth;
    if (!setjmp(levels[level])) halopad_enter_with(cpu, va, HOST_RETURN_VA + 16 * level);
    depth = level - 1;
    if (!callee_pops) cpu->_esp += 4 * nargs;
    if (cpu->_esp != esp0 || cpu->_ebx != ebx0 || cpu->_esi != esi0 || cpu->_edi != edi0 || cpu->_ebp != ebp0
        || (cpu->_eflags & 0x400)) {
        fprintf(stderr, "HALOPAD TRAP: guest callback 0x%08x broke the stdcall convention "
                "(esp %+d, ebx/esi/edi/ebp %s, DF %u)\n", va, (int)(cpu->_esp - esp0),
                (cpu->_ebx != ebx0 || cpu->_esi != esi0 || cpu->_edi != edi0 || cpu->_ebp != ebp0) ? "changed" : "kept",
                (cpu->_eflags >> 10) & 1);
        abort();
    }
    return cpu->_eax;
}

/* The guest stack pointer inside a service (cdecl variable arguments sit above it). */
uint32_t halopad_guest_esp(void) { return halopad_cpu->_esp; }

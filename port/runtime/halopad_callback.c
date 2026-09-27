/* HaloPad guest callbacks (G3): runtime services calling translated Halo code, e.g. a
 * window procedure during CreateWindowExA or DispatchMessageA.
 *
 * The callback runs nested on the same guest stack, below the service's own arguments:
 * arguments are pushed right to left, halopad_enter pushes the host-return sentinel and
 * runs the procedure until it returns there. Windows' callback convention (stdcall) is
 * then checked: the callee pops its arguments and preserves ebx, esi, edi and ebp, with the
 * direction flag clear. A violation stops the program with the callback's address. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

void halopad_enter(_cpu *cpu, uint32_t va);
void *halopad_guest_ptr(uint32_t guest);

_Thread_local _cpu *halopad_cpu;   /* this thread's guest CPU state, set before entering guest code */

uint32_t halopad_call_guest(uint32_t va, uint32_t nargs, const uint32_t *args)
{
    _cpu *cpu = halopad_cpu;
    if (!cpu) { fprintf(stderr, "HALOPAD TRAP: guest callback 0x%08x with no guest CPU on this thread\n", va); abort(); }
    uint32_t esp0 = cpu->_esp, ebx0 = cpu->_ebx, esi0 = cpu->_esi, edi0 = cpu->_edi, ebp0 = cpu->_ebp;
    for (uint32_t i = nargs; i-- > 0;) {
        cpu->_esp -= 4;
        memcpy(halopad_guest_ptr(cpu->_esp), &args[i], 4);
    }
    halopad_enter(cpu, va);
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

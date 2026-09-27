/* HaloPad slice runtime: the minimum host side needed to execute translated Halo
 * functions in isolation (G2d slices). Everything outside a slice's contract stops the
 * program loudly with the guest address or import name; nothing returns a guessed value.
 * Reference tool for slice tests; the real runtime (G2e/G3) replaces it. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CCALL

void halopad_trap_unimplemented(uint32_t address)
{
    fprintf(stderr, "HALOPAD TRAP: instruction at 0x%08x is not implemented by the translator\n", address);
    abort();
}

void halopad_unreachable_simd(uint32_t address)
{
    fprintf(stderr, "HALOPAD TRAP: SIMD code reached at 0x%08x (plain-CPU contract violated)\n", address);
    abort();
}

void halopad_missing_import(const char *name)
{
    fprintf(stderr, "HALOPAD TRAP: Windows import %s has no implementation in this runtime\n", name);
    abort();
}

uint32_t CCALL X86_ReadFsDword(uint32_t addr)
{
    fprintf(stderr, "HALOPAD TRAP: fs:[0x%x] read; thread environment block not implemented in the slice runtime\n", addr);
    abort();
}

void CCALL X86_WriteFsDword(uint32_t addr, uint32_t value)
{
    fprintf(stderr, "HALOPAD TRAP: fs:[0x%x] write of 0x%x; thread environment block not implemented in the slice runtime\n", addr, value);
    abort();
}

uint32_t CCALL X86_InPortProcedure(uint32_t length, uint32_t port)
{
    fprintf(stderr, "HALOPAD TRAP: port input (length %u, port 0x%x)\n", length, port);
    abort();
}

void CCALL X86_OutPortProcedure(uint32_t length, uint32_t port, uint32_t value)
{
    fprintf(stderr, "HALOPAD TRAP: port output (length %u, port 0x%x, value 0x%x)\n", length, port, value);
    abort();
}

void CCALL X86_InterruptProcedure(const uint8_t IntNum, void *regs)
{
    (void)regs;
    fprintf(stderr, "HALOPAD TRAP: software interrupt 0x%02x\n", IntNum);
    abort();
}

uint32_t CCALL X86_ReadMemProcedure(const uint32_t Address, const uint32_t MemSize)
{
    fprintf(stderr, "HALOPAD TRAP: special memory read at 0x%x (%u bytes)\n", Address, MemSize);
    abort();
}

void CCALL X86_WriteMemProcedure(const uint32_t Address, const uint32_t MemSize, const uint32_t value)
{
    fprintf(stderr, "HALOPAD TRAP: special memory write at 0x%x (%u bytes, 0x%x)\n", Address, MemSize, value);
    abort();
}

/* Written by popfd emulation (llasm_pushx.c). The interrupt flag has no effect on a
 * user-mode game; the value is kept so a later pushfd reproduces it. */
uint32_t X86_InterruptFlag = 1;

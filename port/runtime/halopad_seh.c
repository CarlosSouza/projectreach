/* HaloPad structured exception handling (G3): RaiseException and RtlUnwind over the guest's
 * fs:[0] chain of exception registrations, as Windows XP's RtlDispatchException and RtlUnwind
 * run them on x86. msxml4.dll reports parse errors this way (code 0xE0000001), and C++ throw
 * (_CxxThrowException) uses RaiseException too.
 *
 * Records and the context go on the guest stack below the raiser's frame, and each handler is
 * called as cdecl handler(record, frame, context, dispatcher context). A handler that accepts
 * the exception (MSVC's _except_handler3 or __CxxFrameHandler) unwinds with RtlUnwind and then
 * jumps to the __except block or catch continuation itself; that transfer leaves this service's
 * host frame behind, which halopad_callback.c discards when the guest returns past it.
 * Nested exceptions during dispatch, collided unwinds, handlers that change the context and
 * unhandled exceptions stop with the exception code. Faults in translated code (access
 * violations) are not turned into exceptions yet; they stop in halopad_guest.c. */
#include "halopad_win32.h"
#define PTROFS_64BIT 1
#include "llasm_cpu.h"

extern _Thread_local _cpu *halopad_cpu;
uint32_t X86_ReadFsDword(uint32_t ofs);
void X86_WriteFsDword(uint32_t ofs, uint32_t value);
uint32_t halopad_call_guest_ex(uint32_t va, uint32_t nargs, const uint32_t *args, uint32_t entry_ecx, int callee_pops);
extern const uint32_t halopad_import_count, halopad_import_base, halopad_import_stride;
extern const char *const halopad_import_names[];

#define END_OF_CHAIN 0xFFFFFFFFu
#define EXCEPTION_NONCONTINUABLE 0x1u
#define EXCEPTION_UNWINDING 0x2u
#define EXCEPTION_EXIT_UNWIND 0x4u
#define STATUS_UNWIND 0xC0000027u
#define REC_SIZE 0x50u
#define CTX_SIZE 0x2CCu
enum { CTX_FLAGS = 0x00, CTX_EDI = 0x9C, CTX_ESI = 0xA0, CTX_EBX = 0xA4, CTX_EDX = 0xA8, CTX_ECX = 0xAC, CTX_EAX = 0xB0,
       CTX_EBP = 0xB4, CTX_EIP = 0xB8, CTX_CS = 0xBC, CTX_EFLAGS = 0xC0, CTX_ESP = 0xC4, CTX_SS = 0xC8 };

static int trace_seh(void) { static int v = -1; if (v < 0) v = getenv("HALOPAD_TRACE_SEH") != NULL; return v; }

static uint32_t import_address(const char *name)
{
    for (uint32_t i = 0; i < halopad_import_count; i++)
        if (!strcmp(halopad_import_names[i], name)) return halopad_import_base + halopad_import_stride * i;
    return 0;
}

/* The records sit below the caller's stack; handlers run below them. */
typedef struct { uint32_t rec, ctx, dispatch, handler_esp; } frame_area;
static frame_area area_below(uint32_t sp)
{
    frame_area a;
    a.dispatch = (sp - 0x40 - 16) & ~15u;
    a.ctx = (a.dispatch - CTX_SIZE) & ~15u;
    a.rec = (a.ctx - REC_SIZE) & ~15u;
    a.handler_esp = a.rec - 0x20;
    memset(G(a.ctx), 0, CTX_SIZE);
    memset(G(a.rec), 0, REC_SIZE);
    wr32(a.dispatch, 0);
    return a;
}

/* CONTEXT_FULL at the service's return: registers as the raiser left them, esp after the
   stdcall arguments are popped. Segment values are those of a Windows XP user thread. */
static void capture(uint32_t ctx, uint32_t nargs)
{
    _cpu *c = halopad_cpu;
    wr32(ctx + CTX_FLAGS, 0x10007u);
    wr32(ctx + CTX_EDI, c->_edi); wr32(ctx + CTX_ESI, c->_esi); wr32(ctx + CTX_EBX, c->_ebx);
    wr32(ctx + CTX_EDX, c->_edx); wr32(ctx + CTX_ECX, c->_ecx); wr32(ctx + CTX_EAX, c->_eax);
    wr32(ctx + CTX_EBP, c->_ebp); wr32(ctx + CTX_EIP, rd32(c->_esp)); wr32(ctx + CTX_CS, 0x1B);
    wr32(ctx + CTX_EFLAGS, c->_eflags); wr32(ctx + CTX_ESP, c->_esp + 4 + 4 * nargs); wr32(ctx + CTX_SS, 0x23);
    wr32(ctx + 0x8C, 0); wr32(ctx + 0x90, 0x3B); wr32(ctx + 0x94, 0x23); wr32(ctx + 0x98, 0x23);
}

/* RtlpGetStackLimits check: a registration must lie on this thread's stack, 4-byte aligned */
static int frame_valid(uint32_t frame)
{
    uint32_t top = X86_ReadFsDword(4), limit = X86_ReadFsDword(8);
    return !(frame & 3) && frame >= limit && frame + 8 <= top;
}

static uint32_t call_handler(const frame_area *a, uint32_t frame)
{
    _cpu *c = halopad_cpu;
    uint32_t handler = rd32(frame + 4);
    c->_esp = a->handler_esp;
    uint32_t args[4] = {a->rec, frame, a->ctx, a->dispatch};
    if (trace_seh()) fprintf(stderr, "HALOPAD SEH: code 0x%08x flags 0x%x: handler 0x%08x of frame 0x%08x\n", rd32(a->rec), rd32(a->rec + 4), handler, frame);
    uint32_t d = halopad_call_guest_ex(handler, 4, args, 0, 0);
    if (trace_seh()) fprintf(stderr, "HALOPAD SEH:   handler 0x%08x returned %u\n", handler, d);
    return d;
}

uint32_t RaiseException_c(uint32_t code, uint32_t flags, uint32_t nargs, uint32_t args)
{
    _cpu *c = halopad_cpu;
    uint32_t esp0 = c->_esp, ebp0 = c->_ebp;
    frame_area a = area_below(esp0);
    if (nargs > 15) nargs = 15;                                 /* EXCEPTION_MAXIMUM_PARAMETERS */
    wr32(a.rec + 0x00, code);
    wr32(a.rec + 0x04, flags & EXCEPTION_NONCONTINUABLE);
    wr32(a.rec + 0x08, 0);
    wr32(a.rec + 0x0C, import_address("RaiseException"));     /* XP records RaiseException itself */
    wr32(a.rec + 0x10, args ? nargs : 0);
    for (uint32_t i = 0; args && i < nargs; i++) wr32(a.rec + 0x14 + 4 * i, rd32(args + 4 * i));
    capture(a.ctx, 4);
    uint32_t eip0 = rd32(a.ctx + CTX_EIP), cesp0 = rd32(a.ctx + CTX_ESP);
    for (uint32_t frame = X86_ReadFsDword(0); frame != END_OF_CHAIN; frame = rd32(frame)) {
        if (!frame_valid(frame)) hp_unsupported("RaiseException", "exception 0x%08x: registration 0x%08x outside the stack (EXCEPTION_STACK_INVALID)", code, frame);
        uint32_t d = call_handler(&a, frame);
        c->_esp = esp0;
        c->_ebp = ebp0;
        if (d == 1) continue;                                   /* ExceptionContinueSearch */
        if (d == 0) {                                           /* ExceptionContinueExecution */
            if (flags & EXCEPTION_NONCONTINUABLE) hp_unsupported("RaiseException", "continuing non-continuable exception 0x%08x", code);
            if (rd32(a.ctx + CTX_EIP) != eip0 || rd32(a.ctx + CTX_ESP) != cesp0)
                hp_unsupported("RaiseException", "a handler that changes the context's eip or esp (exception 0x%08x)", code);
            return 0;
        }
        hp_unsupported("RaiseException", "handler disposition %u for exception 0x%08x", d, code);
    }
    hp_unsupported("RaiseException", "unhandled exception 0x%08x (no handler accepted it)", code);
}

/* RtlUnwind: call each registration's handler with EXCEPTION_UNWINDING from the chain head down
   to target_frame, unlinking each. x86 Windows ignores target_ip and returns to the caller with
   eax = return_value and the arguments popped, which is what returning from here does. */
uint32_t RtlUnwind_c(uint32_t target_frame, uint32_t target_ip, uint32_t record, uint32_t return_value)
{
    (void)target_ip;
    _cpu *c = halopad_cpu;
    uint32_t esp0 = c->_esp, ebp0 = c->_ebp;
    frame_area a = area_below(esp0);
    if (record) memcpy(G(a.rec), G(record), REC_SIZE);
    else { wr32(a.rec + 0x00, STATUS_UNWIND); wr32(a.rec + 0x0C, rd32(esp0)); }
    wr32(a.rec + 0x04, rd32(a.rec + 0x04) | EXCEPTION_UNWINDING | (target_frame ? 0 : EXCEPTION_EXIT_UNWIND));
    capture(a.ctx, 4);
    uint32_t frame = X86_ReadFsDword(0);
    while (frame != END_OF_CHAIN && frame != target_frame) {
        if (target_frame && target_frame < frame) hp_unsupported("RtlUnwind", "target frame 0x%08x below registration 0x%08x (STATUS_INVALID_UNWIND_TARGET)", target_frame, frame);
        if (!frame_valid(frame)) hp_unsupported("RtlUnwind", "registration 0x%08x outside the stack", frame);
        uint32_t d = call_handler(&a, frame);
        c->_esp = esp0;
        c->_ebp = ebp0;
        if (d != 1) hp_unsupported("RtlUnwind", "handler disposition %u while unwinding", d);
        uint32_t next = rd32(frame);
        X86_WriteFsDword(0, next);                              /* RtlpUnlinkHandler */
        frame = next;
    }
    if (frame == END_OF_CHAIN && target_frame && target_frame != END_OF_CHAIN)
        hp_unsupported("RtlUnwind", "target frame 0x%08x not on the chain", target_frame);
    return return_value;
}

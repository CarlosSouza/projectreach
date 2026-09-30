// xg_runtime.s: entering translated guest code (see xg_host.h).
//
// Guest code runs with x28 = xg_base and uses x27 as scratch. Both are
// callee-saved for host code, so they are saved here.

	.text
	.p2align 2

// uint64_t xg_call(uint32_t fn, uint64_t a..f)
	.globl _xg_call
_xg_call:
	stp	x29, x30, [sp, #-32]!
	mov	x29, sp
	stp	x27, x28, [sp, #16]
	adrp	x28, _xg_base@PAGE
	ldr	x28, [x28, _xg_base@PAGEOFF]
	mov	w27, w0
	mov	x0, x1
	mov	x1, x2
	mov	x2, x3
	mov	x3, x4
	mov	x4, x5
	mov	x5, x6
	bl	_xg_dispatch
	ldp	x27, x28, [sp, #16]
	ldp	x29, x30, [sp], #32
	ret

// uint64_t xg_call_on(uintptr_t stack_top, uint32_t fn, uint64_t a..d)
	.globl _xg_call_on
_xg_call_on:
	stp	x29, x30, [sp, #-48]!
	mov	x29, sp
	stp	x27, x28, [sp, #16]
	str	x19, [sp, #32]
	mov	x19, sp
	mov	sp, x0
	adrp	x28, _xg_base@PAGE
	ldr	x28, [x28, _xg_base@PAGEOFF]
	mov	w27, w1
	mov	x0, x2
	mov	x1, x3
	mov	x2, x4
	mov	x3, x5
	bl	_xg_dispatch
	mov	sp, x19
	ldr	x19, [sp, #32]
	ldp	x27, x28, [sp, #16]
	ldp	x29, x30, [sp], #48
	ret

# G2c — SRW/llasm capability on Custom Edition 1.10 `haloce.exe`

Date: 2026-09-26. Result: **capability report complete; SRW/llasm does not yet translate Halo.** The whole-image run now loads the image, accepts all 55,093 recovered relocations, completes SRW's initial disassembly and fixup passes, and stops in full disassembly. A static scan with SRW's own decoder enumerates everything the llasm backend would still reject, so the gap is known in full without iterating one crash at a time.

Reproduce:

```sh
.venv/bin/python scripts/audit-executable.py          # relocations, SRW hints, instruction list
scripts/build-lifter.sh                                # prints generated/tool-builds/<key>
.venv/bin/python scripts/run-srw.py --build generated/tool-builds/<key>
```

Tool build used: `09571fae21d9-a4d658a6` (SR `ac690ddf`, patch `port/patches/srw-macos-llasm.patch`). Evidence: `docs/artifacts/2026-09-26/G2c/`.

## Whole-image run: what failed and what was fixed

| Stage | First failure | Cause | Fix |
|---|---|---|---|
| Import loading | `Import by ordinal not supported` | Halo imports `OLEAUT32` #8/#9 by ordinal; SRW's ordinal table covers only dsound/WSOCK32/comctl32 | Patch adds `VariantInit`/`VariantClear` |
| Full disassembly | `invalid instruction` at `0x53504a` | A 3-letter ASCII tag (`0x00535046` = "FPS") in image range was treated as a code address, landing mid-instruction | Audit demotes any relocation target inside a decoded instruction (16 cases) |
| Full disassembly | same class | SRW treats code→code references as entries when an adjacent fixup exists | Audit writes `fixup_interpret_as_code.sci` (4,750 decoded targets) and `fixup_do_not_interpret_as_code.sci` (434 data targets) |
| Full disassembly | `Error: -101` at `0x5cce49` (`cmp esi, fs:[0]`) | SRW's Win32 disassembly pass rejects **every** FS-prefixed instruction before llasm conversion; llasm itself only handles `mov`/`push`/`pop` with `fs:[const]` | **Open** — see below |

Diagnostics added to the patch: SRW now prints the address and bytes of an invalid instruction and the last decoded address on any full-disassembly error.

## Static capability scan (exact, with SRW's decoder)

`tools/udis-scan.c` (built and run by `scripts/srw-capability-scan.py`) decodes all 582,765 audited instruction starts with SRW's bundled udis86 1.7.2: **0 invalid**, confirming the audit's boundaries. Halo uses 263 udis86 mnemonics; SRW's llasm backend has cases for **127**. The other 136 cover **11,767 instructions (2.0%) in 265 of 7,219 functions (3.7%)**. (Figures after the audit stopped probing a switch index table at `0x4dec9c` as code; the table in this section predates that correction by a handful of instructions: SSE float 3,753, other 25, and `repe cmpsd`/`cmpsw` counted separately as 17 string compares.)

| Family | Instances | Mnemonics | Functions | Largest |
|---|---|---|---|---|
| MMX / SSE integer | 5,084 | 44 | 101 | `movq` 2,398, `punpckhdq` 539, `punpckldq` 505, `movd` 389 |
| SSE / SSE2 float | 3,770 | 50 | 93 | `movaps` 956, `mulps` 501, `shufps` 400, `movss` 372 |
| 3DNow! | 2,754 | 21 | 83 | `pfmul` 1,326, `pfadd` 755, `pfsub` 152, `femms` 130 |
| x87 not implemented | 134 | 15 | 71 | `fpatan` 64, `ffree` 13, `fscale` 10, `frndint` 9, `fsincos` 9, `fprem` 6 |
| Other | 27 | 7 | 14 | `cpuid` 17, `cmovz`/`cmova` 5, `rdtsc` 2, `cmpsw` 1, `sysret` 1 |

FS-prefixed instructions: 189 in 57 functions — almost all structured-exception-handling frame setup and teardown (`mov [fs:0], ecx/esp` 133, `mov eax, [fs:0]` 38, plus one each of `cmp`, `push`, `pop`). Function-level lists: `generated/analysis/custom-en-1.0.10.0621/srw-unsupported.json` (private).

Other categories the loop asks for:

- **Imports.** SRW loads the 267 static imports. It does **not** process the 9 delay-import DLLs (47 functions); their slots route through the executable's own delay-load helper, which calls `LoadLibraryA`/`GetProcAddress` at run time. Dynamically loaded modules include `d3d9` (`Direct3DCreate9`), `dinput8`, `binkw32`, `vorbisfile`, `keystone`, `strings`, `eula`. All belong to the narrow Windows runtime (`IMPORTS.md`, G3).
- **Unresolved branches.** 5,667 indirect sites (3,661 vtable/struct calls, 596 register, 247 absolute pointer, 34 indexed). SRW resolves these at run time with its pointer-offset mechanism, which is the part G2e replaces with a finite dispatch table.
- **Data in code.** 84 unclassified `.text` regions (8,292 bytes), 66 of them referenced data; supplied to SRW through the hint files.

## What this means for lifter selection

The gap is concentrated: 96% of functions contain nothing SRW's llasm backend lacks. Of the remainder, x87, FS and "other" are small and implementable, either as backend cases or as SRW `instruction_replacements`/`external_procedures`, the mechanism SRW's own Windows ports use. SIMD is the large item: 11,608 instructions in about 260 functions. Two routes:

1. **SIMD is optional.** Halo PC shipped for CPUs without SSE or 3DNow!, and it has a CPUID feature query: detection at `0x5441a0` stores the leaf-1 EDX flags at `0x6bd12c` and the `0x80000001` EDX flags at `0x6bd130`, and a switch at `0x543a30`–`0x543dd8` returns individual feature bits. If every SIMD function has a generic x87/integer counterpart selected by those bits, HaloPad reports a CPU without MMX/SSE/3DNow!, the SIMD functions become unreachable, and SRW needs only the x87/FS/other additions. **This is the decisive next experiment** (call-graph plus oracle run of the feature-dependent initialisation with CPUID controlled).
2. **SIMD is required somewhere.** Add the needed MMX/SSE operations to the llasm backend as helper calls over an explicit guest vector-register file, limited to the functions actually reachable. 3DNow! can always be avoided by clearing its feature bit, because only AMD CPUs had it.

**Bounded alternate (reference comparison only).** xboxrecomp's lifter (`tools/recomp/lifter.py`, Capstone names) matches 226 of Halo's 270 Capstone mnemonics, including MMX via a generic handler and many SSE forms. But it models the x87 stack as C `double` (the PRD rejects this without evidence), emits `/* TODO */` no-ops for unhandled forms (silent stubs), models some packed SSE operations in the low lane only, has no PE frontend and no 3DNow!. It is not a drop-in improvement; it is kept as the comparison point for the G2 selection report.

**G2c status: PASS as a capability report** (every failure category counted by instruction, mnemonic and containing function; whole-image run reproducible). SRW has not produced llasm for Halo yet. That is G2d's work, after the SIMD-optionality experiment decides the scope.

## Update: whole-image llasm output (2026-09-26, later)

SRW now translates the **entire image** in diagnostic mode (`scripts/run-srw.py --diagnostic`, build `ab2a55d63d9a-6f980c58`): all stages to "Finishing" in about 2 s, 46 MB of llasm (33.7 MB code, 12.3 MB data). What made it possible, all recorded:

| Change | Where | Why |
|---|---|---|
| FS forms accepted with a guard | patch: `SR_full_win32.c`, `SR_full_llasm_instr.c` | The Win32 pass rejected every FS prefix; the llasm backend already implements `mov`/`push`/`pop` with `fs:`. The guard fails loudly for any other FS form. |
| `cmp esi, fs:[0]` hand replacement | `config/srw/custom-en-1.0.10.0621/instruction_replacements.sci` | The one FS form without a backend case (CRT local unwind). |
| Explicit traps for unsupported instructions | `scripts/srw-traps.py` | 11,596 `halopad_unreachable_simd` (SIMD functions, unreachable under the plain-CPU contract) and 171 `halopad_trap_unimplemented` (generic-code gaps still to implement). |
| Cross-region flag hints | `scripts/srw-flags.py` → `instruction_flags.sci` | SRW splits code into a procedure at every label and after a conditional jump that is not followed by another; flags do not cross. The script derives 1,101 hint lines from a control-flow graph using SRW's own flag tables (`tools/udis-scan.c` with `udis86_dep.c`), following callee returns where a function returns a result in ZF. |
| Flag-only no-op accepted | patch: `SR_full_llasm_instr.c` | `or eax, eax` with dead flags legitimately emits no code. |
| Image-base literals | `scripts/audit-executable.py` | SRW's llasm path cannot express image-base fixups; the four CRT header checks stay literal `0x400000` and the header page is mapped at the image base (G2e requirement). |
| Diagnostic mode | patch, `HALOPAD_SRW_DIAG=1` | Logs every conversion failure and emits a trap instead of stopping, so one run enumerates all gaps. Strict mode is unchanged and still stops at the first gap. |

### Complete gap census (plain-CPU contract)

| Gap | Count | Notes |
|---|---|---|
| x87 instructions without backend cases | 134 | `fpatan` 64, `ffree` 13, `fscale` 10, `frndint` 9, `fsincos` 9, `fprem` 6, others |
| 80-bit `fld`/`fstp tword` | 164 | CRT math saving and restoring extended values |
| Other integer operand forms | 12 | `rcl bl`, `rol cl`, `bt`/`bts [esp]`, `imul byte`, `not bh`, `fsubr st1, st0` |
| Fused flags on `test r8l, r8h` | 6 | e.g. `test dl, ch` at `0x4b647c` |
| String compares (`repe cmpsd`, `cmpsw`) | 17 | |
| `cpuid` / `rdtsc` / `jecxz` | 17 / 2 / 1 | `cpuid` must return the plain-CPU contract |

Evidence: `docs/artifacts/2026-09-26/G2c/run-*/census.json` (diagnostic run), `generated/analysis/custom-en-1.0.10.0621/srw-traps.json`.

### x87 fidelity finding

SR's llasm support (`SR/llasm-support/llasm_float.c`) keeps the x87 stack as C `double` and ignores the precision-control bits of the control word (only rounding control is used, for integer conversion). The PRD forbids assuming this is equivalent. It is repairable in the support code, which is plain C: if Halo runs with single-precision control (the Direct3D default), rounding each arithmetic result to `float` after computing in `double` reproduces x87 results exactly for add, subtract, multiply, divide and square root (53 ≥ 2×24+2); transcendentals and exponent-range edge cases need oracle comparison. The 80-bit load/store gap becomes exact conversion helpers. The oracle's QEMU-based x87 implements precision control, so it can settle this per function. Measuring Halo's actual control word is part of the first x87 slice.

## Update 2026-09-27: 6 sites left

With lifter `161d2b412a4a-89ccc7fb` the whole-image translation has 6 untranslated instruction sites, down from 153. The 80-bit x87 loads and stores, low-byte rotates, mixed-byte `test`, `fprem` and `fsubr`/`fdivr st(i), st(0)` are now translated. The six left are `rcl bl` ×4 (`0x551d1e`–`0x551d3f`), `imul byte [ecx+0x1d]` (`0x598198`) and `fnstenv`/`fldenv` (`0x5dac13`, `0x5dac22`, the CRT's Pentium FDIV workaround). Each is a named trap. See [G3-RUNTIME.md](G3-RUNTIME.md), "Map loading".

## Update 2026-09-27 (later): 3 sites left

Lifter `24c44b99cfac-05b596a6` translates `rcl` of a byte register by a constant (low or high byte, any count, CF and OF as on x86), which covers the four `rcl bl` sites in Halo's ADPCM sample step `0x551d10`; the `adpcm_step` slice checks it against the x86 oracle. Three sites are left: `imul byte [ecx+0x1d]` (`0x598198`) and `fnstenv`/`fldenv` (`0x5dac13`, `0x5dac22`). See [G3-RUNTIME.md](G3-RUNTIME.md), "Blood Gulch in play".

## Update 2026-09-27: flags across calls

SRW does not carry x86 flags between llasm procedures; `scripts/srw-flags.py` computes the hints that do. It now follows every incoming path of a label that reads flags, including the direct call sites of a function entry: hand-written C runtime code passes flags through `call` (`acos` at `0x5ccd00` calls `0x5d7318`, then `0x5ccd1d` branches on its ZF). 1,029 hint lines; 1 unresolved label (`0x5a142e`). See [G3-RUNTIME.md](G3-RUNTIME.md), "Playing Blood Gulch".

Lifter `9d6c88b47852-7ddadfc0` translates the one-operand `imul` of a byte in memory (AX = AL x m8, signed; CF and OF as on x86), which covers `imul byte [ecx+0x1d]` (`0x598198`) in libpng's transform-info step `0x5980c4`; the `png_transform_info` slice matches the x86 oracle in 300 of 300 cases. Two sites are left, both named traps: `fnstenv`/`fldenv` (`0x5dac13`, `0x5dac22`) in the C runtime's Pentium FDIV workaround, which runs only when `0x63e304` is set, and HaloPad's CPU (no FDIV flaw) never sets it. Translation run `20260928T060918Z-85892`.

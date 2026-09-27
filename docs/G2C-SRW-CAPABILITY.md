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

`tools/udis-scan.c` decodes all 583,266 audited instruction starts with SRW's bundled udis86 1.7.2: **0 invalid**, confirming the audit's boundaries. Halo uses 264 udis86 mnemonics; SRW's llasm backend has cases for **127**. The other 137 cover **11,769 instructions (2.0%) in 267 of 7,247 functions (3.7%)**.

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


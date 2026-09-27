# G2d scope: SIMD code is optional under a plain-CPU contract

Date: 2026-09-26. Result: **PASS for the scoping experiment.** When HaloPad reports a CPU without MMX, SSE, SSE2 or 3DNow!, every traced path that selects a SIMD routine in Custom Edition 1.10 selects its generic x87/integer counterpart. Positive controls (simulated SSE2 and Athlon CPUs) install SIMD routines through the same code, so the gates are live, not dead code.

Reproduce:

```sh
.venv/bin/python scripts/audit-executable.py
.venv/bin/python scripts/srw-capability-scan.py --build generated/tool-builds/<key>
.venv/bin/python scripts/simd-reachability.py
.venv/bin/python scripts/simd-dispatch-experiment.py
```

Evidence: `docs/artifacts/2026-09-26/G2d/simd-dispatch-*/results.json`.

## Contract

HaloPad's CPU identity for this profile (guest-visible `cpuid` and `IsProcessorFeaturePresent`):

- `cpuid` leaf 0: `GenuineIntel`, max leaf 1. Leaf 1 EDX: FPU, TSC, CX8, CMOV only. Extended leaf `0x80000001` EDX: 0.
- `IsProcessorFeaturePresent`: FALSE for 6 (SSE), 7 (3DNow!) and 10 (SSE2).

The translated `cpuid` instruction returns these fixed values; it never reflects the ARM host.

## Where SIMD is selected, and what the plain contract selects

| SIMD user | Gate | Plain-CPU result | Evidence |
|---|---|---|---|
| Math-library function table at `0x615088` (71 entries; 3DNow!/SSE installers `0x59bf15`, `0x59bccd`, `0x59bb5f`) | Selector `0x5953cd(1)`: `IsProcessorFeaturePresent(7)` plus CPUID check, then detection `0x595328` bit 3, then `IsProcessorFeaturePresent(6)` | **0 of 71 entries SIMD**; SSE2 control 42, Athlon control 58 | Oracle, whole selector |
| Halo engine routine at `0x631c54` (`0x4cf650` SSE, `0x4cf7a0` 3DNow!/MMX) | Installer `0x4d07f0` calls Halo's feature query `0x5436a0` with 0x1d and 0x1a after detection `0x5441a0` | query returns 0 and 0; generic `0x4cf4d0` stays | Oracle (detection + query); installer read statically |
| MMX routine `0x589b24` (installers `0x589c3c`, `0x589c74`) | MMX detection `0x595278` (CPUID, registry, `GetSystemInfo`) | generic `0x5898e4`/`0x589a3a` installed; SSE2/Athlon controls install `0x589b24` | Oracle, both installers |
| libjpeg MMX IDCT, upsampling and color conversion (`0x5b67f0`, `0x5b1330`, `0x5ae8c0`, `0x5aecb0`, `0x5af770`) | Halo's JPEG loader `0x587eb0` sets `dct_method` (`cinfo+0x48`) to 1 (fast integer) when MMX detection `0x595278` returns 0; every MMX wrapper checks `dct_method` is 5 or 6 | `dct_method` = 1 → generic paths | Detection by oracle; libjpeg selection read statically (IJG 6b layout) |
| C runtime SSE2 math (`0x5c8b40`, `0x5cb3c0`, `0x5ccf40`→`0x5d7479`, `0x5cdcb2`→`0x5dafb0`) | Flag `0x6bece0`, set by CRT startup `0x5cf251` only if CPUID leaf 1 EDX bit 26 (SSE2) and an OS probe succeeds | flag stays 0 → x87 paths | Read statically |
| CPU probes (`orps` in `0x5436a0`, `emms` in `0x595278`, `movapd` in `0x5cf21c`) | Executed only after the matching CPUID bit is set | not executed | Oracle (detection outputs) |

## What is left for the translator

Of 171 SIMD-containing functions, 130 are reached only through pointers written by the gated installers above, 11 are the directly called roots in the table, 15 are reached only from other SIMD functions, and 15 have no recovered reference (13 libjpeg MMX routines with no absolute address anywhere in the image, 2 runtime-library probes/entries already covered by the SSE2 gate). Under the contract none of them is reached. They stay out of the translated build as **explicit traps**: reaching one is a coverage incident naming the address, never a silent return.

Remaining SRW llasm work for the plain contract is therefore bounded:

| Item | Count | Where |
|---|---|---|
| x87 instructions without llasm cases (`fpatan`, `ffree`, `fscale`, `frndint`, `fsincos`, `fprem`, `f2xm1`, `fldl2e`, `fxam`, `fldenv`/`fnstenv`, …) | 134 in 15 mnemonics, 71 functions | math library and runtime library |
| FS-prefixed instructions (SEH frame link/unlink, one `cmp`, one `push`/`pop` pair) | 189 in 57 functions | SRW's Win32 pass rejects all FS prefixes today |
| String compares without cases (`repe cmpsd`/`cmpsw`) | 17 | generic code |
| Other: `cpuid` 17 (must return the contract), `cmov` 5, `rdtsc` 2, `jecxz` 1 | 25 | detection code, math library |

Residual risk: a pointer to a SIMD routine computed at run time from values that are not absolute addresses would not appear in this analysis. The dispatch table (G2e) turns any such case into a named trap, and the first real run on the Mac will surface it.


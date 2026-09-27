# G2 selection report — native execution architecture

Date: 2026-09-26. **Decision: GO with SRW → llasm → LLVM → ARM64, in HaloPad's original-address model. The bounded alternate lifter is not needed.** G2 closes with one row parked: the physical-device capsule (M07), which needs Chris's device and signing.

## What was proven

| Goal | Result | Record |
|---|---|---|
| G2a executable audit | 583,266 instructions from 7,247 function entries, 0 decode failures and 0 overlaps. 55,109 recovered relocations in SRW format. | [EXECUTION-MODEL.md](EXECUTION-MODEL.md) |
| G2b oracle | Unicorn runs original Halo functions with traps on imports, FS/TEB/TLS, CPUID control and x87 control word. | `scripts/x86-oracle.py`, 17 unit tests |
| G2c capability | Complete census of what SRW's backend lacks; SIMD is unreachable under a plain-CPU contract. | [G2C-SRW-CAPABILITY.md](G2C-SRW-CAPABILITY.md), [G2D-SIMD-SCOPE.md](G2D-SIMD-SCOPE.md) |
| G2d real slices | Six Halo functions (integer, CRT string/memory with nine jump tables, two x87 transforms in Halo's single-precision mode, the map-header validator over real `bloodgulch.map` through HaloPad file services) equal the x86 oracle. | [G2D-SLICES.md](G2D-SLICES.md) |
| G2e address model | Original 32-bit addresses in checked guest memory, a finite dispatch table (119,706 entries), bound imports, and no host code address visible to the guest. All six slices plus a callback slice pass, and six contract cases stop loudly and name the guest address, on **macOS and the iPad Simulator**. | [G2E-ADDRESS-MODEL.md](G2E-ADDRESS-MODEL.md) |

## Why SRW/llasm

- **It translates the whole image.** 119,439 procedures, 283 MB of LLVM IR, and a 21.5 MB ARM64 object compiled in about 70–90 s at `4.5 GB peak. The same object builds for macOS and the iOS Simulator with only the target triple changed.
- **Its failures were local and fixable.** Every defect so far had a specific cause that was fixed in HaloPad-owned code or in the recorded SRW patch, never in the oracle. They are listed in the journal and in each G2 document: import ordinals, FS-prefixed SEH code, flag regions, text-like pointers, three jump-table forms, dropped instruction bytes in SRW's writer, pointer initialization, x87 precision control, and the pointer-offset return path.
- **Nothing is silently wrong.** Unsupported instructions are explicit traps. Unimplemented imports stop with their names. Unknown indirect targets and bad guest addresses stop with the address.
- **The bounded alternate is weaker on exactly the points that matter.** xboxrecomp's lifter matches more mnemonics on paper (226 of 270), but it models x87 as C `double` with no precision control (Halo runs single precision, measured), emits `/* TODO */` no-ops for unhandled forms (silent stubs, which the PRD forbids), handles some packed SSE in the low lane only, has no 3DNow! and no PE frontend. Adopting it would mean building the PE frontend and runtime HaloPad already has, then fixing the same x87 issue.

## Why the remaining translation work is finite

Everything left is enumerated, and each item is a known instruction form or a known missing root.

| Remaining item | Count | Plan |
|---|---|---|
| x87 instructions without backend cases (`fpatan`, `ffree`, `fscale`, `frndint`, `fsincos`, `fprem`, …) | 134 sites | backend cases, each checked against the oracle |
| 80-bit `fld`/`fstp tword` | 164 | exact conversion helpers |
| Integer operand forms (`rcl bl`, `rol cl`, `bt`/`bts [esp]`, `imul byte`, `not bh`, `fsubr st1,st0`) | 12 | backend cases |
| Fused flags on `test r8l, r8h` | 6 | backend case |
| String compares (`repe cmpsd`/`cmpsw`) | 17 | backend cases |
| `cpuid` / `rdtsc` / `jecxz` | 17 / 2 / 1 | `cpuid` returns the plain-CPU contract |
| SIMD routines | 171 functions | explicit traps; never selected under the plain-CPU contract |
| **Audited function entries with no compiled procedure** | **304 of 7,214** | see below |

The 304 untranslated entries break down into 252 with no known reference (found by gap probing or prologue matching), 50 reachable only from those, one reachable only from SIMD code, and **the PE entry point `0x5ccac7`**. In llasm mode SRW does not treat the entry point as a root, because SR's own ports replace startup with a native `main`. So Halo's C runtime startup, and `WinMain` (`0x5445e0`), which only the startup calls, are untranslated. The fix is one line: declare the entry point as a global alias, which makes it a translation root. That is the first G3 step. Any other untranslated function reached at run time traps with its address.

## Risks carried forward

- **iOS address space.** The 4 GiB reservation works in the Simulator, which uses the macOS kernel. A device may need the extended-virtual-addressing entitlement, or a smaller reservation. The device capsule measures this first.
- **Dynamic imports** (46 delay-loaded, plus `GetProcAddress` lookups such as `DirectInput8Create`) need HaloPad's `GetProcAddress` to hand out registered guest addresses. G3.
- **Dispatch cost.** Every indirect transfer does a binary search. It is measured in G3/G4 before any change.
- **Uncertain relocations.** 779 candidates were excluded. A wrong exclusion appears as a trap or oracle mismatch on the path that reaches it.

## Next

G3: the native macOS core initializes. In order:

1. Entry point as a translation root.
2. Enter `0x5ccac7` through `halopad_enter`.
3. Implement each Windows import the startup path actually reaches (heap, TLS, environment, command line, startup info, version, file and registry reads, window and message services) as a real HaloPad service, compared against the oracle where one exists.
4. Close the gap-list items the path reaches.


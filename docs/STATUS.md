# HaloPad status

Updated 2026-09-26. **ACTIVE — phase 2 loop. Translation work is unblocked; no native Halo code runs yet.**

Operating loop: [HaloPad-GOAL-LOOP-PHASE2.md](HaloPad-GOAL-LOOP-PHASE2.md) (replaces G0–G2 of the original loop; inherits the rest).

- **G0′ PASS:** workspace committed locally on `codex/halopad-phase2` at `5c85449`; safety check green; no push.
- **G1a PASS (engineering tier):** `scripts/prepare-patched-client.sh` reproduced the 1.10 files twice in fresh bottles with identical hashes; `scripts/assemble-custom-original.py` built ignored `ref/inputs/custom-original/` (104 files, manifest). Profile `accepted_sha256` = `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`, `input_state` ENGINEERING_DERIVED; `inspect-inputs.py` passes 1.10, fails 1.00. Cross-check: ProcessChecker's same-size 1.10 copy has a different MD5; see `INPUTS.md`.
- **G2b PASS:** `scripts/x86-oracle.py` (Unicorn 2.0.1) loads the accepted image, sets up flat GDT + FS→TEB + TLS, traps every static and delay import by name, records registers/flags/stack/memory writes. CRC32 `0x59f2a2` fixture: 64/64 cases (lengths 0–17, 255–257, 4096, random) plus check value `0xCBF43926`. `tests/test_x86_oracle.py` (8 tests). Commit `8d881ce`.
- **G2a PASS:** `scripts/audit-executable.py` → [EXECUTION-MODEL.md](EXECUTION-MODEL.md). 583,266 instructions from 7,247 function entries; `.text` = 96.62% decoded code, 0.55% jump tables (388), 2.42% padding, 0.42% unclassified (84 regions: 66 referenced data-in-text / 6,877 bytes, 18 unreferenced / 1,415 bytes, all listed privately); 0 failed decodes, 0 overlaps. 5,667 indirect sites (3,661 vtable/struct, 1,129 import, 596 register, 247 absolute pointer). `generated/analysis/custom-en-1.0.10.0621/relocations.csv`: 55,109 relocations in SRW format, self-checked against SRW's loader rules; 779 uncertain excluded. Families: 65,382 x87, 4,898 MMX, 3,927 SSE, 2,754 3DNow!.
- **G2c PASS (capability report):** [G2C-SRW-CAPABILITY.md](G2C-SRW-CAPABILITY.md). SRW now gets through import loading, relocations and fixups on the whole image, and stops in full disassembly on FS-prefixed instructions. Exact scan with SRW's own udis86: 127 of Halo's 264 mnemonics supported; the 137 missing cover 11,769 instructions in 267 of 7,247 functions (SIMD 11,608; x87 134; other 27), and 189 FS-prefixed instructions sit in 57 functions. Patch additions: OLEAUT32 ordinals, failure-location diagnostics. Build `09571fae21d9-a4d658a6`.
- **G2d scoping PASS:** [G2D-SIMD-SCOPE.md](G2D-SIMD-SCOPE.md). With a plain-CPU contract (CPUID FPU/TSC/CX8/CMOV only; `IsProcessorFeaturePresent` 6/7/10 false), every traced SIMD selection picks the generic path: math table 0 of 71 SIMD (SSE2 control 42, Athlon 58), Halo's feature query 0/0, MMX installers generic, libjpeg `dct_method` 1, CRT SSE2 flag 0. Remaining SRW work: 134 x87, 189 FS (SEH), 17 string compares, 25 other; 171 SIMD functions become explicit traps. Audit fix: `.text` addresses used as memory operands are data (removed a mis-probed switch index table).
- **G2d progress — whole-image llasm:** SRW translates the entire image in diagnostic mode (46 MB llasm, ~2 s) with explicit traps, FS support, 1,101 generated flag hints and a hand replacement for the SEH compare. Complete remaining gap list: 134 x87 instructions, 164 80-bit loads/stores, 12 integer forms, 6 fused-flag `test` forms, 17 string compares, 17 `cpuid`, 2 `rdtsc`, 1 `jecxz` (see [G2C-SRW-CAPABILITY.md](G2C-SRW-CAPABILITY.md)). Finding: SR's x87 support uses `double` and ignores precision control; repair plan recorded. Tool build `ab2a55d63d9a-6f980c58`.
- **Lowest unmet goal now:** G2d — close the gap list so strict mode completes, compile the llasm to ARM64, and run the first native-vs-oracle slice (CRC32 `0x59f2a2`); measure Halo's x87 control word. G1b (CrossOver baseline) remains open and parkable.
- **Oracle:** Unicorn 2.0.1 runs original Halo functions on the Mac. Halo's CRC32 at `0x59f2a2` returned `0xCBF43926` for "123456789", popped exactly 12 bytes of arguments, and matched zlib on randomized inputs. Not yet scripted (G2b).
- **Executable facts:** relocations stripped; 267 static imports across 6 DLLs; 9 delay-import DLLs; d3d9/dinput8/etc. loaded dynamically; TLS present; entry `0x5ccac7`.
- **Tooling fix:** `scripts/inspect-inputs.py` compared versions as strings (`621` ≠ `0621`); it now compares numerically. Nine tests pass.
- **Reference environment:** CrossOver chosen for the system-level baseline (G1b). The Parallels "Windows 11" VM is an invalid registration with no files and is not used.

## Parked, waiting on Chris

| Item | Parks | Status |
|---|---|---|
| Legitimate Halo PC product key (used boxed Halo PC) | G1b if the client refuses to start without one; G5–G6 online identity | Not supplied |
| Physical iPhone/iPad + signing | Final G2e capsule row | Not requested yet |
| Second legitimately provisioned player | G5 | Not needed yet |
| Retail `halo.exe` 1.10 + campaign data | G7 | A boxed copy would cover it |

Rights: private-engineering-authorized; publication-not-authorized. No commit or push has occurred yet. No candidate process or Simulator is running.

Known-good commands:

```sh
scripts/doctor.sh
scripts/verify-sources.sh
scripts/check-repo-safety.sh
.venv/bin/python -m unittest discover -s tests -v
```

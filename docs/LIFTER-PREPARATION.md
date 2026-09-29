# SRW/llasm preparation result

Date: 2026-09-06. Result: **PASS for a bounded self-authored tool smoke; G2 NOT PASSED**.

The pinned SRW frontend and llasm converter now build as native ARM64 Mach-O executables. A self-authored PE32/x86 fixture is translated by SRW to llasm, converted to LLVM IR, compiled by Apple clang and executed as native ARM64. The C harness checks 36 input pairs, including unsigned overflow boundaries, and verifies result and guest stack restoration.

This is executed evidence of tool wiring. It is not a comparison with original x86 execution, actual Halo code, whole-program instruction coverage, flags/x87/SSE correctness, arbitrary guest memory, callbacks, threads, or physical iOS. No candidate route is selected by this smoke alone.

## Reproduction and identities

```sh
CMAKE_POLICY_VERSION_MINIMUM=3.5 .venv/bin/python -m pip install -r scripts/requirements-tools.txt
scripts/build-lifter.sh
.venv/bin/python scripts/test-lifter-smoke.py --build generated/tool-builds/f056cf19425c-b11a4f45
```

Use the new directory printed by the build command when rebuilding. It creates a fresh directory rather than silently reusing a stale cache. Its manifest keys the source lock, toolchain lock, patch, builder, compiler and SDK, and hashes both output binaries. The smoke rejects changed identities before executing a tool. Source clones remain clean and push-disabled.

- Source: SR `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3`.
- Tools: SCons 4.8.1, LDC 1.42.0 (DMD 2.112.1 / LLVM 21.1.8), Apple clang 21.0.0.
- Compiler package: official [LDC v1.42.0](https://github.com/ldc-developers/ldc/releases/tag/v1.42.0), with archive SHA-256 and complete extracted file/link tree identity in `toolchains.lock.json`. Local package only; no global installation or security changes.
- Patch: `port/patches/srw-macos-llasm.patch`.
- Fixture target: `arm64-apple-macosx14.0.0`, `-O2 -fno-fast-math -ffp-contract=off -Wall -Wextra -Werror`.
- Successful fresh build evidence: `docs/artifacts/2026-09-06/G0/lifter-f056cf19425c-b11a4f45/`.
- Successful scripted execution evidence: `docs/artifacts/2026-09-06/G0/lifter-smoke-8a1da1d5d6ab/`.
- Initial causal failures and exploratory evidence: `docs/artifacts/2026-09-06/G0/lifter-tools/`.

## Narrow adaptations and observed failures

1. Selecting `OUT_LLASM` is mandatory; SRW defaults to x86 output. Initial native compilation failed because `<malloc.h>` is unavailable. All affected allocation uses inspected were standard C; the patch uses `<stdlib.h>`.
2. After that correction, compilation completed but linking failed on `-static-libgcc`: macOS's `g++` invokes clang++. The Darwin build now explicitly uses clang/clang++ and its default C++ runtime, preserving non-Darwin behavior.
3. Four upstream redundant-parentheses warnings remain visible (`SR_basic.c:52`, `SR_full_llasm_instr.c:4204`, `4211`, `6422`). No global suppression was added. No pointer-width, implicit-declaration or FP warning was emitted in this build. This does not establish semantic correctness.
4. SRW requires `relocations.csv` for stripped relocations. The self-authored fixture contains no absolute addresses, so an explicitly empty relocation inventory is correct for this fixture only. It would not be valid evidence for Halo.
5. Generated llasm includes `extern.llinc` and `macros.llinc`; these are explicitly empty for this import-free arithmetic fixture. Required CPU/memory macros come from the pinned SR support source. No missing Halo import has been stubbed.
6. llasm emits a generic LLVM module without a target triple. Clang's target-override warning correctly failed the strict first compile. The runner now reads the actual target triple from an empty compiler probe and records it in the generated module before compilation; no warning suppression is used.

## The address-model finding

The unmodified llasm global wrapper stores a return-function address in a 32-bit guest stack word after subtracting `_pointer_offset`. The generated return reconstructs a native function pointer by adding that same offset. The fixture therefore places its static stack and generated native code within one checked 4 GiB host window, at a nonzero high base; it rejects an incompatible layout before calling the wrapper. The observed run used offset `0x100000000`.

This is a limited upstream mechanism, not HaloPad's chosen memory contract. Arbitrary high host allocations, invalid/null guest addresses, original guest-code addresses, and independently located host callback objects are not solved. A Halo adaptation must map original guest targets to finite statically compiled dispatch entries, use checked guest-memory translation, and marshal callbacks without truncating unrelated host pointers. Simply enabling `-ptrofs` is insufficient. The current tool result does not justify copying this wrapper into an iOS app.

## Next gate

Return to the lowest unmet goal: identify the authorized Windows reference and obtain the accepted patched Custom input. The supplied archive's 1.0.0.609 client remains rejected. Then inspect actual instructions, relocations, indirect targets and imports before designing the next differential slice. The old supplied executable's ordinal imports and stripped relocation state are clues for that input only, not accepted-profile evidence.

No alternate lifter was attempted. No game code was generated, executed through Wine/Rosetta, or installed on a device. No native Halo core or client exists yet.

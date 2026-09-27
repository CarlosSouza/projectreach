# Pinned source audit

Initial inspection evidence: **SOURCE_ONLY**, except clone/identity verification commands. The dated tool-build follow-up below records subsequent built/executed evidence. Authoritative pins are in `dependencies.lock.json`.

All three exact commits fetched successfully into new ignored clones. Each checkout is clean and detached, has push disabled, and reports no Git submodules. This verifies the Git checkout graph; it does not imply that optional external runtime/build dependencies declared elsewhere are installed or built. References were not edited. Their build scripts were not executed during the initial inspection; later builds run only in ignored source copies as recorded below.

| Reference | Read paths / findings | Consequence |
|---|---|---|
| SR `ac690ddf3010bc3d2c875cb1b5e8da4f53a8d5b3` | Root/SRW/llasm README, SRW/SConstruct, Septerra `Game-Memory.c` Apple pointer-offset paths | Default SRW output is x86; choose OUT_LLASM explicitly in a generated source copy. SCons and D compiler/llasm toolchain need provisioning. The Apple allocator includes low-address/fixed-map logic; do not copy it wholesale as an iOS-safe contract. |
| xboxrecomp `a781596e2181f8d4b15336659897ce07bdd7d56a` | LICENSE, `templates/runtime/recomp_types.h` memory offset, indirect-call predicate and per-thread registers | Top-level comments can lag the implementation. A pointer offset and TLS exist, but code-range permissiveness and x87 `double` need actual differential scrutiny. No selection of this alternate is justified yet. |
| UTP `2f9c9b8f815b1f691a39960bfc96c6a9b8ebda14` | README, RIGHTS_AND_LICENSES, status/completion/PRD/network/data/release documents and Makefile excerpts; `SceneDelegate.swift`; `GameViewController.swift` focus and `releaseGameplayInputs` paths | Study input-owner release and focus transitions. UTP uses existing ARM64 Unreal binaries; its packaging, network claims, FMOD and engine transformations do not solve Halo. No implementation was copied. |

UTP source explicitly releases controller fallback presses, hardware movement, pointer, touch, engine movement/look and menu cursor on focus loss. That is a mechanism to adapt later with Halo mappings and tests, not current Halo input behavior. Its release history and physical observations are upstream evidence only.

The first bounded lifter experiment remains SRW/llasm after the exact Custom input and original reference behavior are available. No generated game source, real Halo function slice, Apple runtime, import shim or native gameplay has been produced. Do not mark G2 from this audit.

Recheck: `scripts/verify-sources.sh`. A dirty/mismatched existing clone fails without reset or automatic update. `scripts/bootstrap-sources.sh` fetches missing exact pins only; a partial failed clone requires inspection and a deliberate repair, not destructive cleanup.

## 2026-09-06 tool-build follow-up

SRW and llasm are now built and executed on ARM64 in isolated generated copies. `port/patches/srw-macos-llasm.patch` records OUT_LLASM selection, standard allocation headers and Darwin clang/C++ driver selection. No reference was modified. Bundled udis86 compilation-unit notices were inspected (permissive redistribution with notice/disclaimer retention); local build copies retain those notices. The simple synthetic fixture links the reference CPU header and llasm macro includes. This is local licensed tool/support use; no Halo code was translated.

LDC 1.42.0 was obtained from the official [pinned upstream release](https://github.com/ldc-developers/ldc/releases/tag/v1.42.0); the archive matches its published SHA-256 and the unpacked tree is independently locked. Its bundled LICENSE records the component license boundaries. See `LIFTER-PREPARATION.md` for execution evidence and limitations.

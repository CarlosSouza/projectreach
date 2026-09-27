# Environment and protected-state inventory

Observed 2026-09-06 on the local Mac. Root revision at entry: `2341fa014be34677eedf30b32d5e5a99c884695d`.

The three originally tracked root documents were already deleted in the worktree and present under untracked `docs/`. `.DS_Store` and `ref/` were also untracked. These existing moves and supplied files were preserved; no reset, clean, staging, commit or push occurred. The supplied documents retain their original contents. Some companion links inside them still use their earlier shorter filenames; use the HaloPad-prefixed files in this workspace.

| Item | Observed value |
|---|---|
| CPU | arm64 |
| macOS | 26.6.2, build 25G83 |
| Xcode | 26.6, build 17F113 |
| macOS / iPhoneOS / Simulator SDK | 26.5; complete output in doctor JSON |
| Apple clang | 21.0.0, clang-2100.1.1.101 |
| CMake / Ninja | 3.27.1 / 1.13.2 |
| Python | 3.8.10; project `.venv` with pefile 2024.8.26 |
| Inspection | 7-Zip 26.02; Apple objdump |
| SCons / LLVM | SCons 4.8.1 in project `.venv`; Apple clang compiles generated LLVM IR directly; standalone LLVM tools not required by the bounded smoke |
| llasm / D compiler | Built natively using locally unpacked, hash-verified LDC 1.42.0; distribution and tree identities locked in `toolchains.lock.json` |
| Deployment targets | macOS 14.0 and iOS/iPadOS 17.0 provisional; synthetic smoke compiled for macOS 14.0; no Halo app build/support claim |
| Booted Simulators / Halo candidates | None observed; none started or stopped |
| Original-client environment | Not identified. Parallels CLI is installed but `prlctl list -a` failed to connect to its service (exit 253). No VM was booted, configured or repaired. |
| Physical devices / signing | Not exercised or accepted |

The root remote is the existing projectreach GitHub repository; no public writes are authorized. Reference clones have every existing remote push URL set to `disabled://halopad-reference-push`. New `.gitignore` rules protect references, original inputs, generated products, private evidence and signing/file types. `scripts/check-repo-safety.sh` checks both the current worktree and staged index; it is not a complete history or arbitrary-secret audit.

`scripts/doctor.sh` records read-only environment, Git/source state, command-name-only process checks and booted Simulators to a unique ignored report. A successful doctor means the implemented preflight checks ran; it does not pass G0 while the reference environment remains unidentified.

Evidence: `docs/artifacts/2026-09-06/G0/bootstrap/` and `docs/artifacts/2026-09-06/G0/doctor-ad36542cb24c/environment.json`.

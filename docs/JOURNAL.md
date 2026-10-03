# HaloPad experiment journal

Append-only. Evidence directories are ignored private records.

## 2026-09-06 — G0 / establish the workspace and identify the next boundary

**Authorization:** Chris assigned `HaloPad-GOAL-LOOP.md` via `/goal`, then explicitly directed use of the supplied `ref/` directory. Requirements and the loop are adopted for this task; research assertions remain source-level evidence, not additional publication or acquisition authorization.

**Hypothesis:** the existing workspace can be protected and the documented exact source pins reproduced without modifying unknown local work. A supplied installer may provide the selected executable, which must be verified rather than inferred from its name.

**Falsifiable result:** references match all three full commit IDs, are clean with push disabled, private paths are ignored and guards reject attempted leakage. Input inspection must either identify 1.0.10.0621 with accepted provenance/hash or fail explicitly. No original or reference execution is required for this read-only step.

**State:** root `2341fa014be34677eedf30b32d5e5a99c884695d`; pre-existing root-document deletions and matching untracked `docs/` files preserved. `ref/HaloCESetup.exe` was present (186,757,648 bytes). Initial directory-only listing missed the installer; subsequent file listing found it before any input-dependent work. No candidate/Simulator was active. No save/profile was selected or modified.

**Commands / observations:**

1. Read all three supplied HaloPad documents. Inspected Git status/revision, host tools, booted Simulators and command-name-only candidate process list. ARM64 Mac/Xcode available; no active project candidate/Simulator.
2. Added `.gitignore`, unaccepted exact-profile configs, source lock and scriptable checks. Wrote initial state and installer hash privately. No source was staged, committed or pushed.
3. `scripts/bootstrap-sources.sh` fetched UTP, SR and xboxrecomp at the exact supplied pins, detached each and disabled all remote push URLs. All succeeded; no Git submodules were declared. Reviewed relevant source/license/build and input/lifecycle paths. No upstream code was copied into the integration source.
4. `7zz l -slt ref/HaloCESetup.exe` identified an NSIS archive. `7zz x -so ref/HaloCESetup.exe haloce.exe > generated/inspection/supplied-installer/haloce.exe` extracted only the named client into ignored storage. No installer/client execution and no credentials.
5. Apple `objdump -p` and pefile inspected the client. Initial `import pefile` failed because it was absent; created project-local `.venv` and installed pinned pefile 2024.8.26, then reran successfully. The original client reports **1.0.0.609**, not 1.0.10.0621. Rejected the selected profile; did not alter expected version/hash.
6. The installed Parallels CLI's `list -a` returned exit 253: “Login failed: Unable to connect to Parallels Service.” No repeated unchanged retries, restart, VM boot or configuration change. Original-client environment remains unidentified.
7. `.venv/bin/python scripts/inspect-inputs.py --profile custom-en-1.0.10.0621 --executable generated/inspection/supplied-installer/haloce.exe` returned exit 2 with version mismatch and no accepted hash. It wrote a private structured PE/import/delay-import/resource/TLS/relocation inventory; no input lock or generated core.
8. `.venv/bin/python -m unittest discover -s tests -v` passed nine tests. Temporary Git repositories prove forced private-input staging and staged disguised executable bytes fail even after a benign worktree replacement. Identity tests reject wrong version/machine/hash and unaccepted profiles.
9. `scripts/doctor.sh` passed implemented environment/source/safety checks. `scripts/verify-sources.sh` and `scripts/check-repo-safety.sh` succeeded. Full G0 remains unmet: original-client environment and selected lifter toolchain readiness are not established. Standalone LLVM tools and SCons were absent at inspected paths; no claim the lifter is built.

**Evidence:** `docs/artifacts/2026-09-06/G0/bootstrap/{initial-state.txt,sources.log,installer-list.txt,supplied-pe.txt,python-dependencies.log,guard-tests.log}`; `docs/artifacts/2026-09-06/G0/doctor-ad36542cb24c/environment.json`; `docs/artifacts/2026-09-06/G1/inspect-f13eeb6d8a2c/pe-inventory.json`.

**Interpretation:** host/source protections and read-only input inventory are executed evidence. Reference implementation findings are source-only. The supplied older client does not satisfy G1 and cannot be the selected-profile translation oracle. A source-level route remains unproven; no evidence invalidates both candidate lifters and no project-level NO_GO is justified.

**Rejected explanations:** matching installer filename is insufficient; extraction is not installation; an installed VM application is not a working authorized Windows baseline; compiling guards is not native Halo execution; offset macros/TLS in references do not pass the iOS architecture capsule. No percentages or downstream gates are promoted.

**Next experiment / stop condition:** identify the authorized original-client environment and acquire the patched Custom client through the accepted original-PC-key route, then run the exact input-inspection command in `HANDOFF.md`. Stop this path on wrong identity, absent accepted provenance or missing baseline, while preserving all evidence. Local-only preparation of the selected lifter remains possible but cannot pass G1/G2. Publication, credentials and physical actions remain explicit boundaries.

**Final verification correction:** the generated acceptance ledger initially included the campaign table header (`Map identity`) because its row filter was too broad. The explicit 36-row assertion caught it; removed that spurious header and rechecked exact M01–M36 identity. Final guard tests still pass (nine tests). Original installer hash and supplied-document bytes match their preserved identities. No project candidate or Simulator was launched during this session.

## 2026-09-06 — G0 independent lifter-tool preparation (second goal turn)

**Previous turn classification:** progress: protected source state, verified wrong supplied input, implemented and tested guards. Re-read current status/handoff/journal, checked Git/source pins and booted Simulators. No newly supplied accepted input or Windows baseline appeared; the same external boundary persists for the second consecutive goal turn.

**Hypothesis:** the selected SRW/llasm path can at least build locally and execute a self-authored PE arithmetic slice as native ARM64 without modifying references or relying on game inputs. **Pass:** exact OUT_LLASM frontend/converter identities, ARM64 binaries and expected native fixture results. **Fail:** causal compiler/converter/runtime failure preserved; no promotion to G2.

**Commands:** installed SCons 4.8.1 in `.venv`; fetched official LDC 1.42.0 macOS ARM64 package, verified published archive checksum and locked extracted tree; built llasm from its pinned D source. Tried OUT_LLASM SRW in an ignored source copy. Initial build failed on malloc.h; stdlib.h substitution exposed macOS static-libgcc linker failure; Darwin clang/C++ selection fixed that without suppressing warnings. Added a documented reproducible patch and license notice.

**Minimized experiment:** authored a 16-byte x86 arithmetic function inside a minimal PE32. SRW initially required an explicit relocation inventory; the fixture has no absolute references, so an empty CSV is correct. The first strict IR compilation failed on a missing target triple; generated an actual compiler target probe and supplied that triple rather than suppressing the diagnostic. One early exploratory command selected a not-yet-created working directory and did not execute; created it before retrying. No missing game behavior was replaced.

**Executed result:** `scripts/build-lifter.sh` created fresh manifest-keyed build `generated/tool-builds/f056cf19425c-b11a4f45`; `.venv/bin/python scripts/test-lifter-smoke.py --build generated/tool-builds/f056cf19425c-b11a4f45` passed 36 arithmetic result/stack-restoration cases as ARM64. No x86 fixture execution, actual Halo differential test, flags/FP/thread/callback/device or gameplay claim. Four upstream redundant-parentheses warnings remain recorded.

**Evidence:** exploratory failures under `docs/artifacts/2026-09-06/G0/lifter-tools/`; fresh build under `docs/artifacts/2026-09-06/G0/lifter-f056cf19425c-b11a4f45/`; scripted execution under `docs/artifacts/2026-09-06/G0/lifter-smoke-8a1da1d5d6ab/`. Exact tool/source/compiler/SDK/patch/input/output identities are in those manifests.

**Interpretation / rejected shortcut:** llasm's wrapper converts native return pointers into offset-based 32-bit words; code and stack must occupy the same representable host window. The bounded fixture checks that constraint before execution, but this is not the arbitrary high-address, checked guest-memory and finite guest-target dispatch contract required for HaloPad. Enabling -ptrofs is not an architectural pass. G0 still lacks an authorized reference environment; G1 still lacks the selected accepted patched client; no goal passes.

**Next experiment:** obtain the accepted 1.0.10.0621 Custom input and original-client environment, then inventory actual code/relocations/imports and choose the first real differential slice. Do not keep expanding synthetic smoke tests in place of that missing evidence. No alternate lifter, game acquisition, VM/device action, public network test or publication occurred. All project candidates and Simulators remain stopped.

## 2026-09-06 — third consecutive external-boundary audit

**Previous turn:** progress, verified by unchanged built SRW/llasm artifact hashes and the saved 36-case native synthetic execution result. No active process/job is being waited on.

**Current authoritative state:** re-read status and journal; checked current Git status, supplied files under ref, candidate command names and booted Simulators. The only supplied acquisition file remains HaloCESetup.exe. Its hash and the extracted older client's hash match the original evidence. The selected profile still has no accepted hash, no ref/inputs installation exists, and no authorized original-client environment has been identified. No new operator response supplied that missing state. All source pins remain clean and push-disabled; repository safety checks pass.

**Audit evidence:** `docs/artifacts/2026-09-06/G0/blocker-audit-3/audit.json`. The same unavailable exact input/original-reference boundary has now persisted across the original goal turn and two continuations. Host protection, input inspection and bounded lifter-tool preparation are completed independently. Additional synthetic cases or speculative shell work cannot resolve the required actual-Halo/reference uncertainty and would not justify advancing the lowest goal. No build or runtime failure is being declared impossible.

**Decision:** mark the goal BLOCKED_EXTERNAL after the required three-turn audit. This is neither completion nor NO_GO. Resume when the accepted Custom Edition 1.0.10.0621 installation and an authorized original-client reference environment are supplied/identified, using the exact command and evidence requirements in HANDOFF.md. Acquisition, credentials, machine configuration and physical/public actions remain operator boundaries under the assigned loop. No unknown work was overwritten, and no candidate/Simulator remains running.

## 2026-09-26 — unblocking review with Chris; phase 2 loop written

**Findings:** The supplied installer is the stock Custom Edition 1.00 file set (all stock maps, shaders, `haloceded.exe`), repackaged in NSIS 2.25. The 1.10 patch (`haloce-patch-1.0.10.exe`, SHA-256 `33818f3f…7508`, Microsoft Authenticode) was fetched from Bungie's original URL via the Wayback Machine. Its cabinet holds `haloupdate.exe` and `patch.rtp`, which targets `haloce.exe`, `haloceded.exe`, `strings.dll`, `binkw32.dll` and `config.txt`. The Parallels service now runs, but its only VM is invalid (no files under `~/Parallels`). No product key exists in the repo or elsewhere on the Mac; a leftover "Halo CE Cracked" Parallels shortcut was found and deliberately not used.

**Commands:** extracted the patch targets from the installer into `generated/patchwork/ce-1.00-to-1.10/`; created CrossOver bottle `halopad-patch` (win10_64); ran `haloupdate.exe processrtp=patch.rtp updateversion=01.00.10.0621`. The updater held a splash window after writing files and was terminated.

**Result:** `haloce.exe` and `haloceded.exe` report 1.0.10.621; `binkw32.dll` and `config.txt` unchanged. `haloce.exe` SHA-256 `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`, MD5 `6388cf3d1f162ce1766a562481f61432` (does not match ProcessChecker's listed MD5; not an independent confirmation either way). `inspect-inputs.py` initially failed on a string version comparison; fixed to numeric; now fails only on the unaccepted hash. Nine tests pass. Relocations are stripped. Unicorn 2.0.1 ran Halo's CRC32 at `0x59f2a2`: `0xCBF43926` for the standard check string, correct stdcall stack effect, matches zlib on randomized inputs (the five apparent mismatches were the harness expecting 0 for empty buffers; Halo correctly returns the input CRC). Capstone's wheel was x86_64-only and was removed.

**Decision:** Chris directed use of the repo copy. Wrote `HaloPad-GOAL-LOOP-PHASE2.md`: G1 split into engineering input (G1a) and CrossOver runtime baseline (G1b, parkable on a key); function-level oracle is the emulator; blocked goals park only their dependents. **Next:** G0′ local commit, then G1a reproduction script.

## 2026-09-26 — phase 2 goal turn 1: G0′ and G1a

**G0′:** created `codex/halopad-phase2`, staged the pre-existing document moves and all scripts/docs/tests/profiles, safety check PASS, committed `5c85449`. Clean status apart from ignored paths.

**G1a hypothesis:** the manual patch result is reproducible by script from the two recorded inputs in a fresh bottle. **Fail:** any differing output hash.

**Failures before the pass (all causal, all fixed in the script):** (1) 7-Zip already expands the cabinet embedded in the patch executable, so extracting `CABINET` separately failed; now a fallback only. (2) The updater stays open with a splash after writing files; `wineserver --bottle` is not a CrossOver option, so the updater was never stopped and the script waited forever. Now the bottle is stopped with `WINEPREFIX=<bottle> wineserver -k` plus a bounded forced kill, and the script fails if the updater is still alive. (3) The patch adds `Content/Gallery/editbox{1050,1080,1440,1600,2160}.png`; hashing now walks subdirectories. The hung run's bottle was removed by the script's exit cleanup on SIGTERM; the user's Sims bottle was verified untouched (last modified Aug 2). Two empty/partial evidence and run directories from failed attempts remain in ignored paths (deletion via `rm -rf` is not permitted in this environment).

**Result:** runs `20260927T013752Z-91939` and `20260927T013822Z-92133` produced identical hashes for all 12 files; patched in ~15 s each; bottles deleted; no Wine processes left. Assembly produced 104 files (8 from the patch; 23 maps) with manifest, read-only. Profile accepted with provenance block; `inspect-inputs.py` reports its provenance and passes 1.10 / fails 1.00; 9 tests pass.

**Cross-check:** ProcessChecker's 1.0.10.621 `haloce.exe` has the same size (2,404,352) but MD5 `7c3fd3b0…2580` vs ours `6388cf3d…ce1766a562481f61432`. Unexplained; engineering-only acceptance recorded, re-check against G1b's key-installed copy. **Next:** G2b oracle harness and G2a audit; G1b dedicated server in CrossOver.

## 2026-09-26 — G2b oracle harness and G2a executable audit

**G2b:** promoted the CRC32 probe into `scripts/x86-oracle.py`. First run faulted on the function's first push at `0xEFFC`: loading GDTR with only an FS descriptor left SS 16-bit. Added flat ring-0 code/data descriptors and reloaded CS/SS/DS/ES/GS. Then 64/64 CRC32 cases and the check value passed. Tests prove FS→TEB/TLS visibility, loud traps for static imports, delay imports and a real Halo call site (`0x479440` → `GetTickCount`), explicit handlers only when supplied, unmapped-access failure, and rejection of the 1.00 image. Capstone 5.0.6 has an arm64 wheel and is now pinned.

**G2a hypothesis:** recursive descent plus jump tables and pointer-seeded entries classify ≥98% of `.text` with no decode conflicts and yield an SRW-valid relocation list. **Iterations (each causal):** (1) first pass: 239 overlaps and 9 bad decodes — pointers from data/immediates into data tables inside `.text` were traced as code (a 16-byte record table at `0x5b9000`). Added a side-effect-free probe decode (rejects undecodable, overlapping, `00 00` and never-emitted instructions) for non-call entries. (2) Large unreached regions were real code (CRT x87 helpers at `0x5da220`, MMX routines at `0x5b2a77`) behind alignment fillers (`lea esp,[esp]`, `mov edi,edi`); the gap sweep now skips fillers and probes. (3) A jump table in `.data` made the gap calculation count non-`.text` bytes. (4) The last 11 overlaps were ordering: lower-confidence pointer entries decoded before later call-reached code; entries not preceded by padding/ret are now deferred until call-reached code is complete — 0 overlaps. (5) 15 relocation targets lay in the PE header: 4 code references (MZ/ImageBase checks) now use SRW's `i` image-base type; 11 data dwords are treated as integers (uncertain). A built-in check mirrors SRW's `LoadRelocations` rules.

**Result:** PASS (see `EXECUTION-MODEL.md`; evidence `docs/artifacts/2026-09-26/G2a/`). Carried forward: 779 uncertain relocation candidates (arithmetic immediates in image range; 313 data dwords pointing into `.text` at non-instruction starts) are validated when a translated path reaches them; the `GetProcAddress` inventory recovers only direct `push offset` names (8), so dynamic imports such as `DirectInput8Create` must be confirmed by tracing. **Next:** G2c whole-image SRW/llasm run.

## 2026-09-26 — G2c whole-image SRW run and capability scan

**Hypothesis:** the built SRW accepts the audited image and relocations; remaining failures can be enumerated completely. **Runs (each changed one cause):** (1) `Import by ordinal not supported`: OLEAUT32 #8/#9, which pefile had silently named; patched SRW's ordinal table. (2) `Error: 3` with no location: added an invalid-instruction diagnostic; it showed `0x53504a`, reached from the constant `0x00535046` ("FPS") stored by `mov [0x657240], 0x535046`. The audit now demotes relocation targets inside decoded instructions (16) and writes SRW hint files for code/data targets in `.text`. (3) `Error: -101`: added a last-decoded-address diagnostic; `0x5cce49` `cmp esi, fs:[0]`. SRW's Win32 pass rejects every FS-prefixed instruction. Rather than iterate crash by crash, built `tools/udis-scan.c` from SRW's udis86: all 583,266 audited instructions decode (0 invalid); 137 of 264 mnemonics lack llasm cases (11,769 instances, 267 functions). Also added `scripts/run-srw.py` (staged, hash-checked, logged runs) and `scripts/srw-patch-edit.py` (regenerates one file's section of the recorded patch). Synthetic lifter smoke still passes (36 cases) on the patched build.

**Interpretation:** the translation gap is concentrated in 3.7% of functions and dominated by SIMD. Halo has a CPUID feature layer (`0x5441a0` detection; feature-bit switch `0x543a30`–`0x543dd8`). xboxrecomp matches more mnemonics on paper but emits TODO no-ops and uses a `double` x87 model; it is not selected. **Next:** SIMD-optionality experiment, then x87/FS/other additions and the first G2d slices.

## 2026-09-26 — G2d scoping: is SIMD optional?

**Hypothesis:** every SIMD routine is selected by CPU feature checks with a generic counterpart, so a plain-CPU contract makes all SIMD code unreachable. **Fail:** any SIMD routine reached from generic code without a feature gate.

**Method:** `scripts/simd-reachability.py` classified 171 SIMD-containing functions by incoming references (130 pointer-only, 11 direct roots, 15 only from SIMD, 15 none). A first pass misclassified `repe cmpsd` as SSE (udis86 shares the name); classification now uses Capstone names. Direct roots were read one by one: CPU probes, CRT SSE2 routines behind flag `0x6bece0`, libjpeg MMX behind `dct_method`, and a 3DNow! tail. Pointer installers: math-table selector `0x5953cd`, Halo's `0x4d07f0`, MMX installers `0x589c3c`/`0x589c74`, libjpeg `0x5b0be0`. Added CPUID control to the oracle (Unicorn `UC_HOOK_INSN`) and `scripts/simd-dispatch-experiment.py` with plain/SSE2/Athlon profiles and explicit handlers (`IsProcessorFeaturePresent` from the profile, registry absent, `GetSystemInfo` single x86 CPU). A first selector run used argument 0, which only restores the generic table; argument 1 enables detection.

**Result:** plain CPU selects generic everywhere (table 0/71 SIMD, Halo query 0/0, MMX installers generic); SSE2 and Athlon controls install SIMD (42 and 58 table entries; query 1 for their feature; MMX routine installed). libjpeg sets `dct_method = 1` when MMX detection fails; the CRT SSE2 flag needs CPUID bit 26. Found and fixed an audit defect: a switch byte-index table (`0x4dec9c`) had been probed as code; `.text` memory-operand targets are now data. **Next:** SRW additions for the bounded list and SIMD functions as traps.

## 2026-09-26 — G2d: SRW produces whole-image llasm (diagnostic mode)

**Hypothesis:** with SIMD as traps and FS/flags handled, SRW reaches output on the whole image, and the remaining conversion gaps form a short, enumerable list. **Sequence (each failure causal):** (1) FS: patched the Win32 pass to strip `fs:` and added a converter guard accepting only implemented FS forms; hand replacement for `cmp esi, fs:[0]`+`je`+`call` at `0x5cce49`. (2) Traps: 11,767 generated replacements. (3) `unknown flags on start of region` at `0x403923`: wrote `scripts/srw-flags.py` (SRW flag tables via `udis86_dep.c`); iterations taught SRW's region rules: replacements are their own regions; `to_set` must go on the path's last instruction, not the writer, or it collides with SRW's fused compare-branch (`mixing flags` at `0x42ac9f`); a conditional jump ends the procedure unless followed by another; flags returned in ZF by a callee (`0x5d72d5`) need hints at its `ret`s. (4) `llasm instruction error` on `or eax, eax` with dead flags: patch accepts flag-only no-ops only when no flags are needed (a first, broader version wrongly covered `test dl, ch`; tightened). (5) Added diagnostic mode (`HALOPAD_SRW_DIAG`); the first diagnostic run crashed in `SR_disassemble_fixup_operand`: image-base fixups index `section[ImageBase]`. The four CRT header checks are now literal constants (header page mapped at the image base under G2e). (6) Diagnostic run completed: 46 MB llasm, census 176 unhandled forms + 6 fused-flag forms.

**Finding:** SR's llasm x87 model is `double` without precision control. Plan: honor precision control in `llasm_float.c` and compare against the oracle; add exact 80-bit conversion helpers. **Next:** close the gap list in strict mode; compile to ARM64; CRC32 native-vs-oracle slice.

## 2026-09-26 — G2d slice 1: Halo CRC32 native on ARM64

**Getting llasm to accept the output (each causal):** (1) `daddr (label + n)` unsupported — caused by text in `.rdata` read as pointers: 3-letter ASCII opcode names ("rcp", "sub") and UTF-16 strings fall inside `.data`'s address range. The audit now treats a text-like dword next to other text as a string (2,376 demoted; this also removed 425 instructions decoded from false entry points). (2) `Proc inside proc`: a replacement followed by an SRW label must close its procedure; traps now end with `tcall <label>|endp` using labels from a first SRW pass. (3) `Instruction outside proc`: traps in code SRW never reaches (functions found only by gap probing) were emitted standalone; traps are limited to SRW-reachable code (484 sites skipped). (4) Stray ` 4`: SRW's `.sci` parser has no comments; a header line with commas became a replacement at a stale address. Notes moved to `config/srw/.../README.md`. (5) `Daddr not a proc`: imports referenced only from data need declarations; `run-srw.py` writes `extern.llinc`. (6) Pointers into `.rsrc` (not emitted by SRW) stay literal. (7) `Instructions after ctcall` in the hand replacement: rewritten in Septerra's `ctcall|tcall|endp|proc` form. `scripts/srw-pipeline.sh` now runs the whole chain; llasm produces 287 MB IR; clang `-O1` builds an 18 MB ARM64 object in ~70 s (4.5 GB peak).

**Slice:** exported `loc_59F2A2` as `c_halo_crc32` via `global_aliases.sci`; wrote `tests/halo_crc32_harness.c`, `port/runtime/halopad_slice_runtime.c` (every unimplemented host service aborts with its name/address; `X86_InterruptFlag` is a real storage cell), `scripts/gen-import-stubs.py` (267 loud IR stubs) and `scripts/run-slice-crc32.py`. First link failed on four runtime hooks (`X86_ReadMemProcedure`, `X86_WriteMemProcedure`, `X86_InterruptProcedure`, `X86_InterruptFlag`); added. **Result: PASS, 201/201 cases equal to the oracle** (return value, stdcall stack effect, no stray writes), check value `0xCBF43926`, executable arm64. This uses SR's pointer-offset model (code, data and stack in one 4 GiB host window), which G2e still has to replace. **Next:** x87 slice with the control word measured; string/switch/real-data slices.

## 2026-09-26 — G2d: six slices, x87 precision, first Windows services

**Measured Halo's x87 mode:** `CreateDevice` flags built at `0x51a94a`–`0x51a983` add `FPU_PRESERVE` only when `0x6b843c` is set, and then Halo itself runs `fninit; fldcw 0x7e` (single precision); otherwise Direct3D 9 sets single precision. The oracle honours precision control (`1/3` = `float(1/3)` at `0x7f`).

**Framework:** generic harness (`tests/halo_slice_harness.c`: stack/register arguments, buffers, pointers into buffers, control word) and `scripts/run-slices.py`; oracle gained pointer arguments/registers and `fpcw`. Slice discovery picked CRT `memmove` (`0x5c83f0`), `strrchr` (`0x5c88c0`), `strncmp` (`0x5c88f0`, needs `jecxz`, deferred), and generic x87 transforms from the math table.

**Failures, each causal:** (1) data references to an aliased procedure (`daddr loc_5834D7`) broke because SRW writes `define` after the data includes — hoisted. (2) Zero-length buffers lost a field in the harness output — marker `-`. (3) `strrchr` crashed in `x86_repne_scasb` on a raw guest address — SR support must be built with `PTROFS_64BIT` (and `-std=c2x` for `nullptr`). (4) `memmove` jumped to 0: llasm fills stored pointers at run time through `ptr_initialize_pointers`; harness now calls it. (5) `memmove` jumped to garbage: masked tables with an unused slot 0 (`and eax,3`), negative-index jumps (`sub ecx,4; jb; jmp [label+ecx*4]`) and downward tables after `neg`; audit now models all three, emits `displaced_labels.sci` and index-adjusting replacements. Two overlaps from marking unused slots as table bytes were removed. (6) Remaining `memmove` crash: entries one byte off — SRW's llasm writer drops instruction bytes from the `.text` data segment and re-aligns labels; patched `SR_output.c` to mirror instruction bytes (292 MB IR now). Result: `memmove` 300/300. (7) x87 slices matched 53-bit but not Halo's 24-bit mode (84/300, 39/300); made `port/llasm-support/` (HaloPad copy) with precision-control rounding for add/sub/mul/div/sqrt: 300/300 in Halo's mode. (8) Map-header slice: implemented `CreateFileA`/`ReadFile`/`CloseHandle` (`port/llasm-runtime/halopad-kernel32.llasm` using SR's `asm-calls.llinc` pattern, C in `port/runtime/halopad_kernel32.c`), `extern.llinc` redirects implemented imports; a first "bad foot" variant was not bad ('foot' is stored as `toof`) — fixed. 6/6.

**Result:** G2d PASS (G2D-SLICES.md). **Next:** G2e address/dispatch contract.

## 2026-09-26 — G2e: original-address model, callbacks, imports; G2 selection

**Hypothesis:** SR's pointer-offset model can be replaced by Halo's own 32-bit addresses in checked guest memory with a finite dispatch table, without changing any G2d result, and the contract holds for callbacks, bad addresses and imports on macOS and the iPad Simulator. **Fail:** any slice regresses, any host code address reaches guest-visible values, or any bad transfer or access continues silently.

**Built:** `scripts/va-model.py` (label values → original addresses, register transfers → `halopad_dispatch`, dispatch table), `port/runtime/halopad_guest.c` (4 GiB no-access reservation, image at `0x400000`, stack, heap, fault handler naming guest addresses, binary-search lookup that traps on unknown addresses), `tests/halo_va_harness.c`, and `run-slices.py --model va --target --run-prefix`. All six G2d slices passed unchanged on macOS (commit `089a596`).

**Callback slice:** found `0x582d1a`, MSVC's vector iterator (`call [ebp+0x14]` per element, `ecx` = element). Halo's own callback at its only call site (`0x589491`) frees through the CRT heap, which isn't available yet, so a capstone scan picked real Halo functions that read `ecx` and only write through it: `0x589484` (zero a 12-byte element) and `0x5cc982` (store vtable `0x5ed490`). 200/200 with guard bytes around the arrays. Fault cases: callback `0x582d1b` (mid-function) traps by address; null and unmapped arrays fault at `0x0` and `0x7f00000c`, where the oracle also faults.

**Leak found:** the linked IR still had 182 `ptrtoint` of code. There were two causes. (1) 174 sites load an import as a value (`mov esi, [__imp_CloseHandle]; call esi`), and SRW emits them as host pointer minus offset, which in the VA model would trap at dispatch. Fix: every static import gets a reserved guest address (`0xFFFE0000 + 16·i`, never mapped), Halo's import table (`0x5df000–0x5df43c`, which held unbound name offsets) is filled at load time, value uses are rewritten, and the imports join the dispatch table. (2) Seven were llasm's C entry wrappers (`c_<alias>`), SR's pointer-offset entry path, which is unused in the VA model and has been removed. The build now fails on any remaining `ptrtoint` of code; every linked module has zero. Import contract: a call through Halo's `CloseHandle` slot returns FALSE and pops 4 bytes; a call through the `Sleep` slot stops with the name; a jump into an import's slot range names the import.

**Simulator:** created the project-owned "HaloPad iPad Pro 13" (iOS 26.5) and left other projects' devices alone. Built with `--target arm64-apple-ios17.0-simulator` and `SDKROOT` set to the simulator SDK, and ran through `simctl spawn <udid>` (environment via `SIMCTL_CHILD_`). Binary platform IOSSIMULATOR, minos 17.0. Identical results: 7 slices and 6 contract cases pass. The 4 GiB reservation works there, but the Simulator uses the macOS kernel, so the device question remains open.

**Result:** G2e PASS on macOS and the iPad Simulator; the physical-device row is parked (M07). Evidence: `docs/artifacts/2026-09-26/G2e/slices-va-*-20260927T0342*Z` and `…T034413Z`.

**Coverage check for the selection report:** 304 of 7,214 audited function entries have no compiled procedure: 252 have no known reference, 50 are reached only from those, and 1 only from SIMD. The last one is the **PE entry point `0x5ccac7`**. SRW's llasm mode does not insert the entry point as a root (`SR_apply_fixup_info` only does so for assembly output), because SR's ports supply their own `main`. So the CRT startup and `WinMain` (`0x5445e0`, reached only from startup) are untranslated. SRW's `global_aliases.sci` inserts roots (`SR_apply_fixup_alias_init`), so the fix is one alias line. **G2 selection: GO** with SRW/llasm ([G2-SELECTION.md](G2-SELECTION.md)).

**Next:** G3. Root the entry point, rerun the pipeline, enter `0x5ccac7`, and implement the startup's imports as real services.


## 2026-09-26 — G3 step 1: the entry point is a translation root

Added `loc_5CCAC7,halo_entry` to `config/srw/.../global_aliases.sci` (documented in its README) and reran `scripts/srw-pipeline.sh` (about 56 s). SRW now emits `halo_entry` and `loc_5445E0` (WinMain): 119,963 procedures, up from 119,439. The VA model has 120,230 dispatch entries and 184 import values (10 new, in the startup code), and removed 8 C wrappers. Untranslated audited entries dropped from 304 to 274: 173 gap-probe and 79 prologue-heuristic entries with no known reference, plus 22 reached only from them. `va-model.py` now takes the target triple from `toolchains.lock.json` when no `haloce.target.ll` exists, and `run-slices.py` resolves `--work`. All 7 slices and 6 contract cases still pass on macOS (`docs/artifacts/2026-09-26/G2e/slices-va-arm64-apple-macosx14.0.0-20260927T035330Z`).

**Next:** a core runner that sets up the guest thread environment (TEB through `fs`, the SEH chain, TLS) and enters `0x5ccac7`. Then implement imports in the order the startup path traps on them.
## 2026-09-26 — G3: Halo's C runtime starts natively; the reachable translation gap list is closed

**Loop:** run the core, read the first trap, implement the real behavior, rerun. `scripts/run-core.py` links the VA translation with the runtime and records every run under `docs/artifacts/2026-09-26/G3/`. Each stop below had one cause.

1. **`GetVersionExA`.** Added the Windows runtime. Services take guest addresses as integers (a guest NULL stays NULL) and trap with their name and the unhandled values (`port/runtime/halopad_win32.h`). They cover guest virtual memory (`halopad_vmem.c`), heaps with host-side bookkeeping (`halopad_heap.c`), process, thread, TLS, critical-section and time services (`halopad_k32_process.c`), and a real TEB with static TLS from the image's TLS directory (`halopad_thread.c`). The reference machine is recorded in [G3-RUNTIME.md](G3-RUNTIME.md).
2. **Structural fix: every import is reached as `hpimp_<name>`.** Implementing a service used to rename the symbol inside the 283 MB translation, which forced a 90 s recompile, and import names like `send` and `bind` would have collided with host symbols. Now adding a service is a relink: a core iteration takes about 5–20 s. `va-model.py` also replaces `haloce.va.ll` only when its contents change.
3. **`mprotect: Invalid argument`.** Apple Silicon has 16 KiB host pages against Windows' 4 KiB. Protection now rounds outward when granting access and inward when removing it. The image's read-only pages (headers, `.text`, `.rdata`, `.rsrc`) are protected on the host, so self-modifying code would fault instead of silently diverging.
4. **Fault at `0x63e1a8` in `.data`.** My section walk read the machine type as the section count (`pe+4` instead of `pe+6`). Fixed.
5. **`GetModuleHandleA("kernel32.dll")`.** Added the module layer (`halopad_modules.c`): XP SP3 module handles, loadable and absent DLLs, and a `GetProcAddress` export registry (every static import, the 46 delay-load imports, and `config/runtime/dynamic-exports.txt`), each with a guest address that dispatches to its service or to a named trap.
6. **`GetProcAddress(kernel32, FlsAlloc)`.** FLS doesn't exist on XP SP3, so the lookup returns NULL with `ERROR_PROC_NOT_FOUND` and the CRT uses TLS, as on the real machine. There is an explicit absent-exports list.
7. **`GetStringTypeW`.** The CRT builds its code-page tables here. I didn't guess the Windows tables: `scripts/gen-nls-tables.py` generates them from Microsoft's `bestfit1252.txt` (unicode.org, sha256 pinned). Checking paid off: best-fit maps U+0191 to `0x83`, not `F`, and has no entry for U+039C, so `_mbcasemap[0xB5]` is `?`.
8. **`fnclex` at `0x5cccf0`, the first gap-list instruction on the path.** In the backend and llasm support I implemented `fldpi`, `fldl2e`, `fpatan`, `frndint` (by rounding control), `fscale`, `f2xm1`, `fsincos` and `fxam`. `fnclex` and `ffree` change no modelled state (the status word has no exception bits or tag word). This needed flag-table entries in `udis86_dep.c` and `funcv` declarations for llasm. Found and fixed on the way: `srw-capability-scan.py` read the unpatched upstream backend, so it never saw HaloPad's own cases.
9. **Indirect call to `0x5cf251`.** This is a CRT initializer reached only through the initializer table (`0x61501c`). The audit had rejected it as data because its probe treats `pushfd` as never-emitted, but the CRT's CPUID check is `pushfd; pop eax`. That idiom is now accepted. The other 155 unresolved data→code values were checked: 150 land inside instructions (FourCC tags such as "ESP", constants), and the rest point at data tables in `.text`.
10. **`cpuid` at `0x5cf282`.** Implemented `cpuid` with exactly the plain-CPU contract (other leaves trap), `rdtsc` (1 GHz from the monotonic clock), `jecxz`, and `repe`/`repne cmpsw`/`cmpsd`. The compares use a support routine that skips the prefix and stops before the last compared element, then one ordinary compare in the translated code sets the flags exactly. I checked the generated code at `0x44dae4` by hand; an oracle slice for a `cmpsd` site is still owed.

**Result:** the srw-traps census now lists **no** unimplemented instructions in reachable generic code. Only the 11,138 SIMD sites remain, and the plain-CPU contract never selects them. The core runs Halo's whole C runtime startup natively (heap, TLS, locale tables, initializers, CPU detection) and enters Halo's own code. **Current stop:** `LoadLibraryA("strings.dll")`, the game's string-resource DLL. Regression on this build: 8 slices (new: Halo's `strncmp` `0x5c88f0`, 200/200, which uses `jecxz` and `repe cmpsb`) and 6 contract cases pass (`docs/artifacts/2026-09-26/G2e/slices-va-arm64-apple-macosx14.0.0-20260927T043109Z`). Tool build `f25d5db51c1f-211a470e`.

**Next:** `strings.dll` exports nothing, so Halo can only use its resources. Map its sections into guest memory as data, without running its `DllMain` (any attempt to run its code has no dispatch entry and traps). Then implement the resource services (`FindResource*`, `LoadResource`, `LockResource`, `SizeofResource`, `LoadStringA`) over PE resource directories in guest memory.

## 2026-09-26 — G3: resources, registry, kernel objects; Halo's WinMain runs to window setup

- **`LoadLibraryA("strings.dll")`.** Halo reads the handle (`0x6bde88`) only in resource calls (`FindResourceExA`, `LoadResource`, `LockResource`, `LoadStringA`, `LoadBitmapA`, resource dialogs), and the DLL exports nothing. So it is mapped as a read-only image at its preferred base `0x3f800000` from the game directory, without running its `DllMain` (its own CRT init). Its code has no dispatch entries, so any transfer into it traps. Added resource services over PE resource directories, with XP's neutral-language fallback (`halopad_resources.c`).
- **`RegOpenKeyExA`.** Halo reads HKLM `...\Halo CE\PID` (setup's product-key-derived ID), HKCU `...\Halo CE\gamma` and `FIRSTRUN`, HKLM `Software\Microsoft\Direct3D`, and HKCU GameSpy `Crypt`. Added a persistent registry (`halopad_registry.c`) seeded from `config/runtime/registry-machine.txt`, which holds only the Direct3D key. **HaloPad never creates PID or any key-derived value**: absent values fail with `ERROR_FILE_NOT_FOUND`, as on a machine where setup never ran with a key. Each core run gets a fresh registry file in its evidence folder.
- **`CreateMutexA`.** Kernel objects on host synchronization (`halopad_sync.c`): mutexes with owner and recursion, manual and auto-reset events, one lock and condition variable for all waits, named objects with `ERROR_ALREADY_EXISTS`, and `CloseHandle` routing. Thread ids are thread-local, ready for `CreateThread`.
- **`VirtualProtect(0x2fec68, 1, PAGE_NOACCESS)`.** `WinMain` (`0x5445e0`) fills 8 KiB of its frame with `0xEE` and makes one page in the middle no-access, as a stack-smash sentinel. Protection is now tracked per 4 KiB page: `VirtualProtect` returns the page's previous protection and `VirtualQuery` reports it, and image pages carry Windows' own protections (`.text` is `PAGE_EXECUTE_READ`). A 4 KiB no-access page inside a 16 KiB host page cannot be enforced without blocking live stack, so it stays accessible. That is the permissive direction recorded in G3-RUNTIME.md; the sentinel would not fire.

**Current stop:** `LoadCursorA`. Startup has reached window setup (USER32), and Direct3D 9 comes next. Per the PRD, USER32 becomes a window and message shim over host-owned windows (the AppKit/Metal view attaches with the graphics work), and Direct3D 9 becomes an API-level façade over Metal built from Halo's actual calls.

## 2026-09-26 — G3: callbacks, window creation; Halo reaches its first-run license check

- **Guest callbacks** (`port/runtime/halopad_callback.c`). A runtime service can call translated Halo code on the same guest stack: arguments are pushed, `halopad_enter` runs the procedure to the host sentinel, and Windows' stdcall callback convention is checked (arguments popped; ebx/esi/edi/ebp kept; DF clear), trapping on any violation.
- **USER32, part 1** (`halopad_user32.c`): `LoadIconA`/`LoadCursorA`, window classes, the desktop window (the Mac's main display), `AdjustWindowRect`, `GetWindowRect`/`GetClientRect` with XP classic metrics, and `CreateWindowExA`, which sends `WM_GETMINMAXINFO`, `WM_NCCREATE`, `WM_NCCALCSIZE` and `WM_CREATE` to Halo's window procedure (`[0x6e1490]`), plus `DefWindowProcA` for those messages. **Correction:** these are implemented but not yet exercised by the core. The license check (`0x544718`) runs before Halo's window-creation function (`0x5191d0`). The callback mechanism itself is exercised: Halo's CRC32 entered through `halopad_call_guest` matches the oracle 121/121, with the stdcall convention check passing (slice `crc32_via_host_callback`).
- **Paths.** `halopad_host_path` maps relative paths and paths under `C:\Program Files\Microsoft Games\Halo Custom Edition` into the game directory; anything else is refused.
- **First-run license (`EBUEula`).** With no `FIRSTRUN` value, Halo loads `eula.dll` and calls `EBUEula("Software\...\Halo CE", <eula.rtf path>, 0, 1)` (cdecl) and exits on FALSE. Disassembly of the real DLL shows that acceptance writes REG_DWORD `FIRSTRUN=1` under `HKCU\<key>`. HaloPad's replacement (`halopad_eula.c`) does exactly that, **but only after the player has accepted this exact license file** with `scripts/accept-eula.sh`, which shows the game's `Eula.rtf`, runs only at an interactive terminal, requires typing `I ACCEPT`, and records the file's SHA-256. Without that record it declines, as the real DLL does when the player declines.

**Result:** the core runs from the PE entry point through Halo's C runtime, `WinMain`, the registry and the single-instance mutex to the license check, then exits cleanly through Halo's own decline path (`ExitProcess(1)`). Evidence: `docs/artifacts/2026-09-26/G3/core-arm64-apple-macosx14.0.0-20260927T044423Z`. Regression: 9 slices (new: `crc32_via_host_callback`) and 6 contract cases pass (the "unimplemented import" case now uses `SetSecurityDescriptorGroup`, since `Sleep` is implemented).

**Needs Chris:** accept (or decline) the Halo license by running `scripts/accept-eula.sh`. The agent will not accept a license on the player's behalf. Everything after first run (the splash bitmap through GDI, the rest of USER32, then Direct3D 9 on Metal) is behind it.

## 2026-09-27 — Direct3D 9 groundwork: inventory, capability contract, COM layer, IDirect3D9

Work that doesn't depend on the license (still unaccepted, so the core still stops at Halo's first-run check):

- **Inventory** ([D3D9-INVENTORY.md](D3D9-INVENTORY.md), `scripts/com-inventory.py`). There are 2,546 static device-method call sites across 41 methods; the device pointer is `0x6b840c` and `IDirect3D9` is `0x6bd168`. Halo copies `D3DCAPS9` to `0x75c420` and reads 13 fields from it, led by `PixelShaderVersion` (95 reads) and `RasterCaps` (24 reads, depth-bias bits). Shaders reach the device as bytecode: Halo decrypts its own `.enc` collections. An earlier offset table put `PS20Caps` at 268; `D3DVSHADERCAPS2_0` is 16 bytes, so it is 264, which I corrected and checked against `sizeof(D3DCAPS9) = 304`.
- **Adapter setup** (`0x580a00`) reads the adapter identifier and caps and hands them to Halo's `config.txt` card database (vendor/device/driver rules).
- **Contract** ([GRAPHICS-CONTRACT.md](GRAPHICS-CONTRACT.md)). This resolves where capability values come from. The identity (Radeon 9700 PRO, driver 6.14.10.6467) only selects Halo's own tuning entry. Every capability, format, mode and multisample answer is a promise about HaloPad's Metal layer, reported absent otherwise. A caps dump from any real card is therefore not needed.
- **COM layer.** `config/runtime/com-interfaces.txt` lists 14 D3D9 interfaces (286 methods, in d3d9.h order). `va-model.py` gives each method a guest address with dispatch, `scripts/gen-com-wrappers.py` generates llasm wrappers for every `hpcom_<I>_<M>_c`, `halopad_com.c` builds read-only guest vtables and keeps object bookkeeping on the host, and unimplemented methods trap as `Interface::Method`.
- **`Direct3DCreate9` and all 17 `IDirect3D9` methods** (`halopad_d3d9.c`). `CreateDevice` traps and prints Halo's presentation parameters until the Metal device exists. DirectInput8/DirectSound8 entry points are registered.
- **Test.** `tests/halo_d3d9_test.c` (`run-core.py --main tests/halo_d3d9_test.c`) creates the object the way Halo does (`LoadLibraryA`, `GetProcAddress`, `Direct3DCreate9(0x1f)` at its guest address) and calls methods through the guest vtable, via dispatch, the wrappers and the stdcall check: **33/33 checks pass**. Regression: 15 slice and contract checks, the core (clean license-check exit) and the unit tests pass.
- **Correction.** Module handles for `dinput8.dll` and the game's DLLs are HaloPad-chosen, not XP base addresses; the comment and docs are fixed.

## 2026-09-27 — First Metal frame: IDirect3DDevice9 on a Metal layer

- **Apple host** (`port/apple/halopad_metal.m`). An AppKit window per device with a `CAMetalLayer`, non-blocking event pumping, an offscreen BGRA8 back buffer and a Depth32Float_Stencil8 depth/stencil target. Clear uses render-pass clears; Present blits into the drawable; readback is for tests. Both runners link AppKit, Metal and QuartzCore (macOS only; the iOS host comes with UIKit).
- **Device** (`port/runtime/halopad_d3d9_device.c`). `CreateDevice` validates the presentation parameters (windowed back buffers take the client size; multisampling and pure devices are refused per the contract) and creates the Metal target on Halo's window. Without `D3DCREATE_FPU_PRESERVE` it sets the translated x87 control word to single precision and round-to-nearest, as Direct3D 9 does. Full state is kept (render, texture-stage and sampler states, transforms, viewport) from Direct3D 9's documented defaults, with a separate validity table so defaults such as `STENCILMASK = 0xFFFFFFFF` aren't mistaken for markers. Also implemented: scenes, whole-target Clear, whole-buffer Present, caps, display and creation queries, cursor visibility, identity gamma ramps (a non-identity ramp stops until it's applied at presentation), and software vertex processing for mixed devices. Everything else still traps as `IDirect3DDevice9::Method`.
- **Test** (`tests/halo_d3d9_test.c`). The class and window are created through their guest exports, with user32's `DefWindowProcA` export as the window procedure, so `CreateWindowExA`'s creation messages went host → guest dispatch → native. Then CreateDevice, the defaults, x87 precision, a scene, Clear to `0xff336699` and Present; the pixel read back from Metal at both corners is `0xff336699`. **61/61 pass.** Regression: 15 slice and contract checks, the core (clean license-check exit) and the unit tests pass.

**Next:** resources, meaning textures (Lock/Unlock into Metal textures), vertex and index buffers, and vertex declarations and shaders (bytecode capture), then the first draw calls.

## 2026-09-27 — Direct3D 9 resources and bindings

- **COM.** Objects the device has bound (textures, streams, indices, declarations, shaders) now survive their last public `Release` until they are unbound, as with Direct3D 9's internal references. `HPCOM_FWDn(Interface, Method, impl)` lines generate wrappers for methods shared across interfaces.
- **Resources** (`port/runtime/halopad_d3d9_resources.c`):
  - Textures: mip chains; the contract's formats with DXT block pitches; `LockRect`/`UnlockRect` on guest memory with rectangle offsets, lock and flag checks and dirty tracking for upload; `AUTOGENMIPMAP` allowing only 0 or 1 levels; LOD and priority for managed textures.
  - Level surfaces share the texture's reference count; `GetContainer` returns the texture.
  - Vertex and index buffers: locks by offset and size, descriptions.
  - Vertex declarations, validated up to `D3DDECL_END`.
  - Shaders: the version must be within the contract (vs 1.1/2.0, ps 1.1–1.4/2.0) and the bytecode is walked token by token to its END; `HALOPAD_SHADER_DUMP` writes each shader out for analysis.
- **Device bindings.** Textures, stream sources (divider 1 only; instancing traps), indices, `SetFVF`/`SetVertexDeclaration` (each replaces the other), vertex and pixel shaders, and shader constants F/I/B with range checks.
- **Test.** `tests/halo_d3d9_test.c` now covers these through guest dispatch, including a texel written through `LockRect` read back via a rectangle lock, the surface/texture shared count, a texture Released to 0 while bound staying alive until unbound, a real vs_1_1 and ps_2_0 program, and constant range checks: **99 checks pass**.

**Next:** draws, meaning shader bytecode → Metal Shading Language translation, pipeline state from render states, and upload of dirty textures and buffers.

## 2026-09-27 — Halo's shaders extracted with Halo's own code; translator scope measured

- **Extraction.** Halo loads `shaders\vsh.enc` and `shaders\EffectCollection_ps_%d_%d.enc` through `0x51d0f0`: ecx is the file name, cdecl, and it reads the file and then decrypts it with `0x51d080`. `tools/halo_shader_extract.c` (`run-core.py --main tools/halo_shader_extract.c`) runs that translated routine and writes the plaintext to `generated/analysis/shaders/`, for analysis only. Added `GetFileSize` and `halopad_call_guest_ex` (sets ecx; stdcall or cdecl). Fixed on the way: `GetFileSize`'s wrapper at first declared its output pointer as a host pointer; it now takes a guest address like the other services.
- **Formats.** `vsh.bin` is `[u32 bytes][bytecode]` records (64 vs_1_1 programs). An effect collection is a u32 effect count, then per effect a name and shader count, then per shader a name, a **u32 token count** and the tokens, ending with a 32-hex-digit checksum string.
- **Census** ([SHADER-CENSUS.md](SHADER-CENSUS.md), `scripts/shader-census.py`, aggregate counts only). There are 804 programs. The collection Halo loads under the contract (ps 2.0) holds 128 ps_1_1, 12 ps_1_4 and 128 ps_2_0 shaders, so the translator needs ps_1_1 in every configuration. Vertex shaders use 13 opcodes plus relative constant addressing. Pixel shaders are dominated by `mul`/`tex`/`mad`/`lrp` with the ps_1_x source modifiers (1−x, bx2, bias, x2), co-issue, `phase`, destination shifts, `_sat`/`_pp` and a few `texm3x2`/`texm3x3` bump and specular operations.

**Next:** the translator (Direct3D shader bytecode → Metal Shading Language), checked against all 804 programs compiling with Metal, then pipeline state and draws.

## 2026-09-27 — Shader translator: all 804 of Halo's programs compile with Metal

- **Translator** (`port/runtime/halopad_shader.c`): Direct3D 9 bytecode (vs_1_1/2_0, ps_1_1–1_4/2_0) → one MSL function per program over a fixed interface (varyings: position, colour 0–1, texture coordinates 0–7, fog, point size; constants in fixed structs). Direct3D rules it reproduces:
  - ps_1_x: constants clamped to ±1, results clamped to ±8 (the contract's `PixelShader1xMaxValue`), colour inputs saturated.
  - Co-issued ps_1_x pairs compute both values before writing.
  - The texture-register meaning differs by version: sampled value in 1.1–1.3, coordinates in 1.4/2.0.
  - `tex`/`texcoord`/`texld`/`texcrd` (projected, _dz/_dw), `texm3x2` and `texm3x3` including the `vspec` reflection (cube by default), `cnd` (r0.a in 1.1–1.3), `cmp`, `lrp`, the matrix macros, `lit`/`dst`, `rsq`/`log` on |x|, `pow` on |x|.
  - vs_1_1 `mov a0` floors and vs_2_0 `mova` rounds; relative constants are bounds-checked (out of range reads 0); `def` constants take precedence and keep their exact bits.
  - Every source modifier, destination shift and `_sat`; alpha test and fog applied after the pixel shader from uniforms; a vertex-side pixel-centre fixup uniform.
  - Anything else is refused with the opcode or register named.
- **Coverage** (`scripts/shader-compile-test.sh`, `tools/shader_compile_test.m`): all **804** programs Halo ships (64 vs, 740 ps, all three collections) translate and compile with Metal.

**Not yet shown: that they compute the right values.** Next is a reference interpreter written from the Direct3D rules, with each program run on Metal against it on random inputs (as with the x86 oracle); then pipelines and draws.

## 2026-09-27 — Shader translator checked against an independent reference: 804/804

- **Method** (`scripts/shader-diff.py`, `tools/shader_run.m`, translator test mode). Each program runs as a Metal compute kernel: same generated body, inputs and outputs in buffers, a kill flag instead of discard, sampling at level 0. It also runs in a numpy reference interpreter written separately from the Direct3D 9 rules, on random inputs and constants. Textures are ramps that bilinear filtering reproduces exactly (2D, volume), rotated per stage. Cube maps are direction-encoding textures, continuous across faces, so Metal's seamless filtering changes only in-between values; a first version with flat per-face colours failed 122 cube programs purely from seamless edge filtering, which is why it was replaced.
- **Translator fixes found by writing the reference:**
  - Scalar instructions read `.w` when the source has no replicate swizzle.
  - `nrm` scales all four components.
  - vs `expp`/`logp` (structured partial-precision results, unused by Halo) are refused.
- **Result** (1,024 cases per program, seed 7): all 804 agree. 795 agree strictly; 9 are within texture precision (differences in at most 0.8% of cases, at most 0.024, only in programs that sample textures, scattered differently for each seed: 8-bit texels and bilinear precision amplified by bx2/x2/shift arithmetic, as on Direct3D 9 hardware). **0 fail.** Evidence: `docs/artifacts/2026-09-27/G4/shader-diff-20260927T053005Z`.
- **Limits of this check:**
  - Both sides were written from my reading of the rules, so a shared misreading would go unseen; comparison with original-client renders (G1b) is still needed.
  - Not exercised: alpha-test and fog paths other than the defaults, projected `tex`, implicit-LOD sampling and mipmaps, and the vertex pixel-centre fixup.

## 2026-09-27 — First Direct3D 9 draws on Metal

- **Metal layer** (`port/apple/halopad_metal.h`/`.m`). Plain C descriptors for pipelines, depth/stencil states, samplers, textures, buffers and draws. Each frame records into one command buffer and shares a render encoder, which clears end; Present commits. Pipelines, depth states and samplers are cached by content and libraries by source. Inputs a shader reads with no vertex element read (0, 0, 0, 1) through a constant-step buffer.
- **Draws** (`port/runtime/halopad_d3d9_draw.c`; shared structs moved to `halopad_d3d9_internal.h`):
  - `DrawPrimitive`, `DrawIndexedPrimitive`, `DrawPrimitiveUP`, `DrawIndexedPrimitiveUP`.
  - Declarations and FVFs map to Metal vertex descriptors by usage and index against the vertex shader's `dcl` inputs.
  - Blend (including BOTH(INV)SRCALPHA, separate alpha, write mask, blend factor), depth/stencil (two-sided), cull (front faces clockwise), wireframe, viewport, slope bias.
  - Constant `DEPTHBIAS` goes into the vertex shader in window-depth units.
  - Alpha test and vertex fog go through the pixel-shader constants.
  - Textures upload dirty levels with format conversion or swizzle; samplers map filters, anisotropy and address modes, with border only in Metal's three colours.
  - Buffers upload into a new Metal buffer whenever dirty, so later rewrites never affect an earlier draw in the frame.
  - Fans become lists; draws outside a scene are invalid; *UP draws reset stream 0 and the indices.
  - Still stopping with a message: fixed-function processing, scissor, table fog, user clip planes, point sprites, sRGB, LOD bias.
- **Test** (`tests/halo_d3d9_test.c`, 118 checks, all passing): a clockwise triangle draws green; a counter-clockwise one is culled under the default `CULL_CCW` and draws red with `CULL_NONE`; a quad covering Direct3D pixel `[-0.5, 0.5]` lights exactly pixel (0,0) and not (1,0) or (0,1), which would fail without the half-pixel shift or with it reversed; a ps_2_0 `texld` quad shows the 2×2 texture's four texels in the four quadrants.

**Next:** fixed-function vertex and pixel processing (Halo sets `SetFVF`, `SetTransform` and `SetTextureStageState`), render-to-texture and `StretchRect`, scissor, and cube and volume textures.

## 2026-09-27 — Fixed-function Direct3D 9 on Metal

- **What** (\`port/runtime/halopad_d3d9_ff.c\`, used by \`halopad_d3d9_draw.c\` whenever no vertex or pixel shader is bound). Each fixed-function state combination becomes a key (128 bytes); the key generates Metal source once and the program is cached.
  - Vertex side: pretransformed (XYZRHW) vertices, world/view/projection, up to 8 lights (directional, point, spot) with the material taken from the material or a vertex colour, specular, texture coordinate generation (camera-space position, normal, reflection) and texture transforms (count 0/2, as Halo uses), vertex fog (EXP, EXP2, LINEAR).
  - Pixel side: the texture-stage cascade with every blend operation except bump mapping, including the COMPLEMENT and ALPHAREPLICATE argument modifiers and the TEMP register; an unbound texture reads white; TFACTOR.
  - Device: \`SetMaterial\`/\`GetMaterial\`, \`SetLight\`/\`GetLight\`, \`LightEnable\`/\`GetLightEnable\`.
  - Scope was set from a census of Halo's code: the operations it uses are COLOROP 1–5, 7, 10, 25, 26 and ALPHAOP 1–4, 6; texture transforms 0 and 2. Everything outside the implemented set (vertex blending, sphere maps, bump mapping, a result into TEMP-only chains beyond current/temp, projected texture transforms of count > 4) stops with the state named.
- **Test** (\`tests/halo_d3d9_test.c\`, now 131 checks, all passing): a pretransformed textured quad, an unlit coloured triangle, a lit triangle whose brightness follows the light direction, and TFACTOR through the cascade. Regressions: 15 slices, 17 unit tests, 804/804 shaders compile; the core still stops cleanly at the license.
- **Limit:** checked against my reading of the Direct3D rules, not yet against original-client renders.

**Next:** render targets (\`SetRenderTarget\`, \`GetRenderTarget\`, \`GetBackBuffer\`, \`StretchRect\`, \`CreateOffscreenPlainSurface\`), scissor, cube and volume textures, then the USER32 message loop.

## 2026-09-27 — Render targets and StretchRect on Metal

- **How Halo uses them** (call sites in \`haloce.exe\`):
  - \`SetRenderTarget(0, ...)\` at 7 sites switches between the back buffer and about nine surfaces kept in a table at \`0x638a1c\` (20-byte entries).
  - \`GetBackBuffer\` and \`GetRenderTarget\` fetch surfaces for \`StretchRect\` copies (\`0x519112\`–\`0x519154\`).
  - The loading screen (\`0x43ed48\`) creates a 640×480 X8R8G8B8 offscreen plain surface in the default pool, locks and fills it, and \`StretchRect\`s it onto the render target.
  - One \`CreateQuery(9)\` (occlusion) at \`0x53a293\`, not done yet.
- **What** (\`port/runtime/halopad_d3d9_targets.c\`; Metal side in \`halopad_metal.m\`):
  - The Metal target now has switchable colour and depth attachments. The device owns a back-buffer surface and an automatic depth surface, both backed by the target's textures.
  - Render-target textures (A8R8G8B8, X8R8G8B8, R5G6B5; default pool, not lockable) are Metal textures that Metal renders into and samples; X8 reads alpha 1 through a swizzled view.
  - \`GetBackBuffer\`, \`GetRenderTarget\`/\`SetRenderTarget\` (index 0 only, as the contract's NumSimultaneousRTs = 1; the viewport resets to the new target), \`Get\`/\`SetDepthStencilSurface\`.
  - \`CreateOffscreenPlainSurface\` (lockable guest memory, uploaded when read); stand-alone surfaces answer \`GetDesc\`/\`LockRect\`.
  - \`StretchRect\` blits same-size, same-format copies and draws a scaled quad otherwise (point or linear).
  - X8R8G8B8 targets keep alpha at 1 (clears write 1, draws and copies mask it), so destination-alpha blending and copies read it as Direct3D does, even when Halo leaves the X byte 0.
  - A surface bound as the target keeps its texture alive.
  - Still stopping with a message: StretchRect of depth surfaces or into non-target surfaces, and a depth clear while a smaller target is bound (Metal would clear the whole depth buffer, Direct3D only the target's area).
- **Test** (\`tests/halo_d3d9_test.c\`, now 169 checks, all passing, run with Metal's validation layer on):
  - Render a 64×32 texture (green clear, red left half, with the 640×480 depth buffer still attached, which Metal accepts), then sample it across the screen.
  - StretchRect it at 2× (the scaled edge falls on the right pixel), copy the back buffer into it and back, and copy a 4×4 offscreen surface whose X bytes are 0 (it reads opaque).
  - The invalid cases: index 1, NULL, the same surface, a rectangle outside the surface, the managed pool, and locking a render target.

**Next:** cube and volume textures, occlusion queries, scissor, then the USER32 message loop, DirectInput and DirectSound.

## 2026-09-27 — Occlusion queries

- **How Halo uses them.**
  - At start-up (\`0x53a260\`) it creates 1,024 \`D3DQUERYTYPE_OCCLUSION\` queries into \`0x67cd88\`. If the answer is \`D3DERR_NOTAVAILABLE\`, it clears the flag at \`0x67cd80\` and does without them.
  - It brackets draws with \`Issue(BEGIN)\`/\`Issue(END)\` (\`0x53acf7\`), then spins on \`GetData(&count, 4, FLUSH)\` while the answer is S_FALSE (\`0x53ae20\`).
- **What** (\`port/runtime/halopad_d3d9_query.c\`, Metal side in \`halopad_metal.m\`):
  - Occlusion counts come from Metal's visibility counters in counting mode. Every draw pass has a counter buffer (a ring of 65,536 slots).
  - A query that spans several passes gets one counter per pass, and they are summed.
  - If the counted draws are still in the frame being recorded, \`GetData\` submits the frame and waits, so Halo's spin ends on its first call.
  - \`Issue(BEGIN)\` again restarts; \`END\` alone counts 0; \`GetData\` while building or with a buffer under 4 bytes is invalid.
  - Still stopping with a message: other query types, a second query begun while one is counting, and \`GetData\` on a query never issued.
- **Test** (now 198 checks, all passing under Metal validation):
  - A 16×16 quad counts 256 samples, and 0 with \`ZFUNC NEVER\`.
  - A query spanning a \`StretchRect\` (two passes) with a further 8×8 quad counts 320.
  - The invalid cases.

**Next:** scissor if Halo uses it, then the USER32 message loop, DirectInput and DirectSound.

## 2026-09-27 — USER32 message loop, activation, window state and host input

- **How Halo uses them.**
  - The main loop (\`0x544e30\`) is \`PeekMessageA(PM_REMOVE)\`, \`TranslateMessage\`, \`DispatchMessageA\`.
  - The window procedure (\`0x544f40\`, a jump table) handles \`WM_DESTROY\`/\`WM_CLOSE\` (it posts quit), \`WM_SETCURSOR\` (it compares \`GetForegroundWindow\`), \`WM_PAINT\` (\`ValidateRect\`), \`WM_SIZE\`, \`WM_ACTIVATEAPP\`, \`WM_DISPLAYCHANGE\`, focus, \`WM_ERASEBKGND\`, \`WM_INPUTLANGCHANGE\`, \`WM_NCHITTEST\`, \`WM_SYSCOMMAND\` (it blocks move, size and the screen saver) and keys. Everything else goes to \`DefWindowProcA\`.
  - \`MsgWaitForMultipleObjects(0, NULL, FALSE, 20/100, QS_ALLINPUT)\` is a sleep that input wakes (\`0x4cb5ce\`).
  - It reads \`SM_SWAPBUTTON\`, activates an existing instance with \`FindWindowA\`/\`GetWindowPlacement\`/\`SetForegroundWindow\` (\`0x546180\`), and subclasses a dialog control with props and capture (\`0x581890\`–\`0x581b90\`).
- **What** (\`port/runtime/halopad_user32.c\` part 2, \`halopad_keys.c\`, \`halopad_input.h\`; AppKit side in \`halopad_metal.m\`):
  - A message queue: posted messages and input in arrival order, then WM_QUIT, then WM_PAINT for invalid windows. It handles filters, \`PM_NOREMOVE\` and mouse-move coalescing.
  - \`TranslateMessage\` puts the host's typed characters (Windows-1252) ahead of the queue.
  - Activation follows Windows' order: \`WM_ACTIVATEAPP\`, \`WM_NCACTIVATE\`, \`WM_ACTIVATE\`, then \`DefWindowProcA\`'s \`SetFocus\`. A first \`ShowWindow\` sends \`WM_SHOWWINDOW\`, the position-change pair, and \`WM_SIZE\`/\`WM_MOVE\`.
  - Other services: \`SetWindowPos\`/\`MoveWindow\`, minimise/restore, \`DestroyWindow\`, \`Get\`/\`SetWindowLongA\` (subclassing, styles with \`WM_STYLECHANGING\`/\`CHANGED\`), props, \`FindWindowA\`, \`GetWindowPlacement\`, capture, \`ShowCursor\`/\`SetCursor\` (host cursor), \`GetCursorPos\`/\`ClientToScreen\`, \`GetSystemMetrics\`, \`GetAsyncKeyState\`/\`GetKeyState\` (as input arrives, and as messages are retrieved, with toggles), \`MsgWaitForMultipleObjects\`, \`SendMessageA\`/\`CallWindowProcA\`, \`wsprintfA\` (cdecl; reads its arguments from the guest stack).
  - \`DefWindowProcA\` now covers what these produce: close, \`SC_CLOSE\`/\`SC_MINIMIZE\`/\`SC_RESTORE\`, Alt+F4, activation, the size/move reply, paint validation, hit testing, cursor, text.
  - Host input: AppKit keys (Mac key codes to Windows virtual keys and set-1 scan codes, left/right modifiers, Caps Lock), mouse in client pixels, the wheel in WHEEL_DELTA units, app activation, and the close button (asks the window with \`WM_SYSCOMMAND(SC_CLOSE)\`).
  - \`MessageBoxA\` stops and shows its caption and text; dialogs, GDI and the clipboard are not done yet.
- **Test** (\`tests/halo_user32_test.c\`, 60 checks, all passing):
  - DefWindowProcA is the window procedure, so default processing runs for real, and a trace hook records every delivery.
  - Covered: the first-show order, WM_PAINT until validated, keys with lParam bits and WM_CHAR, autorepeat, Caps Lock toggle, extended keys, mouse coalescing, screen coordinates, double click, wheel, filters, window data, waits, \`wsprintfA\`, deactivation and reactivation order, minimise/restore, Alt+F4 through to \`WM_DESTROY\`, the close button, and \`WM_QUIT\`.

**Next:** DirectInput 8 on the same input stream, then DirectSound 8, sockets and threads.

## 2026-09-27 — DirectInput 8 on host input; COM objects by hash

- **How Halo uses it.** \`IDirectInput8\` is at \`0x64c52c\`, the keyboard at \`0x64c730\`, the mouse at \`0x64c734\`.
  - The keyboard (\`0x4946b7\`) is \`NONEXCLUSIVE|FOREGROUND|NOWINKEY\`, with \`c_dfDIKeyboard\` (\`0x5ec4ec\`) and a 32-event buffer. Halo reads one \`DIDEVICEOBJECTDATA\` at a time, re-acquires on \`DIERR_INPUTLOST\`/\`NOTACQUIRED\`, and flushes on overflow (\`0x4935b0\`).
  - The mouse (\`0x4947b2\`) is \`EXCLUSIVE|FOREGROUND\`, with \`c_dfDIMouse2\` (\`0x5ec6f4\`). Halo reads \`DIMOUSESTATE2\` each frame and asks the wheel's \`DIPROP_GRANULARITY\`.
  - Game controllers come from \`EnumDevices(DI8DEVCLASS_GAMECTRL, ATTACHEDONLY)\` with callback \`0x494b30\`.
  - For the record, the other globals are DirectSound: \`0x6e13cc\` is \`IDirectSound8\` (seven \`CreateSoundBuffer\` calls), \`0x6e13d0\` the primary buffer, and \`0x6e13d4\` the 3D listener (eight \`CommitDeferredSettings\` calls).
- **What** (\`port/runtime/halopad_dinput.c\`, interfaces in \`config/runtime/com-interfaces.txt\`):
  - \`DirectInput8Create\` checks the version and interface. \`CreateDevice\` accepts the system keyboard and mouse; other GUIDs are not registered.
  - Cooperative levels are validated. Only the standard data formats are accepted, and only the properties Halo uses.
  - \`Acquire\` refuses when the window is in the background; a foreground device loses acquisition with its window (\`DIERR_INPUTLOST\` once, then \`NOTACQUIRED\`).
  - \`GetDeviceState\` returns the 256 keys, or relative mouse counts and buttons. \`GetDeviceData\` handles one or many events, peek, flush and overflow (the newest are dropped), with sequence numbers.
  - Keys are transitions only; DIK codes come from set-1 scan codes. The mouse gives relative counts from AppKit deltas. An exclusive mouse hides and detaches the host pointer while acquired.
  - Game controllers are not offered yet: enumeration finds none and says so once on stderr.
- **COM bookkeeping.** Up to 65,536 live objects, found by a hash of the guest address (was 4,096 with a linear search on every call). Halo alone creates 1,024 occlusion queries, plus textures and surfaces per bitmap.
- **Test** (\`tests/halo_dinput_test.c\`, 40 checks, all passing), using Halo's own data formats from the image:
  - Version and GUID errors, and cooperative-level validation.
  - Buffered keys with repeats dropped and increasing sequence numbers; overflow and flush.
  - Foreground loss and re-acquire.
  - Mouse counts, wheel and buttons with relative reset, and the error codes.
- The Direct3D (198) and USER32 (60) tests, 15 slices and the unit tests still pass.

**Next:** DirectSound 8 (buffers, 3D listener and buffers, onto Core Audio), then game controllers, sockets and threads.

## 2026-09-27 — DirectSound 8: software mixer on Core Audio

- **How Halo uses it** (\`0x549270\`):
  - \`DirectSoundCreate8(NULL)\`, \`SetCooperativeLevel(PRIORITY)\`, \`GetCaps\`.
  - A primary buffer (\`PRIMARYBUFFER|CTRL3D\`), set to 16-bit stereo PCM at 22,050 or 44,100 Hz by the caps' maximum rate.
  - The 3D listener from the primary: distance factor 3.048 (Halo's world unit in metres), its rolloff factor, Doppler 0, all immediate.
  - A probe for hardware 3D voices with \`STATIC|LOCHARDWARE|CTRL3D\` buffers; with fewer than 16 it uses software buffers.
  - Buffer methods are called through registers (indices 3–20 across \`IDirectSoundBuffer\`, the 3D buffer and the listener), so all four interfaces are implemented in full.
- **What** (\`port/runtime/halopad_dsound.c\`, \`port/apple/halopad_audio.m\`):
  - The caps report no hardware voices, as Windows has since Vista, so \`LOCHARDWARE\` fails and Halo takes its software path. EAX (\`IKsPropertySet\`) is \`E_NOINTERFACE\`.
  - Software mixing at 44,100 Hz stereo float, with Core Audio (the default output unit, or Remote I/O on iOS) converting to the device's rate.
  - Volume in hundredths of a dB, DirectSound's pan law, 8/16-bit mono/stereo PCM, and frequency changes by linear interpolation. One-shot buffers stop and rewind; looping buffers wrap.
  - Play and write cursors (the write cursor is 10 ms ahead). \`Lock\` returns two regions across the end and supports \`FROMWRITECURSOR\`/\`ENTIREBUFFER\`. Duplicates share their data.
  - 3D: listener space (normal and head-relative), min/max distance with the rolloff factor (\`min/(min + rolloff·(d − min))\`), mute at max distance, cones, Doppler with the distance factor, and deferred settings committed by \`CommitDeferredSettings\` (immediate changes also update pending ones).
  - Buffers without \`GLOBALFOCUS\` are silent but keep playing while the game is not in front.
  - The COM wrapper generator only reads written-out \`hpcom_*_c\` functions, so the 3D getters and setters are written out.
- **Test** (\`tests/halo_dsound_test.c\`, 56 checks, all passing). The test mixes by hand and checks samples against the formulas above:
  - Halo's set-up sequence, cursors, volume, pan both ways, frequency and the cursor, interpolated resampling, one-shot end and rewind, looping, background silence.
  - 3D: centre, left and right with rolloff, deferred and immediate-during-deferred, head-relative, cone outside and inside, and Doppler at half pitch.
  - Duplicates, lock wrap-around, and the error codes.
  - A separate probe showed the Core Audio unit pulling about 309 ms of audio in 300 ms on this Mac.
- The Direct3D, USER32 and DirectInput tests, 15 slices and the unit tests still pass.
- **Still open for sound:** the \`DSOUND\` ordinal 9 import (\`GetDeviceID\`, likely Bink's), Ogg Vorbis (\`vorbisfile.dll\`, 4 imports) and Bink (\`binkw32.dll\`, 9) are not done.

**Next:** threads (\`CreateThread\` and friends), sockets (WS2_32/WSOCK32) for network play, the remaining KERNEL32 file and time services, then Vorbis, Bink and game controllers.

## 2026-09-27 — Threads, and a runtime that is safe to share between them

- **How Halo uses them.**
  - Ten \`CreateThread\` sites (\`0x4404b0\` … \`0x57a409\`), with stacks of \`0x4000\` or \`0x10400\` bytes; some are \`CREATE_SUSPENDED\` and then given \`SetThreadPriority\` and \`ResumeThread\`.
  - The C runtime's \`_beginthreadex\`/\`_endthreadex\` (\`ResumeThread\` at \`0x5cb649\`, \`ExitThread\` at \`0x5cb54f\`).
  - \`GetExitCodeThread\` polling at 8 sites, \`TerminateThread\` at two shutdown sites, and \`SleepEx\`.
  - The translated code keeps no mutable globals, and \`halopad_enter\` is reentrant, so host threads can run it side by side, each with its own guest CPU.
- **What** (\`port/runtime/halopad_k32_thread.c\`, \`halopad_thread.c\`, \`halopad_sync.c\`):
  - Each guest thread is a host thread (16 MiB host stack) with its own guest CPU (x87 control word \`0x27F\`) and a guest stack of the image's reserve size, or the requested size if larger.
  - Each thread gets its own TEB (now also carrying the process and thread IDs) and a fresh copy of the static TLS template.
  - The thread procedure is entered through \`halopad_call_guest\`, so the stdcall contract is checked. \`ExitThread\` unwinds to the thread's root.
  - The handle is a kernel object, signaled for good with the exit code (\`STILL_ACTIVE\` until then). The thread holds its own reference, so the handle can be closed early.
  - Also: \`ResumeThread\`, priorities (recorded and reported; the host schedules all threads normally), and \`SleepEx\`. \`CreateThread\` leaves the caller's last error alone.
  - \`TerminateThread\` works on a thread that has ended or never started. Stopping a running thread stops the program with the thread named, because it cannot be done safely in the middle of translated code.
- **Shared runtime state made thread-safe:**
  - Per thread: \`GetLastError\`'s value, the TEB, and TLS slots (\`TlsAlloc\` clears the new slot in every thread).
  - Behind recursive locks: the heap and \`Global\`/\`Local\` memory, virtual memory, the COM table (which Direct3D, DirectInput and DirectSound use), and the critical-section table.
- **Test** (\`tests/halo_thread_test.c\`, 27 checks, all passing, and the same in 10 repeated runs):
  - Thread procedures are real one-argument KERNEL32 services entered at their guest addresses.
  - Covered: suspended start and resume, exit codes and signaled handles, \`ExitThread\`, per-thread TLS and last error, 16 threads at once (events, then heap frees), a critical section blocking a worker, priorities, termination before start, and \`SleepEx\`.
  - Four host threads make 20,000 heap operations each with contents intact.
- The Direct3D, USER32, DirectInput and DirectSound tests, 15 slices and the unit tests still pass; the core still stops at the license.

**Next:** sockets (WS2_32/WSOCK32) for network play, then the remaining KERNEL32 file and time services, Vorbis, Bink and game controllers.

## 2026-09-27 — Winsock on BSD sockets (network play groundwork)

- **How Halo uses it.**
  - WS2_32 and WSOCK32 are delay-loaded. Callers call the \`jmp [slot]\` stubs (e.g. \`socket\` at \`0x582b88\`), not the slots.
  - All the calls come from Halo's GameSpy networking library (\`0x5b9000\`–\`0x5c7000\`):
    - UDP sockets (\`socket(2, 2, 0|17)\`) for game traffic and server queries, with 15 \`sendto\` sites and 6 \`recvfrom\`.
    - TCP (\`socket(2, 1, 6)\`) with \`connect\`/\`send\`/\`recv\`/\`shutdown(SD_BOTH)\` for the master-server list.
    - \`setsockopt(SOL_SOCKET, …)\` including \`SO_RCVBUF\`, \`ioctlsocket(FIONBIO)\`, and \`select\` loops.
  - Addresses come from \`gethostname\`/\`gethostbyname\`/\`inet_addr\`/\`inet_ntoa\`.
- **What** (\`port/runtime/halopad_winsock.c\`):
  - \`SOCKET\` values are handles mapped to host descriptors. Addresses convert between Winsock's and the host's \`sockaddr_in\`. Winsock's counted \`fd_set\`s are served with \`poll\`.
  - Errors are \`WSAE*\`, per thread, and also the thread's last error.
  - Winsock behaviours kept:
    - An oversized datagram fills the buffer and fails with \`WSAEMSGSIZE\`.
    - A non-blocking \`connect\` gives \`WSAEWOULDBLOCK\`, and a refused one shows in select's exception set.
    - \`select\` with no sockets and no timeout is invalid.
    - \`inet_addr("")\` is 0, and there is one \`inet_ntoa\`/\`hostent\` buffer per thread.
  - \`SO_NOSIGPIPE\` keeps a broken connection an error instead of a signal. Options, ioctls and families outside the set stop with their values.
- **Test** (\`tests/halo_winsock_test.c\`, 42 checks, all passing, and the same in 5 repeated runs), on real loopback sockets through the delay-load path:
  - Start-up rules, UDP send/select/receive with the sender's address, truncation, non-blocking reads, \`select\` timeout, \`FIONREAD\`.
  - TCP against a host listener: blocking and non-blocking connect, a refused connect both ways, send/recv/shutdown.
  - Byte order, address parsing and formatting, host names and lookup failures.
- All other suites, the 15 slices and the unit tests still pass; the core still stops at the license.
- **Limits:** these are loopback checks. Joining a real Custom Edition server (G5/G6) still needs the game past its license, and later a product key.

**Next:** the remaining KERNEL32 file and time services, then Vorbis, Bink and game controllers.

## 2026-09-27 — A virtual C: drive; asynchronous reads; time, locale and messages

- **How Halo uses them.**
  - Profiles, checkpoints and screenshots live in \`%s\My Games\Halo CE\`, where \`%s\` is \`SHGetFolderPathA(CSIDL_PERSONAL)\` from \`shfolder.dll\`. \`WinMain\` loads it at \`0x544b0c\` and stops if it is missing; the \`-path\` option overrides it.
  - 17 \`CreateFileA\` sites. The streaming reads (\`0x443072\` …) are \`FILE_FLAG_OVERLAPPED\`, with \`ReadFileEx\` completion routines run by \`SleepEx(0, TRUE)\` (\`0x443560\`), plus one \`ReadFile\` with \`GetOverlappedResult\`. The others are plain reads (\`SEQUENTIAL_SCAN\`), read/write \`OPEN_ALWAYS\`, and \`CREATE_ALWAYS\` writes.
  - Also: \`SetFilePointer\` 18 sites, \`WriteFile\` 10, \`FindFirstFileA\` 13, directories, \`CopyFileA\`/\`DeleteFileA\`, attributes and times.
  - \`FormatMessageA(FROM_SYSTEM|IGNORE_INSERTS|MAX_WIDTH)\` into 2 KB buffers, 12 of them for Direct3D codes. \`GetDate\`/\`GetTimeFormatA\` with \`LOCALE_USER_DEFAULT\` (including \`NOTIMEMARKER|FORCE24HOUR\`). The C runtime's locale calls (\`GetLocaleInfo\`, \`EnumSystemLocalesA\`, \`CompareString\`).
- **What** (\`port/runtime/halopad_kernel32.c\`, rewritten; \`halopad_k32_time.c\`):
  - **The drive.** The install directory (also the current directory) is two layers: the game files, which are never written, under a writable layer in \`HALOPAD_STATE_ROOT/install\`. A game file opened for writing is first copied up.
  - Every other \`C:\\` path is in \`HALOPAD_STATE_ROOT/C\`, where the user profile, My Documents and Temp exist from the start. Other drives do not exist.
  - Names are matched without regard to case on any host file system, which iOS needs. Trailing dots and spaces are dropped, and device and UNC names stop the program.
  - Installed files report \`ARCHIVE\`, as Windows installed them. Deleting installed game files or directories stops the program.
  - **Files.** All five creation dispositions with Windows' last errors. Read/write, 64-bit \`SetFilePointer\`, \`SetEndOfFile\`, flush, size and times (creation time set with \`setattrlist\`), delete-on-close.
  - **Enumeration** merges both layers and returns names in NTFS order with \`.\` and \`..\` (none at a drive root), using Windows wildcards.
  - Also: attributes (\`READONLY\` enforced), directories, copies, disk space, the temp and current directories, and \`SHGetFolderPathA\` for the standard folders.
  - **Asynchronous reads** complete when issued, as Windows may. \`ReadFileEx\`'s routine is queued as an APC on the calling thread and runs at its next alertable \`SleepEx\`/\`WaitForSingleObjectEx\`, which then return \`WAIT_IO_COMPLETION\`. Overlapped \`ReadFile\` sets the \`OVERLAPPED\` status and event; reads at end-of-file give \`ERROR_HANDLE_EOF\`.
  - **Time.** \`GetSystemTime\`/\`GetLocalTime\`, \`SystemTimeToFileTime\`, \`CompareFileTime\`, and \`GetTimeZoneInformation\` with the host zone's daylight rules for the year in Windows' form.
  - **Locale.** One locale, English (US), LCID 0x409, code page 1252: \`GetLocaleInfoA/W\` (incl. \`RETURN_NUMBER\`), date/time pictures and flags, and \`CompareString\` with a word-sort approximation. Other locales stop with their number.
  - **Messages.** \`FormatMessageA\` from a system table of the errors HaloPad produces; unknown codes (such as Direct3D's) give \`ERROR_MR_MID_NOT_FOUND\`, as on Windows.
  - Also: \`GlobalMemoryStatus\` (a 2 GB address space), \`IsBad*Ptr\` from the memory map, \`VirtualQuery\` of free memory (\`MEM_FREE\`, as Windows answers, where it used to stop), error mode, priority class, and \`TerminateProcess\` of itself.
- **Test** (\`tests/halo_files_test.c\`, 83 checks, all passing; a fresh temporary state directory over the real game files):
  - Special folders, case-insensitive access to the maps, creation dispositions and last errors, writes and seeks, truncation, copies, enumeration order and wildcards.
  - Read-only and deletion rules, directory errors, new files landing in the writable layer, and an installed file changed with the game files left untouched.
  - \`ReadFileEx\`'s routine running only at an alertable \`SleepEx\`; overlapped \`ReadFile\` with its event and \`GetOverlappedResult\`.
  - Time conversion and the zone's offset, date/time formatting (including Halo's flags), locale values, string comparison, messages, and memory checks.
- All other suites, the 15 slices and the unit tests still pass; the core still stops at the license. The runners now set \`HALOPAD_STATE_ROOT\` (\`generated/halopad-disk\`, ignored).

**Next:** SEH (\`RaiseException\`, \`RtlUnwind\`, exception delivery), \`CreateFileMappingA\`/\`MapViewOfFile\`, GDI and dialogs for the splash and crash paths, then Vorbis, Bink and game controllers.

## 2026-09-27 — What WinMain reaches next: DxDiag, CryptoAPI, timers

- **Method.** A static call-graph walk from \`WinMain\` (\`0x5445e0\`, depth 6, 1,883 functions) lists the imports reached that HaloPad did not implement.
  - Depth 1: COM start-up with \`CoCreateInstance\` and variants; CryptoAPI; \`timeBeginPeriod\`/\`timeEndPeriod\`.
  - Depth 2: \`DSOUND #9\`, dialogs, GDI, console input, Bink.
  - Deeper: version info, security checks, the clipboard. SEH (\`RaiseException\`) only at depth 6.
- **How Halo uses them** (\`0x580e70\`):
  - \`GetDeviceID(DSDEVID_DefaultPlayback)\` (dsound ordinal 9).
  - \`CoCreateInstance(CLSID_DxDiagProvider, IID_IDxDiagProvider)\`, then \`DxDiag_DirectSound.DxDiag_SoundDevices\`; for each device \`szDescription\`, \`szGuidDeviceID\`, \`szDriverVersion\` and \`szHardwareID\` as BSTR variants, matched against the default device's GUID. It identifies the sound card and skips the step if DxDiag fails.
  - CryptoAPI (\`0x5829e0\`, \`0x582890\`): a \`PROV_RSA_FULL\` \`CRYPT_VERIFYCONTEXT\` context and \`CALG_SHA1\` hashes.
- **What:**
  - \`port/runtime/halopad_ole.c\`:
    - \`CoInitialize\`/\`CoUninitialize\` (per thread, \`S_FALSE\` when already initialized) and \`CoCreateInstance\` (\`CO_E_NOTINITIALIZED\` first).
    - \`StringFromGUID2\`/\`CLSIDFromString\`, and \`VariantInit\`/\`VariantClear\` with BSTRs.
    - A DxDiag provider whose tree describes what HaloPad provides: one sound device, the Core Audio output, with the GUID \`GetDeviceID\` reports and \`DirectSoundCreate8\` accepts. Other classes, containers and properties stop with their names.
  - \`halopad_crypt.c\`: SHA-1 and MD5 hashes via CommonCrypto, with size queries, \`ERROR_MORE_DATA\`, and \`NTE_BAD_HASH_STATE\` after the value is read; plus the WINMM timer period calls.
  - An ordinal import is named \`<dll>_ord<n>\` (\`hpimp_dsound_ord9\`).
- **Test** (\`tests/halo_ole_test.c\`, 33 checks, all passing):
  - Halo's sequence end to end: \`GetDeviceID\` fetched by ordinal; DxDiag's device GUID equal to \`StringFromGUID2\` of it; the BSTR's byte-length prefix; \`VariantClear\`; the index past the last.
  - GUID text round trip; SHA-1/MD5 of "abc" against the published digests; timer calls.
- All other suites, the 15 slices and the unit tests still pass; the core still stops at the license.

**Next:** dialogs and GDI (\`DialogBoxParamA\`, \`LoadBitmapA\`, \`GetDC\`, gamma ramps), version info, the console used by Halo's developer console, Bink, then SEH.

## 2026-09-27 — Host windows per window; GDI splash; gamma at presentation

- **How Halo uses them.**
  - The main window (\`0x5191d0\`, reached from start-up through Direct3D initialisation) loads splash bitmap \`0x86\` from \`strings.dll\` (640×480, 24-bit) and selects it into a memory DC compatible with the window's DC.
  - \`WM_PAINT\` (\`0x545072\`) stretches it over the client area (\`GetClientRect\`, \`GetObjectA\`, \`StretchBlt SRCCOPY\`) until Direct3D presents.
  - Direct3D start-up checks the desktop's \`BITSPIXEL\` (\`0x51a80e\`).
  - The brightness setting saves the display ramp (\`GetDeviceGammaRamp\`) and applies one through both Direct3D's \`SetGammaRamp\` and \`SetDeviceGammaRamp\` on the window's DC (\`0x525ae0\`).
- **What:**
  - **Host windows belong to USER32** (\`halopad_metal.m\`, \`halopad_user32.c\`).
    - A top-level window gets its Mac window when it is first shown. It follows hide, minimise, restore, size, title and destroy.
    - The Direct3D device attaches its back buffer to that window instead of creating its own (a window never shown gets one on device creation).
    - Every presentation, GDI or Direct3D, goes through a pass that applies the window's gamma table. So \`SetGammaRamp\` no longer stops the program, and brightness changes only HaloPad's window, not the Mac's display.
  - **GDI** (\`halopad_gdi.c\`):
    - \`LoadBitmapA\` decodes DIB resources (1/4/8-bit palettes, 16/24/32-bit, \`BI_RGB\`/\`BI_BITFIELDS\`) into 32-bit display bitmaps.
    - Window, screen and memory DCs; \`SelectObject\` with Windows' rules (one DC per bitmap, a stock bitmap in new memory DCs); \`DeleteObject\` refused while selected; \`GetObjectA\`.
    - \`StretchBlt SRCCOPY\` (nearest) onto the window's surface or another bitmap. A window not on screen shows nothing.
    - Also \`GetDeviceCaps\` and \`Get\`/\`SetDeviceGammaRamp\`. Fonts, text, pens, brushes and other raster operations stop with their names.
  - An 11-argument wrapper macro was added for \`StretchBlt\`.
- **Test** (\`tests/halo_gdi_test.c\`, 25 checks, all passing), with Halo's real splash resource:
  - A host window only once shown; the bitmap's size and format.
  - \`StretchBlt\` over the 640×480 client area, with the window's corner and middle pixels equal to pixels decoded independently from the resource.
  - Selection and deletion rules, \`BITSPIXEL\` 32, the gamma ramp round trip, \`ReleaseDC\` rules, and the host window gone with the window.
- All other suites (Direct3D still 198 checks, now on a USER32-owned window), the 15 slices and the unit tests still pass; the core still stops at the license.

**Next:** the developer console's input path, version info, Bink (intro movie), then SEH, Vorbis and game controllers.

## 2026-09-27 — Console, version resources, the administrator check, clipboard

- **How Halo uses them** (reached from the main loop \`0x4ca9c0\`):
  - Console input (\`0x499be0\`).
  - A report header with the module path, date and time, and version resource (\`0x4ca2e0\`, through version.dll's delay stubs).
  - "Is the player a local administrator" (\`0x545c50\`, via \`0x4e7680\`): thread/process tokens, a duplicate impersonation token, the Administrators SID, a security descriptor with an ACL and \`AccessCheck\`. The answer goes to \`0x6399a8\` and picks between two paths.
  - Chat paste (\`0x544ed0\`): \`CF_TEXT\` through \`OpenClipboard\`/\`GetClipboardData\`/\`GlobalLock\`.
- **What** (\`port/runtime/halopad_misc.c\`):
  - **Console.** \`haloce.exe\` is a GUI program with no console, so console calls fail with \`ERROR_INVALID_HANDLE\` as on Windows.
  - **Version.** \`GetFileVersionInfoSizeA\`/\`GetFileVersionInfoA\` read \`RT_VERSION\` from the PE file on disk. \`VerQueryValueA\` walks the version tree and returns ANSI copies of strings in the buffer's spare area, as Windows' ANSI API does.
  - **Security.** \`OpenThreadToken\` (\`ERROR_NO_TOKEN\` when not impersonating), \`OpenProcessToken\`, \`DuplicateToken\`, SIDs, absolute security descriptors, ACLs with allowed ACEs, and an \`AccessCheck\` that maps generic rights and evaluates the DACL. It needs an impersonation token (\`ERROR_NO_IMPERSONATION_TOKEN\` otherwise).
  - The player is a member of Everyone, Administrators, Users, INTERACTIVE and Authenticated Users, as Windows XP accounts were by default.
  - **Clipboard.** \`CF_TEXT\` from the Mac pasteboard in Windows-1252 with CRLF, with Windows' open/close rules.
- **Test** (\`tests/halo_misc_test.c\`, 24 checks, all passing):
  - Console failure.
  - \`haloce.exe\`'s version resource: \`VS_FIXEDFILEINFO\` 1.0.10.621, the translation, and \`FileVersion\` "01.00.10.0621" as ANSI text.
  - The security calls step by step, including denial for a group the player is not in.
  - Halo's own \`0x545c50\`, run from translated code, returning TRUE and recording it at \`0x6399a8\`.
  - Clipboard rules. The test does not touch the Mac's clipboard contents.
- **Contract case updated.** The "unimplemented import" slice now uses \`CreateProcessA\`, which HaloPad deliberately never implements (it launches the crash reporter), because \`SetSecurityDescriptorGroup\` is implemented now.
- All suites, 15 slices and the unit tests pass; the core still stops at the license.

**Next:** Bink (intro movie and its sound), then SEH, Vorbis, game controllers and file mapping.

## 2026-09-27 — Bink as Custom Edition meets it

- **How Halo uses it.** The movie player is \`0x43ed20\`; the 640×480 offscreen surface and \`StretchRect\` noted earlier belong to it, not to a loading screen.
  - It calls \`BinkSetSoundSystem(BinkOpenDirectSound, 0)\`, passing the function rather than calling it, then \`BinkOpen(path, 0)\` for bungie.bik, gearbox.bik, mgs.bik and ending.bik.
  - On NULL the movie is skipped. Otherwise it loops \`BinkWait\`/\`BinkDoFrame\`/\`BinkCopyToBuffer\`/\`BinkNextFrame\`, then \`BinkClose\`.
- **What** (\`port/runtime/halopad_bink.c\`):
  - Custom Edition ships none of those movies (checked: no \`.bik\` in the install), so \`BinkOpen\` finds no file through the virtual drive and returns NULL, as RAD's library does.
  - A movie that exists (the retail game, G7) stops the program: Bink video decoding is not done yet.
  - The per-frame calls can only receive a handle \`BinkOpen\` never gives out, so they stop with the handle.
  - Decorated names are mangled as the dispatch does (\`_BinkOpen@8\` → \`hpimp__BinkOpen_408\`).
- **Test** (added to \`tests/halo_misc_test.c\`, now 26 checks): the sound-system set-up, and \`BinkOpen\` of all four movie names giving NULL.

**Next:** Ogg Vorbis (\`vorbisfile.dll\`: custom map sounds), then SEH, game controllers and file mapping.

## 2026-09-27 — Ogg Vorbis with Xiph's libvorbis

- **How Halo uses it** (\`0x548300\`, \`0x548620\`).
  - Two \`OggVorbis_File\` structures inside its sound object (\`+0x8\`, \`+0x2d8\`, 720 bytes each). Halo never looks inside them.
  - \`ov_open_callbacks\` over its own memory reader {position, data, size, end} with callbacks passed by value: read \`0x5481a0\`, seek \`0x5481f0\`, close \`0x548210\`, tell \`0x548230\`.
  - \`ov_read\` into 16-bit signed little-endian PCM, \`ov_crosslap\` between the two for seamless transitions, and \`ov_clear\`. All cdecl, via delay stubs at \`0x5b9210\`–\`0x5b9240\`.
  - The game ships libVorbis I 20020717 (1.0).
- **What:**
  - libogg 1.3.5 and libvorbis 1.3.7 (BSD) are pinned in \`dependencies.lock.json\` (\`ref/xiph\`, ignored) and built once per target into a cached archive by \`scripts/xiph.py\`. \`run-core.py\` and \`run-slices.py\` link it.
  - \`port/runtime/halopad_vorbis.c\` keeps each stream's decoder state on the host, keyed by the structure's guest address, and calls Halo's own reader callbacks through the guest as cdecl functions.
- **Test** (\`tests/halo_vorbis_test.c\`, 11 checks, all passing):
  - A stereo stream (440/660 Hz) from libvorbis's reference encoder (\`tools/vorbis_encode.c\`), decoded through Halo's callbacks.
  - It gives exactly as many samples as ffmpeg's independent native decoder, every sample within 1 count of it, and each channel carries its tone at the right amplitude.
  - Also: a second stream crossfaded into, both cleared, and a non-Vorbis stream refused (\`OV_ENOTVORBIS\`).
  - An earlier fixture from ffmpeg's experimental encoder decoded differently in its second channel in both decoders, and neither followed the tone. That stream was faulty, so the reference encoder is used instead.
- All suites, 15 slices and the unit tests pass; the core still stops at the license.

**Next:** SEH (\`RaiseException\`, handlers through the \`fs:[0]\` chain, \`SetUnhandledExceptionFilter\`), game controllers for DirectInput, file mapping, then the iOS host.

## 2026-09-27 — Keystone.dll and ksimeui.dll translated and loaded

- **Why.** Halo loads `keystone.dll` in WinMain before it creates its window, so the first run after the license would stop there. Keystone is not optional in normal play: it draws the multiplayer chat input (`KeystoneEditbox`) and chat log (`KeystoneChatLog`) from `content/*.ksml` through Halo's own Direct3D device. Only `-safemode` skips it, and every Halo caller checks the pointers.
- **Decision.** Translate it rather than reimplement it. The loop allows a native replacement only when that is smaller or safer than the translation; Keystone is 1 MB with its own D3DX9, libpng and XML layout, and its imports mostly overlap services HaloPad already has.
- **Pipeline** (all scripts take `--module`):
  - `scripts/hpmodule.py` holds the module registry, with hashes in the profile.
  - The audit uses a DLL's own relocation table as ground truth and roots its exports.
  - The audit now recognises functions that never return (`_CxxThrowException`, the CRT exit paths) and gives them to SRW as `noret_procedures.sci`. Without this, SRW decoded a switch table after a throw.
  - The capability scan now also writes the flag table (`udis-flags.txt`), which was previously made by hand.
  - The trap pass drops replacements SRW emitted outside a procedure.
  - The SRW patch names the WS2_32 and OLEAUT32 ordinals, and WINSPOOL ordinal 203 becomes an explicit trap.
  - Bug found along the way: DLL hints had used fixup addresses as targets, which left `fixup_interpret_as_code.sci` empty.
- **VA model and loader.** Described in G3-RUNTIME.md ("Translated game DLLs"). It is one address space with 193,611 dispatch entries, and cross-module imports are bound to real export addresses. The loader maps, binds and runs DllMain on attach and detach, including thread notifications.
- **Test** (`tests/halo_keystone_test.c`, 18 checks, all passing): Keystone and ksimeui load at 0x10200000 and 0x10000000, both DllMains run in translated code, all 17 exports match the file, and unload and reload are clean. `TryEnterCriticalSection` was added (Keystone's CRT uses it on detach).
- All 12 suites, 15 slices and the unit tests pass; the core still stops at the license.

**Next:** run Halo's own `0x51cdb0` (KeystoneCreate plus both chat windows) on a real device and draw a chat line through `KsUpdate`, then SEH.

## 2026-09-27 — Review of bnunu/halo-1 (and the halo-ce-universal ports)

- Reviewed at Chris's request: [REVIEW-HALO1-DECOMP.md](REVIEW-HALO1-DECOMP.md). It is a decompilation of the Xbox pre-release build 2342, with Linux, Windows and Android ports that need the Xbox SDK and the PAL Xbox data.
- It has no Custom Edition support, no PC network protocol, no GameSpy or CD-key code, and no Apple port. Its networking is system link between copies of itself, so it cannot join existing Custom Edition servers, the project's core requirement.
- Decision: keep the HaloPad route. Both repositories are pinned read-only under `ref/decomp/` as an engine reference; nothing from them is linked or copied.

## 2026-09-27 — Chat UI progress: state blocks, Controls.dll, and the next services

- **Direct3D 9 state blocks** (`port/runtime/halopad_d3d9_stateblock.c`): Keystone saves and restores Halo's device state around its drawing with `BeginStateBlock`/`EndStateBlock`. The implementation follows D3D9:
  - setters called while a block records are recorded, not applied;
  - `CreateStateBlock` uses the documented ALL, PIXEL and VERTEX sets;
  - `Capture` and `Apply` work on the marked states;
  - blocks hold references to what they bind.
  - The D3D9 test gained 23 checks (221 in all).
- **Controls.dll**: Keystone loads it by path. It is a third shipped DLL, and its preferred base is Keystone's own `0x10200000`, so the Windows loader relocates it. The profile now has `rebase`: `scripts/hpmodule.py` applies the file's own relocation table at `0x10330000` (only fixup dwords and `ImageBase` change, which is checked), and that image is translated (29,069 procedures). pefile's `relocate_image` + `write` damaged the import directory and is not used.
- **SRW patch**: its fixed 128-byte output buffers overflowed on Controls.dll's 138-character C++ import names, so they were enlarged. Lifter build `e8751a3aad23-3b4081c0` (smoke test passes).
- **Pipeline check**: the refactored pipeline reproduces Halo's translation (run `run-20260927T081739Z-49093`, now the one in use). It differs from the previous run only where the new no-return rule applies: SRW stops after calls to `_CxxThrowException` and two STL throw helpers. All 15 slices pass on it.
- **Small fixes found by running Keystone:**
  - `C:\Program Files\Microsoft Games` now exists on the virtual drive, so `FindFirstFileA` of the install directory returns its own entry (added to the files test).
  - The `lstr*` family was added.
- **Where the chat set-up stops now:** Halo's `0x51cdb0` runs `KeystoneCreate` into Controls.dll's start-up and stops at `GetPrivateProfileStringA` (`controls\controls.ini`).
- **Remaining imports of the three DLLs with no implementation (about 100):**
  - GDI text: `CreateFontA`, `ExtTextOutW`, `GetTextExtentPoint32W`, `GetTextMetricsA`, `CreateDIBSection` and others;
  - OLE BSTR helpers;
  - IMM32 (the IME is turned off in the `.ksml` files);
  - SEH (`RaiseException`/`RtlUnwind`);
  - file mapping, and a few kernel32 and user32 calls.
- All 12 suites, 15 slices and the unit tests pass; the core still stops at the license.

**Next:** `GetPrivateProfileStringA`, OLE BSTRs and GDI text on CoreText, then SEH, until `0x51cdb0` completes and `KsUpdate` draws a chat line.

## 2026-09-27 — Chat set-up: KeystoneCreate and both chat windows succeed; next is MSXML 4

- **New services**, each reached by running Halo's own chat set-up (`0x51cdb0`):
  - `GetPrivateProfileStringA` with XP's rules for value lookup, the NULL-section and NULL-key lists, trimming, quotes, the default and both truncation cases (11 checks in the files test);
  - IMM32 as on the reference machine, where IMM is not enabled: no input context, `ImmIsIME` FALSE, calls on the NULL context fail, and any other context stops the program;
  - `IsWindowUnicode` (every HaloPad window class is ANSI), `GetFocus`'s missing wrapper, and `InterlockedIncrement`/`Decrement`.
- **Registry order**: names resolved only through GetProcAddress now come after every module's static imports, so adding one no longer changes translated code (a Keystone recompile takes 40 s).
- **Diagnostics**: a transfer to an address with no procedure now prints the guest registers and the top of the stack, where the caller's return address is.
- **Test fix**: the UI test now runs Halo's C runtime start-up up to its I/O set-up (`_heap_init`, `_mtinit`, `0x5d3ac3`, `_ioinit`), as the entry point does before WinMain. Without it, `_getptd` called a null `TlsGetValue` pointer; that was a test artifact.
- **Result**: `KeystoneCreate` succeeds and both `KeystoneEditbox` and `KeystoneChatLog` exist. The controls inside them are not built yet, and the first `KsUpdate` asks for `CoCreateInstance(CLSID_DOMDocument40)`.
- **MSXML 4 is part of the reference machine**:
  - Keystone parses its `.ksml` layouts with MSXML 4.0 and has no fallback (a failure at `0x10245906` takes its error path).
  - Halo's own installer ships `redist/msxmlenu.msi` (MSXML 4.0 SP2). Its `msxml4.dll` is 4.20.9818.0 (SHA-256 `9808f05f…`, the same file for the side-by-side and system32 copies), with preferred base `0x69b10000` and a relocation table.
  - Plan: translate it like Keystone and add in-process COM servers to `CoCreateInstance` (class → `DllGetClassObject` → `IClassFactory::CreateInstance`).
- All 12 suites, 15 slices and the unit tests pass; the core still stops at the license.


## 2026-09-27 — MSXML 4 runs translated; SEH works; the chat window parses and validates its layout

- **MSXML 4.0 SP2 is a fifth translated module**:
  - `msxml4.dll` and `msxml4r.dll` are extracted from Halo's own `redist/msxmlenu.msi` (`scripts/extract-reference-components.py`), and `msxml4.dll` is translated like Keystone (87,094 procedures).
  - `CoCreateInstance` now serves in-process classes from `config/runtime/com-servers.txt`.
- **Audit and SRW fixes needed because msxml4 keeps data in `.text`:**
  - Probes may not start at a relocation fixup or at `int3`.
  - Data pointers found inside `.text` are followed.
  - Strings of 6 or more ASCII characters, or 3 or more UTF-16 characters, are not code.
  - `ret imm` must be a multiple of 4 and at most 0x100.
  - Fixups that point inside an instruction go to `literal_fixups.csv`, which the patched SRW honors. SRW also names any unnamed ordinal import `<DLL>_ord<n>`, skips fixups into the PE headers, and ends a code path after `ExitProcess`, `FatalAppExit*` and `ExitThread`.
  - Halo's re-run differs only by the new no-return stops.
- **Services added, each reached by running MSXML:**
  - `FormatMessageA/W` from module message tables;
  - `LoadLibraryExA(AS_DATAFILE)`;
  - `FindResourceW`;
  - BSTRs;
  - semaphores, `DuplicateHandle`, `ResetEvent` and the remaining `Interlocked*` calls;
  - an INI reader fix.
- **x87 fix**: 64-bit and double results of `llasm_float.c` now go through a per-thread guest slot, where they were a host address before. The new slice `float_to_int` passes 200/200.
- **SEH** (`halopad_seh.c`):
  - `RaiseException` and `RtlUnwind` over the `fs:[0]` chain, as XP runs them.
  - Translated code runs in continuation style, so a handler's jump to an outer `__except` block leaves host frames behind. Callbacks now carry per-level return sentinels, and a return to an outer level discards the inner frames with `longjmp` (`halopad_callback.c`).
  - MSXML reports parse errors this way (code `0xE0000001`): the handler chain, the collided frame's unwind and the jump all run as on Windows.
- **SHLWAPI and URL services**:
  - path and URL services, with each ordinal wrap bound to its XP target (`halopad_shlwapi.c`);
  - `UrlCanonicalizeW` ported from Wine for strings without a scheme;
  - an Internet security manager that puts local files in the Local Machine zone (`halopad_urlmon.c`);
  - OLE Automation error objects (`halopad_errorinfo.c`);
  - `VariantClear` for `VT_DISPATCH` and `VT_UNKNOWN`.
- **New test `halo_msxml_test.c` (18 checks, PASS):**
  - DOM from a string;
  - a malformed document rejected with MSXML's own text from `msxml4r.dll`;
  - `480editbox.ksml` validated against `KSML.xsd`, both as a string and exactly as Keystone loads it;
  - an element the schema lacks rejected with the schema's list of allowed elements.
- **Found while debugging:**
  - Keystone loads layouts asynchronously. It reads the file with `ReadFileEx` during `KsUpdate`, then parses it on a later frame.
  - The UI test used to stop after one frame, which was a test artifact. It now draws frames with Halo's message pump until the controls exist, up to 600 frames.
  - New diagnostics: `HALOPAD_WATCH` and `HALOPAD_WATCH_RANGE` log indirect transfers into chosen addresses or across modules, `HALOPAD_TRACE_FILES` and `HALOPAD_TRACE_BSTR` trace file I/O and BSTRs, and missing imports now print their caller.
- **Where the chat UI stops now**: `480editbox.ksml` is parsed and schema-validated, and its `background` URI is canonicalized. Keystone then starts its glyph cache and stops at `SetMapMode` (GDI).
  - Keystone's text path (`0x1021a7b5`) is: a memory DC, `MM_TEXT`, white on black, `CreateFontA(-MulDiv(pt, 96, 72), … ANTIALIASED_QUALITY, VARIABLE_PITCH, face)`, `GetTextMetricsA` and `GetTextExtentPoint32W(L"?")` for the cell, and a 32-bit top-down DIB section.
  - Each glyph is then drawn with `ExtTextOutW(ETO_OPAQUE)`, and coverage is read from bits 4–7 of each pixel into A4R4G4B4 texels.
- All 13 suites, 16 slices and the unit tests pass; the core still stops at the license.

**Next:** GDI text on CoreText (fonts, metrics, extents, DIB sections, `ExtTextOutW`) until `KsUpdate` draws the chat box, then file mapping and DirectInput game controllers.

## 2026-09-27 — The chat UI draws: Keystone's layout, GDI text on CoreText, a chat line on screen

- **Result.** Halo's chat set-up (`0x51cdb0`) plus frames with Halo's message pump now lay out both chat windows (`oEditbox`, `oPrompt`, `oListbox`) within 6 frames.
  - A chat line added through Halo's own `0x4ae8a0` is drawn in the log area by translated Keystone, through Direct3D 9 on Metal.
  - The chat input opened as `0x4ada50` opens it shows "Say:" and the typed text with a cursor.
  - The frame is read back from the Metal back buffer and saved as `chat.ppm` in the evidence directory; its text pixels are checked.
  - The edit box's own background (`gallery/editbox480.png`) is fully transparent, as on Windows.
- **GDI text on CoreText** (`halopad_gdi.c`):
  - `CreateFontA` (14 arguments, read from the guest stack), `SetMapMode`, `SetTextColor`, `SetBkColor`, `SetBkMode`, `SetTextAlign`, `GetTextMetricsA`, `GetTextExtentPoint32W`, `CreateDIBSection`, `ExtTextOutW`, `DeleteDC` and deferred deletion of selected fonts, plus kernel32 `MulDiv`.
  - Fonts: the reference machine has XP SP3's fonts, and GDI's mapper sends "Arial Narrow" (an Office font) to Arial. Arial is drawn with the host's ArialMT faces.
  - Metrics come from the font's `VDMX`, `hdmx`, OS/2 and hhea tables as GDI computes them. They match Windows' own values: Arial 12 gives 12/9/3, −34 gives 39/32/7, and −16 gives 18/15/3.
  - The GDI test gained 30 checks.
- **x87 bug found and fixed.** Since the guest-slot change earlier today, `fst`/`fstp qword` copied the integer scratch field, so every double stored to memory read back as 0.
  - Keystone's CRT `floor`/`ceil` (used to size its glyph texture) returned 0, and the null texture led to a null access.
  - A new check in the Keystone test covers `floor`/`ceil` through translated code.
  - Halo's own code is affected wherever it stores doubles, so this also matters for the core.
- **Direct3D:**
  - `UpdateTexture` (system memory to default pool, level-granular dirty tracking; default-pool textures keep a copy of their contents for upload and still refuse `LockRect`);
  - `d3d9.dll!DebugSetMute` for D3DX;
  - `d3d9d.dll` absent (D3DX checks for the SDK debug runtime).
- **Translator gaps closed with hand replacements** (documented in `config/srw/…/README.md`):
  - Keystone: `and eax, offset` (zlib), 8-bit `imul` (libpng), and `bt`/`bts [esp], eax` (CRT `strspn`/`strpbrk`).
  - Controls.dll: four `and eax, offset` selects.
  - The same CRT `bt`/`bts` sites in `haloce.exe` and ksimeui.
  - Halo's translation is now `run-20260927T110059Z-78970`. It differs from the previous one only at those four sites and the carry flag the following `jae` reads.
- **Also:**
  - `GetUserDefaultLangID` and `GetSystemDefaultLangID` (0x0409);
  - `HALOPAD_TRACE_LAST=1`, which prints the last 32 indirect transfer targets with any trap or fault;
  - the frame walker no longer follows an `ebp` that is not a frame.
- All 14 suites (new: the UI test passes), 16 slices and the unit tests pass; the core still stops at the license.

**Next:** keyboard input into the chat box (Halo's `KsDispatchMessage` and `KsTranslateAccelerator` path), file mapping and DirectInput game controllers, then the core beyond the license once Chris accepts it.

## 2026-09-27 — The runtime builds and passes on the iPad Simulator

- **Split the Apple host.** The shared Metal core is `halopad_metal.m`, and each platform host provides the application, windows, events, the pointer, the clipboard and the screen size:
  - `halopad_host_macos.m` is the AppKit code moved unchanged;
  - `halopad_host_ios.m` is new: off-screen `CAMetalLayer` windows, `halopad_host_attach_view` for the coming app shell, `UIPasteboard`, and the main screen's size;
  - windows present only when their layer is on screen.
- **Builds.** `run-core.py` gained `--run-prefix` (with `SIMCTL_CHILD_` environment variables) and picks the iOS SDK and UIKit for iOS targets. `vabuild.py` compiles translated modules with that SDK.
- **The vorbis test** no longer starts tools on iOS. It reads the stream and ffmpeg's decode from the macOS run's fixtures in `/tmp`.
- **Result on the "HaloPad iPad Pro 13" Simulator:**
  - all 14 suites pass: Direct3D 9 with 221 checks and Metal readbacks, USER32, DirectInput, DirectSound, threads, Winsock, files, OLE, GDI text, misc, Vorbis, Keystone, MSXML, and the chat UI, whose frame matches the Mac's;
  - all 16 slices and contract cases pass.
  - macOS still passes all 14 suites after the split.
  - The Simulator is shut down again.
- The first iOS build compiles Halo and the four translated DLLs for the Simulator in about 3.5 minutes.

**Next:** an iPadOS app shell (UIKit scene, the layer in a view, touch/keyboard/pointer into `halopad_input_event`, game controllers) so the chat UI shows on the Simulator's screen; then the physical-device capsule once Chris provides a device.

## 2026-09-27 — HaloPad for iPadOS: the native core starts in an app and shows the license

- **App shell** (`port/ios/HaloPadApp.m`):
  - a UIKit scene app that runs the native core from Halo's PE entry point on its own thread;
  - Halo's windows attach to the app's view through `halopad_host_set_window_handler` and `halopad_host_attach_view`;
  - Halo's first-run license check (`EBUEula`) presents the game's `Eula.rtf` with Decline and I Accept, and only the player's choice is recorded.
- **Builds.** `scripts/build-ios-app.py` builds `HaloPad.app` for the Simulator (the same link as the core, ad-hoc signed). With `--launch` it installs and launches the app, then screenshots it; it never taps.
- **Core entry.** It is now `halopad_core_run` (`port/runtime/halopad_core.c`), shared by the macOS runner and the app.
- **Result on the "HaloPad iPad Pro 13" Simulator.** Translated Halo runs its C runtime start-up and WinMain to the license check, and the app shows the license (`ios-app-20260927T113055Z/screen.png`). The Simulator was shut down afterwards without a choice being made.
  - On the Mac the core still declines at the license without a record, as before.
- **For Chris.** Accepting can now be done in the app: run `scripts/build-ios-app.py --launch`, open Simulator, read the license and tap. The Mac runner still uses `scripts/accept-eula.sh`.

**Next:** touch, keyboard and pointer input from the shell into `halopad_input_event` (and game controllers), so that once the player accepts, the splash, menus and chat can be driven on the iPad.

## 2026-09-27 — iPadOS input: keyboard, touch and pointer from the app shell

- **What the shell sends.** It queues input from UIKit's main thread with `halopad_host_post_input`, and `halopad_host_pump` delivers it on the thread that pumps (Halo's):
  - hardware keyboard presses, mapped from USB HID usages by the new `halopad_hid_key` table (`halopad_keys.c`) to Windows virtual keys and set-1 scan codes, with the typed characters;
  - touch as the left mouse button, with relative motion for DirectInput;
  - an iPad pointer: secondary button, hover moves, and trackpad or wheel scrolling as `WHEEL_DELTA` units;
  - scene activation as `HPI_ACTIVATE`.
  - Pointer positions are mapped through the letterboxed layer to the game window's client pixels.
- **Tests (the USER32 suite):**
  - the HID table agrees with the Mac table for 22 keys (letters, digits, Enter, Escape, arrows, F-keys, keypad, modifiers);
  - on iOS, a key the shell queues is not visible before the pump and is down after `PeekMessageA` pumps.
  - It passes on macOS and on the iPad Simulator.
- The app still reaches Halo's license screen (`ios-app-20260927T113754Z`). No choice was made, and the Simulator is shut down.

**Next:** game controllers (GameController framework into DirectInput's joystick devices), and on the Mac the D3D device paths Halo uses beyond the license (Reset, render targets, scissor), each verified with its own test.

## 2026-09-27 — Game controllers: Halo's own gamepad set-up works

- **DirectInput presents host controllers as XP's Xbox 360 controller.**
  - Host side: GameController's extended gamepads on macOS and iOS (`halopad_gamepad.m`).
  - New and changed methods: `EnumDevices(GAMECTRL)`, `CreateDevice` by instance GUID, general data-format matching, `GetCapabilities`, `GetDeviceInfo`, `EnumObjects`, the range, dead-zone and saturation properties, `Poll` snapshots, `GetDeviceState`, and unplugging.
- **Test.** Halo's `0x494840` builds its 80-object format and runs its enumeration and object callbacks against an injected controller. It keeps one device named "Controller (XBOX 360 For Windows)" with 5 axes, 10 buttons and 1 hat, and sets −4096…4096 with a 10% dead zone. The state reads back correctly through its 224-byte format, and unplugging behaves as DirectInput does.
- **Result.** The DirectInput suite passes on macOS and on the iPad Simulator, all 14 macOS suites pass, and the iPad app builds with GameController linked.

**Next:** Direct3D 9 features Halo uses beyond the license, each with its own test: `Reset` (for alt-tab and resolution changes), `CreateRenderTarget`/`CreateDepthStencilSurface`, `ColorFill`, `SetScissorRect` and `GetRenderTargetData`. They come from Halo's rasterizer call sites.

## 2026-09-27 — Direct3D 9 Reset

- **`IDirect3DDevice9::Reset`** (Halo's `0x519751`, after it releases its default-pool objects), following Direct3D 9's rules:
  - it refuses with `D3DERR_INVALIDCALL` while default-pool resources, state blocks, or application references to the implicit back buffer or depth buffer are alive. The runtime now counts live default-pool resources and state blocks per device;
  - otherwise it releases every binding and returns every state to its default;
  - it recreates the back buffer and depth buffer from the new present parameters, parsed by the same code as `CreateDevice`.
- **Test** (the D3D9 suite, 10 new checks): refusal with a default-pool vertex buffer alive and with the back buffer held, then `Reset` to 800×600 with the states, viewport and back buffer checked, followed by a `Present` and a pixel read back from the new back buffer.
  - The suite itself had leaked a default-pool vertex buffer, which Windows would also refuse to reset past, so the test now releases it first.
- **Call sites.** Halo's other device-vtable call sites for `CreateRenderTarget` and `UpdateSurface` (`0x58d167`, `0x58d39c`) are inside its statically linked D3DX texture loader. They will be implemented when the core reaches them, with the caller known.

## 2026-09-27 — Typing into the chat box

- **Path under test.** Halo's window procedure passes messages to Keystone with `KsDispatchMessage(ks, msg, wParam, lParam, &handled)` (`0x545850`), and its message loop first offers them to `KsTranslateAccelerator` (`0x544e40`). The UI test now focuses the edit box as Halo does (`KW_SetFocusControl`, `0x4adad2`), then sends `WM_KEYDOWN`, `WM_CHAR` and `WM_KEYUP` for "gg hf", plus a "!" removed with Backspace.
- **Result.** Controls.dll's translated edit box holds "gg hf", read back with `KC_GetAttribute(L"text")` as Halo reads it (`0x4adc4d`), and the frame shows it with the cursor (`typed.ppm`).
- **Services Controls.dll's edit box needed:** `GetKeyboardLayout` (US English, `0x04090409`), `IsDBCSLeadByteEx`, `CharNextExA` and `GetCPInfoExW`. The reference machine's code pages 1252 and 437 are single-byte, and the names match XP's. The misc suite gained 6 checks.

## 2026-09-27 — G1b step 1: the original dedicated server runs in CrossOver

- **Script.** `scripts/reference-server.sh` creates the project bottle `halopad-reference` (CrossOver 26.3, Windows XP template) and copies the game files into it. It checks `haloceded.exe` against the manifest (SHA-256 `7789c4a0…`), then runs the original server with `-port 2302 -ip 127.0.0.1` and an init file: `sv_public 0` (never listed with the master server), `sv_log_enabled 1`, `sv_map bloodgulch slayer`. It queries the server on its port and stops everything it started.
- **Result.** After 40 s the server is alive and answers the GameSpy status query with `gamever 01.00.10.0621`, `mapname bloodgulch`, `gametype Slayer`, `gamemode openplaying`, `dedicated 1` and `numplayers 0`. Its own log reads "Log opened" and then "GAMEINFO SETTINGS MAP bloodgulch MODE Slayer". The Wine processes are gone afterwards.
  - Evidence: `docs/artifacts/2026-09-27/G1b/server-20260927T121035Z/`.
- **This is the unchanged server G5's match will use.** Public servers were not touched.
- **The reference client (`haloce.exe`) was not run.** Its first run opens the license dialog, which needs the player's choice, so that row of G1b is parked on the same license as the native core.

## 2026-09-27 — Second look at bnunu/halo-1

- **Question.** Chris posted `bnunu/halo-1` again, asking whether it can advance the project. The pinned checkouts in the ignored `ref/decomp/` were moved to the current heads (`8036fb8`, `8b4c73a`), and the upstream port `cybersecurity/halo-ce-universal` was read too.
- **Answer: reference only, unchanged.** It is a decompilation of the Xbox pre-release build 2342, with native Linux, Windows and Android ports. Their multiplayer is Xbox lockstep system link under a new "protocol version 2", so they can join neither Custom Edition 1.10 servers nor real Xbox games. They need the August 2001 Xbox SDK and the PAL data of the pre-release build, and neither is sold.
- **New findings.**
  - Its own documentation says parts were reconstructed with files described as original Bungie source and a leaked CEA source tree, so nothing from it enters HaloPad.
  - Only 1,223 of its 5,602 string literals occur in `haloce.exe` (networking 37 of 830, rasterizer 0 of 1,440), so a string-based name map for our binary would be thin.
- **Written up** in [REVIEW-HALO1-DECOMP.md](REVIEW-HALO1-DECOMP.md). The parked items (the license choice, the product key, a device, a second player) are unaffected.


## 2026-09-27 — Halo's Direct3D splash, and all 15 suites on the iPad Simulator

- **iPad Simulator rerun.** All 14 existing suites pass on the "HaloPad iPad Pro 13" Simulator after the input, game-controller, `Reset` and chat-typing changes (all 14 had last run there at `1fc8bf3`).
- **The splash.** Halo draws its first Direct3D frame with `0x519080`: bitmap `0x86` from `strings.dll`, loaded into a 640×480 offscreen surface by the statically linked D3DX (`D3DXLoadSurfaceFromResourceA`, `0x582dfc`), stretched onto the back buffer and presented. It is reached from `WinMain` through `0x4ca9c0` → `0x43ed20` → `0x519630`, just after the license.
  - The new suite `tests/halo_splash_test.c` runs Halo's own function. All 307,200 back-buffer pixels equal the bitmap, decoded independently in the test. 12 checks pass on macOS and on the iPad Simulator.
  - No trap: D3DX locks the offscreen surface directly. Its staging path (`CreateRenderTarget` and `UpdateSurface`, `0x58d123`/`0x58d1d9`) is not taken for the splash.
- **Two static items closed.**
  - `ProcessVertices`: Halo's only call site (`0x51ff05`) is in dead code (`0x51fe90` has no callers and no stored address). Every device method in the static inventory is now implemented.
  - The file-mapping traps (`CreateFileMappingA`/`MapViewOfFile`, `0x5466df`) sit in Halo's crash dialog, after `CreateDialogIndirectParamA`. Normal play does not reach them, so they stay loud traps.
- **Next:** the license is still the gate. Beyond the splash, the core's next steps (the rest of `0x519630`, the main menu and map loading) need the player's choice.


## 2026-09-27 — Halo's warning and error dialogs; the product ID gates start-up

- **What start-up does after the license.** Static reading of `WinMain` (`0x5449c7`–`0x544c38`) lists every check, each reported through `0x582060` with a text from `strings.dll`:
  - Direct3D 9, the Ctrl key held, DirectSound, DirectInput, `shfolder.dll`;
  - an unclean last exit, memory, CPU speed, temporary disk space;
  - the product ID. `0x5829e0` reads `DigitalProductID` from `HKLM\...\Halo CE`, and when it is missing, Halo shows "Your product key is invalid" as a fatal error (flag 1 → `ExitProcess(1)`).
- **Consequence.** The product key gates more than online play: without the product ID its installer writes, Halo stops right after the license. HaloPad never writes product IDs. STATUS now parks everything past start-up on Chris's key, entered through the original installer.
- **Dialogs implemented** (they were traps), since every player without a key, and every start-up warning, reaches them:
  - USER32 dialog manager: templates, `#32770`/`DefDlgProcA`, Button and Static controls with subclassable window procedures, the `Dlg*` calls and `EndDialog`, and a modal loop that presents through the host.
  - GDI: `CreateFontIndirectA`, `GetObjectA` on fonts, `GetTextColor`.
  - `ShellExecuteA` for URLs and documents.
  - `CW_USEDEFAULT` for pop-up windows.
- **Tests.**
  - `tests/halo_dialog_test.c` (26 checks) runs Halo's own `0x582060` and `0x5817e0`, passing on macOS and the iPad Simulator (all 16 suites pass on both): the warning's contents, the link to Halo's support page, "don't show again" honoured on a second call, and the fatal product-key error with only Exit enabled.
  - On the iPad Simulator, a scene (`build-ios-app.py --scene`) shows Halo's Ctrl-key warning as the app's sheet. Screenshot: `ios-app-20260927T124014Z`. Nothing was tapped.
- **Next:** the other start-up checks can be run the same way: `0x580a00` on HaloPad's `IDirect3D9`, the CPU-speed measurement `0x580e70`, the memory and disk figures. That shows which warnings a HaloPad player would see.


## 2026-09-27 — Start-up checks on HaloPad: all pass except the product ID

- **Test.** `tests/halo_startup_test.c` runs Halo's own start-up checks in `WinMain`'s order, with a host that records any dialog. The C runtime starts as the entry point starts it, `_cinit` included; without it, `config.txt` parsing raised R6002 through the CRT's message box.
- **Results.**
  - Machine measurement (`0x580e70`): 1024 MB, 1000 MHz, one display device with 128 MB. Above the minimums of 128 MB and 733 MHz.
  - Hardware check (`0x580a00`): accepts HaloPad's Radeon 9700 PRO through the game's `config.txt`, with no error text.
  - DirectSound, DirectInput and `shfolder.dll` load; no unclean exit is recorded; there is 100 GB free against a 100 MB minimum.
  - No dialog appears before the product-ID check, which fails, as expected on HaloPad.
- **New services this needed.**
  - DirectDraw 7 for Halo's video memory query (`halopad_ddraw.c`): one primary device, `SetCooperativeLevel(DDSCL_NORMAL)`, `GetAvailableVidMem` at 128 MB. Its absence would have been fatal (string `0x79`).
  - `GetLastActivePopup`, `GetActiveWindow` and `MessageBoxA` as run-time exports for the CRT's message box.
- **Consequence.** Once Chris accepts the license, a HaloPad player sees no start-up warning. The product key, through Halo's original installer, is the only remaining gate before the splash and menus.


## 2026-09-27 — Halo's graphics start-up runs on HaloPad; the reference CPU is 2.4 GHz

- **The time-stamp counter now runs at 2.4 GHz** (it was 1 GHz). Halo compares the measured speed with 1000 MHz in four places (`0x51a2ad`, `0x53d70a`, `0x53e3c0`, `0x53e5ac`). At exactly 1000 MHz it took its low-spec branch: a 640 × 480 default resolution and lower detail defaults. That contradicted the Radeon 9700 PRO machine HaloPad describes everywhere else. Halo now measures 2400 MHz, and the startup suite checks it.
- **Rasterizer initialization.** `tests/halo_raster_test.c` runs Halo's own `0x51a240`, the graphics start-up, as a component: it sets only the `WinMain` values the function reads.
  - It succeeds with no dialog: the game window and an 800 × 600 device.
  - It reads `config.txt`, `shaders\vsh.enc` and `shaders\EffectCollection_ps_2_0.enc`, the pixel shader 2.0 path chosen for the Radeon 9700 PRO.
  - It runs every rasterizer subsystem's initialization and lays out the 800 × 600 chat (`600editbox.ksml`).
  - The splash is on the back buffer.
  - No runtime service trapped on the way.
- **Scope.** This is a component test. The core runner remains the only path through `WinMain`, and it stops at the license and then the product ID, as Halo does.


## 2026-09-27 — The game's systems start, and Halo loads ui.map and Blood Gulch

- **Component test** `tests/halo_maps_test.c`: `WinMain`'s set-up values, then Halo's `0x5442e0` (resource maps, graphics, input, sound), then `0x443c50` + `0x442290` for the main menu and Blood Gulch.
  - The systems start with no dialog.
  - Both maps load into tag memory at `0x40440000`: 1,412 and 2,455 tags, with the right `scnr` scenario tags.
  - It passes on macOS and on the iPad Simulator.
- **Translation work it took** (lifter `161d2b412a4a-89ccc7fb`, run `20260927T140130Z-31169`):
  - 80-bit `fld`/`fstp`: 146 sites, with helpers checked against x86 encodings of 1.0, −2.5, 1e10, 0, ∞ and π, and a denormal round trip.
  - `rol`/`ror` of low bytes; mixed low/high-byte `test`.
  - `fprem`; `fsubr`/`fdivr st(i), st(0)`.
  - An audit fix for the x87 status idiom, which recovers the CRT's `fmod` and 16 math functions.
  - Untranslated instruction sites went from 153 to 6.
- **A silent miscompile found and fixed.** A HaloPad translator guard accepted "no code" for `test`/`cmp` of registers as "flags not needed". For `test ch, cl` the real cause was an unimplemented form, and the following `jz` read a stale condition, which compiled to a trap. The forms are implemented now, and a scan of the whole translation finds no remaining case.
- **On the product key.** Chris asked to get around the key so development continues. The key check is not bypassed: no product ID is written, and nothing is skipped in the run path. Development continues through component tests of Halo's own functions, like this one. The full run still stops at the license and then the product ID.


## 2026-09-27 — Halo's main menu runs, on the Mac and on the iPad Simulator

- **`main` runs.** `tests/halo_menu_test.c` calls Halo's own `main` (`0x4ca9c0`) after `WinMain`'s set-up values and the systems start.
  - It initializes the game, loads `levels\ui\ui` and draws the main menu: the 3D ring and ship, the logo, and Multiplayer, Profiles, Settings, Credits, Quit.
  - After 150 frames a test hook sets Halo's quit flag, and `main` shuts down and returns.
  - It passes on macOS and on the iPad Simulator.
- **`tests/halo_game_test.c`** covers `main`'s initialization step by step, and the menu load through `0x4cbc90`.
- **Runtime additions:**
  - `TranslateAcceleratorA` with a null table;
  - `CreateFileA` with the CRT's `SECURITY_ATTRIBUTES`;
  - a test-only `Present` hook;
  - DXT decoding for GPUs without BC formats. The iPad Simulator failed Metal validation on `BC1_RGBA`, and pre-M1 iPads also lack BC. The CPU-decoded frame matches the GPU-decoded one to within 1.5/255 on average.
- **Chris's key.** [user-supplied string omitted] is not a Halo PC product key: those are 25 characters in five groups of five, from Microsoft's key alphabet, which has no 0, 1 or 5. It was not used, and nothing was derived from it. The license and the product ID still gate the real run.


## 2026-09-27 — Blood Gulch in play, on the Mac and on the iPad Simulator

- **Halo loads and runs Blood Gulch.** `tests/halo_bloodgulch_test.c` uses Halo's own start-up script mechanism.
  - It writes `map_name levels\test\bloodgulch\bloodgulch` to a script and passes `-exec` with it. `WinMain`'s argument splitter `0x545a00` builds the arguments, then the test starts the systems and calls `main`.
  - The player spawns in the red base, drawn in first person with the assault rifle, the HUD, the motion tracker and the reticle.
  - After 330 frames `main` quits when asked. There is no input yet.
- **Audit: pointer tables that read as text.** The callback table at `0x636b18` was dropped as a string because `0x566d20`'s bytes are printable. A text-like dword is now a pointer when a neighbour also points into `.text`. Checked every change: six real entries gained (among them three `__except` bodies), six bogus ones gone (three-letter language codes such as "FRB" and "DEL").
- **Audit: `wait` before a `__try` state change** (`C7 45 FC`) counts as code. Without this, the `__except` handlers at `0x546a7e`/`0x546c28` were probed too early and blacklisted.
- **Translator: byte `rcl` by a constant.** Halo's ADPCM sample step `0x551d10` uses it. The new `adpcm_step` slice matches the x86 oracle on 400/400 cases. Untranslated sites: 6 → 3.
- **Lifter and run:** lifter `24c44b99cfac-05b596a6`, run `20260927T145850Z-51456`. 22 suites, 11 slices and 6 contract cases pass.
- **Chris's key, again.** Chris sent [user-supplied string omitted] a second time. It is still not a Halo PC product key: those are 25 characters, `XXXXX-XXXXX-XXXXX-XXXXX-XXXXX`, from Microsoft's key alphabet, which has no 0, 1 or 5. There is nothing it could be typed into, and nothing was derived from it.



## 2026-09-27 — Playing Blood Gulch: walk, turn, fire; a flags miscompile in acos found and fixed

- **Halo takes a player's input.** `tests/halo_play_test.c` sends host keyboard, mouse and button events the way the app shells do, and checks the effects in Halo's own game state (players `0x815920`, objects `0x7fb710`):
  - W walks the player 7.3 units forward, and letting go stops it;
  - 300 mouse counts turn the view about 25° right;
  - the left button fires the assault rifle (magazine 60 → 50).
- **Firing blanked the world.** Every frame while firing showed only the fog colour and the HUD.
  - The draw trace (new `HALOPAD_TRACE_DRAWS`) showed the camera constants had turned NaN.
  - A NaN scan of Halo's data led to the first-person weapon and the camera globals.
  - A temporary check in the x87 helpers found the first NaN: `acos`'s domain-error load, for ordinary arguments.
- **The cause: flags across a call.** The CRT's `acos` calls `0x5d7318` (`cmp` on the exponent) and then `0x5ccd1d`, whose first instruction is a `je` on that ZF.
  - `scripts/srw-flags.py` followed only the post-call path into `0x5ccd1d`, so the `cmp` was fused and ZF went stale.
  - It now follows every path into a label that reads flags, including the direct call sites of a function entry. That adds 24 hints (the CRT math helpers and the CPUID check), and one label stays unresolved (`0x5a142e`).
  - With the fix, the frame while firing shows the world, the muzzle flash and the shot on the tracker, and 0 bare fog pixels.
- **Keystone** links the same C runtime math code and gains 22 hints. It is retranslated as run `20260927T163514Z-67223`. The hints for ksimeui, Controls and MSXML 4 are unchanged.
- **Run and results:** translation run `20260927T162224Z-63690` (lifter unchanged, `24c44b99cfac-05b596a6`). 23 suites, 11 slices and 6 contract cases pass on macOS and on the iPad Simulator.



## 2026-09-27 — HaloPad joins the original dedicated server

- **Halo on HaloPad joins a real server.** `scripts/reference-join.sh` starts the original `haloceded.exe` (private, 127.0.0.1:2310, in CrossOver) and runs `tests/halo_connect_test.c`, which starts Halo with `-connect`. Then:
  - the GameSpy handshake completes;
  - the client loads the server's Blood Gulch and the server spawns the player;
  - the server's own log records `JOIN SUCCESS … (127.0.0.1:2305)`.
  It passes on macOS and on the iPad Simulator, and is now a regression suite on both.
- **Fixes on the way:**
  - `GetProcAddress` of Winsock ordinals (`#115` = `WSAStartup`), now registered by `va-model.py`;
  - `gethostbyname` of the machine's own short name (macOS resolves only `.local`);
  - WinInet's proxy query and WinHTTP's proxy auto-detection, as on an XP machine with no proxy;
  - port collisions on one machine: server on 2310, client `-cport 2305`.
- **Network policy.** `HALOPAD_NET=lan` is the default for tests: only local and private destinations, and no DNS beyond the machine's own name. The version check's lookup of Bungie's host fails as it would offline, and no test reaches a public host. New diagnostic: `HALOPAD_TRACE_NET`.
- **The key, stated plainly.** WinMain puts the key string from Halo's own `0x5829e0` at `[0x6e1468]`, and the GameSpy answer hashes it.
  - The test sets that global from `0x5829e0` itself. With no `DigitalProductID` on this machine, that is Halo's empty string.
  - The private server accepts it and logs `cdkey d41d8cd98f00b204e9800998ecf8427e`, the MD5 of "".
  - Nothing is fabricated or patched. WinMain's own check still stops the normal start-up without a key, and public servers still need a real one.
- 23 suites, the join and 11 slices pass on macOS and on the iPad Simulator. No server is left running, and the Simulator is shut down.

## 2026-09-27 — Two HaloPad clients: Halo's one-key-per-game rule

- **Two clients at once.** `scripts/run-core.py --clients N` builds once and starts N HaloPad instances, each with its own state and evidence folders. `scripts/reference-join.sh` takes `--test` and `--clients`. It now judges every client's result itself, because `run-core.py` exits 0 either way.
- **Separate ports.** Each client needs its own `-port` as well as `-cport` (2320+i, 2305+i), or the second one's bind fails.
- **Result.** Both clients complete the handshake and are logged as joined. The original server then drops whichever arrived second, which shows "Your CD Key is invalid." Both carry the same key hash (the MD5 of Halo's empty no-key string).
- **What it means.** A two-player match needs two legitimate keys, the parked second player. No key is made up.
- **Test status.** `tests/halo_match_test.c` is ready for when a second key exists and stays out of the suites. Its firing check now counts projectile objects, since the spawn weapon may be a plasma pistol, which has no magazine.

## 2026-09-27 — A game started from Halo's menus: Battle Creek Slayer, fire, melee, grenades, death, respawn

- **The menus work.** `tests/halo_host_test.c` goes from the main menu with a fresh profile folder: Multiplayer (profile "New001"), Create Game > LAN, Battle Creek, Slayer, Start Game. Halo hosts the game.
- **What it checks in Halo's game state:**
  - firing, and melee;
  - looking down, and throwing both frags at the player's feet;
  - shields to 0 and health down;
  - death ("New001 committed suicide") and Slayer's respawn as a new unit.
  It passes on macOS and on the iPad Simulator, on repeated runs.
- **Fixes on the way:**
  - an unbounded jump table with a NULL slot (`0x4a7810`), which hid two cases. The audit fix adds 2 relocations; run `20260927T182330Z-93072`.
  - `GlobalReAlloc` with `GMEM_MOVEABLE`.
  - WinMain's GameSpy set-up, now called in the test, which registers Halo's query keys.
- **Tooling:** fault reports now name the translated procedure (host pc and return addresses through `atos`), and `run-core.py --fresh-state` starts from an empty state folder.
- **G4 still open:** vehicles, pickups, audio checked by ear or capture, menu return and map reload, and clean relaunch.
- 24 suites, the join and 11 slices pass on macOS and on the iPad Simulator.

## 2026-09-27 — The Warthog, and picking up weapons

- **Driving.** `tests/halo_vehicle_test.c`: on Blood Gulch the player walks to the Warthog. Halo offers the driver seat ("Press E to enter driver seat of Warthog"), E gets in, W drives it about 10 units, and E gets out. It passes on macOS and on the iPad Simulator.
  - The earlier failure was distance: Halo only offers a vehicle within its search radius. At 1.5 units there was no offer; at 0.6 units there is.
  - Driving into the canyon wall tips the driver out, which is Halo's own behaviour, so the test stops short of the wall.
- **Pickups.** In the host test, after the respawn, the player walks to the nearest loose weapon, holds E when Halo offers it, and the weapon joins its weapons ("Picked up a plasma rifle"). If geometry blocks the way, it tries the next weapon.
- **Graphics.** The fixed-function program cache no longer stops at 256 programs; it is a growing hash table. The Warthog scene needed more.
- **Suites.** The host test runs with one retry (grenade bounces vary from run to run). 25 suites, the join and 11 slices pass on macOS and on the iPad Simulator.


## 2026-09-27 — Public servers, Halo on the iPad's screen, SunPad's touch controls

- **Public servers.** HaloPad joined the Custom Edition servers people play on, listed by the
  master server `s1.master.hosthpc.com` (252 listed, 193 answering, all 1.10): AUSSIES MADNESS 5
  (Blood Gulch CTF), DEADLY ZOMBIES (wizard), POQclan Ice Fields and POQclan Massacre Island
  (Death Island). Each loaded the server's map and spawned the player; the servers' own welcome
  messages are in the screenshots. A full server (16/16) did not let it in. No server refused
  Halo's key value; one that checks keys would. `scripts/public-join.sh`, HaloQuery in `ref/tools`.
- **Halo on screen in the iOS app.** Frames reached the layer but never showed: Halo's thread
  changed its CAMetalLayers without a run loop to commit them. The host now flushes Core
  Animation after each change. The iPad Simulator shows Halo's menu and games.
- **Touch controls and the three-dot menu from SunPad** (`port/ios/HaloPadOverlay.m`,
  [SUNPAD-TRANSFER.md](SUNPAD-TRANSFER.md)): a move stick, a look area and Halo's PC controls in
  games; a menu with Join Server by Address, recent servers, keyboard, console, chat, aspect
  ratio, FPS counter, touch-control settings and a problem report. The iPad Simulator app joined
  POQclan's Death Island game and played at about 26 frames per second.
- **Fixes:** registry values under paths with spaces (Halo's gamma) no longer stop the next
  launch; the app asks for landscape; the host test's combat is timed in frames again (a
  millisecond version stopped the player dying), with the session's sound captured and checked
  (menu music, gunfire, the explosion that kills); the lifecycle test (menu return, map reload,
  quit, relaunch with the saved profile) and `run-core.py --relaunch`.
- **Open:** Simulator audio in the app, an unattended touch test, iPhone layouts, the host test's
  rare hang after the respawn (one run of three timed out at frame 2900).

## 2026-09-27 — Joining from the app's menu; a steadier host test

- **Join Server from the three-dot menu works.** It types Halo's console command, which needed
  Halo's `-console` switch (the app adds it), both of `connect`'s arguments (address and password,
  `""` when empty) and typed keys spread one per 50 ms, since Halo reads its keyboard once a frame.
  The console closes afterwards. Checked on the Mac against the private server
  (`HALOPAD_TEST_VIA=console`: `JOIN SUCCESS "New001"`) and in the iPad app
  (`HALOPAD_OVERLAY_DEMO=join:ADDRESS`): POQclan's Ice Fields and Massacre Island games.
- **Sound in the iOS app:** an audio session lets Remote I/O start on the Simulator.
- **The host test.** Grenades are now thrown every 40 frames until the player dies (a press
  during the previous throw's animation was ignored, which left the player at 0.40 health), with
  a switch to plasma grenades when the frags run out. The pickup keeps to weapons on the player's
  level (within 1 unit of height, 25 units away), and its route is always traced. With the
  final version: 5 of 5 on the Mac, 1 of 2 on the iPad Simulator; the miss was a spawn point
  where both frags fell away (no damage), which the suite's retry covers. On a timeout,
  `run-core.py` now samples every thread's stack (`hang-sample.txt`).
- **Why tests failed mid-session:** a HaloPad app left running in the Simulator held UDP 2302
  and 2303 on the Mac, and the host test and the reference join could not use them. The
  Simulator is shut down after every app run.
- macOS: all 25 suites, the join, the host and lifecycle tests pass. iPad Simulator: the full
  run passed everything except the host test, which failed both tries with the earlier
  fixed-time throws; the final version passed 1 of 2 runs there.

## 2026-09-28 — The touch controls, tested in a game; iPhone

- **Self-test** (`HALOPAD_TOUCH_SELFTEST=1`): in a game on the private reference server, the app
  drives the overlay's stick, look drag, FIRE and JUMP through their touch handlers and checks
  Halo's state: the player moves (3.2 to 4.8 units), turns (39.7 degrees), shoots (a plasma
  pistol's battery, weapon `+0x134`, 1.00 to 0.89) and rises (0.6 to 0.8). Three passes in a row on
  the iPad Simulator, one on the iPhone 17 Pro Simulator.
- **Lost presses:** a long frame delivered a press and its release in one pump, and Halo saw only
  the release. The iOS pump now holds such a release for the next pump.
- **Findings on the way:** a plasma pistol released before full charge fires nothing (FIRE is
  held 1 s in the test); the app scene reads `-cport` as WinMain does; the Mac join test now also
  fires as a network client (battery 0.89, a projectile).
- **iPhone:** landscape, Halo letterboxed at 4:3, controls in the side bars.

- **Halo's own Internet lobby** lists the public servers (171 servers, 78 players, with pings) once WinMain's GameSpy set-up has run; the join test does it in every mode and has a browser mode (HALOPAD_TEST_VIA=browser).

## 2026-09-28 — Map changes, reconnects, passwords, timeouts; the touch controls redesigned; a preview command

- **Network situations** (`scripts/network-scenarios.sh`, private reference server): the server's map
  cycle moves the game from Blood Gulch to Battle Creek and the client follows and spawns on both;
  `disconnect` then `connect` rejoins; a wrong password shows Halo's "Your password was rejected by
  the server."; the right one joins; a dead address shows "Unable to join game.". Fixes: the test
  closes Halo's console after typing (an open console takes the keys and buttons), fires 250
  frames after the spawn and holds the button 45 frames. `reference-server.sh` takes extra server
  commands from `HALOPAD_SERVER_INIT`. The map-change and reconnect runs once timed out under load
  (Halo idling in its message wait); run alone they pass in about 90 seconds.
- **Touch controls redesigned** after Chris found them ugly: a floating stick, FIRE large on the
  right with eight actions on a ring around it, glass circles with SF Symbols and small captions,
  utilities small at the edges ([SUNPAD-TRANSFER.md](SUNPAD-TRANSFER.md)). The self-test passes on it.
- **Preview:** `scripts/ios-preview.sh [--iphone] [--connect ADDR:PORT]` builds the app, opens the
  Simulator and leaves HaloPad running at Halo's main menu (or in a server's game).

- **Byte imul in memory translated** (lifter 9d6c88b47852-7ddadfc0, run 20260928T060918Z-85892): libpng's transform-info step 0x5980c4 matches the x86 oracle 300/300 (png_transform_info slice). Two named traps remain, both in the C runtime's Pentium FDIV workaround. All suites that need no network ports pass on the new run; join, host, lifecycle and the network scenarios wait until the preview Simulator is closed (it holds UDP 2302-2303).

- **G4 reviewed** ([G4-REVIEW.md](G4-REVIEW.md)): every G4 item is met on macOS with evidence; the goal stays formally open only for the original-client comparison (G1b, parked on a key). All macOS suites, the join, host, lifecycle and the five network scenarios pass on run 20260928T060918Z-85892.

## 2026-09-28 — Touch controls, second iteration; the branch on GitHub

- **Touch controls:** left-handed mode (mirrored layout, the ring kept off the motion tracker),
  ring spacing (compact, normal, spread), button labels on or off, and shadows under icons and
  captions. The settings panel holds the three new controls. Screenshots of each on the iPad
  Simulator; the self-test passes in left-handed mode.
- **Pushed** `codex/halopad-phase2` to the private GitHub repository at Chris's request (a backup,
  not publication: G12 still gates that).

- **Maps and mods (M16):** joining POQclan CE01 on the custom map `coldsnap` gets Halo's own "An error
  has occurred loading a map file." and stays in the menus; custom maps belong in
  `HALOPAD_STATE_ROOT/install/maps/`; mod plugins (Chimera, HAC2, OpenSauce) cannot load because the
  module table is fixed and `dinput8.dll`/`strings.dll` are always HaloPad's own.

- **Prepared-data import (G9):** the app bundles its own data (8.5 MB), keeps state in Application
  Support, and takes the player's Halo Custom Edition folder in Documents (Files app, Finder, or
  the import screen's folder picker), accepting it only with the locked 1.10 `haloce.exe` hash and
  the stock files present. On the Simulator with `--device-data`: no folder shows the import
  screen; the copied folder is accepted and Halo reaches its menu from device paths alone.

- **Input lag on the Simulator found and fixed:** `HALOPAD_TRACE_FRAMES` showed 3-6 frames a second for
  30 s after a join; a stack sample put 61% of Halo's thread in `DrawPrimitiveUP` making Metal
  buffers (one per draw plus two per draw for constants, each a driver round trip on the
  Simulator). A per-frame transient arena replaces them: 30 frames a second, longest gap 40 ms.

### 2026-09-28 — regression evidence and touch-settings follow-up

- Previous goal turn: verified wait on the full regression. Recovered session 6682 to completion;
  lifecycle launch 2 failed (`core-arm64-apple-macosx14.0.0-20260928T082315Z`), while the five
  network scenarios and other reported suites passed. G4 relaunch is reopened, not silently
  covered by older successful runs.
- The runner always exited zero after collecting results. It now propagates client failures,
  signals, traps and timeouts, while preserving per-launch evidence. Relaunch also copies the
  first launch's registry into the second launch's evidence directory; previously only the
  filesystem state persisted. Actual child-process tests cover these cases.
- Diagnostic rerun `core-arm64-apple-macosx14.0.0-20260928T104130Z`: launch 1 passed; launch 2
  loaded New001 and navigated Multiplayer. Opening Simulator during the test stopped new
  frames at route step 19; a stack sample shows Halo waiting in MsgWaitForMultipleObjects.
  The Mac host forwards activation changes, and this automated test had not disabled host
  input. Stopped that identified process (SIGTERM, recorded exit -15) after capturing evidence.
  This demonstrates interference in the harness, not the cause of the earlier premature exit.
- Lifecycle automation now uses the existing host-input isolation used by the input suites;
  it still sends normal key events through HaloPad's input boundary. Quit acceptance also
  requires that the scripted confirmation was actually sent, not merely any return from main.
- Touch settings now scroll inside the safe area, with a fixed title/Done row and 44-point
  minimum settings rows. Opening the panel clears held touch input; touches outside it cannot
  reach gameplay while it is open. Simulator preview instructions stop only the selected
  device rather than every Simulator.
- Corrected STATUS's outdated push statement and overclaims about the public soak, physical
  touch delivery and steady-state frame performance. G5/G6 full acceptance, physical devices,
  retail campaign, and other original requirements remain open.
- Isolated saved-profile recheck **PASS**: `lifecycle-recheck-20260928T105102Z`, copied from the
  failing run's state, no per-step screenshot slowdown. 4,702 frames, `ui beavercreek ui`, New001,
  saved profile and scripted quit confirmation all pass. The reduced replay script is retained
  with its evidence. Full consecutive-launch validation with the updated harness is still due.
- Cold iPhone preview exposed a real activation-order defect: if UIKit reports active before
  the guest window exists, `app_active` suppressed the window's first WM_ACTIVATEAPP. Halo
  waited in MsgWaitForMultipleObjects with a black game view; background/foreground restored
  its main menu. Added an early-activation mode to the USER32 test: before the fix,
  `core-arm64-apple-macosx14.0.0-20260928T105956Z` fails the exact first-show message sequence.
  A pending activation is now delivered when the first guest window activates. Final early
  tests pass on Mac (`20260928T110133Z`) and iPhone Simulator (`20260928T110148Z`).
- Source locks verify. All 23 Python tests pass. `xcrun devicectl list devices` reports no devices;
  physical acceptance remains parked. Private GitHub visibility was rechecked before pushing.
- Phone cold-start preview with the pending-activation fix: `ios-app-20260928T110221Z` reached
  Halo's menu at 30 fps without a background/foreground workaround. The touch panel renders
  within the landscape safe area; a real Simulator drag reaches Spacing/Move Controls/Reset
  while Done stays visible (`settings-scrolled.png`); Done closes it. During subsequent native
  UI automation, the game again stopped presenting (0 fps, map `ui`), while the panel remained
  interactive. Cold-start delivery is repaired; interruption recovery is NOT closed by this
  test and must be traced separately (scene transitions and queued activation events).
- Final normal-order USER32 regression passes at `core-arm64-apple-macosx14.0.0-20260928T110453Z`.
  Next lowest-goal experiment: run `.venv/bin/python scripts/run-core.py --work
  generated/srw/custom-en-1.0.10.0621/run-20260928T060918Z-85892 --relaunch --timeout 900 --main
  tests/halo_lifecycle_test.c` after stopping the preview. Require both actual processes to
  pass the map/name/quit assertions and runner exit zero. Then trace the iOS inactive/active
  event sequence around native UI interaction; fail recovery if frames do not resume.
- iPad preview: `ios-app-20260928T110541Z`, same final source, local Blood Gulch via
  `-exec halopad_preview.txt` in the ignored iOS install overlay. Screenshot shows the player,
  HUD and touch controls; trace records 29–30 fps and 299/299 presented drawables. Left this
  single app running for Chris's requested preview on HaloPad iPad Pro 13
  (`E129A00F-D338-4FDC-8AE8-BB243E9BA61B`); iPhone Simulator is shut down and no reference server
  is running. This is a development component scene, not completion of normal licensed startup.

### 2026-09-28 — foreground wake-up and full relaunch

- Previous goal turn was progress: `e252ee2` repaired early window activation, phone settings,
  and regression reporting. Read the goal objective again; the full campaign, interoperability,
  physical-device and release scope is unchanged. Input hash matches the locked profile;
  only the requested iPad preview was active and was stopped before testing.
- Full consecutive lifecycle test on `e252ee2` **PASS**, runner exit 0:
  `core-arm64-apple-macosx14.0.0-20260928T110902Z`. Launch 1: 8,502 frames, map reload and
  `ui beavercreek ui beavercreek ui`; launch 2: 4,702 frames, persisted New001 and
  `ui beavercreek ui`. Both require the scripted Quit confirmation. G4's local relaunch row
  is revalidated; the original-client comparison is still required.
- Found a separate runtime error in MsgWaitForMultipleObjects: host activation invokes the
  guest window synchronously, but the wait only checked queued input/handles afterward. It
  could remain asleep after foreground activation restored the game. Microsoft's API contract
  explicitly requires a system-event wake for foreground activation, even with wake mask 0:
  https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-msgwaitformultipleobjects
- Added an iOS queue-path regression with a real guest window and a nonsignaled event. Pre-fix
  binary `core-arm64-apple-ios17.0-simulator-20260928T111641Z` fails both wake checks with
  WAIT_TIMEOUT despite restoring the foreground window. Binary and replay are retained in the
  ignored evidence. Fixed runtime tracks synchronously handled system notifications and wakes
  the current wait; it does not post a fake key or game action. Final iPad test `…111711Z` passes
  both wake masks and checks no activation replay on the next wait. Mac USER32 `…111808Z` passes.
- Added optional scene/pump activation breadcrumbs (`HALOPAD_TRACE_LIFECYCLE`) and clear the
  touch overlay on scene deactivation. Simulator OS background/foreground checks follow;
  passing the API regression alone does not close real-app recovery.
- Actual iPad app `ios-app-20260928T111910Z`: two Home/background → foreground cycles resume
  local Blood Gulch at 29–30 fps in PID 20447. The second cycle keeps touch settings open;
  `resume-1.png`, `settings-after-resume.png`, and `resume-settings-open.png` record the result.
- Actual iPhone app `ios-app-20260928T112538Z`: Home produces scene inactive/pump activation 0
  at frames 344; reopening keeps PID 21102, delivers activation 1 and resumes 30 fps with
  advancing presented-frame counters. `resume-1.png` and `settings-after-resume.png` retained.
  Native menu and touch settings open afterward while game frames continue. Phone stopped
  and shut down before restoring the iPad preview.
- Scope: these short offline resume checks pass. Lock/unlock, audio interruptions, long online
  suspension/reconnect, pending typed text, held hardware/controller input and physical devices
  still need their own tests. Full lifecycle was run on `e252ee2`; the final additional runtime
  change was checked by USER32 regressions and actual app recovery, not a repeated full matrix.
- Final preview `ios-app-20260928T112758Z` is left running in local Blood Gulch on HaloPad
  iPad Pro 13, PID 21599; Simulator is visible in landscape. iPhone is shut down. Use the
  three-dot menu → Touch Control Settings to inspect the controls. Build output and native
  screenshot retained. Repository safety and whitespace checks pass; private remote visibility
  rechecked before the authorized push.

### 2026-09-28 — cancel unfinished typing on interruption

- Previous turn: **progress**, committed and pushed `bc38545` with foreground wake recovery.
  Re-read the supplied objective and current loop/status. Clean initial tree; source locks
  verify, accepted input hash matches, project bottles remain halopad-patch/halopad-reference,
  no reference server is running, only the requested iPad preview was active. Stopped that
  candidate before the boundary test. Parked inputs have not changed.
- Hypothesis: the overlay's 50 ms typed-key timer retained unfinished text across scene
  deactivation, allowing a partial command to finish after interruption. The old source had
  no text-queue cancellation; clearing touch controls only released buttons/stick/look touches.
- Added main-thread text activation handling. Deactivation clears pending events, immediately
  posts key-up for keys the timer delivered (including Shift), and rejects new queued typing
  while inactive. Activation resumes empty. The scene callbacks invoke it alongside existing
  activation events. Touch clearing also resets fractional look motion.
- New `.venv/bin/python scripts/test-ios-overlay.py` compiles the actual Objective-C overlay
  and timer for a booted Simulator with a capturing host-input boundary. Evidence records
  source/header and executable hashes, build command, output and process exit. First build
  needed CoreGraphics added to its link. The first runtime test used a run-loop wait that
  drained the whole text queue before checking the intended mid-key fixture; corrected the
  test to poll in 5 ms intervals. No runtime fix was made for that harness timing issue.
- Final boundary test `G9/overlay-20260928T113413Z`: **16 checks PASS**, exit 0. Normal uppercase
  and Enter ordering, interruption with a letter/Shift held, no character payload on releases,
  repeated cancellation, inactive input rejection, no replay, fresh lowercase typing, cancel
  before first delivery, movement/fire release and fractional look reset all pass.
- Highest-known gameplay smoke on this source: `G3/ios-app-20260928T113459Z`, local Blood Gulch
  with `HALOPAD_TOUCH_SELFTEST=1`. Move 4.42 units; look 39.7 degrees; fire 60→57 rounds; jump
  height 0.11→0.76. Four handler-driven game-state checks pass. These do not prove multi-touch
  hit routing or every short press surviving multiple host pumps per frame.
- Actual Home/foreground cycle with the native keyboard open kept PID 23229 and resumed 30 fps
  with advancing frame counters. `text-resume-ready.png` retained. The attempted UI console
  typing check is **inconclusive**: the resumed screenshot shows gameplay and the keyboard,
  not a visible console/text line. Do not treat this as end-to-end text cancellation evidence.
  Keyboard dismissed afterward; the same updated iPad preview remains in Blood Gulch.
- Next: trace native-menu console key delivery through the host queue and Halo's input reads;
  short-press visibility and actual touch routing remain open, alongside the full original
  campaign/interoperability/device scope. No gate was narrowed to these passing checks.

### 2026-09-28 — two-stick controls and consistent action spacing

- Chris explicitly asked for continued touch iteration: two sticks and better spacing. The
  preceding queued-text fix was committed/pushed as `be14ac0`; this iteration keeps that fix.
- Replaced the floating MOVE/FIRE-ring layout with fixed MOVE and LOOK targets. Each view owns
  its touch; open-space swipe aim and drag-to-aim on FIRE remain. LOOK's displacement produces
  continuous mouse counts through a display-timed callback, with a radial dead zone and a
  gentler centre response. Release, settings, controller hiding and scene interruption reset it.
  MOVE retains the game's W/A/S/D input mapping; this is not a claim of analog movement speed.
- Dedicated FIRE and rows of SWAP/ZOOM/RELOAD, USE/MELEE and CROUCH/JUMP sit above/beside LOOK;
  THROW/NADE/LIGHT sit above MOVE. Scale is bounded for the complete arrangement, then target
  positions use explicit gaps. Default targets are at least 44 points; the native menu is 44.
  Tablet stick centres move inward to clear the motion tracker. Left-handed mode mirrors the
  groups. Layout keys moved to v3, preserving the user's old v2 settings rather than reusing
  incompatible ring coordinates. Manual custom layouts can still overlap by user choice.
- Extended actual-overlay boundary tests: five landscape sizes, three control sizes, three
  spacing choices and both hands, with phone/tablet safe-area insets. 90 combinations all pass
  rectangle separation, bounds, minimum size and independent stick hit targets. Added held LOOK
  motion, release and dead-zone checks. Final iPad boundary run `G9/overlay-20260928T114719Z`
  and iPhone `G9/overlay-20260928T114935Z` pass all 20 assertions. The final phone boundary run
  includes the cosmetic native-menu corner-radius change.
- Halo iPad `G3/ios-app-20260928T114531Z`: move 4.42 units, swipe 39.7°, fire 60→44 rounds,
  jump 0.11→0.76, held LOOK 17.1° in 0.5 seconds; all five checks pass. iPhone
  `G3/ios-app-20260928T114750Z` also passes all five (LOOK 18.7°), then returns to 29–30 fps.
  Inspected both layouts in the actual Simulator in landscape. An attempted pointer drag on
  the first iPad preview was not conclusive evidence of touch routing; no multi-touch pass is
  claimed. The self-test and view hit tests have their narrower scopes above.
- Phone was stopped/shut down before final iPad rebuild. Next outstanding UI work includes
  actual multi-touch ergonomics, console/short-key delivery, and safe data import. Broader
  campaign, interoperability, normal licensed startup and physical-device gates remain intact.
- Final iPad source preview `G3/ios-app-20260928T115013Z` passes all five gameplay checks again:
  move 4.42 units, swipe 39.7°, fire 60→45 rounds, jump 0.11→0.76, LOOK 29.3°. Left running in
  local Blood Gulch as PID 25733 on HaloPad iPad Pro 13, visible in landscape; iPhone is shut
  down. `landscape-layout.png` retained. Final visual inspection shows distinct sticks, spaced
  actions and the tablet motion tracker clear of the MOVE target. Safety/whitespace checks pass.

### 2026-09-28 — protect the game touch surface from the keyboard proxy

- Previous two-stick work was committed and pushed as `0bc2335`. Continued investigation
  traced the console key through the UIKit queue, host pump and DirectInput buffered read.
  `G3/ios-app-20260928T115440Z` shows both down/up delivered and the pink `halo(` prompt
  visible at the bottom of the game. This does not establish general short-press reliability.
- Found `viewDidLayoutSubviews` assigning full-screen bounds to every root sublayer, including
  the UIView-backed invisible keyboard proxy. Restrict the resize to guest CAMetalLayers and
  explicitly make the proxy reject touch hit tests. Overlay and keyboard view geometry now
  stay with UIKit. No guest input timing behavior changes in this increment.
- Added opt-in console-key tracing and `HALOPAD_OVERLAY_DEMO=console` / `console-keyboard`
  reproductions. Tracing logs only the console scan code, not entered text.
- A keyboard-layout-guide viewport experiment was removed before commit: Simulator reported
  first-responder focus and exposed keyboard accessibility elements, but screenshots did not
  show the software keyboard and the guide did not shrink. Evidence in
  `G3/ios-app-20260928T115935Z` and `G3/ios-app-20260928T120412Z` is diagnostic, not a pass.
  A Simulator keyboard-toggle attempt was followed by a black game surface with Halo waiting
  in MsgWaitForMultipleObjects; Home then foreground restored the same process and image.
  Keyboard visibility/occlusion and that transition remain open for a focused reproduction.
- Final build `G3/ios-app-20260928T120722Z` passes all five handler-driven gameplay checks:
  move 4.42 units, swipe 39.7°, fire 60→44 rounds, jump 0.11→0.76, held LOOK 25.4°.
  The render layer remains 1376×1032 and the keyboard proxy remains 0×0 in the layout trace.
  Inspected the final two-stick layout in the running Simulator; no physical multi-touch pass
  is claimed. iPad preview left running as PID 28415; iPhone remains shut down. The full goal
  stays active with its parked rows unchanged.

### 2026-09-28 — visible console text above the software keyboard

- Previous goal turn is **progress**: `40928b9` corrected root layer sizing and was pushed;
  the final gameplay smoke and console trace supplied fresh evidence. This turn rechecked
  clean Git state, the locked executable hash, pinned sources, project bottles and sole live
  iPad candidate. Parked inputs are unchanged. No reference process was running.
- Hypothesis: the missing software keyboard is presentation state rather than dropped Halo
  input. Added opt-in keyboard frame/focus logging. In `G3/ios-app-20260928T121101Z`, UIKit
  reported a zero-height keyboard at y=1032. Disconnecting Simulator's hardware keyboard via
  I/O → Keyboard → Connect Hardware Keyboard immediately produced a 460-point docked keyboard
  at y=572. The screenshot proves the old full-screen game prompt was covered. The project
  iPad's Simulator preference is now hardware keyboard disconnected for touch testing.
- Put guest Metal layers in a noninteractive render host constrained above the docked keyboard
  with UIKit's keyboard layout guide. `usesBottomSafeArea=NO` restores the full display when
  hidden. While typing, preserve the game's proportions even if stretch was selected; restore
  the saved display choice on dismissal. Pointer coordinates use the same rendered viewport.
  The overlay retains its full-screen geometry; the invisible input proxy still rejects taps.
- `G3/ios-app-20260928T121340Z`: actual software-keyboard taps entered `help` into the local
  console, visibly above the keyboard. Deleted it without submitting. Hide Keyboard restored
  1376×1032 from 1376×572; the game continued at 30 fps. Screenshots and keyboard/frame traces
  retained. This closes the docked iPad visibility reproduction, not all keyboard modes or
  physical-device input. Floating/split keyboard occlusion remains unverified.

- Phone `G3/ios-app-20260928T121616Z` passes the five gameplay checks with the new render
  host. Actual phone console input also works in `G3/ios-app-20260928T122107Z`. Added a native
  Hide Keyboard accessory for the phone and suppressed/released gameplay targets while typing.
  This exposed an iOS 26.5 guide retaining the 44-point accessory height after resignation;
  switch back to the full-view constraint when the reported keyboard frame is offscreen.
- Extended real-overlay boundary tests to verify held MOVE/FIRE/LOOK release, hidden targets,
  no open-space swipe capture while typing, and dismissal without replaying holds. The first
  new assertion incorrectly included the always-hidden settings panel as a gameplay target;
  corrected the fixture to capture the 15 visible gameplay targets before hiding. Final phone
  boundary `G9/overlay-20260928T122344Z` passes all 23 assertions and 90 layout combinations.

- Final phone dismissal `G3/ios-app-20260928T122415Z` restores 874×402 from 874×194, with all
  gameplay targets visible; screenshot retained. Phone stopped and shut down before iPad.
- Final iPad boundary `G9/overlay-20260928T122541Z` passes 23 assertions and 90 combinations.
  App `G3/ios-app-20260928T122626Z` passes all five gameplay checks (move 4.42 units, swipe
  39.7°, fire 60→44, jump 0.11→0.76, LOOK 26.4°). Final accessory/viewport presentation and
  dismissal were inspected on the same build. After reboot/orientation changes, the Simulator
  initially showed only the accessory despite its disconnected hardware preference; refreshing
  the hardware connection and using Hide Keyboard then Show Keyboard restored software keys.
  This Simulator transition remains an observed limitation, not a claim of general keyboard
  lifecycle closure. Final docked keyboard occupies 504 points including the toolbar; the
  game returns from 1376×528 to 1376×1032 on dismissal.
- Left PID 31714 running in local Blood Gulch with console and keyboard closed in landscape.
  iPhone remains shut down; no reference processes. Full campaign/interoperability/licensed
  startup/device gates stay intact; next unblocked work includes safe import, pause-menu touch
  ownership and further keyboard lifecycle checks. No full-goal completion claim.


### 2026-09-28 — release gameplay touches while Halo menus are open

- Previous iteration: **progress**, committed/pushed as `f589887` (docked keyboard visibility,
  input release and full-viewport restoration). Goal remains active. Re-read the objective and
  phase loop; checked clean starting Git state, locked executable hash, pinned sources, project
  bottles and the sole iPad candidate. Parked inputs unchanged; no reference process running.
- Hypothesis: map-name-only overlay visibility leaves controls active over in-game menus.
  First tried queued 50 ms and held 300 ms Escape in the `map_name` development preview
  (`G3/ios-app-20260928T123743Z`, `...124104Z`). Neither opened a widget. This fixture is
  insufficient to diagnose pause delivery; no guest state was patched to force a menu.
- Switched to Halo's normal Multiplayer → Create Game → LAN → Battle Creek → Slayer flow,
  navigating with the connected Simulator hardware keyboard. Actual Pause touch opened the
  menu in `G3/ios-app-20260928T124534Z`, with both sticks/actions still overlaid. Captured
  `pause-before.png`. Mouse acquisition/cursor-display flags stayed unchanged through menus,
  so they cannot determine touch ownership.
- Read-only research: Chimera `c41414e2c729f61a6b98d1f35470a89f8bd18dc0` widget signature
  locates the locked CE loader; inspected that executable's loader/deletion accesses to
  confirm the active root at `0x6b401c`. Runtime observations matched the main menu, child
  widgets, gameplay and pause. Source remains ignored research, not linked/copied code.
  Do not substitute the game-paused flag: multiplayer can keep simulating with a menu open.
- Snapshot map/menu eligibility on Halo's presenting thread, dispatch only ownership changes
  to UIKit. Opening a widget clears held MOVE/FIRE/LOOK and hides gameplay targets, passing
  the surface through. Keep the existing Escape target as a small Back chevron during in-game
  menus; it returns one level or resumes. Keyboard visibility still hides it. Main-menu state
  hides all gameplay controls, including this in-game Back target.
- Real-overlay boundary `G9/overlay-20260928T125353Z`: 30 assertions and 90 phone/tablet,
  handedness, size and gap combinations pass. New checks cover release/no replay, former
  control-center hit testing, Back reachability, keyboard precedence and main-menu hiding.
- Actual final-build app `G3/ios-app-20260928T125447Z`: started another normal local match,
  tapped Pause and Back to resume, opened Game Options using keyboard navigation, then used
  touch Back twice to return through pause to gameplay. Both sticks/actions stayed hidden
  in the child menu and returned on resume. Screenshots `pause-after.png`, `child-menu.png`,
  and `final-preview.png`; frame-state transitions in stderr. Candidate PID 34612 remains
  in local Battle Creek. No phone/reference process started this iteration.
- Remaining: absolute touch selection of Halo's own menu items (cursor remains at its old
  position), short-input edge retention across multiple pumps, true simultaneous multi-touch,
  physical ergonomics, and safe prepared-data import. The hardware keyboard was connected
  for this menu route and was disconnected again before leaving the touch preview. This increment
  closes overlay menu ownership, not complete menu navigation or G9/full-goal acceptance.


### 2026-09-28 — direct finger selection in Halo menus

- Previous iteration: **progress**, committed/pushed as `975bd7a` (menu ownership and release).
  Goal remains active. Re-read the objective; verified the locked executable hash and pinned
  sources, clean starting branch, bottles and sole project preview. Parked inputs unchanged.
- Hypothesis: absolute USER32 coordinates do not move Halo's accelerated software menu cursor.
  Inspected locked CE routine `0x49a220`: cursor at `0x6b400c/0x6b4010`, sensitivities at
  `0x629c64/0x629c68`, acceleration d*(1+abs(d)*sensitivity*.05), clamped to 640×480.
  The original input conversion negates raw Y before the UI routine negates it again.
- Added a menu-only adapter: convert direct touches through the displayed viewport, invert
  acceleration, send ordinary relative mouse events on Halo's presenting thread, observe the
  resulting cursor before pressing, then release on a later frame. No guest writes, function
  injection, changed Halo instructions or replacement menus. Read-only snapshots and UIKit
  requests share a mutex. The alternate `0x6b4009` OS-cursor path is not enabled or claimed.
- Preserve original press coordinates during fast drags, queue up to eight rapid gestures,
  leave a released frame between them, and cancel/release on root changes or scene interruption.
  An unresponsive cursor cancels after eight unchanged correction attempts rather than clicking
  later. Gameplay look/sensitivity behavior is unchanged.
- `G9/menu-touch-20260928T131724Z`: seven gesture-sequence scenarios pass; 72 routes through
  the original x86 cursor routine converge within two corrections across four sensitivities,
  three starting positions and six targets. Oracle fixture writes are test-only. The first
  harness run failed while recording a relative source path after its computations; fixed
  path resolution and reran successfully. Final report records image/source hashes.
- Preliminary iPad `G3/ios-app-20260928T130906Z` verified direct Settings/audio-arrow taps.
  Preliminary phone `...131637Z` was stopped before testing the revised fast-drag implementation.
  Final phone `...131954Z` verifies a sidebar touch leaves the cursor/menu unchanged, direct
  Settings → Audio Setup, and small volume arrows 10→9→10. Screenshot retained. Phone stopped
  and shut down before final iPad; candidates were never run concurrently.
- Final iPad `G3/ios-app-20260928T132504Z`: built and launched revised sources, rotated to
  landscape, then navigated Multiplayer → Create Game LAN → Battle Creek → Slayer → Start
  Game using actual Simulator touches alone. Pause touch hides sticks/actions; direct Game
  Options, original Back and original Resume Game all work. Screenshots `touch-child-menu.png`
  and `final-preview.png` plus cursor/button traces retained. No hardware-keyboard navigation.
- Leave PID 38291 running in local Battle Creek on the sole booted project iPad. iPhone is shut
  down. This closes these tested direct-menu routes, not all G9: general short gameplay-input
  edges, true simultaneous multi-touch, physical ergonomics and alternate pointer modes remain.
  Next unblocked work includes safe prepared-data import and further input/lifecycle checks.
  Original licensed startup, campaign, two-player and device gates remain intact.


### 2026-09-28 — preserve installations during folder import

- Previous iteration: **progress**, committed/pushed as `06bfec0` (direct finger menu selection).
  Read the objective and inherited loop/PRD; starting tree clean. Locked executable hash and
  pinned sources verified; halopad-patch/reference bottles present, no Wine process. The sole
  candidate was iPad PID 38291; parked inputs unchanged. Chose M29's unblocked import safety.
- Hypothesis: the picker removes its destination before copying, so a failed copy loses the
  previous install and selecting the destination itself deletes the source. Confirmed in code.
  Extracted shared Foundation validation/import service; source/stage checks reject wrong
  executable identity, missing/empty/non-regular required files, links/special files and
  ambiguous case-normalized names. Hashing streams instead of mapping the whole file.
- Stage beside destination, validate again, atomically rename/exchange and retain the old tree.
  Same-folder imports validate only; overlapping trees are rejected. Failed copy/validation/
  publication removes only staging. Even failed backup naming keeps the old tree and returns
  its actual path. No changes to guest code, keys or module loading. Actual UI coordinates
  source reading/destination writing with NSFileCoordinator and holds scoped access throughout.
- Added disposable inert-fixture tests exercising real filesystem transactions. Injected
  partial-copy, staged-corruption, publication and backup-rename failures prove preservation.
  Final macOS `G9/import-20260928T134206Z` and iPad `...134207Z` pass all 12 scenario groups;
  phone `...133717Z` passes before a missing-folder wording-only adjustment. No data fixtures
  committed. Updated the import panel to scroll within safe-area bounds and removed conflicting
  width constraints; re-entry controls disable while importing.
- Stopped iPad before phone tests, then shut phone down and returned to iPad. Backed up device
  Application Support state under `G9/import-ui-20260928T133826Z/state-before`; renamed the
  existing game folder to a retained Import Test Source before the UI test. First app build
  caught an accidentally broad edit assigning an import property in the license controller;
  removed that unrelated assignment and rebuilt successfully. The license UI is unchanged.
- Actual app `G3/ios-app-20260928T133918Z`: Files picker → On My iPad → HaloPad → retained source
  → Open. The copy succeeds and the development scene reaches Halo's menu from device paths.
  Screenshot retained; no layout-conflict diagnostic. Independent whole-tree comparison finds
  all 105 copied files byte-identical to source. First comparison helper used a hashlib method
  absent from system Python; reran with a streaming helper successfully. Neither source nor
  previous state was discarded. Current data container is resolved again after app installs.
- This is folder-import safety progress, not M29 completion. The PRD's `.halopad.zip` prepared
  package/manifest workflow remains required, along with full content validation, management,
  crash recovery, provider/device tests and phone UI inspection. See IMPORT.md for evidence and
  limits. These requirements are not replaced by the direct-folder route. Goal stays active.
- Final build/relaunch `G3/ios-app-20260928T134505Z` reports device game folder accepted, starts
  Halo's menu, and runs at 29–30 fps. Created the local New001 profile with the software keyboard
  and reached Multiplayer by touch. CUA then reported user interaction; stopped driving the UI
  and left PID 41176 available to Chris. Only the project iPad is booted, phone shut down.
- Final review also latches successful Check Again before dismissing the sheet, preventing a
  second tap from scheduling another start. Compiled this final UI guard without installing or
  restarting Chris's live preview; its rapid-tap behavior is not separately exercised in Simulator.


### 2026-09-28 — reproducible prepared-data packages bound to the compiled core

- Previous iteration: **progress**, committed/pushed as `adeb744` (transactional folder import).
  Read the objective, inherited loop, PRD package requirements, current inventory/status and
  parked inputs. Starting branch clean; accepted executable SHA and all pinned sources pass.
  Project bottles unchanged, no Wine process. iPad PID 41176 remains live; phone shut down.
  Leave the user-controlled preview untouched. No parked input arrived.
- Hypothesis: package preparation can make M29 progress without disrupting the preview, but a
  profile-name-only manifest would accept data for a different compiled core. Added build-time
  signed-bundle metadata binding five native Halo/module object hashes, dispatch/runtime IR,
  target, locked source identity, bundled inert data and approved stock resource records.
  Only hashes identify native objects; no native code/signing/player state enters the ZIP.
- Added `prepare-game-data.py` and shared `halopad_package.py`: selected stock files plus inert
  core data, exact external expected inventory, streaming SHA-256, deterministic ZIP metadata,
  no-clobber staged publication, post-write verification, and read-only source handling.
  Reject wrong-core/content/manifest, unsafe/aliased paths, links/special files, unsupported
  archive methods, encrypted entries and size violations. No unpacking/execution/downloads.
- App build succeeds without launch/install, producing core identity before ad-hoc signing.
  Package tests initially exposed Python 3.8's missing Path.is_relative_to; switched to resolved
  parent containment. Final 13 package tests plus five real-Git safety tests pass. A source
  mutation after initial hashing is caught before publication. Ambiguous parent components
  are rejected as well as full-path aliases. Forced staging of ignored .halopad.zip is rejected.
- Actual engineering data: `G9/package-20260928T140152Z` contains two independently generated
  archives of 78 stock files + nine inert data files (456,118,096 expanded; 181,798,693 compressed
  bytes). Both archive hashes match; final CLI verification passes. Identity/source hashes,
  build log and tests recorded privately. The packages remain under generated/prepared/.
- Native ZIP parser/import, picker/preparation UI, equivalent full-content folder validation,
  custom-map packaging, restore/remove/recovery and full device/provider acceptance remain
  required. Documented format/trust boundary and next experiment in PREPARED-DATA.md. Do not
  call this complete import support or M29 acceptance. Goal remains active, all campaign,
  licensed-startup, interoperability and physical-device gates retained.

### 2026-09-28 — symmetric thumb reach and balanced touch action groups

- Previous increment: **progress**, private push `7a30e53` (prepared-data packages).
  Followed Chris's renewed touch-layout direction for this pass. Locked executable SHA and
  pinned source checks pass; only the existing iPad candidate PID 41176 is running, no Wine.
  No parked input arrived. Native ZIP import and all original goal gates remain outstanding.
- Replaced percentage-of-width stick positions with symmetric point-based edge offsets.
  FIRE centers over LOOK, CROUCH joins the movement side, and SWAP joins the right grid.
  Keep at least eight points between default targets. Reviewing the prior actual gameplay
  screenshot exposed radar overlap in an initial edge-only design; raised the tablet groups
  to preserve a conservative bottom-left HUD region. Phones retain their lower grip.
  Saved v3 custom layouts are preserved; Reset Layout adopts the new defaults.
- Stick captions obey Labels. Circular views clip their contents to the circle, eliminating
  visible rectangular border remnants in offscreen UIKit renders. No input mappings changed.
- Extended the real-overlay harness with offscreen native UIKit renders at five landscape
  sizes. It has a capture-only host boundary, no guest core or window, and cannot send input
  to the running game; clarified the old stop-candidate comment accordingly. First attempt
  to launch through /usr/bin/env lost Simulator dyld configuration and aborted. Switched to
  SIMCTL_CHILD environment forwarding; all subsequent harness launches pass.
- Final evidence `G9/overlay-20260928T141747Z`: 31 input/layout assertions plus five successful
  PNG writes; 90 size/spacing/handedness/safe-area combinations pass. Strengthened checks
  cover actual eight-point gaps, symmetric bounded reach, fire alignment, crouch side and
  tablet HUD clearance. Inspected final phone/tablet renders. Existing input release/menu/
  keyboard/continuous LOOK checks remain green. Full Simulator app builds without install.
- Preserved the live user-controlled Simulator session and queued the tablet layout PNG in
  Codex for inspection. New controls are built but not installed in that running preview.
  These are native offscreen overlay images, not new in-game acceptance screenshots.
  Actual simultaneous touches, on-device grip comfort, updated in-game HUD verification and
  saved/custom layout behavior remain to be exercised. G9 and the full goal remain active.

### 2026-09-28 — native prepared-package import and rejection/rollback evidence

- Previous goal turn: **progress**, private push `39f04ae` (balanced touch layout).
  Read the objective attachment and inherited requirements. Working tree started clean;
  locked executable SHA, pinned sources and process checks pass. Existing iPad PID 41176
  remains live, phone shut down, no Wine. No parked input arrived. Preserve the user's preview.
- Advanced the required .halopad.zip route. Inspected the pinned UTP ZIP implementation but
  did not copy it; it allocates whole outputs and lacks this format's ZIP64 support. Public
  iOS SDK has zlib headers/library; libarchive has no public SDK header. Implemented an
  independent bounded Foundation/zlib reader using PKWARE APPNOTE field definitions.
- Exact signed-bundle identity/inventory, duplicate JSON-key/type checks, path/name/type bounds,
  central/local record agreement, ZIP64, descriptors, CRC32, streaming SHA-256 and size limits.
  Stock files are staged; inert core-data copies are verified and discarded. The signed runtime
  images remain authoritative. Shared the folder importer's rename/swap publication boundary;
  old installs are always retained, including when backup naming fails. No guest checks changed.
- Connected Choose Prepared Package and Mac preparation text to the security-scoped/coordinated
  picker background path. Folder selection remains available with its existing weaker checks;
  full folder identity/startup validation is explicitly unfinished, not waived. The app builds
  and its signature verifies; did not install or replace the live user's preview. UI/provider/
  gameplay acceptance for this package route remains open.
- First native rejection tests exposed dangling NSError objects escaping an inner autorelease
  pool, causing test-process crashes. Removed the pool and reran the failures. Subsequent
  test-driver imports run on background dispatch workers with an autorelease boundary, matching
  the app's execution context and checking returned-error lifetime. Also tested ambiguous data
  descriptors whose CRC equals the optional signature; record boundaries disambiguate them.
- Final Mac ASan/UBSan run `G9/native-package-20260928T143600Z`: all 16 native tests pass,
  including wrong-core/content, malformed/truncated ZIP64, metadata/type/path attacks, bounded
  expansion, first/replacement import, first/partial write failures, commit failure and failed
  backup naming. iPad worker suite `...143716Z`: same 16 pass. Folder regression remains green
  (12 scenario groups): Mac `G9/import-20260928T142940Z`, iPad `...143303Z`.
- Actual prepared archive 6c24da53… imports into disposable evidence on both platforms. Mac
  `...143600Z` and iPad `...143436Z` compare all 78 installed files with the trusted stock hashes,
  verify nine core-data entries, retain the prior test folder and confirm the archive unchanged.
  iPad real-data run predates only the harness's move from main to a worker. No Halo core/window
  runs in these harnesses. Final app/build/source/core identity recorded under `...143716Z`.
- Goal remains active, M29 remains open. Next: equivalent folder/startup content identity and
  preservation tests, then iPhone/package-picker/provider acceptance, restore/remove and crash
  recovery. All campaign, licensed normal startup, controlled-player comparison, actual touch
  and physical-device requirements remain intact. Current user preview PID 41176 is unchanged;
  no new candidate process, server or Simulator was left running by this iteration.

### 2026-09-28 — complete stock validation for folders and device startup

- Previous goal turn: **progress**, private push `793d414` (native ZIP import). Read the goal
  attachment; inherited full scope remains intact. Started clean. Locked executable SHA,
  pinned sources and project process checks pass; no parked input arrived. Only the existing
  iPad PID 41176 is running; phone shut down, no Wine. Preserve the user's live preview.
- Removed the old executable-only/minimum-files folder check. Extracted the signed inventory/
  path policy into HaloPadDataIdentity, shared with ZIP import. Every stock size and hash now
  matches the bundled identity. Folder selection copies only approved records, using canonical
  output names; source extra installers, DLLs, maps and player files remain untouched.
- A stock-only same-folder selection remains validation-only. A legacy folder with extras is
  prepared via sibling staging and atomic swap; its entire original tree becomes the retained
  backup. Altered/missing stock data cannot be repaired by filtering. Application Support state
  is not moved or rewritten. Custom-map validation/import remains required, not silently accepted.
- Replaced unbounded directory arrays with streaming enumeration and an entry budget. Account
  for the signed inventory's own directory count/depth so the folder validator does not reject
  an otherwise approved package solely for its topology. Open relative components through
  directory descriptors without following symlinks. Copy at most the trusted byte count, flush,
  then hash the entire staged output before publication. Tests inject growth and parent-link
  replacement after preflight, including a link to otherwise matching bytes: both fail safely.
- Device startup clears any inherited game-root shortcut in device-data mode and hashes through
  the import panel on a worker. Check Again uses the same validator and retains the success latch.
  Explicit HALOPAD_IMAGE development paths remain separate. Legacy 105-file imports need a
  stock-copy preparation through Choose Folder before the new build will start them; the UI
  explains source/backup preservation. The live app remains the previous build.
- Final folder tests: 20 reported groups pass on Mac ASan/UBSan `G9/import-20260928T145721Z`
  and iPad `...145651Z`. Include same-size map/module/shader/config corruption, exclusions,
  same-folder backup, invalid identity, excessive entries/nesting, approved deep paths, growth,
  parent-link races, failed/partial writes and publication/backup failures. No warnings under
  -Wall -Wextra -Werror. No new failure was observed in this increment's final checks.
- Native ZIP regression remains 16/16: Mac ASan/UBSan `G9/native-package-20260928T145718Z`,
  iPad `...145720Z`. Both also import the actual engineering folder into disposable evidence:
  exactly 78 approved files match, all 105 source-file hashes are unchanged, prior fixture
  backup survives, and the complete startup validator accepts the copied result. No new Halo
  core or window is started by these harnesses.
- New full app built without installation; record final artifact/signature/source identity with
  the iPad native-package evidence. Actual startup/migration/picker/provider and iPhone coverage
  remain open. Highest game smoke was not rerun because it would replace the user-controlled
  preview. Next: exercise the new startup and folder/package selection on a safely handed-off
  Simulator session, then restore/remove/crash recovery. G9/M29 and the full goal remain active;
  all campaign, normal licensed startup, reference-player, actual-touch and device gates remain.

### 2026-09-28 — touch spacing, thumb tracking and updated Simulator preview

- Previous increment `6af262b`: progress. User explicitly asks to continue improving two-stick
  spacing and to push recent changes. Started clean; pinned sources and locked executable
  hash pass, no Wine, only project iPad booted. External full-goal gates remain parked.
- Latest Simulator inspection found the old preview at Multiplayer. Updated it under the
  user's continuing preview/testing authorization, after backing up Application Support and
  preferences and hashing all 210 Documents files. Evidence: `G9/touch-preview-20260928T150830Z`.
  Old PID 41176 was explicitly stopped; current development-scene PID is 56763.
- Replaced independently positioned top actions with a consistent two-column square grid
  beside LOOK. Lower two rows centre on the stick. FIRE retains its own larger target above
  LOOK; the pitch reserves its full gap at minimum sizes. LIGHT/NADE now match other secondary
  target sizes. Existing v3 custom layouts remain untouched.
- Fixed stick tracking: input previously normalized to the base radius but the visible thumb
  travelled less than that. The thumb now follows the finger until its rim, which is full
  deflection; out-of-bounds drags clamp radially and release returns to centre. MOVE still
  uses the original digital keyboard bindings, not analog movement speed.
- `G9/overlay-20260928T150807Z`: 36 input/layout assertions, 90 geometry combinations and
  five native renders pass. Added actual stick travel/radial/reset checks and shared grid
  alignment across all layouts. Full app build and codesign verification pass.
- Installing the new build exercised the pending startup validator. It correctly rejected
  extra files in the legacy install. Through the actual Files picker, selected that same
  Halo Custom Edition folder, retained its complete backup, then Check Again reached Halo
  with the existing New001 profile. `migration.json` proves all 78 installed files match
  signed stock, all 105 old install files match the retained backup and the separate import
  source is unchanged. No license decision was made on the user's behalf.
- Original menus → Multiplayer → Create LAN → Battle Creek → Slayer → Start Game. All five
  handler-driven gameplay checks pass: move 2.37 units, swipe look 39.7 degrees, weapon battery
  1.00→0.89, jump -0.22→0.44, held LOOK 23.9 degrees. Actual touch Pause/Resume also works.
  CUA drag attempts did not produce clear visual evidence of move/aim, so actual drag routing
  remains unverified; do not equate handler tests with simultaneous physical multi-touch.
- Leave the updated iPad gameplay preview open for the user. Phone remains shut down; no
  separate server/Wine process started. Prepared-package picker, phone migration, recovery,
  short input edges, real multitouch and physical device acceptance remain required. Full
  project scope (online, campaign, licensed startup/reference comparisons) is unchanged.

### 2026-09-28 — trace real touch routing and preserve final swipe movement

- Previous turn: **progress**, `05d45a4` privately pushed. Read the goal objective attachment
  and relevant G9/M27 requirements; full project scope remains intact. Started clean. Pinned
  sources and locked executable hash pass. Only the existing iPad preview PID 56763 was live;
  phone shut down, no Wine, no newly arrived parked input identified.
- Investigated the prior inconclusive CUA drag check instead of treating handler tests as
  actual-touch evidence. Added an opt-in `HALOPAD_TRACE_TOUCH` phase/type/point/timestamp trace.
  It records no typed text or player data and is off by default; builder forwards the flag.
- Backed up state/preferences, then replaced the existing preview with diagnostic PID 58636.
  Clean device-data startup verified the migrated 78-file install and reached Halo without
  another import or license decision. Reached Battle Creek through original menus.
- Concrete finding: right-stick drag delivered direct-touch begin/end at the same timestamp,
  point (76.5,75)→(126.5,75), without any moved callback. Open-screen drag delivered begin/end
  1 ms apart, (644.5,419)→(950.5,419), also with no moved callback. This automation gesture
  provides no meaningful hold duration for continuous stick input. Do not add artificial hold
  time to the game to make that test appear to pass.
- The same evidence exposed a real dropped-movement case: screen and FIRE/THROW swipe handlers
  consumed only touchesMoved. Added shared HPLookDrag tracking of the last consumed point per
  touch. Successful end consumes only remaining displacement once; cancellation/clear removes
  state and ignores late callbacks. FIRE uses stable superview coordinates despite pressed
  scaling. Both surface and button pending drags clear on existing interruption paths.
- `G9/overlay-20260928T152752Z`: 44 input/layout assertions, 90 geometry combinations and five
  renders pass. Eight new checks cover final-only swipe, intermediate+final sequence, unchanged
  endpoint, independent interleaved fingers, cancellation, late callbacks after clear, real
  surface/FIRE host output and overlay interruption. Full app builds and signature verifies.
- Stopped diagnostic PID 58636 and installed the fixed build; final PID 59280. Clean startup
  again passed from device data. In the actual Battle Creek game a CUA surface drag changed
  the view from the open map to the wall. Opposite FIRE drag returned to the open view and
  fired (battery 100→99). Trace confirms direct UIKit begin/end delivery. Before/after images,
  source/binary hashes and observations are under `G9/touch-routing-20260928T152054Z`.
- Leave the fixed iPad gameplay preview open, with low-volume touch trace enabled for subsequent
  diagnosis. Phone remains shut down; no separate server or Wine started. Held physical sticks,
  simultaneous multi-touch, general short key/button edges across runtime pumps, phone actual
  routing, package picker/recovery and all external full-goal acceptance rows remain open.
  No completion/blocked state update is warranted. Next input work should address the runtime
  edge-retention row and real multi-touch, not repeat this final-endpoint check without changes.


### 2026-09-28 — retain quick touch-button edges until Halo reads them

- Previous turn: **progress**, `3eefbd7` privately pushed. Read the full objective and G9/M27;
  source locks and executable identity pass. Started clean with only preview PID 59280 live,
  phone shut down, no Wine. The reference/patch bottles remain present. No new parked input.
- Reproduced a quick virtual FIRE down/up disappearing after eight host pumps before Halo's
  unbuffered DirectInput mouse read. The test failed exactly that assertion; saved red result,
  binary hash and output under `G9/touch-mouse-edges`. Stopped only the existing preview before
  the core harness. Backed up Application Support/preferences before reinstalling.
- Tagged overlay mouse buttons, retained alternating edges until successful state reads, and
  kept physical/touch button ownership independent in DirectInput and USER32. Duplicate events,
  invalid reads, Poll and other-device reads do not consume pending edges. Buffered mouse uses
  its ordinary event stream; bounded backlog drops whole pairs to preserve release parity.
- Added explicit cancellation that purges queued virtual buttons before ordinary events and
  clears already-delivered unread edges. Focus loss and reacquisition discard stale input.
  Opening the three-dot menu was missing its clear-input binding: connected UIKit's documented
  MenuActionTriggered event, which fires before presentation. Existing keyboard/Halo menu/editor/
  lifecycle clear paths also cancel pending mouse edges.
- Final DirectInput suite passes 80 assertions (`G3/core-arm64-apple-ios17.0-simulator-20260928T154845Z`),
  USER32 74 (`...154751Z`), overlay 48 assertions plus 90 geometry combinations/five renders
  (`G9/overlay-20260928T155429Z`). The headless overlay runner checks the real menu registration
  and invokes its registered handler: it has no UIApplication to dispatch sendActions; an
  initial attempt to use that dispatcher failed and is retained under `...155316Z`.
- Intermediate installed PID 62372 reached Battle Creek via original menus from device data.
  Actual UIKit FIRE taps of 1 ms and 0 ms produced shots (first battery 100→99; second muzzle
  flash visible). A 0 ms THROW tap reduced grenades 2→1. Evidence and phase traces under
  `G9/touch-mouse-edges`; no handler selftest used. Final build adds the native-menu binding
  and opt-in clear-input trace; current preview PID 63397 (`G3/ios-app-20260928T155503Z`).
- Final actual three-dot opening increased the clear-input trace count 1→2. Dismissal and
  another short FIRE tap worked (battery 100→99). Leave PID 63397 in local gameplay, phone
  shut down; no separate server/Wine. Full build signature and repo safety checks pass.
- Full project scope remains active. This closes the reproduced virtual mouse-button edge
  defect, not general short keyboard edges, simultaneous held multi-touch, physical ergonomics,
  normal licensed startup/reference-client comparison, campaign or the other parked gates.
  Continue with those acceptance rows; do not repeat successful tap checks without a change.


### 2026-09-28 — revoke stick ownership on interruption

- Previous turn: **progress**, `1eb7876` privately pushed. Reread the goal attachment and
  relevant G9/M27 requirements. Started clean; pinned sources and locked executable hash
  pass. Existing preview PID 63397 was live, only project iPad booted, no Wine; reference/
  patch bottles present. No newly supplied parked input. Full scope remains active.
- Inspected the remaining multi-touch/cancellation boundary. Stick callbacks accepted any
  moved/ended touch and reset only axis values. A late move after native UI cleared input
  could therefore restart W or the continuous LOOK timer. An old touch ending could also
  clear a newer hold.
- Added handler-boundary touch tokens (explicitly not synthesized UIKit/OS routing). The
  first run reproduced four failures (`G9/overlay-20260928T160258Z`). Each stick now keeps
  the touch that began its hold; move/end/cancel must match. Reset revokes ownership before
  sending zero input. No artificial holds or changes to movement speed.
- Final `G9/overlay-20260928T160454Z`: 57 input/layout assertions, 90 geometry combinations,
  five native renders pass. Nine new assertions cover independent simultaneous handler holds,
  release of either while the other remains held, late moves after interruption, alien touch
  events, owner cancellation and an old touch ending after a new begin. Physical simultaneous
  finger input is still unverified; do not promote these handler tests to that acceptance.
- Built and verified the app signature, stopped only PID 63397, backed up Support/preferences,
  installed and launched PID 65091 from device paths with the controlled-LAN handler selftest.
  Original menus → Create LAN → Battle Creek → Slayer → Start Game. Five game-state checks
  pass: move 4.26 units, swipe 39.7 degrees, battery 1.00→0.89, jump -0.47→0.18, LOOK 24.7 degrees.
  `G9/stick-ownership` holds binary/source identities, logs, backups and preview screenshot.
- Leave PID 65091 in local gameplay; phone remains shut down and no separate reference service
  was started. Known-good boundary command: `.venv/bin/python scripts/test-ios-overlay.py
  --device E129A00F-D338-4FDC-8AE8-BB243E9BA61B`. Next input experiment: measure short gameplay-key
  taps (JUMP/reload) at the buffered keyboard consumer and actual game behavior. Do not rerun
  instantaneous CUA drags as evidence of held sticks: the earlier trace disproved that premise.
  Actual multitouch/physical ergonomics, package-picker/recovery, licensed baseline, campaign,
  two-player acceptance and other full-goal requirements remain open; no terminal goal update.


### 2026-09-28 — verify short keys in Halo's own buffered consumer

- Previous turn: **progress**, `282556d` privately pushed. Reread the full objective, current
  status and G9/M27 next experiment. Started clean; locked executable and five pinned sources
  pass. Only preview PID 65091 and project iPad were live; phone shut down, no Wine, reference/
  patch bottles present. No new parked input. Full project scope remains unchanged.
- Disassembled original `0x493520`: Halo already handles a press/release in one keyboard update
  by leaving pressed state at 1 and marking a deferred release; next update clears it. Added
  direct calls to the translated consumer after eight host pumps, using the actual scan→key
  table, for all eleven keyboard-backed overlay controls. Fixture keyboard pointer/enabled flag
  are restored afterward. Runtime and guest code are unchanged; no artificial timed hold added.
- Stopped PID 65091 before the core harness and backed up Support/preferences. All 135 DirectInput
  assertions pass, including 55 new mapping/pressed/deferred-release/next-update/no-replay checks.
  Evidence: `G9/short-key-consumer`, including binary/test hash and private consumer disassembly.
- The old app JUMP selftest held 300 ms and allowed a second try, weakening its claim. It now
  emits one immediate down/up in a single main-thread callback and checks height without retry.
  Rebuilt and signature verified; device-data PID 66822 started Battle Creek via original menus.
  All five checks pass: move 3.11 units, swipe 39.7 degrees, battery 1.00→0.89, immediate JUMP
  -0.22→0.44, held LOOK 23.9 degrees (`G3/ios-app-20260928T161826Z`). Leave this preview running.
- Reviewed the related acquisition boundary: runtime `set_acquired` clears immediate key state
  but not its event buffer. Microsoft's Acquiring Devices documentation describes loss/reacquisition
  but does not establish a buffer-flush guarantee. Do not silently redefine global DirectInput
  semantics from that assumption. Next experiment should test unread virtual keyboard events
  during native UI takeover/focus loss and source ownership, using the actual consumer; compare
  reference behavior if changing generic acquisition semantics.
  Reference: https://learn.microsoft.com/en-us/previous-versions/windows/desktop/ee415221(v=vs.85)
- Known-good consumer command (stop the current preview first): `.venv/bin/python scripts/run-core.py
  --work generated/srw/custom-en-1.0.10.0621/run-20260928T060918Z-85892 --target arm64-apple-ios17.0-simulator
  --fresh-state --main tests/halo_dinput_test.c --run-prefix xcrun simctl spawn
  E129A00F-D338-4FDC-8AE8-BB243E9BA61B`. Consumer delivery is verified; gameplay outcomes for each
  remaining action, rapid repeated taps, actual simultaneous touch, physical ergonomics and all
  broader import/lifecycle/online/campaign/license acceptance rows remain open. No terminal state.


### 2026-09-28 — cancel virtual keyboard input without releasing physical holds

- Previous turn: **progress**, `db01fa3` privately pushed. Read the goal attachment and G9
  requirements; clean start, locked executable hash and all five pinned sources pass.
  Only iPad preview PID 66822 was live; phone shut down, no Wine; project bottles present.
  Parked inputs remain unchanged. Full scope and public-release restrictions remain in place.
- Reproduced four keyboard cancellation failures through Halo's own `0x493520` consumer:
  host-queued/already-buffered taps replayed and a read hold remained down in Halo/USER32.
  Red evidence: `G9/touch-key-cancel/red` (original G3 run `20260928T163017Z`).
- Gameplay keys now carry `HPI_TOUCH`; typed input stays separate. Overlay cancellation follows
  all stick/button releases. Host cancellation purges older virtual key/button edges while
  preserving newly posted input. DirectInput tracks key owners and buffered-reader state, removes
  pending touch transitions, and reconciles affected keys against physical holds. USER32 does
  the same for queued messages/async state. Focus loss cancels virtual keys before deactivation.
  No global physical buffer flush or translated guest change was introduced.
- Green iPad suites: DirectInput 156 assertions (`G3/core-arm64-apple-ios17.0-simulator-20260928T163311Z`),
  USER32 85 (`...20260928T163434Z`), overlay 60 plus 90 geometry combinations/five renders
  (`G9/overlay-20260928T163258Z`). Coverage includes both source ownership orders, either-source
  release, unread vs already-read input, focus/reacquisition, unrelated physical taps/text,
  repeated cancellation and fresh touch after cancel. Evidence and identities in `G9/touch-key-cancel`.
- Stopped PID 66822 before core tests; backed up Support/preferences before reinstall. Built,
  verified signature, installed and launched device-data development scene PID 69365 with
  controlled-LAN handler selftest. Original menus start Battle Creek Slayer. All five checks pass:
  move 3.16 units, swipe 39.7 degrees, battery 1.00→0.89, immediate jump -1.36→-0.70, LOOK 25.7 degrees.
  Actual UI native menu open/dismiss and touch Pause/Resume pass; screenshot/log retained.
- Leave PID 69365 in local gameplay on HaloPad iPad Pro 13; phone remains shut down, no reference
  service started. Known-good commands are the same `run-core.py` DirectInput/USER32 invocations
  and `test-ios-overlay.py --device E129A00F-D338-4FDC-8AE8-BB243E9BA61B`. Stop this preview before
  another core harness. Next input experiment: verify remaining action outcomes (reload/use/melee/
  weapon swap) against game state and distinguish unsupported rapid-repeat behavior from original
  behavior before changing it. Actual simultaneous fingers, physical ergonomics and broader
  lifecycle/import/online/campaign/license acceptance remain open. No terminal goal state.


### 2026-09-28 — verify touch action outcomes and correct the melee oracle

- Previous turn: **progress**, `559447d` privately pushed. Read the goal attachment, status and
  parked rows; start clean, locked executable hash and five pinned sources verified. Full
  project scope remains active; no license, campaign, second-player or device gate changed.
- Added opt-in `HALOPAD_ACTION_SELFTEST=1` to the iOS app/runner: immediate MELEE tap, bounded
  MOVE/LOOK approach to an offered magazine weapon, USE hold, SWAP tap, FIRE hold and RELOAD
  tap. It only uses overlay handlers and reads gameplay state. It does not write guest state.
  It takes precedence over the existing movement selftest. Interrupted or exhausted routes
  report failure; missing/equipped weapon and actual USE ownership are checked.
- Corrected the old host-test false-positive melee predicate: the whole word at `+0x2ac`
  includes an animation index that is nonzero at idle. Initial diagnostics wrongly watched
  the AI melee path (`+0x289`, animations 0x1e/0x1f, routine 0x56d120). The reference repo's
  player-melee branch in `bipeds.c`, checked against original PC instructions, identifies
  timer byte `+0x505`: set at 0x55d226, decremented at 0x55d263. Both checks now use it.
  No product input or translated gameplay change was needed. Corrected G4/RUNTIME claims.
- Evidence `G9/touch-actions` retains every diagnostic: initial/final-stderr runs pass
  pickup/SWAP/FIRE/RELOAD but use invalid melee predicates; trace/held runs also exhaust
  their pickup route. `player-stderr.txt` proves the corrected immediate swing 0→25→0,
  but its label had a C-string formatting bug and its pickup route exhausted. Fixed the label.
  Debugger attached only to the wrong AI routine; it hit zero times, then detached/quit.
  That result is not evidence of a player-melee defect. A resulting session timeout was
  dismissed and a fresh local game created.
- Stopped app PID 73064 before the core harness. Full iPad `halo_host_test` passes, 19
  checks/zero failures (`G3/core-arm64-apple-ios17.0-simulator-20260928T171348Z`): corrected
  melee, projectiles, grenade damage/death/respawn, pickup and three audio assertions.
  3,823 frames and 130.5 seconds of sound; the original-client comparison stays open.
- Final signed app installed/started as PID 73862 on project iPad
  E129A00F-D338-4FDC-8AE8-BB243E9BA61B, using device data and controlled LAN mode.
  Original touch menus create Battle Creek Slayer. All five action checks pass together
  (`accepted-stderr.txt`): MELEE 0→25→0, USE holds while taking e29a002b, SWAP changes
  equipped handle, magazine FIRE 60→51, RELOAD 51→60. Binary identity retained privately.
  Actual UI swipe changes view and native three-dot menu opens/dismisses. Earlier blocked
  routes remain evidence that the pickup navigator is not robust for every random spawn.
- Leave PID 73862 in local gameplay; the timer finished and inputs cleared. Preview: open
  Simulator's **HaloPad iPad Pro 13**, app **HaloPad**. Touch sizing/spacing and Reset Layout
  are under **… → Touch Control Settings…**. Phone stays shut down; no reference process
  started. Stop this exact preview before the next game-core test.
- Next meaningful input work: actual simultaneous finger routing and physical ergonomics,
  rapid repeats relative to original consumer behavior, and movement-stick proportionality
  (MOVE currently maps to digital WASD). Broader lifecycle/import/online/campaign/license
  acceptance remains open. No terminal goal state.


### 2026-09-28 — establish the original analog movement path

- Previous goal turn: **progress**, `6807c4f` privately pushed; the five-action sequence and
  corrected melee oracle are authoritative evidence. This turn reread the goal attachment,
  original loop and M27/M28 requirements. Clean start, all five pinned sources and locked
  executable SHA pass. Only iPad preview PID 73862 was live; phone shut down, no Wine;
  project bottles present. Parked inputs unchanged. Goal scope remains intact and active.
- Hypothesis: Halo's native controller path preserves movement magnitude and can coexist
  with keyboard mappings, allowing touch MOVE to stop emulating digital WASD. Falsifier:
  partial axes collapse to full throttle, bindings overwrite W, source release loses the
  other input, or native movement output differs from original x86.
- Read original instructions and the existing controller implementation. `0x493520` polls
  assigned pads, `0x494d90` copies signed axis words into logical pad state at +0x20, and
  `0x48f850` evaluates both keyboard and controller bindings into forward/strafe floats
  `0x6ad4b8/+4`. Original `0x48e360` takes a 12-byte binding descriptor in ECX and an action
  index in EBX; it updates the selected source's mapping rather than replacing keyboard input.
  Private disassembly in `G9/analog-movement`.
- Extended `halo_dinput_test` using test-only neutral mappings and logical slot setup, then
  Halo's own binding setter. Initial run `20260928T172733Z` had two fixture mistakes: W's
  internal index was guessed instead of read from the original DIK table (it is 32), and
  quarter deflection expected truncation instead of nearest rounding (683 counts). Corrected
  both; these were not product defects. An intermediate declaration collided with the CPU
  header's ecx macro; renamed the parameter entry_ecx.
- Final native iPad suite: 175 assertions, zero failures, `G3/core-arm64-apple-ios17.0-simulator-20260928T173438Z`.
  Covers five accepted original bindings, ten throttle samples, W with neutral pad, W plus
  half strafe, release W while strafe continues, and both released. Fixture thresholds are
  explicitly 1; quarter/half/full forward are 0.166748/0.444336/1.0 after the 10% dead zone.
- Added `scripts/test-analog-movement.py --native-evidence <that G3 directory>`. It requires
  a successful native run and ten exact-bit samples, then executes original x86 `0x48f850`
  in Unicorn with equivalent input fixtures and no import handlers. All ten match bit-for-bit
  and return with the correct stack delta (`G9/analog-oracle-20260928T173451Z`). No emulator
  is linked into a target. Native binary/log and source identities are retained.
- Stopped PID 73862 before core tests. After the completed harness, relaunched the existing
  installed app as PID 76313 with only controlled-LAN mode (no automated touch driver).
  Original touch menus create Battle Creek Slayer. No reinstall or input/profile edits were
  needed for this investigation. Preview logs: `G9/analog-movement/preview-*.txt`.
- Next bounded implementation: a distinct cancelable touch analog source and stable device
  identity, with original input configuration that preserves physical controller slots and
  user mappings. Fixture slot/threshold writes must not enter production. Verify release
  before Poll, native-menu/focus cancellation, handoff, profile changes and partial/full
  gameplay motion before switching the default MOVE control. Current overlay remains digital.
  Stop PID 76313 before another core test; no reference process started. All larger campaign,
  online comparison, import/lifecycle and exact-device gates remain open. No terminal state.

## 2026-09-28 — G9 / tighter touch spacing and no look replay after native menus

- **Classification: progress.** Full goal remains active. Reread the objective attachment;
  started clean at e6788cb. Five source pins and locked executable SHA pass. Parked inputs
  unchanged. Latest user asks for continued touch-control/spacing iteration and already
  authorized private GitHub pushes. No reference process started; phone remains shut down.
- Screenshot showed small actions spaced according to the large FIRE diameter. Decoupled
  the two clearances and reduced compact/normal/spread gaps to 8/12/18 points before scaling.
  Kept aligned sticks, HUD clearance, minimum targets and saved v3 custom layouts. FIRE is
  now blue. Inspected native phone/tablet renders and the installed iPad layout.
- Also found that touch look was untagged and survived native-menu cancellation. Hypothesis:
  queued or already-pumped look replays after cancellation while physical input should survive.
  Before-fix evidence `G3/core-arm64-apple-ios17.0-simulator-20260928T174609Z`: four target
  regressions fail. A fifth failure was fixture contamination: the queue-barrier physical W
  tap reached the later analog sample. Fixed fixture isolation by consuming those events.
- Touch motion now has source ownership; host cancellation purges older virtual events,
  DirectInput removes unread virtual deltas/events, and physical/fresh motion survives.
  Final 181 assertions pass in `G3/core-arm64-apple-ios17.0-simulator-20260928T174735Z`.
  Overlay: 61 assertions and 90 layouts pass (`G9/overlay-20260928T174631Z`).
- Stopped preview PID 76313 before core tests. Backed up device Application Support/preferences
  into ignored `G9/touch-spacing-look-cancel/before-install`, rebuilt and installed the scene.
  PID 78478 runs on the project iPad with LAN policy and the one-shot touch self-test, now
  finished: all five gameplay outcomes pass (movement, swipe, FIRE, immediate JUMP, held LOOK).
  Original menus created Battle Creek Slayer. Actual CUA swipe and three-dot open/dismiss
  verified afterward. No held or recurring input driver remains. Preview left running.
- Initial intention was analog-source integration; prioritized the user's visible spacing
  request and the reproduced look-cancellation defect. No analog source/configuration changes
  landed. Next: cancelable analog MOVE plus original configuration without overwriting saved
  physical mappings; physical multi-touch remains unverified. Campaign, online comparison,
  import/lifecycle and device gates remain open. Stop PID 78478 before further core tests.


## 2026-09-28 — G9 / analog source, original multiplayer semantics, stick feedback

- **Classification: progress.** Full goal remains active. Continued from 2a9cb81;
  read the objective and inherited loop. Locked executable and all five source pins
  pass. Private origin verified; earlier push authorization remains in effect.
  Parked inputs and broader acceptance gates unchanged. No reference process started.
- Implemented a stable, distinct virtual touch controller, absolute-axis queue lane,
  cancellation of both pending and polled movement, focus loss/reacquisition, and
  preserved physical controller state. Overlay switches source only after original
  configuration succeeds and clears its old source first. Default source remains off.
- `tests/halo_touch_move_scene.c` activates an unassigned device in a free unbound
  logical slot via Halo's original script evaluator, then its original binding setter.
  Development only: production profile ownership, rollback and reload are not claimed.
- Initial partial-movement tests failed in apps 3/5/6; app4 happened to sample the
  raw stage. Coherent frame snapshots alone did not fix the discrepancy. Captured
  data (`G9/touch-analog-device/partial.bin`) replayed as 0.222330734 in original x86
  AND native (`G3/core-arm64-apple-ios17.0-simulator-20260928T181619Z`), despite 1.0
  being observed at capture. This ruled out the reduced consumer calculation.
- Temporary return/write tracing in apps7/8 found 0.222330734 at the consumer return,
  followed by 1.0 between original locations 0x473d23 and 0x473d2f. The original
  command-builder deliberately applies 0x473c30 for nonzero game-mode word. Twelve
  native/oracle comparisons confirm the +/-0.05 quantizer. Corrected the stage
  expectation; no clamping workaround, movement patch or translation change made.
  Removed temporary hot-path tracing. Captures and logs remain ignored.
- Final native runs: iPad 210 assertions (`G3/...20260928T182711Z`), Mac 206
  (`G3/...20260928T182902Z`). All 22 original comparisons pass in
  `G9/analog-oracle-20260928T182753Z`. Overlay passes 67 assertions, five renders,
  90 layouts (`G9/overlay-20260928T182808Z`). A trial aligning THROW with the top
  action row reduced stick clearance and failed 56 layouts; reverted that geometry.
  Kept symmetric stick/grid spacing and added held-touch feedback and circular hit
  areas so invisible corners no longer consume open-screen aiming gestures.
- app9 PID 85430 is the final installed analog development scene. Original touch
  menus created Battle Creek Slayer with backed-up New001; five gameplay checks and
  seven analog/cancel checks pass (`G9/touch-analog-device/app9-err.txt`). Snapshot
  identities are in app9-identity.json. The earlier backup attempt used a stale data
  container after install; the successful backup was taken before gameplay/config
  and is accurately recorded in backup.json. Final app9-profile-diff.json shows the
  profile hash unchanged; playlist and last-map files changed, none added/missing.
- Left one requested preview running on HaloPad iPad Pro 13, device
  E129A00F-D338-4FDC-8AE8-BB243E9BA61B, PID 85430, LAN policy. The one-shot test
  drivers finished; no repeating input remains. Native menu open/dismiss and corner
  swipe verified afterward. Phone remains off. Stop this exact preview before any
  new core run. This scene is explicitly opt-in, not the production entry point.
- Next bounded experiment: use the same backed-up development scene to switch
  profiles, return to menu and load another map; compare the original slot/binding
  tables before/after, make touch readiness track ownership, and fall back cleanly
  if no slot is free. Fail on changed physical mappings or stale held input. Only
  then promote default analog configuration. Actual simultaneous fingers and exact
  physical-device ergonomics remain unverified. No goal completion/terminal claim.


## 2026-09-28 — centered sticks and original binding cleanup

- Turn classification: progress. Continued G9 touch work and retained the active
  phase-2 goal; no complete/blocked claim. Existing licensed-startup, physical-device,
  campaign and second-player gates remain parked. User authorization permits the
  private GitHub push and Simulator preview.
- New opt-in runtime binding manager revalidates the touch identity, association,
  four installed axes and otherwise empty slot. It cancels queued/delivered input
  before revocation, uses original axis unbind/deactivate commands, rolls back partial
  setup, preserves changed/player/physical mappings and retains failed cleanup ownership.
  Unchanged failures are not retried every frame. Default production scene unchanged.
- Policy suite: 25 assertions pass under ASan/UBSan. Actual iPad app PID 88069:
  original New001 Battle Creek setup, five gameplay checks and seven analog/cancel
  checks passed; returning to menu released device 1 / slot 3 with original calls.
  Created New002 via original Profiles UI and software keyboard Send; Sidewinder
  loaded in the same process, but analog configuration was unavailable. Digital
  fallback was selected. Cause remains open; do not claim successful analog profile
  switching. Returned to New001. Its blam.sav hash is unchanged from pre-install backup.
- Simulator keyboard capture alone did not accept profile entry. Reopening Show
  Keyboard and using the actual software Send key worked. Capture Keyboard restored off.
- Layout iteration: 144-point base thumb pads, 1.10 tablet scaling, vertically centered
  action columns, aligned THROW/FIRE, reserved row overhang and bounded edge reach.
  Retained custom placements. Five UIKit renders generated; phone/tablet examples visually inspected;
  67 overlay assertions and 90 layout configurations pass in
  `G9/overlay-20260928T185646Z`. First trial exceeded max tablet reach by 0.38 points;
  corrected group inset, with the failed trial retained at `...185615Z`.
- Final layout app PID 88982 passed gameplay but one analog sample retained the
  previous quarter-axis value during a half-axis phase; the wall-clock delay had
  no movement/frame progress. Added a frame number to coherent observations and
  require three fresh frames after each posted input (independent of result value).
  Bounded timeout fails and clears input. This changes only the opt-in acceptance
  driver, not movement. That run is retained as `layout-app-*`.
- Rebuilt final app PID 89632: New001 Sidewinder via original touch menus. All five
  gameplay outcomes and seven analog/cancel checks pass (`final-app-err.txt`).
  Original ready device 1 / slot 3 confirmed. Opened/dismissed native three-dot menu
  afterward. One-shot drivers are finished; no repeating test input remains.
- Evidence base `docs/artifacts/2026-09-28/G9/touch-binding-lifecycle/` contains
  before-install and before-layout-install backups, profile comparisons, final source/
  binary hashes (`final-identity.json`), logs and `final-preview.png`. Final New001
  saved profile remains byte-identical to the original backup.
- Preview left running on HaloPad iPad Pro 13, iOS 26.5,
  E129A00F-D338-4FDC-8AE8-BB243E9BA61B, PID 89632, LAN policy, explicit analog scene.
  Stop that exact app before another core run; phone remains off. Normal startup
  acceptance is not claimed. No Wine or reference server started this increment.
- Next bounded experiment: capture enumeration/associations/reserved bindings on
  New002 activation to explain safe fallback; then verify profile saving during
  ownership and same-map restarts. Physical simultaneous touches/ergonomics remain
  unverified; do not enable analog by default until lifecycle acceptance is complete.


## 2026-09-28 — fresh-profile touch slot reuse and persistence evidence

- Previous goal turn: progress (ffea25d and verified layout/input improvements).
  This turn: progress. Re-read objective and loop; source pins and locked client
  hash match, clean starting tree. One project iPad booted, no Wine/reference
  processes. Parked inputs unchanged. No full-goal completion claim.
- Hypothesis: New002 fallback was an existing touch association, not absent device
  enumeration. Opt-in numeric configuration diagnostic proves device 1 is the
  distinct touch source already assigned to logical slot 1; all of its bindings
  are empty. Physical/host device 0 stays in slot 0. Two other slots are free.
  Evidence: `G9/touch-profile-config/diagnostic-err.txt` (PID 91198).
- Reproduced two policy failures before fixing. Manager now borrows an existing
  reciprocal, empty touch assignment and records whether it activated the device.
  Cleanup restores the empty axes and preserves borrowed associations, while
  owned activations still use original deactivation. Bound player slots and
  conflicting physical assignments remain untouched. Thirty-one policy assertions
  pass under ASan/UBSan (`policy-before.txt`, `policy-after.txt`).
- PID 91551, New002 Sidewinder: five gameplay and seven analog/cancel checks pass
  using slot 1. Return to main menu releases it; Battle Creek in the same process
  reuses slot 1. These are original activation/binding paths and actual game data,
  with the UI acceptance drivers invoking real overlay handlers. Not physical
  simultaneous finger acceptance.
- Tested actual original profile saving during touch ownership. Opening settings
  and OK with no changes leaves all 8192 bytes identical. Setting the test
  profile's color from default White to Blue produces a real Saving dialog. Only
  offsets 0x11a–0x11b and the four-byte checksum at 0x1ffc change; checksum verified
  as complement of zlib CRC32. No touch bindings are persisted. Original profile
  reapplication invalidates live bindings; manager releases/reconfigures correctly.
- Relaunch PID 92034 loads the saved New002 and starts Battle Creek through original
  menus: all five gameplay and seven analog/cancel checks pass again. Then returned
  to menu, selected original New001, and loaded Battle Creek in the same process;
  analog is ready using the existing touch assignment in slot 1. New001's saved
  profile is byte-identical to pre-test backup. New002 is a blue test fixture with
  its old file backed up. No user-owned profile data was restored or overwritten.
- Backups and evidence: `docs/artifacts/2026-09-28/G9/touch-profile-config/` includes
  before-install state, source/binary identity, diagnostic/fixed/relaunch logs,
  both color-save snapshots/diff, final profile hashes and live screenshots.
  The fixed and relaunched apps are the same binary. Old candidates were terminated
  explicitly before each next launch; no reference process started.
- Leave one requested preview, HaloPad iPad Pro 13 iOS 26.5,
  E129A00F-D338-4FDC-8AE8-BB243E9BA61B, PID 92034, New001 Battle Creek, LAN policy.
  One-shot acceptance drivers finished; no repeating input. Stop this app before
  another core run. Phone remains off. This is still the explicit development scene.
- Next: test editing an original controller binding while the borrowed touch slot
  is active, preserving the player's edit and avoiding any save contamination.
  Then controlled natural server restarts/reconnects with touch input. Physical
  hot-plug and simultaneous fingers remain open; do not promote default analog or
  close G9 on the strength of the color-save path alone.


## 2026-09-28 — touch editor alignment and controller settings save

- Continuing active goal, starting clean at 162da47. Objective and current evidence
  read; user requested continued spacing/twin-stick iteration and private pushes.
  Existing runtime/source identity checks and state backup precede this step.
- Investigated the original controller editor on New002. It lists the distinct
  touch source but its movement bindings are empty in the saved profile. Original
  setter disassembly writes only the selected table entry; no evidence of the
  suspected cross-controller action clearing. No runtime binding change made.
- Saved touch-device horizontal sensitivity 3→4 through original Controls Setup /
  Advanced / OK / profile OK. Only offset 0x957 and CRC change; New001 unchanged.
  Manager releases/reacquires slot 1 during profile application. Evidence
  `G9/touch-binding-edit/controller-settings-diff.json` and settings log. This
  is settings-save acceptance, not an action-binding edit. Backup in `before/`.
- Layout improvement: snap released controls to centres within eight points,
  rejecting snaps that crowd controls or cross safe bounds. Drag itself remains
  continuous. Minimum 44-point target applies to individual saved scales too.
  Reset editor hint when reopening. No default or user-position reset.
- `.venv/bin/python scripts/test-ios-overlay.py`: 71 assertions plus five rendered
  previews, 90 layouts, zero failures (`G9/overlay-20260928T193648Z`). New tests
  cover pair alignment, intentional offset, obstructed snap and minimum size.
- Stopped PID 92034 before rebuilding/installing the explicit touch-move scene.
  PID 94466 loads saved New002 and Battle Creek through original menus; five
  gameplay and seven analog/cancel checks pass. Original profile hashes remain
  unchanged by reinstall/relaunch. Exact source/app identity in `editor-identity.json`.
- Live editor opens with revised hint, selection and Done work. CUA drags select
  without moving; AX slider sets its value but does not persist a scale. Do not
  count those as real drag/resize acceptance. No tablet layout preferences were
  created. Game swipes after closing editor turn the scene.
- One requested playable iPad preview remains: E129A00F-D338-4FDC-8AE8-BB243E9BA61B,
  PID 94466, New002 Battle Creek, LAN policy, explicit development scene. Drivers
  are one-shot and finished. Phone stays off; no reference process started.
- Next bounded experiment: controlled server map transition/disconnect/reconnect
  while the touch manager owns its slot; log release/reacquisition and verify
  neutral input followed by movement after each transition. No full-goal closure;
  original controller-action edit and physical simultaneous fingers remain open.


## 2026-09-28 — iPad touch input through server map change and reconnect

- Previous goal turn: progress (0961fe4, editor improvement with verification).
  Current turn: progress. Read the objective, current journal/status, verified all
  five source pins and locked CE executable hash. Clean starting tree; one iPad
  preview, no Wine processes. Parked inputs unchanged. Full goal remains active.
- Hypothesis: the touch manager and UIKit cancel a held MOVE on a natural server
  map change, then acquire the profile slot again without replaying stale input.
  A subsequent original-console disconnect/reconnect must start neutral and accept
  new movement. This addresses G8/G9 integration without claiming G5's second-client
  requirement.
- Added an explicit bounded iPad driver selected by
  HALOPAD_TOUCH_TRANSITION_SERVER=127.0.0.1:2310. It uses overlay handlers and typed
  original console commands only. Map/unit/input observations are captured together
  on Halo's Present thread. Eight assertions, 210-second deadline, no repeated
  input after completion. Other self-test modes are mutually exclusive with it.
- Backed up the app Library before writes; stopped PID 94466, rebuilt the verified
  work directory with tests/halo_touch_move_scene.c, installed, launched PID 96070
  with LAN networking, -connect 127.0.0.1:2310 -cport 2305 -console.
- Original dedicated server: CrossOver 26.3, project bottle halopad-reference,
  loopback/private, tests/server/mapcycle.txt (one-minute Slayer). Removed the
  runner's global executable-name pkill; cleanup uses only its own bottle.
- All eight transition checks pass. Initial Blood Gulch slot 1 is neutral; .5 MOVE
  yields axis -1820 and original forward 1. Natural Battle Creek transition
  releases/reacquires slot 1 and resets axis/forward to zero. New MOVE works;
  disconnect reaches ui with slot -1; reconnect respawns on Battle Creek neutral,
  new MOVE works, final release is neutral. Original server records the natural
  change and separate quit/rejoin, independently of app assertions.
- Evidence: docs/artifacts/2026-09-28/G9/touch-network-transition contains backup,
  app logs, exact source/binary identity, redacted current server session, server
  configuration/version, status query and screenshot. New001 and New002 remain
  byte-identical to pre-test backup. No runtime mapping fix was required.
- Stopped exact server runner 95973; its child and all recorded Wine/server PIDs
  are gone. Stopped candidate 96070 before relaunching the same binary locally.
  Final preview PID 96288, New002 Battle Creek, LAN, explicit analog development
  scene: all five gameplay and seven analog/cancel checks pass again. One-shot
  drivers finished. One project iPad remains, phone off, no reference server.
- Shell syntax and diff checks pass. Default layout was unchanged, so its prior
  71-assertion/90-layout verification was not repeated. No physical-finger or
  second-original-client acceptance inferred from handler-driven observations.
- Next bounded experiment: abrupt private-server loss while MOVE is held. Require
  original timeout/disconnect behavior, neutral input on return, then restart that
  server and reconnect with fresh movement. Preserve the active preview before
  replacing it; do not treat normal map-cycle success as timeout acceptance.


## 2026-09-28 — held MOVE survives abrupt server-loss recovery

- Previous goal turn: progress (01b5ffa, original-server map/reconnect acceptance).
  Current turn: progress. Objective read in full; current tree clean at 01b5ffa,
  five pins and locked executable hash verified. One iPad PID 96288, no Wine
  processes; parked inputs unchanged. Full goal remains active.
- Added an opt-in loss branch to the existing bounded transition driver. It keeps
  MOVE held while the external test stops the private server. A fresh-file gate
  allows reconnect typing only after an external status query confirms restart;
  that gate does not assert or alter any guest outcome. Original spawn, neutral
  input, movement and release remain required. Normal map-change mode unchanged.
- Backed up Library, stopped PID 96288 before rebuild/install. Used the verified
  cached translation and explicit tests/halo_touch_move_scene.c. Candidate PID
  97675 joined original dedicated server on loopback 2310, client 2305, LAN policy.
- Initial neutral and MOVE checks pass (slot 1, Y=-1820, forward=1). Recorded a
  successful server status query, then terminated exact runner 97583. Its server
  97637 and recorded Wine processes exited; a new UDP query timed out. This is
  abrupt process loss, without an in-game graceful disconnect command.
- Halo naturally displayed “Network connection lost”, returned to ui and released
  the touch slot. Screenshot and log corroborate it. Restarted original server
  through a new runner, required a fresh status answer, then created restart-ready.
  Driver dismissed the dialog with Escape and typed Halo's connect command.
- Rejoined Blood Gulch neutral, new MOVE reaches the original consumer, final
  release returns neutral. All seven assertions pass. Original server logs
  independently record separate initial/restarted sessions and successful joins.
  No runtime fix required. Both saved profiles remain byte-identical to backup.
- Evidence in G9/touch-server-loss: timestamped stop/outage/restart/query events,
  original connection-lost and rejoined screenshots, redacted server-session logs,
  settings/version, exact source/app hashes, before-state and profile hashes.
- Stopped restarted runner 97782 and verified its recorded server/Wine PIDs gone.
  Driver already invalidated its timer after PASS; no repeating input remains.
  Dismissed the subsequent original lost-connection dialog and started a local
  Sidewinder match through original menus in the same app process. One iPad preview
  remains, PID 97675, New002, LAN, explicit analog development scene; phone off.
- This is handler-driven network/input evidence, not physical two-thumb or
  second-original-client acceptance. No graphics/layout/runtime changes, so the
  prior layout and general gameplay suites were not needlessly repeated.
- Next bounded experiment: bring the current analog scene and transition driver
  to the project iPhone Simulator. Back up its independent data, use one project
  Simulator at a time, verify fresh-profile slot ownership, natural map change and
  reconnect there, then restore the requested iPad preview. Physical ergonomics
  and original controller-action edits remain separate open paths.

## 2026-09-28 — iPhone touch transitions and accessible control settings

- Previous goal turn: progress (01b2e4f). Current turn: progress. Full objective
  read; tree clean at start, pins/executable verified, parked inputs unchanged.
  Kept one candidate and one project Simulator throughout; backed up both Libraries.
- Fresh project iPhone 17 Pro (iOS 26.5) had no game folder or saved profiles.
  Prepared the exact current core's private archive, placed it in app Documents,
  and selected it through Choose Prepared Package → Files Browse → HaloPad.
  All 78 installed stock records match the signed inventory. This closes this
  local-provider first-import route; remote providers and replacement stay open.
- Existing binary PID 99748 passes eight original-server transition checks:
  Battle Creek → Blood Gulch with MOVE held, neutral respawn, disconnect releases
  slot, reconnect starts neutral, new movement and release. Fresh touch slot 3;
  Halo used temporary names without a saved profile. Redacted original server
  logs corroborate map change and rejoins. No input/runtime fix required.
- Phone UI revealed Touch Control Settings below networking and four text-input
  entries. Moved control settings first and grouped Keyboard & Chat into a submenu.
  Actual rebuilt menu → settings → Done verified. Geometry unchanged; prior
  71-assertion/90-layout coverage remains applicable. Simulator scrolling/drags
  did not establish physical scrolling or saved layout edits; no such claim made.
- Stopped exact phone/server before rebuilding. Final phone PID 688 passes all
  five gameplay and seven analog/cancel checks against a fresh private original
  server. Same imported device data validates after reinstall/relaunch.
- Stopped exact app 688 and server runner 591 (earlier runner 99681 also stopped),
  verified no reference Wine/server processes remain, shut down phone. Installed
  same final binary on project iPad, retained both independent profiles byte-for-byte.
  Original menus start New002 Battle Creek Slayer; PID 1492 passes five gameplay
  and seven analog/cancel checks. One-shot drivers finished, preview left playable.
  Fresh UDP status confirms beavercreek, one player, openplaying.
- Evidence: G9/iphone-touch-network, including import and binary/source identities,
  both app runs, redacted server sessions, settings/menu/preview screenshots,
  before-state backups and unchanged iPad profile hashes. Normal entry and physical
  two-thumb acceptance remain open. Updated an old status row to acknowledge the
  earlier supplied string; removed that literal string from two old journal entries.
- Next bounded experiment: original controller-action binding edits with held MOVE,
  verifying ownership revocation and save/reload without overwriting player bindings.
  Preserve current preview and profile backup before replacing it. Full goal active.

## 2026-09-28 — retain device assignments after player binding edits

- Previous goal turn: progress (dc32a17). Current turn: progress. Full objective
  read; clean starting tree, five source pins and locked executable hash verified.
  Only project iPad preview PID 1492 active; no reference Wine/server processes.
  Both project bottles present; parked external inputs unchanged. Goal stays active.
- Ownership review reproduced five failures: edited axes were retained, but cleanup
  deactivated a touch device HaloPad had originally activated. This disabled the
  player binding and could move the device to another slot on the next frame.
- Cleanup now deactivates only if the assignment is empty after removing installed
  axes. Player axis/button/hat/menu bindings keep the reciprocal active assignment.
  The manager drops ownership and leaves that bound assignment unavailable for its
  own movement mapping. All 35 policy assertions pass with ASan/UBSan.
- Added tests/halo_touch_edit_scene.c as a separate development-only experiment.
  It uses original script activation/deactivation and binding setter calls, plus
  the host touch source and original polling; no guest input-table/player writes.
  It releases an empty assignment to exercise activated-here ownership, holds MOVE,
  binds X+ to forward through the original setter, verifies revocation, preserved
  assignment/player binding, removed temporary axes and neutral input, removes the
  test binding, verifies neutral recovery and restores the original association.
- Backed up Library and stopped preview 1492 before rebuilding. First candidate
  3052 aborted on the held-axis assertion: its hold preceded UIKit's asynchronous
  readiness cancellation. Preserved the failed log; confirmed process exited and
  saves unchanged. Driver now waits ten frames before starting that hold.
  Final experiment PID 3285 passes all 16 assertions in a private original-server
  Blood Gulch match. Original server join corroborated; profiles byte-identical.
- Stopped exact candidate 3285 and reference runner 2928; no Wine/server processes
  remain. Rebuilt the usual analog scene, excluding the edit experiment. Final
  iPad preview PID 3574, New002 Battle Creek Slayer, started via original menus.
  Five gameplay and seven analog/cancel assertions pass. Both profile hashes still
  match backup; UDP status confirms beavercreek, one player, openplaying. One iPad
  remains active, phone off, one-shot drivers finished.
- Evidence: G9/touch-player-binding (red/green policy output, failed and accepted
  integration logs, exact source/binary hashes, saved-state backup, redacted server
  session and final preview identity/screenshot). No visual geometry change, so
  previous layout verification was not repeated. Normal startup, actual menu
  binding edits/save/reload and physical-controller/two-thumb acceptance stay open.
- Next bounded experiment: perform an original controller-action edit through
  Halo's menu on a backed-up test profile, then save/reload and verify only the
  intended setting persists. Restore a usable preview after that test.

## 2026-09-28 — original keyboard binding save/reload with analog touch

- Previous goal turn: progress (42357fd); current turn: progress. Objective read
  in full, clean tree, five pins/executable hash verified. One iPad PID 3574,
  no reference processes, both project bottles present, parked inputs unchanged.
- Backed up the independent iPad Library before edits. Original Pause → Change
  Settings → Controls Setup shows empty saved HaloPad Touch Move axes. Its capture
  prompt requires a controller button/axis while gameplay controls are hidden.
  Canceled cleanly. Actual controller-action capture remains open; no synthetic UI
  injection or claim that the following keyboard test covers that path.
- Changed New002 forward W → I in original Controls Setup. Simulator hardware
  keyboard capture did not assign I; disabled it and used ⋯ → Keyboard & Chat →
  Show Keyboard, then the software i key. Original capture accepts it. Saved via
  Controls OK and Profile OK. Only W/I entries (0x174/0x180, 16-bit) and final CRC
  change; no temporary analog mapping saved. New001 untouched. CRC validates.
- Stopped exact PID 3574 and relaunched the same verified cached binary as 5315,
  LAN policy, explicit analog scene. Original menus start New002 Battle Creek.
  Five gameplay and seven analog/cancel checks pass with W unbound. Reopening
  Controls Setup visibly shows I after relaunch, corroborating the saved-file diff.
- Restored W through the same original menu and software keyboard, saved again.
  Both profile files match pre-test backup byte-for-byte, including checksum.
  Resumed gameplay in the same PID 5315; completed one-shot drivers do not repeat.
  One iPad remains, phone off, no reference processes. Fresh UDP status confirms
  beavercreek, one player, openplaying. No rebuild or runtime change this turn.
- Evidence: G9/touch-menu-save (before-Library, capture limitation screenshot,
  edited-save copies and exact differences, reloaded-I screenshot, app results,
  restored-profile hashes, binary identity and live preview screenshot).
- Source inspection identifies the next substantive gap: touch action buttons
  still emit fixed default keyboard/mouse bindings; remapping a game action can
  therefore break its touch button even though analog MOVE stays independent.
  Next bounded experiment: reproduce a JUMP remap via original Controls Setup,
  then implement binding-aware touch dispatch with correct release ownership and
  no player-profile mutation. Keep actual controller capture/physical fingers open.
  Full goal remains active; this is a completed keyboard persistence check only.

## 2026-09-28 — touch actions respect original keyboard/mouse remaps

- Previous goal turn: progress (d2f36b6); current turn: progress. Full objective
  reread; clean tree, pinned sources and locked executable verified. One iPad
  PID 5315, phone off, two project bottles, no Wine; parked inputs unchanged.
- Identified original action-name table 0x5f9d10 (29 entries) via 0x492c20;
  original setter 0x48e360 confirms keyboard 0x6ab330 and mouse 0x6ab40a.
  Added HPI_ACTION host events and read-only runtime resolution before USER32/
  DirectInput. Buttons and digital MOVE use actions; Pause/Back stays Escape.
  Holds retain their resolved input across remapping. Shared touch controls
  release only after the last owner; cancellation preserves physical input.
- DirectInput red run 20260928T210725Z fails JUMP remap and next-binding checks.
  Its mouse remap check initially had an invalid fixture (queue disabled, mouse
  still buffered from earlier cases); fixed setup rather than input semantics.
  Final 20260928T211329Z passes 226 assertions, including original keyboard
  polling, remapped mouse tap retention, collision ownership, focus loss/regain.
  UIKit overlay 20260928T210859Z passes 71 assertions and 90 layout combinations.
- Backed up saves. Launched old cached app PID 7745, used original Settings →
  Controls Setup → Actions and software keyboard to remap JUMP Space → J.
  Only J/Space entries and CRC change in New002; New001 unchanged. Relaunched
  old binary PID 7855: Battle Creek gameplay JUMP fails, height -1.36 unchanged;
  other four checks pass. This is the actual pre-fix gameplay reproduction.
- Stopped old app, rebuilt regular analog development scene, installed and
  launched PID 8274. Same saved J profile, original menus, Battle Creek: all five
  gameplay and seven analog/cancel checks pass; JUMP rises -1.36 → -0.70.
  Reopened Controls Setup, visibly verified J, restored Space and saved through
  the original UI. Both blam.sav files match backup byte-for-byte. Resumed 8274.
- Evidence G9/touch-action-bindings: red/intermediate/final logs, source/binary
  hashes, pre/remapped saves and byte diffs, original-menu screenshot, live
  preview and UDP status. Current preview: New002 Battle Creek, LAN policy,
  iPad E129A00F-D338-4FDC-8AE8-BB243E9BA61B, one player/openplaying. One-shot
  drivers finished; no server/Wine or second Simulator retained.
- Limitation: wheel-only/controller-only or wholly unbound actions deliberately
  produce no replacement key. Next bounded work: remapped digital MOVE gameplay
  acceptance, then unavailable-action feedback/support for those binding types.
  Physical controller capture, fingers/ergonomics and full phase-2 goals remain
  open. This is progress on touch compatibility, not completion of the goal.

## 2026-09-28 — remapped digital fallback in real gameplay

- Previous goal turn: progress (74acddf, runtime fix and gameplay evidence).
  Current turn: progress. Read objective in full; clean tree, five source pins,
  locked executable hash, one iPad PID 8274, two project bottles, no Wine.
  `devicectl list devices` reports no physical devices. Parked inputs unchanged.
- Backed up profiles, changed New002 Move Forward W → I using original Controls
  Setup and software keyboard, then saved. Only two keyboard entries and CRC
  change; New001 untouched. Stopped exact PID before each test/build switch.
- Added five permanent assertions to the DirectInput fixture using Halo's
  original setter: W becomes backward, I becomes forward; semantic touch MOVE
  reaches the original consumer as forward with I pressed and W neutral, and
  release/menu cancellation return to neutral. Full suite passes 231 assertions,
  G3/core-arm64-apple-ios17.0-simulator-20260928T212237Z. No runtime fix needed.
- Built base `tests/halo_app_scene.c` rather than the analog scene. It never
  enables the virtual device, and its linked readiness/slot hooks are the weak
  false/-1 defaults (nm evidence retained). PID 10441, same saved I profile,
  original-menu Battle Creek: five gameplay checks pass, movement 3.07 units,
  JUMP -0.22 → 0.44. This closes remapped digital MOVE gameplay acceptance;
  prior analog tests alone could not establish it.
- Reopened Controls Setup after relaunch and captured I. Restored W using the
  original menu, saved, verified both profiles exactly match their backups.
  Stopped 10441; rebuilt analog scene. Executable SHA-256 exactly equals the
  previously accepted binary (G9/touch-action-bindings/final.json), so no repeat
  of unchanged five-plus-seven assertions. Installed/relaunched PID 10825,
  New002 Battle Creek, LAN policy, ready device 1/slot 1. UDP confirms one
  player/openplaying. No test drivers enabled in the final preview.
- Evidence G9/touch-digital-remap: saves/diffs, build logs, weak-symbol and binary
  identities, gameplay output, reloaded-I screenshot, restored profiles, preview
  image and final metadata. Only test/docs changes this turn; preview runtime
  remains 74acddf. One Simulator/candidate, no reference process retained.
- Next substantive touch gap: controller-only/wheel-only bindings currently
  yield no input and have no unavailable-action feedback. Implement an honest
  usable path while retaining physical input and cancellation ownership. Keep
  physical fingers/controller capture and full phase-2 acceptance open.

## 2026-09-28 — wheel-bound touch actions and cancellation

- Previous goal turn: progress (d6f3f43, digital remap acceptance). Current turn:
  progress. Read full objective; five source pins and locked executable verified,
  clean tree, one iPad PID 10825, phone off, two project bottles, no Wine. No
  physical devices; parked inputs unchanged. Stopped exact app before builds.
- Extended semantic touch resolution to original wheel Z+/Z− after keyboard and
  mouse buttons. One notch on press, none on release or held-down repeat.
  Original converter 0x494980 reverses DirectInput wheel sign; adapter follows
  it. Original binding setter establishes the table, which runtime only reads.
- Added independently cancelable wheel ownership to DirectInput state/buffer and
  USER32 messages. Cancellation retains physical scrolling; successful reads
  clear consumed touch contribution so later cancellation cannot subtract it.
- DirectInput red 20260928T213315Z has seven failures: three cancellation and two
  missing-resolver failures, plus two invalid physical baseline fixture checks.
  The fixture lacked original wheel granularity and assumed the wrong sign;
  corrected from GetProperty and original disassembly, not counted as runtime
  fixes. USER32 red 20260928T213346Z has one missing-cancellation defect plus
  two later queue checks contaminated by that leftover message. Evidence kept.
- Green DirectInput 20260928T213729Z: 248 assertions, including both wheel
  directions through original poll/consumer, no repeat and physical ownership.
  Green USER32 20260928T213841Z: 86 assertions. No geometry changes; existing
  71-assertion/90-layout UIKit suite was not repeated for unrelated runtime work.
- New test-only halo_touch_wheel_scene.c uses original setters to unbind Space
  and assign wheel Z+ to JUMP in memory, with eight setup guards and no save.
  PID 12863, original-menu New002 Battle Creek: all five gameplay and seven
  analog/cancel checks pass; immediate JUMP rises -0.22 → 0.44, MOVE 3.11 units,
  FIRE battery 1.00 → .89. This does not establish original-menu wheel capture.
- Both saved profiles remain byte-exact to backup. Stopped fixture PID 12863,
  rebuilt/reinstalled regular halo_touch_move_scene.c with the new runtime.
  Final preview PID 13346: New002 Battle Creek, one player/openplaying, LAN
  policy, touch device 1/slot 1. No test driver enabled in the final preview.
  Binary SHA-256 265b9458747a0a75aa1d0956558886103c79a211c52b4d620a46a8853497b7e8.
  CUA visibly verifies both sticks and action grid. One candidate/Simulator;
  no Wine/reference process retained. Evidence G9/touch-wheel includes logs,
  pre/post save hashes, source/binary identity, UDP status and screenshots.
- Remaining touch compatibility work: controller-only/unbound action feedback
  and usable mapping path. Physical fingers/controller capture and ergonomics,
  normal startup, original-client comparison and other phase-2 gates stay open.

## 2026-09-28 — visible recovery for unavailable touch bindings

- Previous goal turn: progress (26b5f11, wheel action/cancellation support).
  Current turn: progress. Full objective read; clean tree, five source pins,
  locked executable hash verified. One iPad PID 13346, phone off, two project
  bottles, no Wine; devicectl reports no devices. Parked inputs unchanged.
- Added read-only availability mask using the runtime action resolver, sampled
  only on Halo's thread and dispatched as a value to UIKit when changed.
  No guest binding/state writes. Buttons show a small warning badge and accessible
  explanation for missing keyboard/mouse alternatives. Actual tap shows a
  five-second hint: Pause → Change Settings → Controls Setup. Digital MOVE names
  its unavailable direction; independent analog MOVE requires no keyboard bind.
- Availability is presentation only. Both edges still reach runtime ownership,
  including release after a remap during a hold. Restoring a binding removes
  stale feedback; menu/focus cancellation clears the hint. Hint passes touches
  through and fits phone/tablet safe areas; established two-stick spacing stays.
- DirectInput G3/core-arm64-apple-ios17.0-simulator-20260928T215205Z passes 254
  assertions (six new mask assertions: keyboard, mouse, two wheel directions,
  unbound JUMP and controller-only movement). UIKit final overlay-20260928T215658Z
  passes 94 checks, 90 layouts and ten renders; inspected phone/tablet warning
  appearance. Initial compile caught an overly broad edit plus ARC weak-pointer
  dereference; corrected before successful tests/builds. No runtime red defect
  claimed from that compile failure.
- Initial ignored original-setter fixture PID 14865 unbound Space in memory,
  proving live badge/hint display. The original menu reads its own saved profile,
  so this alone could not prove player recovery. Captured original binding
  instructions (Backspace = clear); used software keyboard in original Controls
  Setup → Actions → JUMP to clear Space and save New002. Only bytes 452/453 and
  final CRC differ; New001 unchanged. Temporary fixture source retained only in
  ignored evidence, then discarded in favor of regular analog build.
- Relaunched regular scene PID 15240 with saved unbound profile: badge and actual
  tap hint appear; original menu visibly retains ??? for both JUMP slots.
  Assigned Space using software keyboard, saved, resumed: badge/accessibility
  warning disappears in same process and a new tap produces no stale hint.
  Both profile files return byte-for-byte to backups through the original UI.
- Relaunched same regular binary PID 15463 with one-shot gameplay drivers.
  Original-menu New002 Battle Creek: all five gameplay and seven analog/cancel
  checks pass, immediate JUMP -0.22 → 0.44. Drivers have finished. Preview remains
  playable, touch device 1/slot 1, LAN policy, UDP one player/openplaying. Binary
  SHA-256 482e85bedd6feb3c79377a61d2b571b0c238d28490bb7c71b929686f0b74f914.
  Evidence G9/touch-binding-feedback includes logs, save diffs, original-menu and
  live screenshots, source/binary hashes and final container/process metadata.
- Recovery feedback is closed for keyboard/mouse-unavailable actions. Direct
  touch synthesis for controller-only bindings is still unimplemented; a player
  can use the verified original-menu route to add an alternative. Physical
  controller capture, simultaneous fingers and ergonomics remain open. Full
  phase-2 objective remains active; normal startup/original baseline and physical
  architecture acceptance are still parked, not satisfied by this touch work.

## 2026-09-28 — lock/unlock with held sticks (iPad and iPhone)

- Previous goal turn: progress (097b240, unavailable-binding recovery).
  Current turn: progress. Full objective and original loop reread; clean tree,
  five source pins and locked executable hash verified; two project bottles,
  no Wine, no physical devices. Parked inputs unchanged.
- Hypothesis: STATUS listed lock/unlock as unverified. A held MOVE/LOOK across a
  real lock should resume neutral with no replay. Fail condition: nonzero axis or
  forward/strafe on resume, continued turning, or fresh input not working.
- Added HALOPAD_TOUCH_LIFECYCLE_SELFTEST (opt-in, HaloPadApp.m). Uses the existing
  Present-thread observation plus yaw; records real sceneWillResignActive/
  sceneDidBecomeActive timing on the main thread; posts no lifecycle events.
- iPad PID 16777: armed with MOVE -1820/forward 1 and LOOK 14.7°; Device → Lock
  for 60.2 s (0 fps while locked), Home, swipe/Home to unlock. All six checks
  pass: resume neutral, no replay after 2 s, fresh MOVE/LOOK, clean release,
  still after release. Profiles byte-identical.
- iPhone: first launch idled on the menu for over an hour (session gap), so the
  240 s driver timed out in phase 0 (log kept as iphone-timeout-err.txt; not a
  runtime failure). Phone had no profile; Halo created New001 via original menus
  and software keyboard Enter. Relaunched PID 22072, backed up New001, same local
  Battle Creek run, 35.8 s lock: all six checks pass; profile unchanged.
- Restored iPad (it booted in portrait; rotated to landscape through Simulator).
  Regular preview PID 22493 without test drivers: New002 Battle Creek, LAN
  policy, one player/openplaying, binary e853a7bc…adb906 (includes the opt-in
  driver; inert without its environment variable). Phone shut down.
- Open: audio interruption, online suspension/reconnect across lock, console
  typing across interruption, physical devices and the parked phase-2 gates.

## 2026-09-28 — online lock with held sticks (private reference server)

- Previous goal turn: progress (620b920, local lock/unlock on iPad and iPhone).
  Current turn: progress.
- Hypothesis: a lock during an online session either keeps the session and resumes
  neutral, or sends Halo to its own menu with the touch slot released and permits
  a clean reconnect. Fail conditions: stale movement or turning after resume,
  a held slot in the menu, or input not working after reconnect.
- Extended the opt-in lifecycle driver with HALOPAD_TOUCH_LIFECYCLE_SERVER
  (loopback only), adding the disconnect-and-reconnect branch and a 420 s budget.
- Server: scripts/reference-server.sh --port 2310 (haloceded 1.10, CrossOver 26.3).
  Run 1 aborted: a stray accessibility-index click hit the LOOK stick before
  locking (log kept, not counted). Relaunching right after force-quit: Halo showed
  "Unable to join the game server!"; the server logged QUIT 18:42:16 and no join
  failure record. Restarted server (two variables changed; cause unconfirmed).
- Run 3, PID 24035: JOIN 18:44:12; held MOVE -1820/forward 1 and LOOK 14.7°;
  locked 78.2 s; polls showed 1 player throughout and there was no QUIT; the session
  survived on bloodgulch; all six checks PASS; server still listed New002 afterwards.
  Reconnect branch did not run (Simulator did not drop the session).
- Stopped the server runner; no Wine processes remain. Preview PID 24302 without
  drivers: New002 Battle Creek, LAN policy, 1 player. Binary 801ab0fe…613b28.
  Profiles byte-identical to backups.
- Next: exercise the reconnect branch by forcing a real drop (for example the
  existing server-loss path while locked), check audio interruption, then continue
  down the open G9/M30 rows. Physical-device suspension stays parked on hardware.

## 2026-09-28 — lost connection during a lock, then reconnect

- Previous goal turn: progress (367e678, online lock kept the session).
  Current turn: progress.
- Forced the untested branch: joined 127.0.0.1:2310 (JOIN 18:48:33), armed held
  MOVE/LOOK, locked, stopped the server 18:48:54, restarted it 18:49:29 (0 players),
  unlocked after 51.7 s. Five checks PASS: menu instead of stale play, slot -1,
  neutral respawn after console reconnect (JOIN 18:49:41), MOVE/LOOK work, clean
  release. PID 25028; same binary as 367e678 (801ab0fe…613b28).
- Server stopped; no Wine. Preview PID 25297, Battle Creek, no drivers. Profiles
  byte-identical.
- Remaining M30 rows: audio interruption, console typing across interruption,
  physical suspension. Next: audio interruption on the Simulator if it can be
  triggered faithfully, otherwise document why it needs hardware.

## 2026-09-28 — real public match from the iPad app

- Chris redirected: stop lifecycle micro-tests; the build must play and join
  servers with real people. Listed 244 master-server entries; picked AUSSIES
  MADNESS 2 (stock sidewinder CTF, 7 humans, not full, no password).
- Run 1 (PID 25928): joined, spawned Blue, teammate Pirate visible, "Welcome
  New002". Self-test reported move/look/jump FAIL, yet screenshots showed the view
  moved and the rifle fired (60→41, bullet hole). Cause: g_unit read players[0],
  another player on a populated server. Fixed to select the entry whose local
  index (+2) is not -1 (layout from tests/halo_match_test.c).
- Run 2 (PID 26595, rebuilt): joined, all five checks PASS: moved 4.42, turned
  39.7°, rounds 60→44, jump -3.84→-3.19, LOOK 6.8°. Server roster: New002 ping 48
  Red alongside YOUR SISTER, Darling, Sneak, Pirate, CPU-MD, ToMmy.
- Left the iPad in the match for Chris to take over.

## 2026-09-28 — join from Halo's own Internet Lobby

- Previous goal turn: progress (d15a543, public match with touch checks).
- Launched the regular app with HALOPAD_NET=internet only. Taps: Multiplayer →
  Join Game Internet → Get List (169 servers, 92 players, pings 24–38 shown on
  page 1) → AUSSIES MADNESS 3 row → Join Game. Spawned Blood Gulch CTF, Red,
  sniper. Server roster: New002 ping 24 plus four named players; score 12–12.
- A real FIRE-button tap produced a sniper tracer; the HUD ammo stayed at 008 ×4
  (probably server-side infinite ammo, not proven). Left the iPad in the match.

## 2026-09-29 — three-dot menu, controllers, full screen, device build, README

- Chris: stop public-server testing; polish menu, controllers, install, README; fix the
  SunPad menu-button bug; windowed Simulator. Supplied a private key (valid 25-character
  Microsoft key alphabet, unlike the earlier strings).
- Menu button bug reproduced: after tapping away, the button became an empty square
  outline without dots for several seconds (both before and after a first fix attempt
  with custom targeted previews). Cause: a custom layer-styled UIButton gives iOS 26's
  menu morph no shape to return to. Fix: UIButtonConfiguration (capsule, symbol, colors)
  plus no focus ring and a round pointer shape. Verified on screen: dots persist through
  repeated open/tap-away cycles, menu and in-game.
- Menu reorganized into three groups: play (join by address, recent servers, Leave Game
  in a game), setup (Controls, Keyboard & Chat, Display, Add Custom Maps), Help (Report a
  Problem, HaloPad on GitHub, About). Report a Problem opens a prefilled GitHub issue
  (title, label, device/OS/build/map/frames/controllers) or shares text; verified the
  exact URL handed to Safari in the Simulator log. HaloPad sheets now own the keyboard.
- Controllers: Halo 1.10 has device defaults in ui.map only for 2003-era controllers
  (xbox 045E:0285); HaloPad presents pads as Xbox 360 (028E), so a pad had no bindings.
  New halopad_gamepad_defaults.c applies Halo's own Xbox layout through setter 0x48e360
  to an empty slot (never over bindings), activating the pad with input_activate_joy.
  Menu navigation (A/B/D-pad/Menu → Enter/Escape/arrows) outside gameplay. DirectInput
  suite: 11 new checks pass incl. stick walks forward/strafes through Halo's consumer.
  Live: the Simulator's virtual gamepad got the layout on slot 0, touch kept slot 1.
- Windowed Simulator: iPadOS 26 defaulted the device to Windowed Apps (Settings →
  Multitasking & Gestures). Switched it to Full Screen Apps; Halo now fills the screen.
  App prefers the minimal window-control style; README documents the setting.
- Device build: --iphoneos builds arm64-apple-ios17.0, embeds extended-virtual-addressing
  and increased-memory-limit entitlements, writes HaloPad.ipa (ad-hoc; no signing identity
  on this Mac, no device connected). docs/INSTALL-IPHONE.md written.
- Private-server controls: five touch checks pass on 127.0.0.1:2310 Blood Gulch.
- Key: the repo's HaloCESetup.exe is a repack titled "Halo CE Cracked Setup" (seen when
  launched in a throwaway bottle; stopped immediately, bottle deleted). The official
  halocesetup_en_1.00.exe mirrors are unreachable (vaporeon.io, halomaps.org). The key
  still needs the official installer; not used otherwise.

## 2026-09-29 — clean-checkout setup on the installation Mac

- Started from clean `main` with only the third-party Halo CE repack in ignored `ref/`.
  No handoff kit, 1.10 patch, accepted `haloce.exe`, or official installer was found.
  The repack was not run.
- Xcode 27 and an Apple Development signing identity are present. The physical iPad
  appears in `devicectl` as unavailable. Four cached development profiles were
  inspected; none is for `dev.halopad.HaloPad` or grants Extended Virtual Addressing.
- Created `.venv`, bootstrapped pinned sources, and ran `scripts/doctor.sh` successfully.
  The pinned Unicorn build initially failed because current CMake removed compatibility
  with versions below 3.5. Re-running pip with `CMAKE_POLICY_VERSION_MINIMUM=3.5`
  succeeded. README and lifter preparation instructions now use that setting.
- Installation and hardware gameplay are pending a matching handoff kit, a profile
  granting both memory capabilities, and a connected/trusted iPad. No device launch,
  touch/controller/menu/private-match, or frame-rate result is claimed by this setup.

## 2026-09-29 — supplied handoff, Mac compatibility probe, phase 3 loop

- Found `ref/handoff/HaloPad-iPad-test/`. Verified its prepared package against the
  bundled app: 87 records, 78 stock; no private bytes were staged for Git. The
  original app and package stayed unchanged. `scripts/test-native-package.py` passed
  16 native package cases on iPad Air 13-inch (M4) Simulator, including the real
  package and byte-for-byte stock comparison. Evidence:
  `docs/artifacts/2026-09-29/G9/native-package-20260929T062903Z/`.
- A copied app was locally re-marked for iPhoneSimulator and ad-hoc signed under
  `generated/simulator-probe/`; this is an unsupported compatibility probe, not a
  source rebuild. On iPad Air Simulator it launched from a directly copied verified
  installed folder to
  Halo's main menu. Created New001, used Multiplayer → Create Game → LAN → Battle
  Creek → Slayer → Start, and spawned. Tapped FIRE; plasma-pistol charge decreased
  100→99. Touch overlay was initially hidden by the Simulator's virtual-controller
  setting; turning that option off showed it. Sequential menu taps and view drag
  worked. Simultaneous thumbs, real controller, private server, and physical-device
  behavior were not tested.
- Three-dot menu kept its round dots. Controls and problem-report UI opened; the
  report was cancelled. Three-dot Leave Game failed to leave the hosted match;
  Halo's pause-menu Leave Game succeeded. The menu may raise a software keyboard
  behind the popover; investigate rather than call it accepted.
- The same copied app reached the landscape Halo main menu on iPhone 17 Pro
  Simulator. Saved `docs/artifacts/2026-09-29/G11/iphone-17-pro-landscape-menu.png`.
  Further iPhone keyboard/match controls remain unproven in this run. This
  Simulator is not an iPhone 14 performance proxy.
- During the first probe, installing the device-platform kit over the pre-existing
  iPad Pro M5 Simulator bundle replaced its app binary; its data container was not
  erased. Launch there was denied because that binary targets iPhoneOS. The prior
  Simulator app binary was not available to restore. All later runs used the fresh
  iPad Air Simulator. Preserve the iPad Pro container and report this limitation.
- Wrote [HaloPad-GOAL-LOOP-PHASE3.md](HaloPad-GOAL-LOOP-PHASE3.md), with independent
  signing/install, input, private network, iPhone 14 performance, icon, and release
  candidate gates. Apple Developer portal currently waits for account sign-in;
  no cached profile has yet been created. Physical iPad is not present.

## 2026-09-29 — original iOS/iPadOS icon source and packaging

- Drew a new HaloPad H/orbit mark in `assets/HaloPadIcon.svg`, with no game artwork
  or logo. `scripts/render-app-icon.sh` renders an opaque 1024×1024 PNG from it;
  the checked-in `Assets.xcassets/AppIcon.appiconset` makes the result reproducible.
- `actool` compiled the asset for both `iphoneos` and `iphonesimulator` at iOS 17
  deployment target, emitting `Assets.car`, phone/tablet PNGs, and the phone/tablet
  `CFBundleIcons` plist entries. `build-ios-app.py` now merges those entries and
  compiles the icon into an app before signing. Its actual packaging helper was
  run on a minimal device app directory and produced `Assets.car` plus both
  phone/iPad plist keys. Python syntax and diff checks pass.
- A full source app cannot be built on this Mac until the accepted private 1.10
  inputs and reference-machine files are present. Home Screen appearance on iPhone
  and iPad therefore remains the D5 acceptance gate; no claim is made from the
  raw 1024-pixel preview alone.
- Device recheck: `devicectl` now lists Chris's physical iPhone 14 as connected;
  the iPad remains unavailable. Five cached Xcode profiles were decoded; none
  covers `dev.halopad.HaloPad` or grants both memory entitlements. The Apple
  Developer tab still shows its sign-in form. No device app was installed.

## 2026-09-29 — fail-closed device profile preflight

- Added `scripts/device_profile.py` and made both the prebuilt signing and source
  build paths require an exact HaloPad App ID, matching installed signing
  certificate, unexpired development profile, and both memory entitlements.
  `scripts/install-device.sh` also checks the physical device UDID before it
  copies, signs, or installs anything. Duplicate certificate names require a
  unique SHA-1 selector.
- Five synthetic profile tests pass (valid, missing memory capability, wildcard
  App ID, wrong device/certificate, expired/distribution). Python compilation
  and shell syntax pass. A real cached AgePad profile was rejected by the full
  install command for its wrong App ID and missing Extended Virtual Addressing;
  it exited before staging or device mutation. The original handoff kit remains
  untouched, and the iPhone 14 has not received HaloPad.
- Apple Developer sign-in is still at its initial form; no matching profile
  exists locally. The iPad remains unavailable, while the iPhone 14 is connected.
- Certificate inspection corrected the target team: all three Apple Development
  identities have certificate subject `OU=VKDH2T9UTF`, and the cached profiles
  use the same team. Their displayed names end in `P52SY73DYK`; that suffix is
  not the provisioning team. The future HaloPad App ID/profile must be created
  under `VKDH2T9UTF` and matched to one of these certificate SHA-1 values.

## 2026-09-29 — fresh phone import and keyboard layout

- An iPhone 17 Pro Simulator check was interrupted by an unrelated YomiBoy UI
  test runner on that same device. HaloPad remained alive and suspended; no
  HaloPad crash was found. Moved to the booted iPhone 17 Simulator, where no
  HaloPad install or competing test runner existed.
- Installed the same converted Simulator **copy** of the handoff app on iPhone
  17 (iOS 26.5), copied only the private matching `.halopad.zip` into its new
  Documents container, and used the actual **Choose Prepared Package → Files →
  Browse → Halo-CE.halopad.zip** path. HaloPad reached the main menu. A direct
  SHA-256 comparison found all 78 installed stock files present and identical
  to the already verified package import. Evidence:
  `docs/artifacts/2026-09-29/G11/iphone-17-imported-menu.png`.
- On that iPhone, the three-dot Join Server form opened. Focusing address
  showed the landscape software keyboard; tapping `1` entered `1`; Cancel
  dismissed both keyboard and form. With the keyboard open, the current kit's
  long alert message displaced the title and optional password field above
  the visible screen (`G11/iphone-17-join-keyboard-clipped.png`). The source
  alert now relies on its title and field placeholders, removing that message.
  The overlay input suite and 90 layout cases pass on iPhone 17 Simulator, but a
  live check of the changed alert requires a source-built app; the handoff
  binary still contains the old message. No private server was joined.

## 2026-09-29 — phone profile text path and physical iPad discovery

- On the fresh iPhone 17 Simulator install, Halo's Profiles screen opened the
  profile-name dialog with `New001` selected. Tapping the guest field did not
  summon a native keyboard. The three-dot **Keyboard & Chat → Show Keyboard**
  action did: an iOS keyboard appeared, a virtual `a` replaced `New001`, and
  virtual Return saved profile `a`. **Hide Keyboard** restored the full view;
  the Select Profile screen showed the new profile. The game view shrank to a
  small rectangle while the landscape keyboard was open, so phone text
  readability remains an acceptance concern. The first menu attempt selected
  the submenu header rather than Show Keyboard; the corrected row worked, and
  a traced run recorded `focus 1` and a docked keyboard frame. Evidence:
  `G11/iphone-17-keyboard-trace.log` and
  `G11/iphone-17-virtual-keyboard-profile-saved.png` (ignored).
- With that profile, touch navigation reached **Multiplayer → Create Game → LAN
  → Battle Creek → Slayer → Server Setup**. Start Game has not yet produced an
  observed phone match, so gameplay controls remain pending in this run.
- `devicectl` and Device Hub exposed an iPad Pro paired record, and a read-only
  installed-app query failed when CoreDevice said that record's device was
  locked. Chris clarified that **no physical iPad is connected**; the record
  must not be treated as an attached test device. Only the iPhone 14 is
  connected. Its read-only app inventory lists no `dev.halopad.HaloPad`, so
  there is no existing HaloPad container to back up before its first install.
  No device data was changed at that point. Apple Developer was at the initial
  sign-in form; none of the previously cached profiles was HaloPad's.
- The iPhone 14's CoreDevice details confirm a wired physical connection,
  iOS 26.6.2, and Developer Mode enabled. The hardware setup and app-container
  preservation gate were ready before installation.
- Chris signed into Apple Developer team `VKDH2T9UTF`. Registered explicit
  `dev.halopad.HaloPad` with Extended Virtual Addressing and Increased Memory
  Limit, then generated development profile `HaloPad iPhone 14 Development EVA
  Memory` for the connected iPhone. The portal's two devices were both named
  “Chris’ iPad Pro”; device detail confirmed record `53A6ZZG335` is actually
  UDID `00008110-001C2C641123A01E`, model iPhone 14. Corrected that portal
  label to “Chris's iPhone 14”. Three local Mac development certificates are
  in the profile; the AltStore certificate is excluded.
- Brave blocked the portal download endpoint. A minimal ignored Xcode project
  in `ref/profile-fetch/` requested this exact manual profile with
  `-allowProvisioningUpdates`; Xcode fetched it into its standard profile
  cache. UUID `765efc99-dab7-49e0-b386-d847cc001315`; expires 2027-09-29.
  `scripts/device_profile.py` passed exact App ID, development, certificate
  `0D9207CDF3FA18B08D7B4EA62C7879BEB958185F`, iPhone UDID, expiry, and
  both kernel memory entitlements. The temporary Xcode probe built; it was
  never installed.
- `scripts/install-device.sh --app ... --package ... --device
  00008110-001C2C641123A01E` signed a copy of the supplied handoff app,
  installed `dev.halopad.HaloPad`, and copied the 173.4 MB package to its
  Documents. The original kit was not modified. `devicectl` launched the
  physical app. QuickTime wired capture showed the package import screen
  behind an iOS headphones prompt. Import, menu, touch, online, and phone
  performance acceptance remain open.

## 2026-09-29 — source-built iPhone 14 install and first physical menu

- The supplied private handoff package held the accepted 1.10 `haloce.exe`,
  four modules, 78 stock game files, and reference-machine content. Restored
  them only into ignored `ref/inputs/`, verifying the package manifest and
  locked profile hashes. This is byte-verified handoff provenance, not a fresh
  CrossOver installer/patch reproduction. `inspect-inputs.py`, executable
  audit, lifter smoke (36/36), game/four-module SRW pipelines, and VA model
  passed. The Unicode `bestfit1252.txt` mapping was checked against the
  script's pinned SHA-256.
- A fresh `--iphoneos` build using the development scene entry initially
  failed during packaging: `build-ios-app.py` selected the Simulator object
  directory for device core identity. Changed it to use the requested target.
  The signed arm64 device app and IPA then built; `codesign --verify --deep
  --strict` passed, and decoded entitlements contain the exact App ID, team,
  Extended Virtual Addressing, and Increased Memory Limit. Its original icon
  compiled into `Assets.car` with iPhone/iPad `CFBundleIcons`. Core identity
  `a40934eac9383796ba5bd566d2d8bc2e28f6f54c4396208ae7d7fb738006ac00`;
  its 87-file private package passed preparation and independent verification.
- Before replacing the installed handoff app, copied its Documents and Library
  to ignored `G11/iphone14-pre-source-install-backup/`. The old package's SHA-256
  was identical to the handoff source. Installed the source app in place on
  physical iPhone 14 `00008110-001C2C641123A01E`; copied the new package to
  Documents without removing the old one. `devicectl` readback confirmed the
  bundle and both packages. Launched successfully. After Chris dismissed an
  iOS headphones prompt and locked the phone, iPhone Mirroring connected.
- Through the actual physical Files picker, selected
  `Halo-CE-source-20260929.halopad.zip`; HaloPad imported and reached Halo's
  main menu. CoreDevice listed `Documents/Halo Custom Edition` with 78 files
  (plus directories). A new `Test14` profile was entered and saved. The native
  Keyboard & Chat control activated the phone's keyboard mode, and Mirroring
  routed Mac text entry; physical on-screen key taps still need acceptance.
  The source Join Server form displayed title, address and optional-password
  fields, and buttons without the earlier Simulator clipping. Numeric address
  input and Cancel worked; no connection was attempted.
- iPhone Mirroring pointer clicks moved its pointer over Halo's rendered menu
  but did not select Create Game → LAN. A Return key activated the previously
  selected Join Internet option. UIKit overlays remained clickable. Source
  inspection found menu tap routing limited to `UITouchTypeDirect`; a small
  change now routes primary `UITouchTypeIndirectPointer` menu clicks through
  the same menu tap path while retaining secondary-click behavior. The revised
  signed app/IPA built and the post-import Documents folder was backed up to
  ignored `G11/iphone14-pre-pointer-fix-backup/`. **This revised build has not
  been installed or tested on the phone yet:** Chris requested direct phone
  access, so iPhone Mirroring was quit and the installed app left in place.
  Hardware gameplay, two-thumb controls, private online, frame/thermal data,
  and physical Home Screen icon checks remain open.

## 2026-09-29 — iPhone 14 control iteration and widescreen probe

- Preserved Documents and accessible Library contents before each in-place
  source rebuild/install under ignored `docs/artifacts/2026-09-29/G11/`.
  The signed app remained on physical iPhone 14
  `00008110-001C2C641123A01E`; the matching imported game folder and two
  prepared packages remained. Protected SplashBoard Library content could not
  be copied, so the backup is not a complete Library snapshot.
- Raised default touch look multiplier to 1.5, extended the control to 6, and
  relaxed the per-frame aim delta cap for slow frames. Renamed the Controls
  entry to **Look Speed & Touch Settings…**. Replaced the plain-text controller
  layout alert with a native scrollable Controller Guide. Added a keyboard
  accessory **Enter / Accept** action. In iPhone Mirroring, created profile
  `TouchQA` through Halo's profile menu with Show Keyboard and the new action.
  These checks prove the button path, not physical fingertip ergonomics.
- The earlier three-dot `disconnect` action did not leave a local-host game.
  Changed it to **Open Leave Game Menu…**, which sends Escape to Halo. A local
  Battle Creek LAN Slayer game started, the action opened the original pause
  dialog, and its **Leave Game** returned to the main menu. No public match was
  joined. Mirrored primary pointer clicks now select Halo menu entries.
- CoreDevice returned a 1024-pixel nonplaceholder H/orbit icon from the installed
  bundle, also visible in iPhone suggestions. The physical iPad icon still
  needs inspection when that device is connected.
- In a stationary Battle Creek view, the overlay showed 30 FPS. A 30-second
  physical Time Profiler sample (`G11/iphone14-battle-creek-timeprofiler.trace`)
  contained 9,791 CPU-sample ms; `halopad_lookup` consumed 2,698 ms and
  thread-yield/Sleep 2,234 ms. This points to CPU dispatch as an experiment
  candidate but does not explain slow map loading or prove sustained play.
- Halo's Video Setup lists 1280 × 720. Applied it on the physical iPhone after
  backups. Halo displayed its video-test warning, then exited to Home Screen.
  The build includes an implementation of the previously missing
  `SwitchToThread` import, but this did not make the resolution switch work.
  Relaunch reached the main menu again. Keep 800 × 600 at Original aspect to
  avoid distortion. The precise remaining crash cause is open.
- Rewrote the installation guide for the actual iPad handoff: a newly connected
  iPad needs its own UDID in the development profile, container backup, in-place
  install of the matching app/package, and physical gameplay checks. Updated
  the public-facing README with the current limits. No public release was made.

## 2026-09-29 — physical iPhone resolution reset diagnosis

- The source `SwitchToThread` implementation was absent from the generated VA
  runtime IR consumed by the earlier phone build. Regenerated the VA runtime
  from `port/llasm-runtime/`, rebuilt and signed the app, and added a build
  preflight that rejects missing or stale generated runtime IR. Disassembly of
  the new device binary showed the actual `SwitchToThread_c` call.
- The next physical 1280 × 720 attempt trapped on Windows Text Services
  Framework's language-bar CLSID. Returning the ordinary COM
  `REGDB_E_CLASSNOTREG` with a null output pointer let Halo continue. The next
  attempt trapped on `IDirect3DQuery9::GetData` before `Issue`; returning
  `D3DERR_INVALIDCALL` without altering Halo's initialized sample count let
  the reset proceed. Added focused native regression cases. The iPad Simulator
  OLE suite passed with zero failures; D3D9 passed 231 checks, zero failures.
- After an in-place install on the physical iPhone 14, Halo displayed its
  **Video settings changed** confirmation over a 16:9 game image at about
  30 FPS. The mode subsequently read 800 × 600 again. The app relaunched to
  the main menu without lost profile or imported game data. The confirmation
  may have timed out, or pointer mapping/settings persistence may be wrong;
  sustained widescreen play is not yet accepted. Keep 800 × 600 Original aspect
  for normal use until the confirmation path is rechecked.
- Backed up the existing phone container before each in-place install under
  ignored `docs/artifacts/2026-09-29/G11/`, including
  `iphone14-pre-query-fix-backup/`; the data container retained the `TouchQA`
  profile and imported game. The latest signed app and its matching private
  87-file package are local build artifacts. No public game was joined. The
  physical iPad remains a paired but disconnected record, so its profile,
  install, controller, gameplay, and video checks await connection.
- The static Battle Creek Time Profiler trace identifies guest dispatch and
  thread yielding as candidates, but there is still no measured cause for
  map-loading stalls or a demonstrated sustained iPhone performance gain.
- Follow-up isolated the apparent 1280 × 720 fallback: the earlier relaunch
  happened before saving the parent profile settings. Repeated the physical
  switch, accepted Halo's video-test **OK**, pressed **OK** in **Edit Profile
  Settings**, and observed its save dialog finish. Relaunch returned with the
  wider 16:9 layout. Halo's Video Setup had shown 1280 × 720 before that save;
  the post-save relaunch displayed the same wide layout.
- From that relaunched build, created a local LAN Battle Creek Slayer game
  through Halo's menus, reached first-person play with the touch overlay
  visible, opened the pause menu, and used **Leave Game** to return to the
  wide main menu. Mirrored pointer clicks selected the menus. Momentary overlay
  readings were 29–30 FPS; this is not a frame-time distribution or heat test.
  No public server was joined. Direct-finger aiming, two-thumb play, controller
  input, longer play and physical iPad acceptance remain open.

## 2026-09-29 — physical iPad and controller diagnosis

- Connected Chris's actual iPad Pro 12.9-inch (6th generation, iPadOS 27.0),
  UDID `00008112-001D485114DBC01E`. Cached development profile
  `774ca5c5-030a-4b41-ab5e-a31fc3d3594a` passed exact App ID, certificate,
  device UDID, expiry, Extended Virtual Addressing and Increased Memory Limit.
  Installed the signed source app in place, imported its matching package from
  Files, reached Halo's main menu and created profile `New001`. The iPad had no
  previous HaloPad container. Read back `Documents` and `Library` to ignored
  `generated/device-backups/ipad-20260929-first-import/` (625 MB). No public
  server was joined.
- In a local LAN Battle Creek Slayer match, menu selection, move/look drags,
  FIRE, JUMP, three-dot menu, Controller Guide, Look Speed settings and Halo's
  original pause/Leave Game path worked. Remote pointer drags did not prove
  simultaneous physical thumbs. Chris then connected an Xbox controller during
  play. The overlay hid but gameplay input did not reach Halo, while Menu/Start
  still worked. After fully closing and reopening HaloPad with the controller
  connected, Chris confirmed movement, crouch and the original controller
  layout work. The attached keyboard/trackpad are a separate input path and
  did not prevent that startup controller path.
- The cause is Halo's one-time DirectInput `EnumDevices` at startup. UIKit sees
  a late controller and used to hide touch, but Halo has no guest joystick
  device for it. An experiment invoking Halo's original enumeration callback
  after startup crashed twice in the iPad Simulator DirectInput harness at an
  indirect null guest transfer. That experiment was removed, not installed.
  The new source candidate keeps touch visible unless a currently connected
  physical pad has a registered DirectInput device; the Controller Guide says
  to reconnect before launch and maps Halo's generic “Button 6” to Xbox RB.
  DirectInput and overlay tests passed with zero failures; physical recheck of
  that new candidate is pending.
- Chris reports noticeable iPad jitter when joining and loading new assets.
  A 20-second Time Profiler trace of the live physical match is in ignored
  `docs/artifacts/2026-09-29/G11/ipad-live-controller-stutter.trace`. Thermal
  state stayed Nominal; there were 2,197 sampled CPU stack leaves, including
  610 stacks in `draw`, 156 in `halopad_metal_pipeline`, and 118 in
  `halopad_metal_sampler`. The sample does not prove which individual frame
  stalled. The next discriminating run enables `HALOPAD_TRACE_FRAMES` at launch
  to record presents, >100 ms gaps, pipeline/library creation and DXT decoding
  during the same local map-load path. Do not claim an iPad performance fix yet.
- Replaced the first H/orbit icon with an original geometric beacon in
  `assets/HaloPadIcon.svg` and its 1024-pixel asset-catalog PNG. The revised
  source app signs; device icon appearance awaits the next install. README now
  describes the physical iPad checkpoint, controller restart requirement and
  planned PadForge personal-build path. Project Reach remains the repo name,
  HaloPad the app name; public release and PadForge support are still gated.
- Added a draft `padforge.json` with iOS on Apple Silicon Mac explicitly
  `planned`, no runnable backend, no advertised player target and no public
  binary. PadForge's `check-manifest` accepts it. The next gate is an
  independently reproducible build from the player's verified Custom Edition
  installation, including a public input verifier, dependency bootstrap and
  ignored output audit. The existing signed development build is not that path.

## 2026-09-29 — full-entry diagnosis, restored play, and late-controller slot

- A newly signed iPad candidate accidentally omitted `--scene tests/halo_app_scene.c`.
  It therefore ran Halo's full original entry point. Its first launch trapped
  on `d3d9.dll!DisableD3DSpy`; added a narrow no-op export because there is no
  D3D spy layer in HaloPad. The next full-entry launch reached Halo's original
  **Your product key is invalid** fatal dialog. The iPad's registry readback,
  compared with its pre-install backup, had no `DigitalProductID` in either
  version. The in-place install preserved the container; the earlier playable
  handoff app was confirmed by its binary strings to use the development scene.
  No product key or derived value was written into the repository or logs.
- Rebuilt and installed the development-scene app in place with the same bundle
  ID. It reached the main menu with existing profile `New001`, then created a
  local LAN Battle Creek Slayer match. The revised icon appeared on the iPad
  Home Screen. The user continued local play; no public game was joined.
- With `HALOPAD_TRACE_FRAMES=1`, the physical iPad recorded a 456 ms present
  gap and two >100 ms gaps in one 10-second interval around a local map change;
  27 shader libraries and 24 pipelines were created, their aggregate reported
  time under 0.01 s. Most subsequent intervals presented about 30 FPS with no
  >100 ms gap. This confirms an intermittent stall, not its CPU/I/O cause or
  a performance fix. Evidence is private under `G11/ipad-frame-trace-playable-fix.log`.
- The same build was launched while the Xbox controller was absent, so Halo's
  one-time DirectInput enumeration could not register it later. Added a stable,
  neutral physical controller slot on iOS. Its device persists while a pad is
  absent and reports the first connected controller when one arrives or is
  replaced, without calling Halo's unsafe enumeration callback. The iPad
  Simulator DirectInput harness passed with zero failures, including late
  connect, replacement, and disconnect. Physical install and resolution-reset
  acceptance remain open while Chris is playing the current iPad build.
- Signed, offline development-scene candidates with this stable-slot change are
  preserved under ignored `generated/device-candidates/ipad-20260929-stable-controller/`
  and `generated/device-candidates/iphone14-20260929-stable-controller/`.
  `codesign --verify --deep --strict` passed. Neither was installed: Chris was
  actively playing on the iPad and using the iPhone. The installed iPad build
  remains the earlier development scene, which needs the controller connected
  before app launch. The iOS Simulator DirectInput suite passed (zero failures)
  at `G3/core-arm64-apple-ios17.0-simulator-20260929T124619Z`. A Mac-target
  attempt failed to link an unrelated existing `halopad_host_post_input` symbol;
  it is not an iOS test failure.
- Chris supplied a private 25-character Halo PC product key in chat for a
  legitimate full-entry test. It was not echoed into a command, stored in a
  file, entered into software, or committed. No original Halo PC installer or
  licensed Windows installation was found in the current inputs; the available
  `ref/Halo CE/HaloCESetup.exe` is the Custom Edition repack and includes
  `PidGen.dll`, but its installation/key path was not run. The old CrossOver
  binary path in `scripts/prepare-patched-client.sh` is unavailable on this
  Mac; only Wine Stable at `/opt/homebrew/bin/wine` was found. Next work should
  keep key material outside source and public logs, establish an original
  installer-produced `DigitalProductID`, and only then retest full startup.

## 2026-09-29 — stable-slot candidate on physical iPad

- Revalidated the cached development profile against the iPad UDID, exact App
  ID, signing certificate, expiry and both memory entitlements. The signed
  stable-slot app passed `codesign --verify --deep --strict`. Copied the current
  iPad `Documents` and `Library` to ignored
  `generated/device-backups/ipad-20260929-pre-stable-controller/` before an
  in-place install. `devicectl` installed `dev.halopad.HaloPad` successfully;
  profile `New001` remained available afterward.
- Launched the updated app with `HALOPAD_TRACE_FRAMES=1`, created a local LAN
  Battle Creek Slayer match through Halo's menus, and observed first-person
  play with the touch overlay. No public game was joined. In the entry interval,
  286 frames presented in ten seconds (28.5/s) with a 454 ms longest gap and
  two gaps over 100 ms. The same interval created 27 shader libraries and 25
  pipelines, with aggregate creation time reported below 0.01 s. Following
  intervals generally presented 300 frames per ten seconds with no >100 ms
  gap. Private log: `G11/ipad-stable-controller-device.log`. This measures the
  stall but does not identify its CPU, I/O or scheduler cause.
- The test app later exited with code 0 while the user was opening iPadOS
  Bluetooth settings; no crash or resolution change was observed in that
  session. The Xbox controller first showed a connection spinner in the iPad's
  Bluetooth list and then disappeared from saved devices after the user
  unpaired it. HaloPad cannot receive a gamepad until iPadOS pairs it. The
  physical late-connect and post-video-change tests remain open. Bluetooth
  pairing troubleshooting is in progress; no Bluetooth settings were changed
  by the agent.
- An isolated Wine prefix was initialized under ignored
  `generated/license-probe/prefix/` to inspect the supplied Custom Edition
  installer path without entering or recording the private product key. The
  installer command exited 1 without useful output or a visible UI; no
  `DigitalProductID` was provisioned. The original Halo PC installer path is
  still the full-entry gate.

## 2026-09-29 — iPad crash fix, diagnostic log and controller/pointer input

- Chris reported repeated crashes on the physical iPad. The two newest
  CoreDevice crash reports (`HaloPad-2026-09-29-225935.ips`, `-230425.ips`)
  match the installed stable-controller binary by UUID (987BE07F). Both abort
  on Halo's thread in `hp_unsupported` from `draw` / `DrawIndexedPrimitive`.
  The string operand of `draw.cold.5` identifies the trap: "more than 8
  sampler configurations for one pixel shader". Pixel-shader variants are now
  a growable list freed with the shader resource.
- Added `port/runtime/halopad_log.c`: an always-on, change-only diagnostic log
  at `Documents/HaloPad Logs/HaloPad.log` (1 MB plus one rotated file), also on
  stderr. Runtime traps log a `CRASH:` reason before aborting. Logged:
  controller connect/disconnect, DirectInput acquire/release and refused
  reacquire, physical pad presence, Halo's device-to-player assignment,
  menu/game state, controller menu keys (and presses ignored during play),
  Direct3D device creation and resets, app active/inactive, and frame stalls
  over 300 ms with the shader compiles inside them. It does not record typed
  text, profile names, chat or server addresses. Three-dot **Help → Share
  Diagnostic Log…** shares the file. Runtime call sites use a guarded macro;
  the slice harness links the logger directly.
- Controller: the left stick navigates Halo's menus like the D-pad; while the
  HaloPad text keyboard is open, A sends Enter and B sends Escape, then hide the
  keyboard (a controller player could not reach the keyboard bar's Enter /
  Accept during a match). Once set up, a physical controller that Halo leaves
  unassigned is reattached to its previous player slot if that slot is free.
  Gamepads are foreground-exclusive DirectInput devices, so any scene
  deactivation releases them; the log now records whether Halo reacquires
  them. That is the leading hypothesis for the "controller stops after
  reconnecting or after touch" report; it is not proved.
- Trackpad: hover over Halo's menus sent raw deltas through Halo's menu
  acceleration, so Halo's cursor drifted from the iPadOS pointer. Hover now
  uses the menu-touch state machine as a move-only gesture (latest spot only,
  superseded by a click, never presses).
- Tests: menu touch 9/9 plus 72 original-x86 cursor routes
  (`G9/menu-touch-20260929T135217Z`); DirectInput and D3D9 iPad Air Simulator
  suites 0 failures (`G3/core-arm64-apple-ios17.0-simulator-20260929T142722Z`
  and `…T142736Z`, the latter logging a 640×480 → 800×600 reset); overlay 0
  failures (`G9/overlay-20260929T143222Z`); Python 59 OK, 16 skipped. The VA
  runtime IR was regenerated first because the `DisableD3DSpy` source was newer.
- Backed up the iPad container to ignored
  `generated/device-backups/ipad-20260929-pre-crash-fix/` (Documents 618 MB,
  Library 28 MB), installed signed candidate
  `generated/device-candidates/ipad-20260929-crash-fix/` (UUID A0D8AA4E) in
  place, read back profile `Kahris` and its saves, and launched it to Halo's
  menu with the log active. The matching iPhone candidate is signed and
  verified but not installed.
- Chris's traced session (ignored `G11/ipad-video-change-device.log`) after
  replacing the controller battery: steady ~30 FPS; first-use shader compiles
  in play cost 0.45–1.15 s per 10 s window, with gaps up to 1,038 ms. Two later
  gaps of 12.4 s and 8.2 s are unexplained (possibly app switching). Device Hub
  coordinate input failed in this session (screenshots and accessibility
  worked), so the agent did not drive the iPad video-mode change.
- Open: hardware confirmation of the crash fix, the controller
  reconnect/touch-switch bug from the new log, trackpad alignment, iPad
  video-mode change, and shader-compile stalls. Public release remains a
  separate rights decision: the app bundle contains Halo's translated
  executable and DLL images, and the development scene starts without Halo's
  product-key check.

## 2026-09-29 — public-build pass: install script, controller keys, diagnostics

- The repository is now public (Chris's decision). Reviewed the public builder
  path. `scripts/install-device.sh` built with the experimental
  `tests/halo_touch_move_scene.c` (which adds a second, virtual DirectInput
  controller that competes for player slots) while the install guide and every
  physical build use `tests/halo_app_scene.c`; the script now uses the tested
  scene. Under macOS's stock bash 3.2 the script's source-build path also
  stopped with "WORKARG[@]: unbound variable" whenever `--work` was omitted
  (reproduced with `set -u` and an empty array); it now uses the 3.2-safe
  expansion. `scripts/doctor.sh` passes on this Mac.
- Halo's joystick loop (x86 0x493760–0x493838) polls only devices assigned to
  a player, skips polling while 0x6b4768 is set, and calls Acquire only after
  DIERR_NOTACQUIRED or DIERR_INPUTLOST; any other error reads neutral.
  HaloPad's Poll returns those two codes after a focus loss, so ordinary
  deactivation should recover. The log now records Poll result changes, the
  0x6b4768 pause flag, and a one-minute health line in games (FPS, controller
  readiness, menu, device count).
- Controller menu keys held when a controller disconnects or the app goes
  inactive are now released. A held arrow key in Halo's keyboard state
  swallowed the next press of that direction, one candidate for "D-pad down
  does nothing in the menu".
- Report a Problem's text share now attaches the log file; the GitHub issue
  body and issue template ask for it. README updated for late controller
  connection (Simulator-tested), controller menu/text keys, the crash fix,
  warm-up advice for recording, the diagnostic log and current known issues.
- Tests: DirectInput suite 0 failures
  (`G3/core-arm64-apple-ios17.0-simulator-20260929T145307Z`), overlay 0
  failures (`G9/overlay-20260929T145358Z`), menu touch with 72 cursor routes
  (`G9/menu-touch-20260929T145404Z`). The iPad log showed no play since the
  23:35 launch, so the signed `ipad-20260929-diagnostics-2` build (UUID
  971874D9) was installed in place with the 23:33 backup still current; the
  `Kahris` save remained, and the app relaunched to Halo's menu. The iPhone
  candidate is signed but not installed.
- Note for the public history: the Sept 28 journal commit 373cc86 still
  contains the first five characters of the supplied product key; the current
  tree is redacted. Removing it from history needs an explicit rewrite decision.

## 2026-09-30 — icon, renderer fallbacks, background shader compiles, CD-key dialog

- Icon: generated four original concepts with the image tool (orbital arc,
  hex shield v1/v2, d-pad star; none uses Halo's logo, helmets or text), kept in
  `assets/icon-concepts/`. Chris chose the orbital arc. It is cropped in for
  small sizes and installed as `AppIcon.png`; a masked preview stayed legible at
  40 px. README references updated. Staged in signed candidates, not yet installed.
- Stability: every Direct3D renderer trap except unsupported texture formats now
  degrades instead of aborting, each distinct case logged once: unusual sampler
  filters (linear), border colours (nearest Metal border), address modes (wrap),
  LOD bias, clip planes, table fog, sRGB writes, MSAA state and scissor (ignored),
  out-of-range blend/compare/stencil values (safe defaults), and point lists,
  fixed-function vertex blending, untranslatable shaders, missing streams and
  Metal-rejected pipelines (draw skipped). Rejected pipelines are remembered so
  they are never recompiled per draw.
- Performance: in the app, new shader libraries and render pipelines compile on
  a background queue (three at a time) and only their draws wait a frame or two;
  every compiled library is saved to `Caches/HaloPad/shaders` and precompiled at
  the next launch. `HALOPAD_SYNC_SHADERS=1` restores synchronous compiles; tests
  stay synchronous. Frame-stall log lines now include pipeline and texture-upload
  time. Measured at map load in the iPad Air Simulator: the worst Blood Gulch
  frame went from 1,720 ms (37 synchronous shader compiles) to 799 ms (57 in the
  background) to 381 ms with warm-up (88 remembered libraries); on the physical
  iPad the worst load frame was 349 ms with 56 shaders compiled in the
  background. Remaining load stalls are texture uploads and Halo's own loading.
- Benchmark withdrawn: a development benchmark hosted maps through Halo's
  console (`sv_map`). On both the Simulator and the iPad every hosted map
  returned to the front end within about a second, and on the iPad Halo showed
  "Your CD Key is invalid." Hosting runs Halo's CD-key check against the key
  string HaloPad's development scene reads from Halo's own 0x5829e0, which is
  empty without an installer-written `DigitalProductID`. The steady-state FPS
  figures from those runs therefore describe the front end, not gameplay, and
  are not claimed. The benchmark code was removed; HaloPad was relaunched on the
  iPad without it. Menu-hosted LAN games and online joins had worked before
  without this dialog. A valid key typed by the player is not a substitute:
  Halo reads the installer-written product ID, and HaloPad does not generate one.
- Tests: D3D9 and DirectInput iPad Air Simulator suites 0 failures
  (`G3/core-arm64-apple-ios17.0-simulator-20260929T233416Z`, `…T233434Z`);
  overlay 0 failures (`G9/overlay-20260929T233434Z`). The iPad was backed up
  to ignored `generated/device-backups/ipad-20260930-pre-async-shaders/` before
  installing the background-compile build; signed icon builds for iPad and
  iPhone are staged under `generated/device-candidates/*-20260930-icon-async/`.

## 2026-10-01 — Xbox build 59, short analog input and checkpoint reload

**Scope:** Chris requested focused iPad Simulator passes on this Mac, the
one-app Windows/Xbox picker, and a recent upstream Xbox build. Existing checkout;
no physical install, Xbox IPA, publication, controller re-pairing or recurring job.

**Experiment/result:** build 59 (`8fb1647e18a68e164529321bdcbe24557801f23b`)
fixes upstream weapon-swap crashes and log spam. Freeze that revision. Before
the host fix, short stick gestures published nonzero then zero between guest
polls; right-look gestures could also end before the display tick published them.
The portable buffer now retains one unread nonzero sample per axis, preserving
held input and stronger physical-pad merge. Touch cancellation/hiding clears
pending state, including Start/Back cancellation. Look publishes on actual
touch events. Input tracing is opt-in, values only, off in normal builds.

Real UI checks on **HaloPad Xbox iPad**, iPadOS 26.5: one swipe changes Campaign
to Multiplayer, opposite returns; create New001; Normal campaign; four-direction
look tutorial; X exits tube; a forward gesture moves the standing view slightly;
Start opens pause; normal Save and Quit completes. Restart the app and select
Xbox → Campaign → New001 → Pillar of Autumn → Normal: the game-in-progress
checkpoint reloads the cryo-bay sequence, without the initial ship cinematic.
All this uses isolated saves, no init script, bot or guest-action injection.
The upstream new-profile debug unlock-all-levels labels are not completion proof.

The save-backed update routine passed Mac menu/campaign/match (2,074 ticks / 16
scripted shots) and all three Simulator cases (941 ticks / 6 shots), without
render probes/overrides. Campaign and match captures visually reviewed: campaign
presentation works; pale match triangles and geometry/texture artifacts remain.
Accepted pin moves from `b47f237d` to build 59 as an **experimental development
baseline**. Rebuilt the normal app without a revision override and installed in
place; picker and About visibly show experimental status and installed `8fb1647e`.
About explicitly separates full campaign, split-screen/system-link and
cross-edition multiplayer limits. Real Simulator saves remain byte-identical to
the update backup after regression and final reinstall (container UUID refreshed).

**Evidence:** ignored `ref/xbox-build/passes/2026-10-01/campaign-controls.8B6vcC/`,
`campaign59.sXluwg/` (including `checkpoint-reloaded.png`), `build59-picker.png`,
`build59-accept.log`; update cases under `ref/xbox-build/{smoke-results,simulator-results}/20261001-133246-8fb1647e/`;
save backup `ref/xbox-build/save-backups/20261001-133246-from-b47f237d/`.
68 Python tests, 16 skipped; physical-device SDK syntax and repository tree/index
guards pass. Builds and game inputs stay private. No physical acceptance claimed.

**Next discriminating experiment:** sustained movement and simultaneous look/fire,
weapon pickup/swap and a later checkpoint reload, then isolate remaining rendering
artifacts. Same-build reload is not old-snapshot compatibility or full progression.

## 2026-10-01 — build 60, artifact isolation and Mac build identity

**Scope:** continued focused iPad Simulator passes in the existing checkout.
No physical install, controller pairing change, IPA, push or publication.

**Upstream:** build 60 (`bfbac35761335c28aac7a47bf0c578ea37764810`), released
04:01 UTC, fixes text-glyph cache edges. Its single source change does not claim
to fix world geometry. The save-backed update routine accepted this frozen
experimental pin after all six Mac/Simulator smoke cases passed. Mac match:
2,101 ticks / 16 shots; Simulator: 685 ticks / four shots. The final picker app
was rebuilt after acceptance; About shows `bfbac357` without PREVIEW. Xbox still
says EXPERIMENTAL. Windows still routes to its existing missing-data setup;
this does not establish Windows gameplay or installer provisioning.

**Rendering:** source/destination diagnostics put cliff striping and pale weapon
geometry before the final blit. Querying all 192 vertex constants across 122
programs found no interior uniform-location holes. Replacing unsynchronized
mapped uploads with BufferSubData still shows the artifacts. Both experiments
were removed rather than changing production behavior without a demonstrated
fix. Private evidence is preserved under `build59-geometry-trace/`,
`build59-uniform-trace/` and `build59-buffer-subdata/` in today's pass folder.

An auxiliary Mac CPU-texture run overlapped a Simulator match, failed with
`WSAEADDRINUSE`, and is invalid rendering evidence. A serial fresh build-60 run
with S3TC disabled passes (1,800 ticks / 15 shots); its captured terrain/weapon
view lacks the obvious Simulator cliff stripes, but timing/viewpoints differ.
This does not establish a driver bug, matched fidelity or hardware behavior.

**Test safety:** the Mac builder now records executable/guest hashes and the
exact revision. Smoke checks reject a missing/stale manifest before launch and
report the binaries' identity rather than the later checkout HEAD. Relative
evidence paths are made absolute. Five fixture tests cover the guard. The real
serial build-60 run uses it successfully. Network cases must run serially when
they share the same host port.

**Save/menu proof:** copied the isolated build-59 New001 save to a new private
folder, then used the actual picker → Xbox → Campaign → New001 → Pillar of
Autumn → Normal controls. Build 60 recognizes a game in progress and resumes
the cryo-bay sequence without the initial ship intro. This is one 59→60
checkpoint, not universal snapshot compatibility. Evidence:
`campaign60-compat.Gh2yDp/checkpoint.png`, logs, `build60-about.png` and
`build60-pc-route.png`. The real Simulator saves match
`save-backups/20261001-140820-from-8fb1647e/simulator-save` byte-for-byte after
gates, final installation and manual tests. The app is left on the normal picker,
with no diagnostic/forced-selection environment.

**Validation:** 73 Python tests, 16 skipped; whitespace and tree/index guards
pass. Updates and private evidence are detailed in XBOX-SIMULATOR-PASSES.md.
The broader goal remains active: next isolate texture sampling / vertex output
with a small reproducible case, then sustained controls, weapon pickup/swap and
a later checkpoint. Full campaign, audio, human multiplayer and physical
acceptance remain open. No temporary checkout or private game artifact published.

## 2026-10-01 — build-60 sampling/depth diagnostic pass

**Scope:** stayed on the dedicated HaloPad Xbox iPad Simulator. Authenticated
upstream release lookup still reports build 60, `bfbac357`; kept that frozen
experimental pin. No physical install, IPA, controller pairing change or push.

**Findings:** disabling anisotropy leaves cliff stripes (841 ticks / six shots).
Replacing EQUAL with LEQUAL also leaves stripes (1,092 / nine). Replacing EQUAL
with ALWAYS changes artifacts (989 / seven), but bypasses occlusion and is not
a player workaround. All modes were confirmed by startup/function logs and
their source captures were visually reviewed. Evidence: `build60-no-anisotropy`,
`build60-depth-lequal`, `build60-depth-always` in today's private pass folder.

**Better comparison tooling:** `--stationary-match` disables scripted movement,
shooting and automated gathering, requires an explicit match rendering diagnostic,
and labels its result separately from combat acceptance. Default combat still
requires shots. The EQUAL and ALWAYS stationary runs pass their rendering checks
(1,205 and 1,281 ticks, zero shots), with steady camera positions within each run.
However, positions differ between launches because spawns vary. ALWAYS exposes
hidden surfaces; these frames are not matched fidelity or driver-defect proof.
See `build60-stationary-{equal,always}`. The opt-in depth hook is Simulator-only,
accepts only lequal/always, and leaves normal/physical depth behavior unchanged.
Existing extension hiding is exposed only in rendering diagnostics.

**Verification:** final rebuilt normal app passes menu, a10 campaign load and
scripted match (1,108 ticks / six shots), under `build60-depth-normal-regression`.
77 Python tests pass, 16 skipped; physical SDK syntax, whitespace and tree/index
guards pass. Actual About navigation shows installed `bfbac357`, built today,
without PREVIEW. Done returns to the edition picker; final screenshot is
`build60-depth-final-picker.png`. App is left there without forced selection or
diagnostic environment. Rediscovered the new data-container UUID; real Xbox saves
still match `save-backups/20261001-140820-from-8fb1647e/simulator-save` byte-for-byte
after all probes, reinstall, normal gates and About navigation.

**Next discriminating experiment:** use a same-process fixed view or an identical
draw/depth pair to separate shader conversion, depth/bias state and software-driver
behavior. Do not globally weaken depth checks. Then sustained controls, weapon
pickup/swap and a later checkpoint. Goal remains active; full campaign, audio,
human multiplayer and physical acceptance are still unverified.

## 2026-10-01 — reversible depth reproduction, not a bypass fix

**Progress:** replaced different-spawn comparisons with native EQUAL → ALWAYS
→ restored EQUAL captures in one stationary match. In `build60-depth-paired/`,
the camera stays `(98.7, -149.7, 0.7)`; stripes disappear during the bypass and
return after restoration. The bypass also overdraws hidden base surfaces.
Visually reviewed all three source images. Result: 3,365 ticks, zero shots;
rendering evidence only, not combat or an adopted workaround.

**Control:** an asset-free 128x128 perspective triangle in RGBA8 /
DEPTH24_STENCIL8 redraws correctly with EQUAL within one program and across
linked programs sharing invariant position code but different active varyings.
Both cover 6,728 pixels, zero failures, GL error 0 (`build60-depth-controls/`).
A general equal-depth failure is disproven; a shader-specific driver issue,
conversion error or state mismatch remains possible.

The hardened runner checks complete, fresh PPM payloads and confirmed switches,
and keeps the stationary helper alive for the full experiment. Its repeated
`build60-depth-paired-verified/` run passes all three captures and both controls:
2,890 ticks, camera fixed at `(84.9, -161.7, 0.7)`. Visually reviewed the same
stripe removal/restoration. Missing, stale, partial and incomplete control
results are covered by fixture tests. All modes remain opt-in and Simulator-only.

**Concrete next target:** upstream's existing frame-240 draw trace and shader
dump (`build60-depth-draw-trace/`) identify VS17 LEQUAL/depth-write and VS41
EQUAL/no-write terrain passes with matching 1,398-index counts and printed
c[0..3] values. These GLSL position calculations share dot expressions but
differ in surrounding instructions/order. This does not prove identical full
inputs. Replay the exact vertices, uniforms and state before attributing the
bug or changing conversion. The original trace helper expires after 70 seconds;
its later menu screenshot is not world-rendering proof. Subsequent stationary
helpers now cover the entire run. Generated shaders/constants stay private.

**Normal verification:** rebuilt the personal Simulator app. All normal
menu/campaign/scripted-match gates pass (`build60-paired-normal-regression/`):
1,176 match ticks / eight test shots. 81 Python tests pass, 16 skipped; physical
SDK syntax, whitespace and current tree/index safety guards pass. Real Xbox
saves still match the build-60 update backup byte-for-byte after reinstall and
all probes. Upstream lookup still reports build 60; the accepted pin remains
`bfbac357`. No physical install, IPA, pairing change, push or publication.
The full goal remains active; no rendering fix or full campaign acceptance claimed.
Final About navigation confirms `bfbac357` without PREVIEW; Done returns to the
normal edition picker (`build60-paired-settled-picker.png`). The earlier immediate
relaunch screenshot captures a white startup transition, not the settled app.

## 2026-10-01 — exact terrain inputs and numeric shader replay

**Progress, not a rendering fix:** added opt-in Simulator-only read-only draw
capture and transform-feedback replay. `build60-draw-inputs-matched/` captures
VS17 LEQUAL/write and VS41 EQUAL/no-write terrain draws, both 144 indices, complete
and GL error 0. Indexed position inputs (1,728 bytes each) and projection constants
are bit-identical. Viewport/depth range match and polygon offset is disabled;
stencil value masks differ (1/3), whose correctness is not assumed. The visually
reviewed capture-run screen still shows stripes and pale geometry (2,359 ticks,
zero scripted shots). Original shaders are neither modified nor relinked by capture.

**Failed approaches recorded:** `build60-draw-inputs/` missed draws by retaining
the earlier spawn's 1,398-index filter. Selection now uses exact shader bytes and
depth state, then matches count. First numeric replay (`build60-draw-replay/`)
returned GL_INVALID_OPERATION and zero outputs because ES3 forbids indexed draws
during active transform feedback. This was a diagnostic API error, not a driver
bug. Corrected replay expands all enabled attributes in original index order and
uses DrawArrays; gl_VertexID-dependent sources are rejected.

**Numeric result:** `build60-draw-replay-expanded/` and the runner-hardened repeat
`build60-draw-replay-verified/` produce 144 vertices in each pass, GL error 0.
Coordinates are bit-identical: zero changed vertices and component/NDC-depth
deltas 0. Replay changes linkage and omits textures, pixel constants and raster
depth/stencil. It does not establish invariance of the original linked programs
or a driver defect. Next isolate that pair's raster/depth/stencil state while
accounting for compiler/linkage perturbation; normal EQUAL remains unchanged.
Generated game-derived buffers/shaders stay private under ignored `ref/`.

**Verification:** normal rebuilt app passes menu/campaign/scripted-match in
`build60-draw-normal-regression/` (1,277 ticks / nine shots); campaign opening and
match frames reviewed, remaining artifacts explicit. 93 Python tests run with
16 skipped and no failures. Physical SDK syntax passes after adding the existing
SDL include path to the diagnostic command; no physical installation. Actual
About displays accepted `bfbac357` without PREVIEW; Done returns to the normal
edition picker (`build60-draw-final-picker.png`). Rediscovered data-container
UUID `51CD4A39-C0B5-4754-998C-A8F8EF99A708`; real Xbox saves match the build-60
update backup byte-for-byte after all probes and final navigation. Latest upstream
release lookup still reports build 60. No IPA, push, publication or pairing change.
Goal remains active; full campaign, audio, human multiplayer and hardware remain open.

## 2026-10-01 — captured-program depth controls pass; full scene defect remains

**Progress from the prior numeric replay:** isolated rasterization now uses
unmodified captured vertex/fragment sources, normal linkage without TF, and
original indexed buffer layouts. A same-program EQUAL redraw controls the base;
the second program draws EQUAL then ALWAYS without relinking between those two
draws. Defined black 2D/cube textures remove incomplete sampling ambiguity.
Targets can use renderbuffers or textures in RGBA8 / DEPTH24_STENCIL8. Pixel
uniforms stay at defaults; stencil/blend/cull/scissor/offset are disabled. This
is a depth control, not original scene/texture fidelity.

**Evidence:** the first 144-index pair covers 3,785 pixels with zero EQUAL loss
(`build60-draw-raster/`). A larger 1,257-index read-only capture has identical
position bytes and projection constants (`build60-draw-inputs-large/`, 2,411 ticks).
The hardened `build60-draw-raster-defined-{texture,renderbuffer}/` controls both
cover 145,994 pixels with zero same-program/EQUAL losses and zero ALWAYS coverage
differences, GL error 0. All 1,257 numeric clip outputs are also bit-identical.
The live capture-run frame still shows terrain stripes and pale geometry.

**Do not use the preliminary ALWAYS counts as driver-defect evidence:** the
earlier large-pair probe used a separately relinked reference and incomplete
textures, with inconsistent texture-backed ALWAYS coverage. Both ambiguities
were removed in the hardened repeat. An Objective-C selector/ternary syntax
error in the first same-program build was repaired; only the rebuilt final
library/app supplies the hardened evidence. A slow ad-hoc pixel-counter process
was interrupted and replaced with a read-once inspection; no files were deleted.

**Stencil experiment:** native → bypass → restored native in one stationary
match (`build60-stencil-paired/`, camera 85.2/-157.8/0.6, 3,007 ticks) leaves
stripes/pale geometry in all reviewed source frames. Its temporary source toggle
was removed. Guest trace disabled-stencil values are not universal: native flags
show stencil enabled for the later captured pairs. `build60-draw-state-verified/`
captures 1,755 indices per pass, exact positions/projection, GL error 0; actual
color/depth attachments are textures (8 red / 24 depth bits), with depth, stencil,
scissor and culling enabled, and blend disabled/enabled for base/second pass.
Its reviewed match still has artifacts (1,201 ticks, no scripted shots).

**Next discriminating action:** inspect intervening depth writes and full
draw/texture state in the live striped scene. Passing isolated shader controls
do not establish original scene correctness or a software-driver defect.
Normal EQUAL is unchanged; all new capture/raster paths remain opt-in,
Simulator-only, with private generated data under ignored ref/.

**Final verification:** `build60-raster-normal-regression/` passes normal
menu/campaign-opening/scripted-match gates (1,235 ticks / nine test shots).
Opening cinematic and match frames reviewed; remaining terrain artifacts are
explicit. The frame-0 GL_INVALID_OPERATION diagnostic is also present in both
earlier normal regression builds; later sampled frames report error 0. No
claim of a wholly error-free renderer. 99 Python tests run, 16 skipped, no
failures; physical SDK syntax and tree/index safety guards pass. The rediscovered
real Simulator save directory (`EAA895CD-7568-4A43-A443-2EB3767DD8B2`) remains
byte-identical to the build-60 update backup after all reinstalls, probes, normal
checks and final About navigation. About shows accepted `bfbac357`, no PREVIEW;
Done returns to the normal picker (`build60-raster-final-picker.png`). Latest
upstream release is still build 60. No physical install, IPA, push or publication.
Goal stays active; full campaign/control/audio/human multiplayer/hardware gates remain.

## 2026-10-01 — live texture storage matches CPU uploads

**Progress:** extended Simulator-only, opt-in draw captures with framebuffer and
attachment identities, sampler/unit/mip/swizzle metadata and optional 2D level-0
readbacks. A separate READ framebuffer preserves the game texture and restores
read/pack state. Supported tightly packed RGBA8 uploads are copied for comparison;
level-0 TexSubImage invalidates the CPU reference. Python validation checks actual
file lengths and comparison bytes rather than trusting metadata flags. Generated
game textures/shaders/buffers remain under private ignored ref/.

**Evidence:** `build60-texture-state/` captures 1,257 indices per pass (1,312
ticks); both draws use read/draw FBO 3, color texture 1 and depth texture 2,
scissor 0/0/640/480, with no sampled attachment alias. This excludes a switched
target for that pair, not intervening depth writes. `build60-texture-pixels/`
captures 1,086 indices (1,297 ticks), with six reviewed level-0 previews. The
256x128 texture 83 has bands/atlas-like regions, but legitimate padding and raw
storage swizzles prevent treating appearance alone as decoder proof.

`build60-texture-upload/` captures 1,755 indices (1,292 ticks), exact position
identity, the same framebuffer/attachment/scissor values, complete captures and
GL error 0. All five unique supported 2D upload references equal GPU readback
bytes: base textures 86 (4x4) and 85 (128x256), second-pass 102 (256x256),
103 (512x512), 104 (256x256). All previews and the live screen were reviewed;
the screen still has terrain stripes/pale geometry. The later texture 85 is not
the earlier texture 83: random spawn changes which large draw pair is captured.
Raw previews do not apply texture swizzles. Cube faces/higher mips are not read,
and bound 2D objects need not all be sampled. GPU writes are not tracked, so a
future mismatch alone would not prove upload corruption or a driver bug.

**Next discriminating action:** compare CPU-decoded rectangular Xbox textures or
lightmap data with an independent reference, and inspect intervening depth
writes/full scene state. Matching upload/storage bytes does not prove decoding,
UVs, mip sampling or correct shading. No normal rendering change or EQUAL bypass.

**Final verification:** rebuilt installed app passes normal menu/campaign-opening/
scripted-match checks without render/input diagnostic flags in
`build60-texture-normal-regression/` (1,347 ticks / 11 test shots). Campaign opening
and match frames reviewed; terrain artifacts remain explicit. 105 Python tests,
16 skipped, no failures; physical SDK syntax and tree/index safety guards pass.
About shows accepted `bfbac357` without PREVIEW and Done returns to the normal
picker (`build60-texture-final-picker.png`). Rediscovered real data-container
UUID `8F069831-9623-40CA-9B02-E1E1FB56030A`; real Xbox saves match the build-60
update backup byte-for-byte after all reinstalls, probes and navigation. Latest
upstream lookup at 08:47 UTC still reports build 60, published 04:01 UTC.
No physical install, IPA, push or publication. Goal remains active; full campaign,
audio, human multiplayer and hardware gates are separate.

## 2026-10-01 — independent decode closes the rectangular RGB565 lead

**Prior goal turn: progress.** CPU upload/GPU-storage identity narrowed the
rendering investigation but did not verify the original Xbox decode. This pass
captures original level-0 RGB565 bytes and compares them with an independent
coordinate-bit Morton-address / RGB565-expansion oracle. The layout is checked
against [xemu's documented rectangular layout](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/swizzle.c),
not a copied/reused upstream mask/spread implementation. Asset-free fixtures
cover rectangular/degenerate address bijections, primary colors, linear padding,
malformed sources and actual differing pixels.

**Capture safety:** opt-in `XG_CAPTURE_XBOX_TEXTURES` requires pixel and shader
capture. The runner derives the cache symbol from the exact installed ELF after
manifest hash verification. Host reads the reviewed arm64_32 cache ABI through
bounded, failure-returning VM reads, validates GL object/format/dimensions and
physical source range, and copies source bytes without guest calls/writes.
Only RGB565 is independently supported. Cache layout is pinned-source-specific;
other formats and invalid records are explicit, not universal verification.
TexSubImage invalidates references; generated textures remain private under ref/.
An initial Simulator SDK syntax check rejected `mach_vm.h`; switched to supported
`vm_read_overwrite` plus a full-width-address assertion before the successful
build. An additional overflowing-symbol bound was added and rebuilt afterward.

**Concrete evidence:** `build60-xbox-source/` passes 65 stationary seconds,
1,350 ticks, no scripted shots, complete/error-0 1,086-index snapshots, exact
position identity. Texture 87 is swizzled RGB565, 256x128, 65,536 source bytes;
all 32,768 independently decoded pixels match upload and GPU storage, zero
differences. SHA256 `f4f84c68…946bc5f` equals the previously banded texture 83,
so this is a direct repeat of that lead. Preview and live frame reviewed; terrain
stripes/pale geometry remain. Normal pre-bound-hardening regression passes all
three gates (`build60-xbox-source-normal-regression/`, 1,303 ticks / eight shots),
with campaign-opening and match frames reviewed.

**Boundary / next action:** this verifies the selected texture's unswizzle and
RGB565 conversion, not the game's production of its source bytes, UV/shader/
sampling correctness, other formats, cube faces/mips or every scene draw. Next
inspect intervening live depth writes and remaining sampling/shader state.
Keep EQUAL unchanged; no renderer/driver-defect claim or normal render change.

**Hardened-build repeat:** `build60-xbox-source-final/` uses the rebuilt/reinstalled
symbol-bound-hardening source and captures another RGB565 texture, 73 (128x256).
All 32,768 pixels again match the independent decode, zero differences; complete/
error-0 1,080-index snapshots, exact positions, 1,354 ticks, no test shots.
Preview/live scene reviewed; stripes remain. Both rectangular orientations have
actual evidence, but no universal source-data or scene-fidelity claim.

**Final normal acceptance for this diagnostic change:** installed hardened build
passes normal menu/campaign-opening/scripted-match gates without render/input
diagnostic flags (`build60-xbox-source-final-normal/`, 1,305 ticks / ten shots).
Campaign-opening/match frames reviewed; visual defects remain explicit. 113 Python
tests run, 16 skipped, no failures; both Simulator capture syntax and physical
SDK syntax pass. Tree/index safety guards pass. About shows accepted `bfbac357`
without PREVIEW and Done returns to the normal picker
(`build60-xbox-source-final-picker.png`). Real save data in rediscovered container
`5311614A-26EA-48A5-B1A9-58D3B3D8E452` matches the update backup byte-for-byte
after all probes/reinstalls and final navigation. Upstream latest remains build
60 at 09:17 UTC. No physical install, IPA, push or publication. Goal remains
active; full campaign/control/audio/human multiplayer/hardware acceptance is open.

## 2026-10-01 — calibrated same-frame live depth observation

**Prior goal turn: progress.** Independent RGB565 decoding narrowed the texture
lead; this pass measures the live depth texture before/after selected native
base/EQUAL draws. Opt-in Simulator-only `XG_CAPTURE_DEPTH` samples into an owned
RGBA8 target, without writing game depth or relinking game shaders. It restores
program/VAO, targets, viewport, texture/sampler, enables, color mask and affected
pack/unpack state. Each snapshot validates the encoding path against an owned
32F depth texture containing .25/.5/.75/1. The validator requires calibration,
finite normalized values, matching target/frame, exact positions/projection and
a measurable base-draw response. These are float32 normalized samples, not raw
integer D24 storage. Private files remain ignored under ref/.

**Preliminary result:** `build60-live-depth/` measures intervening changes, but
lacks the subsequently added calibration/frame markers. The hardened validator
intentionally rejects it; it is not accepted same-frame causal evidence.

**Hardened repeat:** `build60-live-depth-calibrated/` passes 65 stationary seconds,
1,345 ticks, no scripted shots, complete/error-0 1,320-index captures with exact
positions/projection. All four depth observations calibrate, capture and restore
without GL errors, on texture 2 in presentation frame 120. Base draw changes
163,265 pixels; 54,517 change between base/EQUAL. Of the base-changed pixels,
21,611 later become closer, none farther; EQUAL writes no depth. Live scene
reviewed: terrain stripes/pale geometry remain.

**Boundary / next discriminating experiment:** closer surfaces may legitimately
occlude the terrain; these counts do not prove incorrect writes or a driver bug.
A changed-depth mask is not complete raster coverage. Correlate original native-
linked EQUAL coverage with copied live depth and full captured pixel state,
distinguishing valid occlusion from incorrectly missing visible fragments. Do
not relax EQUAL. The accepted pin remains build 60 (`bfbac357`); upstream latest
was still 60 at 09:31 UTC. No physical install, IPA, push or publication.

**Final regression / UI / preservation:** `build60-live-depth-normal-regression/`
passes normal menu, campaign opening and scripted match (1,303 ticks / eight
shots), without input/render flags. Frames reviewed; visual defects remain.
122 tests run, 16 skipped, no failures; Simulator/physical SDK syntax and
tree/index safety pass. Cold launch, About (accepted `bfbac357`, no PREVIEW) and
Done return to the normal picker, captured in `build60-live-depth-final-picker.png`.
Real Xbox saves in rediscovered container `2D21FB1C-C826-4A94-8715-68D38276A18C`
remain byte-identical to the build-60 update backup after all installs/probes/UI.

**New upstream while testing:** at 09:53 UTC latest is now
[build 61](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-61),
`f8937c6179757774c75f4e7d36de446fabd3dcc8`, published 09:40 UTC, "Add high-res HUD
textures". Build 60 stays accepted; next pass reviews/freezes 61 and runs the
save-backed Mac/Simulator update gates plus visible HUD/menu review before
promotion. Do not mix 60 diagnostic results into 61 acceptance. Goal remains
active; full campaign/control/audio/human multiplayer/hardware gates stay open.

**Update hazard found during review:** 61 adds `hires` to the texture description
and `override` to cache entries. The existing optional original-byte reader is
therefore ABI-specific, even when `texture_buckets` resolves correctly. Added a
runner guard allowing that diagnostic only for the reviewed full build-60 SHA,
with an asset-free rejection fixture for 61/unknown revisions. Review/adjust the
ABI before enabling original-byte capture on 61; normal update gates do not use
that diagnostic. Final Python suite after this guard: 123 tests, 16 skipped.

## 2026-10-01 — build-61 candidate, HUD and copied checkpoint

**Prior goal turn: progress.** Calibrated live depth observation established
same-frame intervening changes, not a causal renderer fix. Upstream released
build 61 while that frozen pass ran. Latest lookup at this pass's start still
reports 61 (`f8937c6179757774c75f4e7d36de446fabd3dcc8`). Reviewed the exact new
HUD sources, bitmap tag lookup, PNG embedding, renderer/sampler changes and
cache ABI. All fetched code/assets/generated translation remain private under
ignored ref/. No upstream executable was downloaded.

**Candidate gates before promotion:** normal update without `--accept` backs up
saves to `save-backups/20261001-185807-from-bfbac357`, compiles 61 locally, and
passes all six Mac/Simulator menu/campaign-opening/scripted-match cases. Mac
2,130 ticks / 16 shots; Simulator 1,324 ticks / 11 shots. Evidence under
`smoke-results/20261001-185807-f8937c61/`,
`simulator-results/20261001-185807-f8937c61/` and
`passes/2026-10-01/build61-candidate-update.log`. All captured frames reviewed;
menu and HUD are visible, but Simulator terrain stripes/pale geometry remain.
Defaults enable `display.high_res_hud`; campaign/match recognize all 15 HUD
bitmaps, with no decode-failure log. This is not a universal fidelity/performance
acceptance. The script restores the build-60 checkout/Mac build afterward;
installed 61 remains correctly labeled PREVIEW.

**Normal navigation / isolated save:** cold launch without a forced engine/map
shows both edition cards. About identifies `f8937c61` as an incomplete preview;
Done returns to picker. Xbox → Campaign (touch A) → copied New001 → Pillar of
Autumn → Normal recognizes an in-progress game and restores the cryo-bay/look
tutorial, not the opening ship cinematic. Private evidence:
`campaign61-compat.xCtR9E/checkpoint.png` and logs. The save is copied from the
earlier build-60 compatibility fixture (originally a build-59 checkpoint), not
the real player's directory. One checkpoint is not all-save compatibility.
Real Simulator saves in rediscovered container
`8A7EC32F-70B5-45D5-BB07-E366B28F8431` remain byte-identical to the update backup.

**Update report hardening:** its dependency diff previously emphasized Android
imports and missed the important renderer/cache changes. Expanded that report
to the platform renderer, HUD tag hook, cache load hook and asset embedder. The
next acceptance run visibly lists the new cache/HUD/sampler files. Existing
original-byte diagnostic refuses 61 until its cache ABI is reviewed/adapted.
`sh -n`, 123 Python tests (16 skipped) and physical SDK syntax pass. A separate
save-backed `--accept` repeat is running; no promotion claimed here yet.

**Acceptance repeat:** `build61-accept.log` and both result folders at
`20261001-191048-f8937c61` repeat all six cases successfully: Mac 2,130 ticks /
17 shots; Simulator 1,358 ticks / 11 shots. Simulator menu/campaign/match and
Mac match frames reviewed; HUD visible, existing fidelity defects explicit.
The accepted lock now pins full `f8937c61`. The source checkout is clean at 61.
App rebuilt/reinstalled without `XBOX_REV` after promotion, evidence
`docs/artifacts/2026-10-01/G3/ios-app-20261001T101755Z`, private log
`build61-final-app.log`. Final normal regression (`build61-final-normal/`) passes
all three Simulator cases (1,348 ticks / nine shots). Goal remains active, not
full campaign, audio, human multiplayer or hardware acceptance.

**Final readback:** final accepted-app campaign/match frames reviewed; defects
remain. About shows accepted `f8937c61` without PREVIEW; Done returns to the
normal edition picker (`build61-final-picker.png`). Windows card still reaches
the existing missing-data/setup screen (`build61-windows-setup.png`), not Windows
gameplay or license provisioning. Real Xbox saves in final rediscovered data
container `12A61E04-970B-439E-A567-C0AFA5DD5F54` match the acceptance backup
byte-for-byte after all installs/probes/menu navigation. Latest upstream is still
61 at 10:24 UTC. 123 tests, 16 skips, no failures; both SDK syntax, shell syntax,
whitespace and tree/index safety pass. No physical change, IPA, push or
publication. Next resume the native-linked coverage/live-depth experiment on
the frozen 61 pin, then sustained touch/weapon/later-checkpoint acceptance.

## 2026-10-01 — Frozen build-61 native pixel replay

Previous goal turn was progress: accepted 61, save-backed gates and local
commit `ac40a2f`. This pass retains that exact pin and the dedicated Simulator.
Chris's new physical-iPad report of focus/shading instability is a visual gate,
not directly observed here. No physical changes or publication are authorized.

Added opt-in native framebuffer replay in `xg_depth_capture.m`, gated through
the existing draw-capture runner. Original linked program/VAO/uniforms/textures/
samplers/blend/stencil/raster state are retained; live color/depth/stencil are
copied to owned texture-backed targets. EQUAL/ALWAYS/EQUAL-repeat cannot write
game targets. A calibrated sample of the copied depth must match live depth,
and native repeat/actual live color must match exactly. Metadata, frame,
program/target, response and file-layout failures reject the diagnostic.
Normal depth semantics remain unchanged.

First Objective-C syntax attempt rejected an ARC goto crossing an object
initialization; moved it before the guard. First synthetic test inheritance
duplicated a conflicting fixture test; made native cases independent. Both
repaired before final tests. First live renderbuffer clone is rejected on
1,161 small channel-value differences versus real color despite exact repeat.
Changed private target storage to textures, matching the game's attachment
kind. Next run captured no qualifying >=1,000-index pair at its different
spawn, correctly rejected. Final >=144 threshold captures a 1,944-index pair;
position/projection identity and all GL/calibration/restoration controls pass.

Private evidence: `ref/xbox-build/passes/2026-10-01/build61-native-calibrated/`;
library/app logs have the same prefix, app evidence
`docs/artifacts/2026-10-01/G3/ios-app-20261001T105149Z`.
65-second stationary match, 1,091 ticks, zero scripted shots. Presentation
frame 96; original depth texture 2, cloned depth 136. Calibrated copied depth
and native EQUAL/repeat/live color are byte-identical. Base changes 60,763
depth samples; later depth changes 176,212, including 24,262 closer base-written
pixels. EQUAL has no depth changes. EQUAL color response 36,501; ALWAYS 61,643.
Full color difference 27,701 is not missing coverage. ALWAYS-only response
25,142: 24,262 at base-written/later-closer locations and zero at unchanged
base depth; 880 outside the changed-depth mask. Reviewed original native
EQUAL/ALWAYS and final gameplay images. ALWAYS overdraws a large hidden surface;
normal scene still striped/pale. This pair does not prove incorrect EQUAL.

Next target the material draw that actually introduces a visible stripe,
retaining exact native replay/copied-depth controls. Do not adopt ALWAYS,
declare a driver bug, or equate test/menu/save gates with visual correctness.

Final normal regression `build61-native-normal/`: all three cases pass, 1,180
match ticks / nine shots, no render/input diagnostic flags. Campaign/match
screens reviewed; graphics still defective. Actual About/Done navigation
confirms accepted `f8937c61` without PREVIEW and leaves the normal picker.
Final real-save container `509882A5-2652-4667-A5B0-EAB43C24BFE7` matches
`20261001-191048-from-bfbac357/simulator-save` byte-for-byte. 132 tests, 16 skips,
no failures; Simulator and physical SDK syntax pass. No physical install,
pin update, IPA, push or publication. Goal active; this turn is diagnostic
progress, not a rendering fix or completion.

## 2026-10-01 — Original draw timeline finds the stripe-producing detail pass

Previous turn was progress (`f73a23c`): calibrated native pixel/depth replay,
with visual defects explicitly open. This pass freezes accepted 61 again.
Added bounded base-batch skip and a one-frame original color timeline, recording
indexed and immediate GL draws without changing render state/shaders/uniforms.
The runner validates/canonicalizes frame/skip options; timeline guards cover
metadata, unchanged state, same frame, shader sources, complete file lengths,
paired/contiguous records and overflow. Reused the existing state-restoring
color readback rather than adding another renderer.

App source build/install log `build61-color-trace-app.log`, evidence
`docs/artifacts/2026-10-01/G3/ios-app-20261001T111331Z`. Private diagnostic
`build61-color-timeline/`: 90 seconds, 2,015 ticks, no scripted shots. Selected
second eligible 984-index VS17/VS41 pair in frame 101. Depth controls and exact
positions/projection pass, but native clone repeat differs at 682 pixels and
live color at 9,932. **Rejected native replay; aggregate diagnostic fails.**
Do not describe that as a successful full-pixel replay or adopt an EQUAL bypass.

Independent original timeline: all 211 before/after draw pairs in presentation
frame 120 pass its validator, GL error 0, framebuffer 3, unchanged recorded
state. Reviewed RGB contact sheet and full-resolution stages; disable alpha
compositing for inspection, since early scene passes intentionally modify alpha.
Draw **119** changes the rear walls from unstriped to obvious horizontal bands:
program 84, 402 indices, GL_TRIANGLES, GL_EQUAL. Exact captured sources match
VS7 (`vs007_0.glsl`) and `ps_0c014f79.glsl`. It changes 51,756 color pixels, not
51,756 proven defective pixels. The selected VS41 draw 80 textures ground;
the actual stripe-producing detail pass is later and uses VS7.

Next capture the corresponding VS17/VS7 geometry/constants/pixel state and
calibrated depth, with exact native repeat/live controls where valid. Earlier
VS41 controls cannot establish VS7 invariance. This is symptom localization,
not a driver-bug finding or a visual fix. Goal active; physical-iPad report
remains open, with no physical changes, pin promotion, IPA or publication.

Final normal `build61-color-normal/`: automated menu/campaign/match gates pass,
1,254 ticks / ten shots, diagnostics disabled. Campaign and match screens
reviewed; severe missing terrain/background exposure remains, so do not call
this visual acceptance. About confirms accepted `f8937c61` without PREVIEW;
Done returns to the normal picker. Real Xbox save container
`1A5AEFF0-09B3-47DB-94BD-D8C371C56AEC` matches the acceptance backup
byte-for-byte after final navigation. 140 tests, 16 skipped, no failures;
both SDK syntax and whitespace/tree/index safety pass. This turn is progress
through original-draw localization, not a graphics fix or goal completion.

## 2026-10-01 — VS7 coordinate replay passes; native material samples remain inconclusive

Previous turn `03ea061` localized the wall stripes to an original VS7 detail
draw, not the earlier VS41 material pass. This pass freezes accepted build 61
and adds a small diagnostic-only equal-shader selector, whitelisted to VS7/VS41
and validated before launch. Default capture remains VS41. Normal rendering,
depth semantics, private Xbox inputs and accepted upstream pin are unchanged.
Source-built Simulator library/app pass; app evidence
`docs/artifacts/2026-10-01/G3/ios-app-20261001T113653Z`.

Three stationary VS17/VS7 captures have exact position/projection identity and
passing calibrated same-frame depth controls. The 228-index frame-104 and
531-index frame-108 pairs repeat/match native live color exactly but have zero
EQUAL/ALWAYS color response: rejected as inconclusive. The second eligible
1,686-index frame-103 pair fails exact native repeat/live color by 43 pixels
each: rejected, not a usable ALWAYS-versus-EQUAL causal comparison. At
base-written pixels, later closer depth counts are respectively 23,876 / 11,202
/ 17,649, with zero farther; EQUAL changes no depth. These samples are not
established as the earlier visibly stripe-producing material.

Private evidence `build61-vs7-live/`, `build61-vs7-first/`,
`build61-vs7-second/`; strict guard failures deliberately leave their aggregate
results failed. `build61-vs7-clip/` replays the saved 1,686-index pair on the
Simulator with transform feedback: all coordinates bit-identical, zero changed
vertices and zero component/NDC-depth deltas. Replay changes linkage, so this
does not prove original-program invariance. The simplified raster probe still
rejects the pixel shader's discard path; no raster proof or guard relaxation.

Next select the actual color-changing striped material and correlate original
before/after color with its texture/alpha inputs before another native replay.
Do not confuse shader identity with material/visibility identity. Rendering
remains defective; Chris's physical-iPad report remains open and was not directly
observed or changed. This is diagnostic progress, not a graphics fix. Goal active.

Final normal `build61-vs7-normal/`: all three automated cases pass, 1,261 match
ticks / nine shots, no render/input diagnostics. Campaign/match frames reviewed;
severe wall banding remains. Actual About/Done confirms accepted `f8937c61`
without PREVIEW and leaves the normal picker. Real saves in container
`DA23CE8C-1C94-4472-9788-C9AA87C62AA2` remain byte-identical to the acceptance
backup. 141 tests, 16 skipped, no failures; both SDK syntax, whitespace and
tree/index safety pass. No physical install, pin update, IPA, push or publication.

## 2026-10-01 — Visible wall-band draw has stable material inputs

Previous turn `06c7dbf` made progress through VS7 coordinate replay, with all
sampled native pixel results rejected and graphics still defective. Freeze
accepted 61 again. Added a read-only exact-source material observer to the
original one-frame timeline, retaining separate before/after sampled textures,
sampler/filter/swizzle state, alpha/combiner and vertex/UV constants. Reuses
the existing texture readback and validation; normal rendering is unchanged.
Runner requires both color-frame and texture flags. Asset-free tests cover
missing/changed/cross-frame/error material data and nonfinite uniform bits.
Host equal-shader whitelist now explicitly loads no target for invalid names.

`build61-material-timeline/`: 90 stationary seconds, 2,083 ticks, zero shots;
all diagnostic gates pass. Original frame 120 validates 211 before/after draw
pairs and 17 matching VS7/`ps_0c014f79` material pairs. Reviewed draw 117,
program 82, 402 indices, EQUAL: unstriped rear walls become banded, 51,756 color
pixels change. Its two 16x16 gradient textures are linear/clamp-to-edge, base
and max level 0; sampled bytes match CPU uploads and stay identical through
the draw. Recorded uniforms also stay identical, alpha reference 0. This
excludes in-draw changes/upload corruption for those samples, not all texture
decoding, UV/interpolation, shader or depth faults. The small textures are not
proof of placeholder assets or missing imports.

Added an optional exact-index-count filter (3..100000), unchanged default.
`build61-material-402/` uses it with native/depth/material flags for 90 seconds,
2,034 ticks, zero shots. Different spawn has no 402-index pair, so its paired
and native-depth tests **fail without a capture**. The independent trace still
validates: 294 draws / 22 unchanged material pairs. Reviewed draw 176 (1,377
indices) again introduces bands, 12,988 changed color pixels. No new native
pixel/depth proof. Counts alone cannot select a repeatable scene; next attach
the probe to the visible material draw inside the same original frame, or
compare a correctly built Simulator ANGLE/Metal candidate. Current logs identify
Apple Software Renderer, not the physical Windows edition's Metal route.

App evidence `docs/artifacts/2026-10-01/G3/ios-app-20261001T120429Z` and final
filtered app `ios-app-20261001T120952Z`; matching private logs/pass folders
retained. No upstream pin change, physical install, IPA or publication. Goal
active; this turn yields stable material data at the actual symptom, not a fix.

Final normal `build61-material-normal/`: all three automated cases pass,
1,295 match ticks / nine shots, no render/input diagnostics. Reviewed campaign
and match frames: severe banding and pale/missing terrain remain, not visual
acceptance. Actual About/Done confirms accepted `f8937c61` without PREVIEW and
returns to the normal Windows/Xbox picker. Real saves in rediscovered container
`647C8BF3-6DC3-48A5-81A2-89654BF1AEE5` match the acceptance backup byte-for-byte.
150 tests, 16 skipped, no failures; both SDK syntax, whitespace and tree/index
safety pass. Upstream checkout clean. No physical install, pin promotion, IPA,
push or publication. Goal remains active with graphics/audio/controllers and
broader campaign/hardware acceptance still open.

## 2026-10-01 — Source-built Simulator Metal preview improves sampled scenes

Changed the experiment from repeated Apple depth probes to a proper ANGLE/Metal
Simulator build. Reuses official WebKit ANGLE source lists, independently pinned
at `a1fb7ce122d0cd99f7d6cc82775f02565e266ece` / ANGLE `eb725ace1839`, compiled
with the actual Simulator SDK. Source remains outside GitHub; no upstream edits.
Opt-in renderer has its own archive/manifest and EGL/CAMetalLayer path. Apple
and physical defaults and Xbox guest pin 61 remain unchanged. Packaging verifies
renderer/source/hashes, refuses physical ANGLE and marks the build PREVIEW.

Initial real match images lack the pronounced horizontal wall bands but swap
red/blue. Asset-free tests isolate that: equal-depth and swizzle-independent
blits pass, texture sampling silently ignores R/B swizzle. ANGLE explicitly
disables the native swizzle feature on Simulator. Enabling it through EGL passes
the sampling control on this Mac/iPadOS 26.5; confined to the optional preview.
Rebuilt menu/campaign/scripted-match gates pass (1,530 ticks / 12 shots), and
reviewed images restore colors without pronounced cliff bands. A 45-second
stationary follow-up passes (933 ticks / zero shots) and shows the same sampled
improvement. Unmatched viewpoints, broader motion, later campaign, audio and
hardware remain unaccepted. Retain the one startup GL blit error as an open lead.
The first cold menu's black dump failed honestly; later capture passes with a
30-second ANGLE-only allowance, not a relaxed image gate.

Evidence `ios-app-20261001T130852Z` and private `build61-angle-swizzle*` pass
folders. Final picker/About/Done verified with Xbox PREVIEW, real saves unchanged
byte-for-byte in rediscovered `6363645B-5CB1-48F7-A108-B6E3D2810148`.
Accepted Apple app preserved privately for rollback; scratch ANGLE dependency
retained for rebuilding. 157 tests / 16 skipped, both Apple SDK syntax and ANGLE
Simulator syntax pass; physical ANGLE explicitly rejected. No physical iPad
changes, IPA, pin promotion, push or publication. Goal active; next fixed copied
checkpoint/motion comparison and normal controls/reload. Chris's physical
Windows/Metal shading/focus report is not resolved by these Xbox Simulator tests.

### 2026-10-01 — copied checkpoint renderer A/B and actual preview controls

Same frozen Xbox 61, same isolated checkpoint bytes, ordinary Campaign menus and
untouched cryo-bay view at 640x480: Apple has large black floor polygons and wall
bands; ANGLE/Metal draws the floor without those pronounced bands. Static edges
align, NPC/prompt timing differs; not pixel-synchronized or a proven driver cause.
Preserved screenshots/PPM/logs under private `angle-checkpoint.B09wUK` pass root.

Restored preview in place. Four-direction touch look advances the tutorial; X
exits the tube; a short forward drag reaches the guest and displaces the view.
Actual pause-menu Save and Quit finishes and returns to the main menu. A new
process reloads through picker/Campaign into the last cryo-bay checkpoint, not
the ship intro or unsaved exit position. The Metal save copy changes during that
load; original fixture and Apple copy unchanged. No later checkpoint claim.
Real Simulator saves match the acceptance backup; no physical device change.
No renderer code or guest pin change. Keep PREVIEW and Apple fallback. Next
sustained movement/look, simultaneous controls, later checkpoint/reload and
broader image review. Chris's physical iPad shading/focus report remains open.
98 Xbox tests, whitespace and tree/index safety pass; upstream and ANGLE source
checkouts clean. Final picker/About/Done rechecked with PREVIEW, real saves still
byte-identical. Evidence-only changes, no IPA, push or publication. Goal active.

### 2026-10-01 — later Halo campaign and explicit moving-camera diagnostic

Normal ANGLE PREVIEW picker/Campaign/Halo/a30 flow reaches first-person pod play.
Actual RT taps reduce rifle 60→58 and Y swaps to the pistol. Short forward drags
are not sustained walking. Save and Quit completes; a fresh process recognizes
Halo in progress and restores its pod checkpoint with rifle 60. Separate save
copy throughout; no claim of later mission progression or unsaved-state restore.
Private evidence `angle-later-campaign.Yu2u87`, including before/save-quit/cold
captures and logs. No engine/UI changes or app installation.

Added bounded a10/a30 selection to Simulator smoke and explicitly diagnostic
campaign bot motion. Requires the targeted campaign plus rendering diagnostic;
results label the input mode, check the requested map, and ordinary menu/campaign
passes clear inherited bot/network-test settings. 90-second scripted a30 pass
reaches outdoor valley views with firing and movement, lit 0.941, renderer and
presentation captures passing. Reviewed terrain/tree/ring views lack pronounced
bands; sampled screenshots are not temporal or physical visual acceptance.
Startup blit error remains open. Normal menu/a10/scripted-match regressions also
pass (1,530 match ticks / 12 shots). 103 Xbox unit tests, whitespace and safety
pass. Ordinary two-edition picker restored with PREVIEW; real saves byte-identical
to backup, upstream/ANGLE source clean. No physical install, IPA, push or release.

Live upstream check finds four new commits through `c55e4e2` (13:21:22 UTC):
expanded HUD/scopes, meter coverage-alpha, CRC replacement guards and widescreen
flat menu fills. Read-only patch review identifies increased potential HUD memory
and another texture-description ABI change. Recorded as next save-backed candidate;
not installed/promoted. Keep physical iPad shading/focus report separate and open.
Goal active: next candidate build/memory/HUD/save checks, then sustained human
controls, genuinely new checkpoint progression and moving-image review.

### 2026-10-01/02 — build 64 update, HUD and cross-pin pod reload

Frozen upstream `c55e4e2b` / build 64, reviewed its expanded HUD, CRC guards,
coverage-alpha shader fields and flat menu fills. Candidate Mac/ANGLE Simulator
menu/a10/scripted-match pass; guarded acceptance rerun passes again (Simulator
1,530 ticks, 12 shots). Save-backed pin now build 64, experimental only. Both
Apple-default and opt-in ANGLE libraries rebuilt against the exact new guest;
renderer pins remain independent. Outgoing/candidate app copies retained.
Explicit unit fixture keeps old texture-cache byte diagnostics rejected for
the changed ABI. 103 Xbox unit tests pass.

Normal menus load a copy of the build-61 Halo/a30 checkpoint. Actual RT reduces
rifle 60→58, Y switches to pistol, Zoom draws a circular 2x scope, pause panel
and Save and Quit work. Cold process recognizes Halo in progress and restores
pod with rifle 60; not the unsaved shots/pistol. Narrow snapshot fixture result,
not new progression or general save compatibility. Real saves remain separate.
`top` menu/pod samples 145M/172M; ps RSS is a different metric. `vmmap` failed,
so no physical-footprint result. A separate 90-second scripted a30 diagnostic
passes, lit 0.935, with outdoor terrain/tree/ring/HUD sample reviewed. No full
temporal, sniper-meter, audio, human-control or hardware acceptance. Existing
startup blit and source-build warnings recorded. Physical Windows/Metal
shading/focus remains open; no device install, IPA, push or publication. Goal
active. Evidence under private `upstream-c55.foFBTU` pass root.

### 2026-10-02 — a50 night/sniper and moving-image evidence

Added a50 to bounded Simulator smoke and its fail-closed map/case tests.
90-second scripted rendering diagnostic passes, lit 0.577, ANGLE identity,
presentation captures and 69/69 HUD. Retained video and sparse/dense contact
sheet samples show damage flashes, weapon/reload and night terrain; this is
sampled temporal evidence, not continuous stability or reference acceptance.
No renderer change. Normal menu new a50 play separately verifies sniper 2x/10x
scope and night vision. Mostly ground-facing scope views draw ladder/border
without obvious missing sections; finite look drags do not prove sustained pans.

An initial partial a50 snapshot copied into an a30 profile did not establish
normal continuation. Actual new-level menu launch followed by Save and Quit
does: Saving completes, fresh process recognizes Truth and Reconciliation in
progress and restores its opening checkpoint. Not a newly reached later
checkpoint or general cross-pin compatibility. Real save directory still
matches both backups. 103 Xbox tests pass, upstream/ANGLE checkouts clean;
live remote HEAD remains accepted build 64. Physical iPad shading/focus still
open. No physical install, IPA, push or publication. Goal active. Private
evidence: `temporal64.wTBR3I` pass root; recordings have no audio stream.

### 2026-10-02 — output-callback audio diagnostic

Previous Simulator videos carry no audio. Added opt-in Simulator-only bounded
capture of HaloPad's own float output: skip ten seconds, retain four, allocate
at setup, no callback file I/O, completion handoff to game-thread writing.
Analyzer fails closed on missing/truncated/invalid/nonfinite/silent captures;
counts starvation/range excursions without claiming quality or sync. Six new
tests (including actual native helper under sanitizers) bring Xbox suite to
109 passing tests. Normal iPhoneOS source syntax check passes; no device build.

Frozen build64/ANGLE one-app candidate rebuilt and installed in place after
real-save copy. Guest/renderer pins unchanged. Menu and normal-input a50
launch audio windows deliver finite/non-silent 48 kHz stereo, RMS 0.1805/0.1066,
peak 0.8450/0.6093, zero counted starvation and out-of-range samples. No speaker,
effect fidelity, deadline or sync acceptance. Normal no-capture menu/a10/match
regressions pass (1,560 ticks, 13 shots). Private pass `audio64.U2hPbE` retains
app, logs, screenshots and samples. No physical install, IPA or publication.

Read-only PC sampler review finds ignored mipmap LOD bias, but no actual
nonzero-use evidence in the checked physical logs. Record as a targeted fidelity
experiment, not a diagnosis of the shading/focus report. The Xbox preview and
PC renderer are distinct paths. Goal stays active for full graphics, audio and
controller/hardware acceptance.

Final ordinary launch restores both edition choices; About/Done provenance
round-trip passes. Real Simulator saves in the rediscovered `074F7EDF` container
match this pass's full pre-install copy and the original acceptance backup.
Whitespace and tree/index guards pass; upstream/ANGLE sources remain clean.

### 2026-10-02 — PC sampler lead and missing Mac host input API

PC component baseline first fails to link on `halopad_host_post_input`, newly
required by shared touch cancellation. Restored the Mac API via its existing
synchronous AppKit/Halo-main-thread delivery; not a stub or extra queue. iOS
queuing unchanged. Native input harness now exercises this actual API on both
platforms. All 264 native Mac checks pass, no physical-controller claim.

Fresh-state PC Blood Gulch component runs reach first-person frame 300, present
330 frames and exit normally. Existing bounded draw trace now records sampler
filters/LOD/aniso. Full frames 300–301: 248 requests, 932 bound observations,
all LOD bias zero. Ignored bias is not active in this sample; no cause or fix of
the physical shading/focus report established. Bound/request counts are not
GPU submissions; stationary screenshot is not temporal fidelity. No sampling
or depth workaround adopted. Complete logs under G3 `…162406Z`, `…162729Z`;
native input under `…162857Z`, private pass `pc-lod.c5yJyx`. Microsoft/Apple
reference review supports a future shader-side bias path if actual use is found.
No installation or physical data change. Goal stays active.

### 2026-10-02 — native Xbox disc import and actual stale-archive rejection

Private `disc-import.1pB6xv` pass hardens bounded XDVDFS/map validation, unique
staging and exclusive publication; 17 native fixture regressions pass. Real own
USA disc and 104 bounded synthetic cases run without ASan/UBSan findings. First
integrated UI attempt exposes an old linked Xbox archive and wrongly accepts a
path-traversal fixture only in an isolated root. Add runtime source hashes to
library manifests; missing/changed hashes now reject packaging, with two tests
and a real rejected build. Rebuild the frozen library and main app normally.

Corrected Simulator executable `5ea1369d…5768df9`: actual malformed-image
rejection, Editions return, real Files disc import, 24 exact map hashes, fresh
New001 creation, Normal a10 cinematic, cold menu/profile recognition. Fresh
summary The Maw/Legendary is unexpected and remains unexplained, not progression.
Normal PC route retains the unaccepted EULA; leave final ordinary app at picker.
All existing game bytes survive; normal logs/caches and last-edition preference
change. 128 Xbox tests, five input guards and 13 executed package checks pass
(16 package checks skipped). No physical operation, visual fix, pin update, IPA,
push, publication or cleanup. Goal stays active; detailed failed/successful
attempts and proof boundaries are in [the pass ledger](XBOX-SIMULATOR-PASSES.md#native-disc-import-validation-and-stale-library-gate-2026-10-02).

### 2026-10-02 — upstream profile semantics and Xbox focus-loss reset

Private `profile-lifecycle.BXKtCE` pass resolves the fresh The Maw/Legendary
summary: all ten saved flags are `0x0f`, matching upstream build 64's explicit
new-profile all-difficulty unlock and its summary's highest-level logic. No save
rewrite or retail completion claim. Earlier outside-pod checkpoint proof stands.
Official remote HEAD readback still matches frozen `c55e4e2b`.

Corrected asset-free actual UIKit-handler fixture reproduces six focus-loss
failures before the fix. Its first version wrongly relied on control dispatch
without UIApplication; retain that failed fixture separately. Xbox touch pad
now clears live/unread input and ownership on deactivate/activate, refuses
inactive/hidden presses and requires fresh touches. All 16 handler checks pass,
including short Start taps, normal cancellation and controller hiding/removal.
This is handler-boundary simultaneous state, not OS-held multi-touch routing.

Rebuilt library/main app `58111bcb…29f093` installed in place only on dedicated
Simulator. Actual normal-menu copied outdoor a30 checkpoint → RT 60→58 → Home
→ same-PID 91997 resume → fresh RT 58→57 → Y/Zoom/finite look/move/Start work.
Both actual lifecycle reset logs appear; Save and Quit completes. Inputs release
before Home, so no physical-held interruption claim. Empty XG_DATA/SAVE makes
the first ordinary launch select an empty development path; unset them to
restore Play Xbox. PC validates files and reopens unchanged unaccepted EULA.
Final ordinary app is left at both cards. Full audit preserves all real Xbox/PC
game bytes, preferences and PC registry; only normal logs/caches/system snapshots
and scene metadata change. 128 Xbox, five input guards, 13 executed package
checks plus 16 native UIKit checks pass (16 package checks skipped). No physical
change, visual fix, pin update, IPA, push, publication or cleanup. Goal stays
active. See [profile/lifecycle pass](XBOX-SIMULATOR-PASSES.md#upstream-profile-summary-and-xbox-touch-focus-loss-2026-10-02).

### 2026-10-02 — live PC ground-material color arithmetic

Private `pc-live-color.L5KDQS` fixture borrows actual encoded Battle Creek
geometry, constants, textures and samplers through the existing diagnostic hook.
Independent float targets capture four samples, four interpolated coordinates,
fog and unmodified pixel-shader output; original target bytes remain unchanged.
Initial compile has an array-name collision; first linked run passes host checks
but spawns indoors and fails the exact-material gate. Preserve both attempts.
Second bounded run `G3/…215644Z` exits zero with 24 assertions/30 motion frames.
Three ground draws cover 1,396,060 pixel observations; an independent original
bytecode interpreter matches all RGBA values exactly. Base alpha is one and fog
off in this sample; synthetic calibration covers mixed alpha/fog, not gameplay.
Three analyzer tests (including five bad-capture subcases) pass and a deliberate
color residual is detected. Reviewed derived contact sheet still shows grain
in both the original target and independent material/detail sample images.
Sampling/interpolation are observed inputs, not independently verified; no
depth/blend visibility, original-driver, temporal or physical acceptance claim.
No tracked runtime change or installation. Ordinary integrated app resumes at
PID 95450 with the unchanged `58111bcb…29f093` executable. Full audit preserves
all real game bytes, preferences and PC registry; normal logs/Metal caches and
system snapshots change. Goal active. Next matched affected/reference rendering,
not a speculative shader/filter fix. See [live material pass](XBOX-SIMULATOR-PASSES.md#pc-live-ground-material-arithmetic-2026-10-02).

### 2026-10-02 — empty launch overrides and save-backup regression

Private `launch-overrides.x2mHge` pass fixes the previous empty-variable Add Disc
failure with three nonempty-path checks in HaloPadXbox.m. New asset-free native
fixture includes actual launch/backup helpers with synthetic paths, isolated
defaults/bundle and inert engine; first fixture compile has covariance/nullability
warnings, corrected without changing production behavior. Baseline has 12 failed
checks, including cascades after the missing backup, not 12 independent defects.
Fixed source passes all 20 checks: ordinary/unset/empty/nonempty paths, exact
save copy, same-pin no-op, isolated marker, copy failure and retry. Existing
16 UIKit checks and 128 Xbox Python tests pass, as do five input guards and
13 package checks (16 package checks skipped).

Preserve full Documents/Library and outgoing installed/builder apps; rebuild
normal PC-entry app without a scene. In-place install only on dedicated Simulator:
executable `e594a1f9…15feed`, frozen Xbox/ANGLE libraries unchanged. Actual
empty-variable picker shows Play Xbox, About/Done works, ordinary Windows route
validates existing files then reopens original unaccepted EULA. Never Accept.
Final ordinary picker PID 8646. Real Xbox runtime never opens saves; full audit
preserves game bytes, preferences and PC registry. Logs/system snapshots/scene
metadata change. No physical operation, graphics fix, pin update, IPA, push,
publication or cleanup. Goal active; [full evidence](XBOX-SIMULATOR-PASSES.md#empty-launch-overrides-and-revision-backup-regression-2026-10-02).

### 2026-10-02 — fail-closed maintenance save backups

Private `update-backups.Z0EiBQ` pass reproduces four maintenance gaps: ignored
checksum failure, accepted corrupted copy, swallowed/empty Simulator container
lookup and reused same-second backup. A frozen HEAD script fails five of nine
independent controlled tests; fixed source passes all nine. One earlier rerun
spans a source edit and is explicitly excluded; later subcases are split to
avoid a baseline promotion contaminating the following case. Unique mktemp
folders, real byte/directory comparisons and required checksum creation/readback
now gate candidate actions. Inert Git/build/device boundaries test orchestration,
not actual compiler/smoke acceptance or upstream promotion. Empty saves remain
valid only after successful container inspection.

Execute only the backup block against actual own Mac and dedicated Simulator
save folders: 121 checksums verify in retained `save-backups/…072643…jZyQEJ`.
No fetch/build/pin acceptance. Ordinary picker restored, PID 25509, executable
still `e594a1f9…15feed`; guest/library/pin unchanged. Retained full-file audit
preserves real game/disc/package bytes, preferences and PC registry, with normal
log/system snapshot changes. All 137 Xbox Python tests pass. No physical operation,
installation, graphics fix, IPA, push, publication or cleanup. Stop games during
maintenance; directory comparison is not atomic snapshot or save-format proof.
Goal active; [maintenance evidence](XBOX-SIMULATOR-PASSES.md#update-routine-save-backup-failure-gates-2026-10-02).

### 2026-10-02 — separately built iPhoneOS ANGLE preview, no installation

Private `angle-device-build.wGZhkZ` pass extends the pinned renderer's build-only
path to iPhoneOS. Preserve outgoing libraries and older device-app output (which
contains an older PC-only IPA); no cleanup. SDK/feature metadata and separate
archives reject cross-platform reuse and physical swizzle overrides. Initial
20-test baseline has five failures/one error; corrected final 21 manifest tests
are included in all 143 passing Xbox Python tests. Device `--launch` fails before
preparation; physical builds cannot invoke the Simulator launcher.

First actual device compile fails on a Simulator-only depth-probe call; guard
both depth and draw replay calls. Rebuild succeeds; align standalone minimum
17.4 with main app/CMake's 17.0 and retain intermediate outputs/logs. Final
combined device executable `259fb98f…f90cd`, device library `8c218df2…3a7e4`,
Simulator library `0d8de61e…7f998`; guest unchanged `102885c2…fdaaa`. Actual
IOS load commands, matching manifests/SDK/features and ad-hoc signature verify.
New output has no IPA; no profile is embedded, so not hardware-install acceptance.
Separate Simulator renderer probe passes depth/blit/swizzle controls, plus
16 UIKit and 20 launch/save-helper checks. Installed app `e594a1f9…15feed`,
real game bytes/saves, preferences and PC registry remain unchanged. No install,
physical operation, shader-workaround promotion, pin move, Xbox IPA, push,
publication or cleanup. Hardware graphics/audio/controller and physical shading/
focus remain open. Goal active; [device-SDK evidence](XBOX-SIMULATOR-PASSES.md#device-sdk-angle-preview-build-without-installation-2026-10-02).

### 2026-10-02 — current-source integrated Simulator runtime

Build normal one-picker source `84547a5` with the freshly separate Simulator
ANGLE archive, preserving frozen pins. Terminate only the dedicated Simulator
app; preserve installed app and full 6.4 GB data container and compare bytes.
In-place installation rediscovered both changed container UUIDs. Candidate and
installed executable match `96e8c317…fa652`, strict signing passes. Actual UI
verifies picker/About/Done, Xbox menus, copied outdoor a30 checkpoint, finite
touch look, weapon swap, 2x scope and Save and Quit. Retain a 62.313-second
private video; no sustained-control/fidelity inference. Rapid adjacent down
gestures advance only one menu item; separately observed navigation reaches
Save and Quit safely. Final Windows route reaches the original EULA after
installed-data verification; do not accept it. Ordinary picker restored PID
67405. Full real game/save/package bytes and PC registry preserved, final
preferences match; logs/cache/system snapshots differ. Retain older audit's
preference assertion failure and private comparator-exit handling mistake as
limits, then enforce current-backup file/registry/key rules. No runtime change,
physical operation, pin movement, IPA, push, publication or cleanup. Goal active;
[integration pass](XBOX-SIMULATOR-PASSES.md#latest-source-integrated-simulator-runtime-2026-10-02).

### 2026-10-02 — last-owner release for shared Xbox touch buttons

Reproduce actual-handler failures for duplicate RT/A owners: releasing one
finger incorrectly clears the other held action. Minimal map-based last-owner
release fixes both, without changing layout, physical merge or touch buffering.
Baseline 24 checks fails two (`20261001T232844Z`); fixed 24 passes
(`20261001T232950Z`). All 143 Xbox Python tests pass. Rebuild separate Simulator
ANGLE library and normal combined app after preserving outgoing outputs; full
app/data backup compares before in-place install. Installed executable
`3a933fea…95e8a`, library `50a549c3…8dae2`. Actual copied outdoor normal-menu
checkpoint and single RT 60→59 verified. Shared Device Hub switched to a physical
iPhone display before any game action; select only dedicated Simulator and open
a separate window for further input. No physical test input. Final ordinary
picker PID 74408. Real game/save/package bytes and PC registry preserved, only
last-edition preference/log/snapshots change. Device preview predates fix and
must rebuild before hardware use. No rendering claim, pin move, Xbox IPA, push,
publication or cleanup. Goal active; [touch-owner evidence](XBOX-SIMULATOR-PASSES.md#shared-touch-button-ownership-2026-10-02).

### 2026-10-02 — current-source device readiness, without installation

Audit confirms only the selected engine starts; Xbox's aligned 4 GiB guest
reservation remains a physical feasibility gate. Five synthetic provisioning
checks pass, not verification of an actual profile. Preserve old device output
and archive privately; actual stale archive is refused by source-hash guard.
Rebuild iPhoneOS ANGLE archive and normal combined personal app from `68cb779`.
Artifact verifier passes both SDK libraries, frozen guest identity, automatic
hardware features, strict ad-hoc signature, IOS/iOS 17.0 load commands and no IPA.
Executable `af1be628…0dc436`; device archive `341f6c0d…134702`. No profile,
installation, launch, Simulator mutation or physical credential inspection.
Existing install helper lacks backup; require explicit full data preservation
and readback before eventual in-place installation. Hardware-window and affected
edition/map questions remain unanswered. Goal active; rendering complaint and
physical graphics/audio/controllers remain open. No graphics fix, pin change,
Xbox IPA, publication or cleanup. [Readiness evidence](XBOX-SIMULATOR-PASSES.md#current-source-device-readiness-2026-10-02).

### 2026-10-02 — blocked audit after device-preview preparation

Previous goal turn: no progress, not a verified wait. Three consecutive audit
turns retain the same unanswered coordinated-iPad-window requirement; the build
is terminal and no live process/job is being waited on. First audit reads actual
Mac/Simulator retained results, confirms installed Simulator hash and verifies
121 save-backup checksums. Second checks clean checkout and device-build report.
Third reconfirms clean checkout at `a57634d` and actual prepared device executable
SHA-256 `af1be628abbf5381160953031a91d7d817397a14999b5b25b9a4ada3830dc436`.
None provides new physical behavior evidence. Do not manufacture further small
Simulator edits to avoid this boundary. Record blocked goal, not completion.
Resume needs Chris's coordinated shared-iPad window; then check actual profile,
back up/read back full data and install in place before graphics/audio/controller
acceptance. Rendering comparison also needs the affected edition/map. No hardware
action, source pin change, IPA, publication or cleanup.

### 2026-10-02 — authorized physical iPad preview and preservation

Chris authorizes hardware use, resolving the previous window boundary. Validate
exact wired M2 iPad Pro/iPadOS 27.0 and real profile/device/certificate/memory
entitlements. CoreDevice Documents backup fails mid-transfer; preserve partial,
use installed AFC serially for full backup and independent readback. First AFC
readback disconnects; retain failure and nested successful retry. Correct the
root-layout audit, then verify all 320 files / 3,348,625,943 bytes before install.
Preserve source ad-hoc app; sign only private staged copy, executable
`90437ca6…06362e`. Strict signature passes and in-place installation succeeds.
Installed bundle metadata verified, not an independent installed-executable hash.

Ordinary picker/About/Done → Xbox → fresh isolated New001 → Normal → a10
cinematic/cryo bay observed. Actual hardware logs 4 GiB guest memory, ANGLE Metal
Apple M2 automatic features, 640×480 rendering and 48 kHz stereo initialization.
Frame 0 GL 0x502 retained; sampled later frames zero, not every-frame/FPS proof.
Sampled cryo floor lacks earlier Simulator Apple black polygons, no matched
hardware/reference comparison or reported shading/focus resolution. Short
mirrored look gestures are small; ask for direct fingers/already-paired controller,
do not alter Bluetooth. Capture via QuickTime skill/CUA: first 396.6-second movie
has unverified mono source; explicit second iPad screen/audio capture 31.37 seconds
has non-silent stereo. Unsaved compositions retained locally, not moved/shared;
speaker/audio quality remains open. Console transport disconnects but actual
process query confirms PID 7089 live, not a crash claim.

Post-run full AFC readback succeeds. Retain initial mistaken log-path audit,
correct that exact known log location. Final preservation audit passes all 151
protected original Documents files, PC registry and preferences. Only config
new defaults, known logs, fresh isolated saves and OS/Metal/dyld scene caches
change. No original save opening or real revision-backup acceptance; live copy
is not an atomic test-state snapshot. Leave isolated cryo-bay run for direct
input feedback. No runtime edit, pin movement, Xbox IPA, push or publication;
goal incomplete. [Hardware evidence](XBOX-SIMULATOR-PASSES.md#physical-ipad-angle-preview-2026-10-02).

## 2026-10-02 — phase 4 Simulator-only shared controls

Chris supersedes hardware work with Simulator-only troubleshooting and asks for
one wrapper-owned touch interface across PC/Xbox, independent upstream imports.
Write phase-4 operating loop. Existing goal tool still reports its historical
blocked objective; do not falsely complete it to replace the objective. Work
continues under this new instruction without a hardware dependency.

Reuse PC HPOverlay, introduce an optional input handler and a small default-Xbox
action adapter, and carry relative look through upstream SDL mouse motion.
Keep guest64/ANGLE pins unchanged. Preserve existing PC routing and share touch
settings/layout; keep Xbox System Link menu separate from PC-only actions.
Alias ownership and lifecycle cancellation get direct handler regressions.
128 overlay assertions and 143 Xbox tests pass after two retained compile-error
corrections. Library and actual combined Simulator app build and install in place.

Normal-menu a30 test verifies aim/fire together, movement, swap, zoom, pause,
settings/Done, Save and Quit and cold checkpoint reload. This does not prove
post-checkpoint action persistence, sustained multi-touch, remapped Xbox profiles
or PC gameplay. Menu-aware A/B labels remain a usability gap. Internal640x480
and soft grass persist; no texture-fix claim. Prior app/data cloned and compared;
post-run real Documents game/save files, PC registry and preferences unchanged.
Private pass: shared-controls.cwPagg. No hardware, EULA, IPA, push or publication.

Source review identifies build66 text/title/name changes, NicholasDominici's
logical-vs-native resolution support and zimm3rmann's aligned vertex uploads /
occlusion scratch-query separation. No third-party code imported. Next: preserve
control checkpoint, run the established build66 candidate gate separately, then
use exact paired scene evidence for resolution versus material/filter defects.
[Detailed evidence](XBOX-SIMULATOR-PASSES.md#shared-pc-and-xbox-overlay-2026-10-02).

## 2026-10-02 — build 66, filtering experiment and repeatable capture

Keep wrapper-owned shared controls separate from the guest update. Build 66 first
passes all six candidate cases, then an acceptance rerun fails its single final
campaign PPM while the later independent screenshot shows the cinematic. Retain
the failure; lock correctly stays at 64. Replace campaign's single-image criterion
with a retained late sequence requiring two complete visible frames, rejecting
stale/partial/duplicate captures. Preserve renderer/map-load/signal checks and
unchanged menu/match gates. 145 Xbox tests pass; focused capture fix is `7b253ab`.

Fresh acceptance passes all six cases: Mac match 2,100 ticks/17 shots, Simulator
match 1,565 ticks/13 shots, three visible late campaign samples. Pin 66 `f2ba71d9`.
Rebuild full combined PC/Xbox app rather than leave the gate's PC scene fixture.
Installed/generated executable `e47cd803…dd4fb31` and strict signature match.
Normal picker/About/Done, copied a30 checkpoint, fire-and-drag 60→59, swap, 2x zoom,
pause and Save and Quit work. Restore ordinary picker. ANGLE remains preview;
accepting the guest is not full graphics/progression or hardware acceptance.

Separate PR35 `f9a4eb57` experiment tests identical checkpoint/camera at 1x/16x
anisotropy. More ground detail appears at 16x, but 640x480 softness persists.
Preserve both captures/logs and full experiment app, do not promote the unmerged
PR into the release pin. Other Apple ports provide resolution/alignment leads;
no third-party code copied into tracked HaloPad source. Desktop menu-relative-
mouse shortcut rejected because Android compiles a no-op menu callback.

Final private-data audit: 196 Documents files, only log changed; original inputs,
saves, packages, PC registry and preferences unchanged. Only OS snapshots differ
in Library. No hardware access, EULA acceptance, IPA, push or publication. Full
goal remains incomplete: A/B menu clarity, remapped profiles, sustained touch,
higher internal resolution and temporal/material fidelity still need work.
See [pass evidence](XBOX-SIMULATOR-PASSES.md#build-66-repeatability-and-campaign-capture-2026-10-02).

## 2026-10-02 — Xbox menu labels without a second touch layout

Add presentation-only A/B/X/Y badges and accessibility hints to shared controls;
retain original icons, captions, action mapping and hit boxes. PC stays unlabelled.
Xbox Controls guide documents MOVE/A/B, drag aim/fire and the Default guest profile
limitation. No unsupported menu-state inference or hard-coded guest offsets.

134 native assertions pass, including 270 layout combinations; 145 Xbox tests pass.
Full combined build and strict signature pass. Actual Simulator verifies guide/Done,
A/B navigation, copied a30 load, rifle fire/drag 60→59, Y swap, 2x scope, pause,
Save and Quit, then cold ordinary picker. Installed executable `077ca7c9…9f57c52`;
build 66 and ANGLE preview unchanged. Captures/app/data/checkpoint retained in
`ref/xbox-build/passes/2026-10-02/menu-labels.qGJNj7/`.

Audit again preserves all 196 Documents files (only log changes), preferences and
PC registry; only OS snapshots differ in Library. No hardware, IPA or publication.
Visible texture softness remains. Next discriminating graphics experiment is real
render-target scaling, checking HUD/scopes/depth separately from filtering.
Non-default guest profiles and sustained human multi-touch remain unaccepted.

## 2026-10-02 — Separate render resolution from texture filtering

Trace Android's forced 1x screen scale. Test a six-line private guest adaptation
using the existing desktop-capable target scaling path, preserving logical layout.
Same copied a30 checkpoint at 1x/2x yields verified 640x480/1280x960 source and
viewport readbacks; drawable remains 1376x1032. Sharper geometry/HUD, still soft
ground. Matched scopes remain centered; swap, zoom, fire/drag and pause respond.
Initial frame-0 GL 0x502 exists at both scales; later presentation readbacks have
complete framebuffers and no prior/read error. Not full GL or depth acceptance.

Do not promote yet: draw/depth replay assumes 640x480; Android atomic visibility
results omit scale normalization, while the tested ES3.0 fallback reports boolean
visibility as a large count. Need independently validated depth/effects/match and
a reproducible, separately identified local adaptation before a default change.

Preserve private branch `halopad-private-render-scale-20261002` at `97b45239`,
experimental app and evidence in `ref/xbox-build/passes/2026-10-02/render-scale.tXrS4o/`.
Restore clean accepted build 66, guest image identical to backup; generated and
installed app executable `2d563e96…734cd8d`, strict signature and About/Done pass.
All 196 real Documents files retained (log only changed); registry/preferences
unchanged and Library only OS snapshots. No hardware, IPA, publication or push.

## 2026-10-02 — Real depth evidence for scaled rendering

Extend the opt-in Simulator depth observer with GPU-reported texture extents,
bounded dimensions, schema-2 metadata and strict scaled parsing. Legacy 640x480
captures remain supported; unsupported color/raster tools keep their old limit.
151 Xbox tests pass; 22 focused depth/native-pixel tests repeat after schema
type hardening. Full experimental and accepted-guest app builds pass.

Actual 2x match: four calibrated/error-free 1280x960 depth observations, 1,071
indices, frame 11,614; base changes 635,254 samples, 116,388 later overwritten
closer, EQUAL writes zero. Actual 1x regression: 1,398 indices/frame 7,257,
76,393 base changes, 27,698 later overwritten closer, EQUAL zero. Matching
positions/projection and frame checks pass within each pair; different spawns
prevent a direct cross-resolution count comparison. Lobby timing failures and
successful retries preserved. A stand-in client is not a human/network acceptance.

Evidence `ref/xbox-build/passes/2026-10-02/scaled-depth.TyfGJB/`. Restore accepted
build 66; keep render-scale experiment private and unpromoted. Next: reproducible
local adaptation identity, visibility effects and broader graphics validation.

Restored installed/generated executable `f5709a50…f24a126` and strict signature
verified; About/Done returns from `f2ba71d9` to the ordinary picker. Audit keeps
196 Documents files (only log changed), original saves/inputs, PC registry and
preferences. Library changes are Metal caches, OS snapshots and scene state.
No hardware, IPA, publication or push.

## 2026-10-02 — Reproducible guest adaptation and snapshot identity

Add opt-in `render-scale-v1` with strict build-66 renderer input checks, source
restoration and separate manifest identity. The accepted upstream checkout stays
clean; actual builds reproduce both private adapted guest `556cc14c…9a9e48bc`
and unadapted guest `a16a3271…8cc89`. Package mismatches and adapted pin updates
are refused. Shared controls remain host-owned, not part of guest modifications.
Full Simulator app reaches the copied a30 checkpoint at 1280x960 through normal
menus; fire/drag (60→59 ammo), swap, centered scope and pause respond. Existing
material softness and frame-0 GL error remain; no broader effects acceptance.

Save backups now key by revision plus guest hash, including same-pin adaptations
and return to default. 167 Xbox tests and 26 real launch/save-helper Simulator
assertions pass on synthetic data. Repair the fixture's stale SDL/overlay linking
boundary discovered during this check. Default guest/app restored, strict signing
and installed/generated executable `c54e09c6…cf5e608` verified; ordinary picker
visible. Original saves, PC registry and preferences preserved; only log and OS
snapshots changed. Private evidence `guest-adaptation.jrHaaE`. Next: effects and
broader material/temporal rendering checks. No hardware, IPA, push or publication.

## 2026-10-02 — World filtering candidate at fixed render resolution

Keep upstream build 66 pinned. PR35 remains open; its source/policy supports
anisotropy for world textures rather than the game's usual 1x sampling. Add a
small original `render-quality-v1` adaptation with the existing strict input,
restoration and manifest boundary. Opt-in `HALO_TEST_ANISOTROPY=4/16` clamps to
GPU support and excludes high-res HUD, point and non-mipmapped paths. It does
not weaken a stronger explicit game request. 170 Xbox tests pass, including
13 compiled fragment cases and distinct quality/scale-only package identities.

Candidate guest `a2f07097…655d8599`; private evidence `world-filtering.a3Zc93`.
Three copied a30 checkpoints, same camera and 1280x960 targets: 4x/16x show more
ground detail than 1x, with continued soft slopes. Runtime reports 16x GPU cap.
Shared drag/fire (60→59), swap, centered scope, grenade (1→0) and pause respond.
69 sampled presentation source readbacks are complete/error-free; pre-existing
frame-0 GL error remains in each run. This does not accept flare/explosion
fidelity, all materials or physical performance. Source still shows ES3.0
boolean visibility fallback and missing Android atomic-count scale correction.

Default guest rebuild again matches `a16a3271…8cc89`. Audit preserves original
saves, PC registry and preferences; log, snapshots and saved scene state differ.
Keep quality opt-in. Next: a small pre-launch original/sharper quality choice and
broader effects/scene checks. No hardware, IPA, push or publication.

Final default app restored to ordinary picker: installed/generated executable
`eb1e2fb0…af1f66f` and strict signature verified. Post-restoration audit narrows
differences to log and OS snapshots; original saves/registry/preferences intact.

## 2026-10-02 — Xbox quality selector on the shared launcher

Add one host-owned pre-launch Original/Sharper (Preview) setting, visible only
for `render-quality-v1`. Original default; Sharper requests 2x targets and 4x
world filtering. Persist separately from Windows; leave explicit development
overrides usable. Unknown guests and malformed choices fail back safely.

Actual combined Simulator app verifies Cancel, About/Done, cold-persisted
Sharper and return to Original, and copied a30 launches through normal menus.
No quality launch overrides: logs prove 640x480/1x versus 1280x960/4x. Windows
card reaches its license screen with copied PC inputs/state; no acceptance or
PC gameplay claimed. 170 Xbox tests plus 35 Simulator launch/save/quality
assertions pass. Private evidence: `quality-picker.Y3ufr2`.

Keep preview installed at Original picker, executable `92db1be7…88e8a7e`, strict
signature and installed/generated hash verified. Pre-install app/data retained.
196 original Documents files audited: only app log changes. PC registry, all
original saves and existing preferences except last-engine selection unchanged;
only new preference is `HaloPadXboxGraphicsQuality=original`. OS snapshots differ.
No upstream pin move, hardware, IPA or publication. Next: isolate remaining
effects/material/temporal defects in a different view, not more quality-picker
plumbing or another identical a30 filtering comparison.

## 2026-10-02 — Observe the live boolean visibility path

Add bounded opt-in `XG_TRACE_QUERIES=1` observer behind guest GL wrappers.
It records existing result values/targets, not extra GPU queries or waits.
172 Xbox tests pass, including generator ordering/guest-pointer conversion and
compiled observer aggregation, opt-in behavior and 180-row limit under sanitizers.

Actual copied a30 menus/camera sweep at Original quality: 180 summary buckets,
11,866 zero and 103 one result reads, none above one. Queries are active; this
is not individual-flare coverage attribution. Source confirms guest positive
results become one million, lens-flare target visibility saturates, and ANGLE
Metal both requests boolean visibility and converts its result to true/false.
General texture blur/shading is not established as a consequence. A camera sweep
around the nearby escape pod did not isolate a partial sun/flare, so do not claim
reproduction of that visual defect or change coverage based on these aggregates.
Next: one known flare's geometry/test area and partial occluder/reference, then
an explicitly counted backend capability if warranted; rescaling boolean results
is not a fix. Android atomic scale normalization is a separate untested path.

Evidence `query-visibility.VodWdU`; full combined app strict-signed and installed
hash `5a677b38…2d73201e` matches generated output. Restart with observer disabled
still reaches ordinary Xbox menu, then return to Original picker. Original
saves/game files and PC registry preserved; only app log, last-engine selection,
OS snapshots/scene state differ. No pin promotion, hardware, IPA or publication.

## 2026-10-02 — Blood Gulch flare scene and free-look route

Previous turn made progress by observing boolean visibility; this pass moves to
a concrete visible sun effect rather than repeating aggregate query counts.
Same installed app, Original quality, Simulator only. Fresh verified data backup,
isolated saves and local protocol stand-in. Stationary 180-second smoke passes
at tick 5010; manual-camera 300-second smoke passes at tick 8580, zero scripted
shots and presentation captures in both. Background free-look, without Fire,
reveals the sun. Small reversible yaw drags retain colored reflections with a
partly clipped sun, remove them outside view, then restore them on reversal.
Screenshots and exact spawn/gesture route: `flare-view.3Y5L6x` and the Simulator
passes document. Spawns differ between runs; coordinates are not a portable test.

Source rectangle area is not viewport-clamped. This is a useful edge-comparison
route, not proof of wrong brightness: corona size need not equal query area,
no per-flare query attribution or original-renderer reference is captured here.
No speculative count/texture fix. Next correlate that rectangle/result with
matched edge views or reference output; broader shading/focus defects stay open.
The two reviewed iOS fork heads remain unchanged. No upstream import or pin move.

Return to ordinary picker, Original graphics, trace off. Final audit: 196 original
Documents files, only app log differs; no files added/removed. Only saved scene
state differs in Library; preferences and PC registry unchanged. Documentation
and evidence only, whitespace/tree safety checks pass; no fresh unit-suite/build,
hardware, IPA, push or publication claim.

## 2026-10-02 — Correlate the sun rectangle and boolean result

Previous pass produced a concrete view; this pass measures the geometry rather
than repeating aggregate counts. Add opt-in `XG_TRACE_QUERY_RECTS=1` CPU observer
of existing query begin/end, immediate uploads, draws and returned results.
Validate pinned-layout rectangles, reject extra/indexed/malformed draws, evict
ID collisions, log at most once per ID per second and 4096 rows. No GL calls,
GPU waits, result changes or upstream source patch. Simulator harness forwards
exact opt-in only under render diagnostics. 174 tests pass, including compiled
observer sanitizer checks and generated hook ordering. Full combined build and
strict installed signature/hash match `69a35886…a8bbc0a`; guest unchanged.

Five-minute Original Blood Gulch smoke reaches tick 8580, no scripted shots.
Free-look finds a far-depth rectangle moving with the sun. Inside: area 2401,
result 1. Partly clipped: (-42,203)-(16,261), area 3364, result 1 although at most
928 pixels (27.6%) are in the viewport. Outside: (-66,189)-(-7,248), result 0.
The guest's boolean fallback saturates target visibility to 255 in the partial
case, instead of a counted target bounded around 70. This is a specific coverage
loss; not a measured brightness ratio or general texture/shading diagnosis.
Private screenshots/logs/builds under `query-rect.ILuHKK`. Harness ends before
reverse gesture, no reverse measurement claimed. GL IDs are not flare tag IDs;
association uses geometry/depth and visible sun movement. No original reference
image or hardware test. Next a separately identified counted backend capability,
with cross-pass accumulation and scale normalization, not CPU edge clipping.

Observer-off 30-second menu smoke passes with no query logs; screenshot reviewed.
Return to ordinary picker at Original. Pre-install app/data retained; all original
saves/game files and PC registry preserved, preference dictionary unchanged.
Only app log and OS snapshots differ. Upstream tree clean, pin unchanged; no IPA,
hardware, public publication or push. Full objective remains incomplete.

Final review preserves the per-slot logging timestamp on collision, preventing
query-ID churn from bypassing throttling; added compiled assertion and all 174
tests pass again. Rebuilt/signed/installed final hash `9df901c…03501ee` matches
generated output. Earlier `69a35886…a8bbc0a` remains the five-minute trace evidence
identity; geometry/renderer behavior is unchanged by this logging-only edit.

## 2026-10-02 — Counted Metal backend, independently GPU-verified

Previous pass established a specific coverage gap. Implement isolated opt-in
ANGLE `counted-visibility-v1`: exact-input-checked generated ContextMtl/DisplayMtl
and shader copies, no external source edits. Request Metal Counting at begin
and continuation; use saturating 64-bit limb addition instead of OR when
combining render passes. Ordinary QueryMtl boolean conversions are untouched.
A private render-thread bridge exposes resolved counts, rejecting invalid,
active and non-occlusion queries. Ordinary build-ios explicitly forces option
OFF; the candidate uses `ref/xbox-build/out/angle-counted-simulator` only.

Asset-free Simulator pbuffer fixture passes 56 actual GPU checks, 28 with each
verified allowBufferReadWrite feature state. Includes exact full/half/zero,
depth occlusion, query reuse and independent IDs, two render targets with flush,
64/128/512 target sizes, conservative query and unchanged GLES boolean results.
The actual sun-edge rectangle's equivalent viewport produces exactly 928;
fully outside produces zero. This demonstrates counting and cross-pass sums,
not guest scaling, game rendering, physical performance or a completed fix.
Final evidence `generated/xbox-counted-tests/20261002T112532041905Z`, library
`100c2ad6636c1d0f49777f0d4a44bc60c7d511e31a8408db2ec65b7bf5f9f232`.
177 Python tests pass; generation preserves sources, rejects input/anchor drift,
records output/bridge/recipe identity and normal builder forces OFF.

Preserve initial failed shader-anchor configure, copied-source include compile
and missing-CoreGraphics fixture link diagnostics; all were corrected and later
build/run results, not those failures, supply acceptance. No game install,
saved-state access, upstream pin promotion, hardware, IPA or publication.
Next wire a separately identified private guest/backend capability, normalize
scaled-target counts, then compare the actual Blood Gulch sun and world occluder
with fallback/rollback. Do not substitute more identical synthetic checks for
that integration or claim the user's broader texture/shading report resolved.

## 2026-10-02 — counted guest integration and actual sun-edge verification

**Hypothesis:** the tested counted Metal backend can replace the guest's boolean
fallback through an explicit private bridge and maintain correct logical-pixel
scaling at both quality settings, without changing ordinary GLES semantics.

Implement `render-visibility-v1` as a distinct, exact-input-checked guest recipe.
Intercept its private query token in the generated host wrapper before GLES;
require the paired backend, and normalize counts by recorded target-scale area
in the guest. Disable atomic counting for this recipe. Require Simulator/ANGLE,
separate the library directory and bind backend identity into the build/package
manifest. Missing/mismatched identities fail packaging. Default backend stays
OFF and accepted upstream pin is unchanged. Upstream checkout is clean after
the temporary recipe build. Generated code/game inputs stay private.

181 Xbox Python tests pass, including sanitized compiled normalization, unique
anchors, interception and manifest refusal; 36 native Simulator save/launch/
quality checks pass (`generated/xbox-launch-tests/20261002T113800918597Z`).
Combined app builds, strict signature verifies, installed/generated executable
matches `a5a8c6449b6545ee06256aadfa1541f4867e029520a132c486245691061f807e`.
App and original data backed up/read back before in-place installation.

Private `counted-game.X2E6Wq/live1`: Original Blood Gulch passes 300 seconds,
tick 8490, no shots. Manual free-look correlates the sun with full 2601, narrow
edge 406, outside 0, return-partial 1710 raw samples. The partial counts exactly
match in-viewport test pixels. Actual reflections fade and return. `live2` uses
the real picker Sharper choice: 1280x960/4x filtering, 240 seconds/tick 6780.
Full area 2550 returns 10200; the narrow slice returns 1624, outside zero. This
is raw-count evidence; guest division is separately exercised by the exact
compiled fragment, not logged directly in game. Camera views differ slightly.
Both runs retain startup GL 0x502; later sampled frames are zero. No assertion
of all-frame correctness, performance, original-renderer parity or world-depth
acceptance. The specific all-or-nothing sun coverage loss is corrected on this
route; broader texture/shading complaints remain unresolved.

Restore Original through the picker. Readback finds all 196 original Documents
files intact except the known log; PC registry/preferences unchanged. Reviewed
Library differences are Metal caches and OS scene/snapshot state. Preserve app/
data backup and screenshots/logs. No hardware, Xbox IPA, pin promotion, push or
publication. Next world-geometry occlusion and a different effects/material
scene, not another identical observer or synthetic fixture pass.

Final observer-off 30-second menu smoke passes, retained screenshot shows the
Xbox menu, and cold launch returns to Original-quality edition picker. No query
trace rows. Preservation audit passes again after this run. Current tree/index
safety and whitespace checks pass. Candidate remains installed for Simulator
follow-up; no promotion to full gameplay/graphics acceptance.

## 2026-10-02 — foreground occlusion and sniper night-vision checks

Keep installed counted-visibility candidate from `44556bc`, Original quality
and both upstream pins. Clone Simulator data before testing. Private evidence:
`ref/xbox-build/passes/2026-10-02/world-occlusion.KRW3Xt/`.

Stationary Blood Gulch passes 360 seconds/tick 10410 with zero shots. Manual
free-look hides a blue doorway light behind the weapon and reveals it again.
All query rectangles remain inside 640x480: visible 20/25, covered 0/36,
revealed 25/25, near silhouette 30/30. This is actual foreground-depth evidence
beyond viewport clipping, not BSP-wall acceptance or brightness measurement.
Short Move drag does not translate the player: CUA has no sustained hold in
this route; keep correct release semantics. No proven control defect from that.

a50 night campaign passes independent 120/300-second smoke runs. The first
deadline prevents scope interaction; the second captures 2x but stops before
Light. Expected harness cleanup is not a crash. A manually owned copied-save
launch first reaches the missing named-profile dialog; retain those logs, do
not claim menu resume. Direct-map init then reaches the sniper scene and verifies
shared 2x/10x/unscoped Zoom plus Light night vision on/off, with screenshots.
No matched original-renderer parity, sustained multi-touch or all-frame GL claim.

Stop manual app explicitly, cold-launch ordinary Original-quality picker.
Readback: 196 original Documents files preserved except known app log; no adds
or removals; Library, preferences and PC registry unchanged. Documentation-only
pass, no rebuild or unit-suite rerun. No hardware, pin promotion, IPA, push or
publication. Next distinct transparent/material or wall-occlusion case, with
reference comparison where practical. Broader rendering and touch goals remain.

## 2026-10-02 — Silent Cartographer material route

Extend smoke harness/map validation to b30. 181 Xbox Python tests pass. Actual
180-second Original-quality Simulator run passes map/renderer/nonblack/presentation
gates; manually turn from beach landing toward water with shared free-look.
Preserve `beach-material.KCw0xS` screenshots/logs and data clone. Water, shoreline
bands and translucent exhaust are visible, not reference-matched. Opening washout
clears during the sequence; no definite new renderer failure or fix established.
Pinned source uses separately rendered water ripple mips and a sampled composite;
capture those levels/state next rather than changing terrain filtering blindly.
Ordinary picker restored. Original Documents differ only in known log; preferences
and PC registry unchanged; four Metal cache files changed. No runtime rebuild,
pin promotion, hardware, IPA or publication. Normal-menu save/reload also remains.

## 2026-10-02 — bounded water mip diagnostic

Add opt-in two-snapshot readback of bound null-data 128-square mip chains to the
existing Simulator capture path. Level reader restores checked GL state, records
errors, sampler/program/binding and pixels; validator fails incomplete chains.
First generation-only hook yields no captures during 180-second b30 (`live`),
correctly failing that diagnostic. Do not mistake it for missing water textures:
upstream copies fully rendered chains without generating a tail.

Final draw-time capture (`water-mips.ixmd5T/draw`) passes 120-second b30. Texture
87, frames 120/180, complete 128/64/32/16 levels; RGB changes 45758/11900/2907/535.
Zero readback/prior errors, state restored. Trilinear/linear/repeat sampler.
Source pattern supports likely water-ripple attribution, not shader-tag identity
or proof every bound unit is consumed. This rejects missing/frozen sampled levels,
not normal orientation/blending/final fidelity. 184 tests and full signed combined
build pass; installed SHA `346533d7c066fa1146d94224f78eb59f692f9c3579143c3f7623fc1447df0f02`.
Guest/pins unchanged. Observer-off menu passes 30 seconds with no captures despite
parent flag; retain prior app/data and rejected observer logs. No hardware/IPA/
publication. Next consuming material shader or reference comparison, not repeated
mip-existence checks or a claim that the user's graphics problem is resolved.

## 2026-10-02 — water reflection consumer identified

Use existing diagnostics only on installed `5bd669d` candidate. Private
`water-shader.km9ZLU/trace` b30 passes 90 seconds with frame-120 guest trace and
shader dumps. Water reflection draw: VS24, 198 indices, modes64621, stage0
128-square four-level render target with -0.6 bias, stage3 64-square five-level
cubemap. Generated matching fragment sources contain normal/dot/reflection
lookup; VS writes required coordinates. Reflection expression agrees with the
NVIDIA texture-shader spec section3.8.13.1.18, except zero-denominator guard.
No Xbox precision, cube orientation or final-pixel parity claim. No justified
renderer patch; preserve concrete route for matched-reference comparison.
Known initial GL0x502 remains; later sampled errors zero. Cold Original picker
restored; original saves/preferences/PC registry preserved except app log.
No build, unit-suite rerun, hardware, pin change, IPA or publication.

## 2026-10-02 — counted candidate normal-menu save/reload

Private `normal-save66.vLJy5R`: unchanged installed app, full original-data clone,
copied build-64 New001 checkpoint. Normal picker/menu path loads a30 outside pod.
Shared Fire 60->59 and Look respond, Pause and individual stick menu steps reach
Save and Quit. Wait for main menu, snapshot saves, terminate. Cold process via
same normal menus restores checkpoint 60/120, one grenade and original camera.
This closes current counted candidate's bounded existing-profile gate, not new
progression or all-upgrade compatibility. Profile The Maw/Legendary summary stays
incorrect; graphics complaint remains open. Real Documents retain all196 files
with only app log changed; Library/preferences/PC registry identical. Ordinary
Original-quality picker restored. No build/tests rerun/hardware/pin/IPA/publish.

## 2026-10-02 — build 73 candidate and upgrade checkpoint

Freeze upstream build73 `d1c7243c`; accepted lock remains66. Review changes to
scoreboard, player names, postgame title and network protocol9->10; Android
imports/renderer/saved-game source unchanged. Preserve installed app, full data
and output tree in private `upstream73.NGS3eC`. Existing counted ANGLE recipe
builds and packages normal combined Simulator app; codesign/install pass.
Executable `59764ae4…ac8c`. Menu30s/a10 60s/scripted-match65s pass bounded gates;
a10 last sample black, screenshot bridge visible, so no sustained-render claim.
Copied build66 named checkpoint loads through ordinary picker/menu path. Shared
Fire/Look/Pause/Save and Quit work; cold process restores60/120 and onegrenade.
Original Documents only log changed, Library only SplashBoard replacements,
preferences/PC registry unchanged. Ordinary picker PID77214 restored.

Captured cyan shield-HUD line remains unexplained, absent on cold reload.
New scoreboard scroll uses wheel/PageUp/Down with no current shared-touch route;
overflow not yet runtime-reproduced. Next test that input/display gap and compare
HUD artifact with66. Unadapted acceptance still required before pin promotion.
No runtime edits, unit-suite rerun, hardware, IPA or publication. Earlier journal
profile-summary wording is clarified by the already documented upstream unlock-all
policy; no new save defect was found. Full evidence/limits in Simulator ledger.

## 2026-10-02 — build 73 scoreboard touch bridge, visual gate open

Implement held Scoreboard vertical drag in shared overlay as optional engine
hook. Xbox maps 80 points/page to paired Page Up/Down; PC stays hold-only.
Cancel pending pages on release/interruption but retain required key-up. Wheel
route rejected because guest may consume it as weapon switching when closed.
185 Xbox tests pass, including ASan/UBSan queue checks; 141 native overlay
assertions pass. Correct initial misplaced event block and wrong-lifecycle
fixture; failure logs retained privately. Normal app rebuilt/signed/installed,
SHA `39f06f77…7198e`. No guest/pin/renderer change.

Private `scoreboard73.ctwOYT`: app/data backup, local two-player fixture (one
stand-in, no roster overflow), before postgame screenshot. Actual after drag
emits BACK/four PageDown press-release pairs/BACK release; camera/weapon stable,
Fire charge100->99. 53.42s capture and samples do not establish scoreboard
visibility. Next isolate BACK/fade versus guest rendering; do not claim working
end-to-end paging. Full record in Simulator ledger. Both helpers exit0 and
recording stopped; ordinary picker PID5172. All196 Documents preserved except
known log; Library OS snapshots/scene state only, prefs/PC registry identical.
No hardware, IPA, push, publication or accepted-pin promotion. Graphics open.

## 2026-10-02 — scoreboard Back/fade discrimination

Unchanged73 app `39f06f77…7198e`, private `scoreboard-observe73.0c6lPL`.
Read-only LLDB swap probe with exact ELF globals distinguishes input loss from
short hold: actual CUA drag reaches guest Back1, suppression0, fade0.0814 and
scoreboard_open1 for one rendered sample, then zero. Upstream power1.9 curve
means only~0.85% opacity. Two PageDown pairs logged. Previous sampled video's
negative result does not establish a rendering failure; final pixels/overflow
still not accepted. Do not alter held semantics to suit automation.

Initial debugger pause disturbed networking; write-watch EXC_BAD_ACCESS also
stalled loading until detach/re-attach with Darwin ignored-exceptions configured
before attachment. Not a new crash, nor performance evidence. Probe never
writes guest memory. Both helpers exit0, breakpoint deleted/debugger detached,
ordinary picker PID14557 visually restored. All196 Documents preserved except
log; Library OS scene state only, prefs/PC registry identical. No product code,
rebuild/install, pin change, unit rerun or hardware. Next cyan HUD/material
comparison, retaining separate sustained-scoreboard and overflow gates.

## 2026-10-03 — cyan streak is not tied to replacement HUD

Private `hud73.hWLeWC`, unchanged73 guest/app. Back up full app data, load two
independent copied66 checkpoints through normal menus. High-res HUD true/false
videos both reproduce the cyan streak; it appears away from shield before Look,
then moves with world to align near shield. Guest readback confirms hud_enabled0
in false run. Narrow detector validated with original screenshot: high5767
frames/347 hits, low11407/790 in original strip; low center strip769. These are
video frames, not game FPS. Correct ffmpeg's initial resampling to passthrough.
Map metadata contains a30 beam-emitter device/shader/effects. Likely world beam,
not exact draw attribution or fidelity acceptance. Retire HUD texture hypothesis,
do not hide effect. Next matched upstream desktop material/effect reference.

Recordings stopped, read-only debugger detached, ordinary picker PID28067.
All196 Documents preserved except log; Library OS scene state only; preferences
and PC registry exact. No product edit/rebuild/install/unit rerun, accepted-pin
change, hardware or publication. Full goal and graphics acceptance remain open.

## 2026-10-03 — independent upstream desktop rendering reference

Private `desktop73.Fi7gKD`: existing Mac wrapper is not independent and its66
manifest does not match current73 guest output. Preserve it. Docker daemon is
unavailable; no VM started. Download official pinned73 Windows release and
Mesa26.2.3 MSVC software driver, verify both GitHub SHA256 digests, and deploy
only per-app x86 DLLs in a new private Wine9 prefix. Actual OpenGL4.6 llvmpipe
context renders menu, then b30 for244.39 seconds with16 screenshots and exit0.
No translation/ANGLE on this reference, no default Wine-prefix/system install.

Full app-data backup then unchanged73 Simulator b30 pass180s succeeds. The
visually corresponding640x480 beach view shows the same broad ground softness
and blocky waterfall on both paths. Static-region mean RGB differences3.19,
5.91,9.94/255 (animated waterfall6.92), no pass threshold or exact camera/time
identity claim. This narrows those particular symptoms, not temporal water,
shading/focus or all graphics. Reuse the reference for moving materials next.

Both test handles complete. Picker PID55014/Original AX verified. All196
Documents retained except log; Library byte-identical, prefs/PC registry exact.
Executable hash unchanged39f06f77…7198e. No runtime edit/rebuild/install/unit
rerun, accepted-pin promotion, hardware, IPA or publication. Goal stays active.

## 2026-10-03 — restore water draw state after ES mip copies

Private `water-reference73.eG38SP`: independent desktop and existing Simulator
sequences reveal flat green water only on Simulator. `prepare_draw` binds targets
before texture assembly; ES fallback mip blits reset framebuffer0 and scissor
before the current reflection draw. Add separately identified `render-water-v1`
to preserve both framebuffer bindings and scissor, with exact source/anchor
guards, clean upstream restoration, and existing counted-backend/SDK restrictions.
No shader math/filter/pin/control changes. Prior recipe identity preserved.

188 Xbox tests pass, including sanitized C restoration and packaging checks.
Launch fixture initially fails linking the prior scoreboard callback; add its
inert aborting stub.37 Simulator helper checks then pass. Full build/codesign and
in-place install succeed. App `aa0c46d8…7077d`, guest `83dd49b6…45c5`.
Actual before/after source frames restore blue water/ripples and closely resemble
desktop; ordinary app screenshots without optional readback hooks verify this
is not capture-only. Menu30s/a10 60s/Blood Gulch65s pass (tick1530/12shots).
Rollback app/output and original real-data backup retained. Accepted66 unchanged;
no hardware/IPA/publication. Next normal controls/save reload on this candidate
and Sharper water/depth. Full goal and broader fidelity remain open.
Final picker PID78201/Original AX-verified.196 Documents retained except known
log, prefs/PC registry exact; Library changes only Metal cache and OS snapshots/
scene state. Prior app/output remain private and recoverable; no real save marker
advance. Repository safety and whitespace checks pass.

## 2026-10-03 — water candidate shared controls, checkpoint and Sharper

Private `water-accept73.wCglTA`; unchanged installed `aa0c46d8…7077d`/water-v1,
source73/accepted66. Full real-data backup, isolated copy of existing checkpoint.
Normal picker/Campaign/New001/Halo/Normal route restores a30 outside-pod state.
Actual shared Fire/Look/Swap/2x Zoom/unzoom/Pause/Move-menu/A work. Save and Quit
returns to main menu; cold PID99163 repeats the ordinary route and restores
rifle60/120 and checkpoint camera. Recorded scope/menu/reload screenshots and
before/after save hashes. Device Hub AX stalls once at picker; visible coordinate
click dispatches successfully with no app fix. No new progression acceptance.

UI-select Sharper, fresh isolated b30 PID4246:101.55 seconds,20 native screenshots,
172 BMPs (not FPS). Actual1280x960/effective4x; complete sampled framebuffers and
zero sampled GL errors. Restored detailed water and normal foreground occlusion
visible in Pelican/cabin views. Not exhaustive depth/fidelity; ground remains
soft. Fresh picker confirms persisted Sharper; restore Original through UI.
Final picker PID16731.196 Documents retained except log, Library only OS scene
state, preferences/PC registry exact, executable unchanged. No rebuild/unit
rerun/install, pin change, hardware/IPA/publication. Next different moving
material/effect against desktop or unmet controls/product gate. Goal active.

## 2026-10-03 — shared settings and layout regression contracts

Private `shared-settings73.ugJORz`, full backup and isolated copied checkpoint,
unchanged water73 candidate. Add12 native assertions for registered settings/
editor handlers, cancellation, mirrored identities, look sensitivity and saved
PC/Xbox geometry with separate phone/tablet keys. Helper lacks UIApplication
dispatch, so verify registration and invoke actual handlers directly; fixture
position is not real drag proof. Correct test-hook misuse, a diagnostic compile
error and UISlider near-2 float expectation. Final153 PASS,90 layouts/0 failures.

Actual PID39747 normal guest campaign route, shared settings UI left-handed on:
left Fire60->59 and background left Look rotate camera. Short automated LOOK
stick produces no visible turn; no sustained/multi-touch acceptance. Generic AX
switch ambiguous, coordinate clicks sometimes ineffective; visible drag restores
off, AX0 verified. UI-off creates explicit false key absent in original backup;
audit catches it. Remove only that test-created preference via Simulator defaults
with exact container domain, cold-launch picker PID58756. All196 Documents retained
except normal log; preference keys/values and PC registry identical; Library
otherwise plist encoding/OS scene state only. App SHA unchanged. No runtime edit,
rebuild/install, upstream pin change, hardware, IPA or publication. Goal active;
next a different moving material/effect, not repeated settings/checkpoint tests.

## 2026-10-03 — localize bridge bands to shadow sampling

Private `effects-reference73.xzSmz9`, unchanged water73 candidate and desktop73
reference, full backup/isolated saves. Paired a10 opening captures show similar
exterior glow but long black bridge-floor bands only on Simulator. Native color
timeline235 complete draw pairs:0091/program212/1062 indices introduces a band,
VS30 shadow projection. Larger lightmap darkening is not the band-producing draw.
Correct private image-orientation analysis; do not confuse largest change with cause.

First exact-source material snapshot lands in a different camera shot; follow-up
frame3100 captures217 color pairs/25 shadow draws. Actual0073/program215/1068
indices adds a floor line. Bound128x128/maxlevel0 shadow has seven nonzero edge
samples and linear/CLAMP_TO_EDGE sampling. Upstream projection/convolution request
BORDER; ES fallback substitutes edge and ANGLE Metal lacks the extension. Strong
mechanism evidence, no fix yet. Next separately identified faithful border-sampling
candidate and visible A/B with legitimate shadows preserved, then water/control
regressions. Avoid speculative depth changes or hiding effects.

All captures exit0; ordinary Original-quality picker PID90385 restored.196 real
Documents unchanged except log; preferences/PC registry exact; Library Metal cache
and OS scene state only. Installed app SHA unchanged, upstream clean. No runtime
edit/build/install, unit rerun, hardware, pin change, IPA or publication. Goal active.

## 2026-10-03 — correct bridge border sampling in private Simulator candidate

Previous turn progress: shadow draw/material proof. New `border-fix73.zpl1rD`,
full prior app/output/real-data backups. Implement narrowly guarded
`render-border-v1`: single-level 2D point/linear border color, correct half-texel
and corner blending, per-draw uniforms before serial shortcut. Other paths stay
native/unchanged. Guard second shader source, restore both temporary inputs, keep
old water identity; new module included in provenance. No depth changes or shadow
suppression.194 Xbox tests/38 native launch-save-quality checks pass. Full build,
codesign and in-place install succeed; app `dc469db1…5997d`, guest `2d03ab18…b6b6`.

a10 PID6415,181.4469s/474 BMP/18 native captures: matched bridge view loses bands,
nearby frame3204 retains character shadow; exterior glow remains. b30 Original
PID8160,101.0809s/175 BMP/20 native captures retains detailed water and foreground.
Local match65s passes tick1532/12 shots with reviewed screen; menu30s passes.
Original-quality ordinary picker PID11624 restored. Real196 Documents unchanged
except app log; preference dictionary/PC registry exact, Library only Metal/OS
state. Nested upstream clean, accepted66 unchanged, candidate73 frozen. No hardware,
IPA, publication or cleanup. Next exact-build normal controls/save/cold reload
and Sharper regression; broad rendering/human multi-touch remain open. Goal active.

## 2026-10-03 — border-candidate control/save/quality acceptance

Previous turn progress, unchanged a43d9cd app `dc469db1…5997d` verified installed.
`border-accept73.vtqWlq`: full real-data backup, copied water-pass checkpoint and
linked maps. Normal PID18874 picker/campaign/profile/Halo/Normal loads outdoors;
Fire60->59, Look rotation, pistol Swap/2x Zoom, Pause and Move/A Save and Quit
observed. Copy saved tree before cold PID22487; normal route restores checkpoint
location and rifle60/120/one grenade. No new progression/multi-touch acceptance.

UI-selected Sharper: b30 PID24634,100.8045s/210 BMP/20 native screenshots,
1280x960/effective4x, detailed water and foreground visible. Fresh picker confirms
persistence. a10 PID26491,181.2334s/246 BMP/36 screenshots: corresponding bridge
floor remains free of the long black bands at higher resolution. Wider framing
retained/disclosed, not pixel parity. Ordinary startup frame0 has known0x502;
later samples0; diagnostic quality captures samples0, not every-frame proof.

Original restored through UI, cold ordinary picker PID28582. Real196 Documents
unchanged except log; preferences/PC registry exact, Library Metal/plist encoding
only. No runtime edit/rebuild/pin change/hardware/publication. Next non-default
profile reproduction on a copy: fixed default adapter versus upstream preset
mapping. Explicit guest interface needed if adapting it; no profile reset or
hard-coded offsets. Goal active; broader fidelity/human multi-touch still open.

## 2026-10-03 — reproduce alternate-profile touch action mismatch

Previous turn progress; unchanged installed `dc469db1…5997d` verified. Private
`profile73.YKiZSk`, full real-data backup and copied border-pass checkpoint.
PID32958 normal menus set copied New001 Button Settings Southpaw, thumbsticks
Default, then Save Changes. Normal campaign route resumes checkpoint: shared
Fire spends grenade1->0 without firing; Throw fires rifle60->59. Pause works.
Screenshots and changed profile retained; campaign snapshot unchanged.

Source review identifies authoritative resolved preferences and active-widget
state before raw device polling as candidate bridge inputs. Existing relative-
mouse callback is not a menu signal; keyboard emulation is raw pad too. Next
implement guarded/versioned touch-only mapping with menu navigation and held-
input transition tests. No global hardware remap/profile reset/guessed offsets.
Detailed implementation gates recorded in XBOX-SIMULATOR-PASSES.

Ordinary Original picker PID42922 restored.196 real Documents unchanged except
app log; Library, preference values/keyset and PC registry exact. Upstream clean,
app hash and pins unchanged. No runtime edit/build/unit rerun/hardware/IPA/push.
Goal active; this is runtime defect proof, not a completed control fix.

## 2026-10-03 — shared-input-v1 fixes Southpaw touch semantics

Previous turn progress; implemented the identified guest/host boundary. Exact
hash-guarded input/import patch exposes scalar resolved bindings/stick layout/menu
state before device polling. Touch-only host remapping preserves raw menus,
clears queued input at context boundaries and blocks held controls until release.
Physical controller routing and saved profile settings are unchanged. New
adaptation includes existing border/water/quality fixes; source transaction now
covers four files and restores upstream cleanly, including failure handling.

198 Xbox tests pass, including sanitized native mapping tests across all five
button/four stick layouts;153 native overlay assertions and39 launch/save/quality
checks pass. Build/sign/in-place install succeeds. Private backups/evidence:
`ref/xbox-build/passes/2026-10-03/input-bridge73.BvdqSP`. App `48f118f3…213e`,
guest `652fbebb…de17`, recipe `71781a2a…1fd`; full hashes in Simulator ledger.

PID52279 ordinary menus load the copied Southpaw reproduction checkpoint. Fire
now fires rifle60->59 with grenade1 unchanged; Throw consumes grenade1->0 with
rifle59 unchanged. Pause/Move/A Save and Quit returns to main menu. Saved isolated
tree retained. After the overlay suite's own relaunch, deliberate isolated cold
PID57563 loads the same checkpoint normally and Fire again fires60->59 without
consuming the grenade. Context logs prove alternate bindings and menu/gameplay
transitions; profile card Default text is not the button preset. No fresh
progression claim; screenshots preserve counters.

Final ordinary Original picker PID58269.196 real Documents preserved except app
log; only two OS SplashBoard snapshots replaced in Library. Preferences/keyset
and PC registry exact. Installed app SHA reverified, nested upstream clean,
accepted66/upstream73 unchanged. No hardware/IPA/push. Next actual alternative
A/B-binding preset and Default regression, then exact-build graphics/local-match
checks; legacy movement feel and sustained multi-touch remain unaccepted.

## 2026-10-03 — Jumpy and Default shared-input runtime checks

Previous turn progress; unchanged installed `48f118f3…213e` verified before/after.
Private `presets73.WnjD7M` holds full real-data backup and separate genuine copied
profiles. PID61051 normal settings changes copied Southpaw to Jumpy and saves.
Context `70513246:fedc` proves the alternative A/trigger bindings. Shared Jump
shows brief viewpoint rise/return; Throw consumes grenade1->0, Fire rifle60->59.
Retained recordings include idle/tool latency, not sustained play; a nearby
grenade pickup after Jump explains its later0->1 count. No profile-byte edits.

PID70320 uses an independent copied Default profile, normal checkpoint route.
Fire60->59/grenade1; Throw grenade1->0; pistol Swap, background Look and Pause
respond. No fresh progression or Jumpy cold-reload claim. Ordinary Original
picker PID73921 restored;196 real Documents differ only by app log, Library by
one OS scene-state file. Preferences/keyset/PC registry exact. No runtime/build/
unit rerun/pin change/hardware/publication. Next exact-build water/border/local
match; all-preset runtime feel and sustained human multi-touch remain open.

## 2026-10-03 — shared-input Original graphics and local match

Previous turn progress. Same installed `48f118f3…213e` verified before/after;
live remote main still73/d1c7243c, accepted66 unchanged. Private
`input-render73.wmTYV3` full backup/isolated captures. Original b30 PID76028
100.858s retains detailed water; a10 PID77037 180.696s retains engine glow and
removal of bridge bands against prior/desktop comparison. Rendering diagnostics,
not normal-menu or human-play claims. Local-match65s passes tick1533/12 scripted
shots/lit.980; known startup blit0x502 retained. Smoke helper explicitly disables
upstream update prompts;31 focused/199 Xbox tests pass. No app rebuild.

Ordinary Original picker PID81615 restored. Real196 Documents differ only by
app log; Library only four Metal cache files; preferences/keyset/PC registry
exact. No hardware/pin promotion/IPA/publication. Next alternate-thumbstick
runtime move/look check; exact-build Sharper and broad fidelity remain open.

## 2026-10-03 — Southpaw thumbsticks and bounded gesture evidence

Previous turn progress. Same `48f118f3…213e` app, no runtime/pin change. Private
`sticks73.gpETY0` contains full real backup and genuine isolated Southpaw-stick/
Default-button profile created through normal settings. PID86046 saves it and
loads a30; swipe Look visibly rotates. PID90343 cold normal-menu reload reports
sticks1/valid1. Trace proves shared Move Y routes to axis3 and X to axis2, but
began/moved/ended occur within0–1ms; no clear movement or held LOOK acceptance.
Fire60->59/grenade1 works. Preserve the distinction from Southpaw button tests.

Added native assertions for60 repeated polls, unchanged context, release,
single-poll quick drags and held-axis blocking across preset changes under all
five button/four stick combinations.199 Xbox tests pass. No artificial gesture
extension. Ordinary Original picker PID92696 restored; app SHA unchanged.
Real196 Documents differ only by app log, Library only four Metal cache files;
preferences/keyset/PC registry exact. No hardware/IPA/publication. Next Sharper
on this exact candidate, not repetition of the zero-duration gesture. Sustained
multi-touch and broader graphics fidelity remain unaccepted.

## 2026-10-03 — exact shared-input Sharper regression

Previous turn progress. UI-selected Sharper on unchanged `48f118f3…213e` runs
b30 PID95027 for101.053s and a10 PID95776 for181.716s, at1280x960/effective4x.
Private `sharper-input73.30PwZ5`: full real backup, independent scene state,
20/36 composited screenshots, no periodic BMP/video. Water detail, bridge-band
fix and engine glow remain visible. No pixel/full-fidelity or human-play claim.
Startup blit0x502 persists with first swap read1/draw1 rather than drawable0.
Pinned source reveals Present's draw0 binding precedes a potentially stateful
framebuffer_get(read-source) cache miss. Targeted runtime attribution/order-fix
A/B is the next experiment, not another unchanged graphics repeat.

Original restored through UI; ordinary picker PID97340. Real196 Documents differ
only by app log, Library only one OS scene-state file; preferences/keyset/PC
registry exact. App SHA unchanged; nested source clean; pins unchanged. No new
unit run, rebuild, hardware or publication. Free space fell2.7->1.8GiB; private
scene roots occupy730MiB combined including generated state. Capture guard was
1GiB. Avoid another large capture/build without rechecking disk; no deletion.

## 2026-10-03 — first-blit ordering proof and unbuilt source fix

Previous turn progress. `first-blit73.HJp1Wy` isolates real backup and copied b30
state. PID1574 register-only LLDB trace confirms draw0 -> both1 -> read1 -> first
presentation blit, then0x502/read1/draw1 in stderr. PID2378 separate unchanged-app
launch: one-shot debugger GL draw0 binding before that blit removes the error;
frame0 now read1/draw0/error0. Both debugger sessions detached. This is explicit
diagnostic state modification, not a fixed binary or sustained acceptance.

Implemented cumulative guarded render-present-v1: move read-FBO resolution before
selecting/clearing drawable. Full source hash/unique anchor, old recipe identities
stable, shared-input/quality/backend checks retained.202 Xbox tests/40 native
launch-save-quality checks pass. Actual upstream input accepted in memory only;
single-line move verified, checkout unmodified. No guest build/install: disk fell
1.8->1.2GiB. Installed `48f118f3…213e` SHA unchanged, Original picker PID3923;
real196 Documents only log, Library only OS scene-state change, preferences/PC
registry exact. No hardware/IPA/publication/pin promotion. Next safe disk
headroom then candidate build and exact-binary regression; source fix not yet
accepted. Do not rerun the already confirmed debugger experiment as progress.

## 2026-10-03 — first-blit fixed binary built and validated

`present-build73.V8BrrS` preserves real container, outgoing app and guest output.
Losslessly compressed456 completed BMPs in the prior border pass's two Sharper
frame directories, verifying integrity and exact decompressed bytes before
removing raw duplicates; .bmp.gz and SHA receipt retain recoverable evidence.
This recovered about1.5GiB. Disk later independently reports166GiB free; that
larger change is not attributed to this work. No unrelated cleanup.

Built render-present-v1 pinned73/d1c7243c, codesign verified and installed in
place: app ed257ad5…af30b, guest4ac7e842…b60. Cold30s menu/60s a10/65s local
match pass with frame0 read1/draw0/error0, no startup0x502.202 Xbox tests pass.
Normal PID16953 shared Fire/Swap/swipe Look/Pause and Save and Quit work on an
isolated genuine save copy; PID22537 cold-loads that outside-pod a30 checkpoint
via actual picker/menus. Not sustained multi-touch, online or broad-fidelity proof.

Ordinary Original picker PID24556 restored. Real196 Documents only log changed;
Library only two OS snapshots replaced; preferences/keyset/PC registry exact.
Nested upstream clean, accepted66/upstream73 unchanged. No hardware/IPA/push.
Next exact-binary Original/Sharper water/border regression, then a different
remaining material/effect issue. Detailed hashes and evidence are in the pass
ledger. Goal remains active; source-only disk blocker is superseded.

## 2026-10-03 — presentation candidate both-quality regression

Exact installed ed257ad5…af30b / guest4ac7e842…b60 unchanged. Private pass
present-render73.bShFII backs up the real container, runs four independent100s
diagnostic a10/b30 captures, and retains80 native PNGs plus runtime logs. Original
640x480/1x and actual UI-selected Sharper1280x960/4x retain detailed water, the
bridge shadow-band correction and translucent display. All four cold starts have
read1/draw0/error0, no startup0x502. Not pixel parity, FPS, full fidelity or new
normal-menu progression. No runtime change/unit rerun. Upstream remote main and
official latest release still73/d1c7243c; locks unchanged.

Original picker PID47075 restored. Real Documents only app log changed; Library,
preferences/keyset and PC registry exact. No cleanup/hardware/IPA/publication.
Next a different matched desktop-versus-Simulator world-effect comparison, such
as the a30 beam, without assuming it is erroneous. Goal remains active.

## 2026-10-03 — cyan beam independent reference comparison

Private beam-reference73.H8tioZ preserves real data and uses unchanged Simulator
ed257ad5…af30b plus existing official desktop73/Mesa. Fresh-map pair reaches
inside-pod gameplay and is insufficient to view the beam. Source-backed upstream
debug_camera_save/load provides a controlled outside-pod camera: preserve native
position, move8 units along forward, send identical camera.txt to both independent
test states. Explicit loopback-only console listeners terminate with timed runs.
Later camera readback was attempted after termination and refused, so no matrix
parity claim. Captured world geometry aligns visually.

Bright beam/pulse appears at the same location on desktop00948 and Simulator20220;
comparison image and narrow-strip metrics retained. Presence is not a HaloPad-only
artifact; unmatched effect phases prevent exact intensity/temporal conclusions.
No rendering patch justified by this result. Ordinary Original picker PID66501
restored and checked via dedicated simctl screenshot (shared Device Hub switched
to another project; no interaction there). Real saves/preferences/PC registry
preserved; only app log/OS scene state differ. Pins/runtime unchanged, no hardware,
publication or unit rerun. Next unaccepted Boxer/Green Thumb runtime mappings;
overall fidelity and human multi-touch remain open.

### 2026-10-03 — Boxer shared-touch preset runtime

Unchanged Simulator ed257ad5…af30b, copied genuine save only. Dedicated HaloPad
Device Hub window avoids sending input to the other project's iPhone simulator.
Ordinary settings UI selects/saves Boxer; mapping71563240:fedc valid1 sticks0
appears in gameplay. Recorded Melee swings rifle without spending grenade/ammo;
Fire60→59, Throw1→0 with rifle59, Swap/2x Zoom/Pause pass. Save and Quit completes;
cold launch89489 reloads checkpoint and retains Boxer mapping. Ordinary Original
picker93343 restored. Private boxer-green73.40aZqB includes434.9s video, settings/
cold-checkpoint images, melee contact sheets, real-data backup and audit reports.
Real Documents only app log and Library only OS scene state changed; preferences
and PC registry exact. No runtime/build/pin/public changes. Green Thumb remains
next; directory name is not evidence it was tested. Broad rendering fidelity,
sustained multi-touch feel and physical acceptance remain open.

### 2026-10-03 — Green Thumb shared-touch preset runtime

Unchanged Simulator ed257ad5…af30b, independent default-save copy. Normal settings
UI saves Green Thumb buttons/Default sticks; mapping765f3240:1edc valid1 sticks0.
Shared Melee swings pistol and Zoom separately enters/exits2x despite native
Green Thumb's B/right-stick swap. Swap, Fire, Reload and Pause pass; reserve64→63
after reloading the one fired round. Jump tap captured but not independently
accepted. Save and Quit completes; cold process32078 loads the same outside-pod
checkpoint and retains Green Thumb mapping. Original picker34933 restored.
Private green73.Omi7kU retains86.0s video, settings/checkpoint images, action/melee
sheets, real-data backup and audits. Real Documents only app log and Library only
OS scene state changed; preferences and PC registry exact. Upstream checkout
clean, no runtime/build/pin/public changes or unit rerun. Next controlled graphics
comparison of another unresolved material/effect. Do not repeat passing preset
checks as new progress. Broad fidelity and human multi-touch remain open.

### 2026-10-03 — a50 night landing independent reference

Private night-reference73.QOuaoQ: unchanged Simulator ed257ad5…af30b/Original
versus official Windows73/Mesa/Wine, independent fresh a50 state. Desktop runs
184.696s/214 captures; Simulator180.181s/188 captures. Both show the white landing
transition, translucent exhaust, dust and spotlight pools. Matched settled
regions differ0.722/0.198/0.261 RGB levels for cliff/ground/tree; no synchronized
matrix/effect-phase or broad-fidelity claim. No new renderer fix warranted.
Both processes end; ordinary Original picker42061 restored. Real saves/prefs/PC
registry unchanged; only app log and four Metal cache files differ. Nested
source clean, executable hash unchanged, no unit/build/pin/public changes.
Next missing update gate: Mac manifest is still66, unadapted73 acceptance remains
open. Recheck live upstream, freeze candidate, preserve adapted app/shared output
and data before that separate workflow. Do not repeat this settled night view.

### 2026-10-03 — upstream74 baseline and PC-safe updater

Live release/main moved to74/80d30410: desktop Mesa Intel draw flush, excluded
from the Android guest path. Freeze revision, preserve full real data and outgoing
app/output in upstream74.p84GNQ. Unadapted guest1373456e…5a66 builds; Mac9cf6fe7c
passes menu/a10/match(tick2130/17 shots). Normal-entry combined Simulator81e4b04e
passes the same gates(tick1530/12 shots) on ANGLE. Known startup0x502 remains;
render-present-v1 source guard rejects74. No adapted74 or save compatibility claim.
Updater regression reproduces --scene replacement of the PC entry point; remove
it, add --device-data. All11 update fixtures/203 Xbox tests pass. Restore exact
73/ed257ad5 preview in place after giving backup clone an .app suffix; Original
picker50980 verified. Real saves/preferences/PC registry exact; app log/OS caches
and snapshots differ. Shared outputs and clean checkout remain74, accepted66
unchanged; both candidate/outgoing artifacts preserved. Next review-aware74
adaptation identity preserving historical manifests, then runtime/save regression.

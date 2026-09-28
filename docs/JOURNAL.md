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
- **Chris's key.** `8437-1920-5563-7741-A` is not a Halo PC product key: those are 25 characters in five groups of five, from Microsoft's key alphabet, which has no 0, 1 or 5. It was not used, and nothing was derived from it. The license and the product ID still gate the real run.


## 2026-09-27 — Blood Gulch in play, on the Mac and on the iPad Simulator

- **Halo loads and runs Blood Gulch.** `tests/halo_bloodgulch_test.c` uses Halo's own start-up script mechanism.
  - It writes `map_name levels\test\bloodgulch\bloodgulch` to a script and passes `-exec` with it. `WinMain`'s argument splitter `0x545a00` builds the arguments, then the test starts the systems and calls `main`.
  - The player spawns in the red base, drawn in first person with the assault rifle, the HUD, the motion tracker and the reticle.
  - After 330 frames `main` quits when asked. There is no input yet.
- **Audit: pointer tables that read as text.** The callback table at `0x636b18` was dropped as a string because `0x566d20`'s bytes are printable. A text-like dword is now a pointer when a neighbour also points into `.text`. Checked every change: six real entries gained (among them three `__except` bodies), six bogus ones gone (three-letter language codes such as "FRB" and "DEL").
- **Audit: `wait` before a `__try` state change** (`C7 45 FC`) counts as code. Without this, the `__except` handlers at `0x546a7e`/`0x546c28` were probed too early and blacklisted.
- **Translator: byte `rcl` by a constant.** Halo's ADPCM sample step `0x551d10` uses it. The new `adpcm_step` slice matches the x86 oracle on 400/400 cases. Untranslated sites: 6 → 3.
- **Lifter and run:** lifter `24c44b99cfac-05b596a6`, run `20260927T145850Z-51456`. 22 suites, 11 slices and 6 contract cases pass.
- **Chris's key, again.** Chris sent `8437-1920-5563-7741-A` a second time. It is still not a Halo PC product key: those are 25 characters, `XXXXX-XXXXX-XXXXX-XXXXX-XXXXX`, from Microsoft's key alphabet, which has no 0, 1 or 5. There is nothing it could be typed into, and nothing was derived from it.



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

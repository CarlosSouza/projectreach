# HaloPad goal loop — phase 2: the unblocked route

Written 2026-09-26. This loop **replaces G0–G2 of [HaloPad-GOAL-LOOP.md](HaloPad-GOAL-LOOP.md) and inherits everything else**: G3–G12, the loop steps, process hygiene, native-boundary, network, campaign and data discipline, the unblocking ladder, testing rhythm, evidence and terminal states. Requirements remain in [HaloPad-PRD.md](HaloPad-PRD.md). Where this file and the original loop disagree about G0–G2, this file wins.

## Why a second loop

The first loop stopped at `BLOCKED_EXTERNAL` because it tied three things to one gate: the exact 1.10 executable, a running original-client baseline, and a legitimate key. All translation work waited on all three. On 2026-09-26 two of those constraints fell away:

1. **The exact executable exists.** The supplied `HaloCESetup.exe` holds the stock Custom Edition 1.00 files. Bungie's official Custom Edition 1.10 update (`haloce-patch-1.0.10.exe`, Microsoft-signed, fetched from the original Bungie URL via the Wayback Machine) patched them without an installation or key. Result: `haloce.exe` and `haloceded.exe` both report **1.0.10.621**. `haloce.exe` SHA-256 `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`.
2. **Function-level "original behavior" does not need Windows.** Unicorn 2.0.1 on the Mac loaded the patched image and ran Halo's own CRC32 routine at `0x59f2a2` (stdcall, 3 args). It returned the standard check value `0xCBF43926`, restored the stack exactly, and matched zlib on randomized inputs. An emulator that runs original x86 functions is a valid differential oracle for G2.

Chris directed on 2026-09-26 that the repo copy be used to proceed. That is the acquisition decision for the **engineering** profile. It does not settle distribution rights (still G12) or give anyone an online identity.

## What still needs Chris (the only external actions)

| Need | Blocks | Does not block |
|---|---|---|
| A legitimate Halo PC product key (used boxed Halo PC, ~$25) typed in by Chris | Running the original client if it refuses to start without one (G1b); later online play as a real player (G5–G6) | G1a, G2, G3, most of G4 |
| A physical iPhone/iPad plus signing for the architecture capsule | Final G2e acceptance | Mac and Simulator work |
| A second legitimately provisioned player | G5 original-client match | Everything before G5 |

Ask for these once, in the handoff, when a goal actually reaches them. Never stop the whole loop for one of them.

## Blocker policy (changed)

A blocked goal parks **only itself and the goals that truly depend on it**. The loop moves on to the lowest unblocked goal. Report `BLOCKED_EXTERNAL` for the whole loop only when every remaining goal is blocked. The three-turn audit applies to that whole-loop state, not to a single parked goal. Record parked goals in `STATUS.md` under "Parked, waiting on Chris".

## Oracle ladder

Each kind of claim has one oracle. Use the lowest that answers the question.

1. **Function level — x86 emulator (Unicorn) on the Mac.** Registers, flags, stack effect, memory writes and return value of one original function or call graph, on controlled inputs and real map data. Imports are trapped: an unexpected import call fails the case loudly instead of returning a guess. Tool-only; never linked into a HaloPad target.
2. **System level — original client/server in CrossOver bottle `halopad-reference`.** Boot, menus, local Blood Gulch, dedicated server, network sessions. Record the Wine/CrossOver version with every result.
3. **Disputes — real Windows.** A Windows VM or PC is needed only if (1) and (2) disagree in a way Wine could explain, or for G5 acceptance if CrossOver's networking is in question. Not a prerequisite to start.

## Goal stack for this phase

Work the lowest unmet, unparked goal. Each goal is met only with evidence under `docs/artifacts/<date>/<goal>/` and the named document updated.

- **G0′. Protect the workspace.** Every file in this repo except `.gitignore`d material is untracked or shows a deleted original. Run `scripts/check-repo-safety.sh`, then make a local commit on branch `codex/halopad-phase2` that records the existing moves and all scripts/docs. No push. Pass: clean `git status` apart from ignored paths; safety check green; commit hash in `STATUS.md`.

- **G1a. Engineering input locked and reproducible.** (M02, engineering tier)
  - Write `scripts/prepare-patched-client.sh`: extract the patch targets from `ref/HaloCESetup.exe`, run `haloupdate.exe processrtp=patch.rtp updateversion=01.00.10.0621` in a **fresh** CrossOver bottle, stop the updater when the files are written, then hash. Run it twice from scratch; hashes must be identical.
  - Extract the complete stock 1.00 file set (maps, shaders, DLLs, `controls/`, `content/`), overlay the patched files, and place the result in ignored `ref/inputs/custom-original/` with a manifest (path, size, SHA-256, origin: installer / patch).
  - Set `config/profiles/custom-en-1.0.10.0621.json` `accepted_sha256` to the reproduced hash and `input_state` to `ENGINEERING_DERIVED`, and add a `provenance` block naming installer SHA-256, patch SHA-256, method and Chris's 2026-09-26 direction. `scripts/inspect-inputs.py` must then **pass** for the patched file and still fail for the 1.00 file.
  - Record one independent cross-check attempt. Known: ProcessChecker lists a 1.10 `haloce.exe` MD5 `7c3fd3b07a6c41a4dbdbdba1b4442580`; ours is `6388cf3d1f162ce1766a562481f61432`, so that listing is not an independent confirmation. Do not spend more than one bounded research step on this; a later key-installed copy (G1b) is the natural cross-check.

- **G1b. Reference runtime baseline in CrossOver.** (M02 baseline) *Parkable.*
  - Build bottle `halopad-reference` from `ref/inputs/custom-original/` (copy, never the original folder).
  - First `haloceded.exe` (the dedicated server): start it on a controlled local port with Blood Gulch Slayer, capture its console. Pass when it runs a game loop.
  - Then `haloce.exe`: launch and record exactly what happens. If it boots, reach the main menu, play local Blood Gulch, then join the local `haloceded`. Capture screenshots and logs.
  - If it refuses to start or demands a key: stop, record the exact message, and **park G1b** until Chris supplies a key through the normal installer. Do not write registry product IDs, generate keys, use published keys or use cracked executables. The leftover "Halo CE Cracked" Parallels shortcut and 2022 saves on this Mac are not inputs.
  - Public servers are off-limits in this phase.

- **G2a. Executable audit.** (M04) Implement `scripts/audit-executable.py` → `docs/EXECUTION-MODEL.md` + private JSON manifest. Must cover: sections (`.text` 0x1dd156 bytes, `.data` 0x206bdc, `.tls`), 267 static imports across 6 DLLs, delay imports (WS2_32, WSOCK32, DSOUND, WINMM, binkw32, vorbisfile, WININET, VERSION, SHELL32), dynamically loaded modules (d3d9, dinput8, ddraw, keystone, strings, eula, NVCPL, …), TLS, entry point `0x5ccac7`, function-boundary candidates, jump tables and indirect call/jump sites. **Relocations are stripped** (`IMAGE_FILE_RELOCS_STRIPPED`, reloc directory size 0): produce `relocations.csv` by scanning code and data for image-range absolute values, classify each (code immediate, data pointer, jump table, false positive), and validate uncertain ones with oracle runs. The report names every unclassified region.

- **G2b. Oracle harness.** (M05 tooling) Promote the CRC32 probe into `scripts/x86-oracle.py`: load the locked image at its base, map `.data`/`.tls`, run a named function with stack/register/memory inputs, trap imports, record outputs to JSON. Pin `unicorn` in `scripts/requirements-tools.txt` (`capstone`'s wheel was x86_64-only and was removed; use Apple `objdump` or build capstone for arm64). First fixture: CRC32 `0x59f2a2` on 50+ random buffers including length 0–17 and 4096.

- **G2c. Whole-image SRW/llasm run.** Feed the locked `haloce.exe` + `relocations.csv` to the built SRW/llasm (`scripts/build-lifter.sh` output). Record every failure category (unsupported instruction, unresolved branch, data-in-code, import) by count and by call path. Pass = a complete, honest capability report, not zero errors.

- **G2d. Real Halo slices, native vs oracle.** (M05) Compile translated functions to ARM64 and compare with G2b, in this order: CRC32 `0x59f2a2`; the SSE routine at `0x59f397`; one x87 math routine; one string/memory routine; one switch/jump-table function; then a higher-level slice on real data (map-header validation or checksum over `maps/bloodgulch.map`). Each slice: identical outputs, flags where observable, stack effect and memory writes. Fix translation, never the oracle.

- **G2e. Address, dispatch and callback contract.** (M06–M07) Replace llasm's pointer-offset return trick (see `LIFTER-PREPARATION.md`) with checked 32-bit guest memory and a finite build-time dispatch table from guest addresses to compiled functions. Test callbacks, invalid/null addresses and a real Halo slice under this contract on macOS and the iPad Simulator. Physical-device capsule is required for final G2e acceptance; if no device/signing is available, park that one row and continue to G3 on the Mac.

- **G2 selection report.** `docs/G2-SELECTION.md`: SRW/llasm results, failures, fixes, generated size/compile time, why the remaining work is finite, or evidence that the bounded alternate lifter is needed. A NO_GO needs the reproductions required by the original loop.

- **G3 onward:** as written in [HaloPad-GOAL-LOOP.md](HaloPad-GOAL-LOOP.md). G4's gameplay comparisons use G1b if it passed; otherwise G4 can proceed on function- and data-level oracles but cannot be marked passed until G1b provides the original-client comparison.

## Additional hard rules for this phase

- The emulator and CrossOver are **reference tools**. Any HaloPad target that links Unicorn, runs Wine or executes original x86 bytes fails its gate.
- Never fabricate, generate or reuse product keys or key hashes. Never patch out a key or version check in original or translated code.
- The patched client, extracted game files, oracle traces and generated code stay in ignored paths. `check-repo-safety.sh` runs before every commit.
- Each CrossOver bottle is project-owned and named `halopad-*`. Leave the user's other bottles alone. Stop `haloupdate`, `haloce` and `haloceded` processes the session started before it ends.
- Commits are local, on `codex/halopad-*` branches. No push.

## Session start (additions)

After the original checklist: confirm the locked `haloce.exe` hash matches the profile; list `halopad-*` bottles and running Wine processes; read "Parked, waiting on Chris" in `STATUS.md` and check whether any parked input has arrived.

## Suggested `/goal` objective

> Follow docs/HaloPad-GOAL-LOOP-PHASE2.md in /Users/chrissotraidis/GitHub/projectreach. Work the lowest unmet, unparked goal from G0′ through the G2 selection report, then continue into G3 per the original loop. Park goals that need Chris (product key, physical device, second player) and keep going on the rest; only report BLOCKED_EXTERNAL when every remaining goal is parked. Keep STATUS.md, JOURNAL.md and evidence current; commit locally, never push.


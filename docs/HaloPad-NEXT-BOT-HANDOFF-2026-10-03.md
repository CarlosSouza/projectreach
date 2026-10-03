# HaloPad: focused replacement goal loop

Prepared 2026-10-03 at Chris's request. The previous goal is **PAUSED**.
This is a handoff, not authorization to resume this chat or launch another bot.

## Start here

- Primary checkout: `/Users/chrissotraidis/GitHub/projectreach`.
- Branch: `codex/xbox-engine`; implementation/evidence baseline: `bc8379c`.
- Simulator only: `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, HaloPad Xbox iPad,
  iPadOS26.5; app `dev.halopad.HaloPad`. No physical iPad access authorized today.
- Accepted Xbox pin: build74, `80d30410c8db28f4008b92f4e012a1b046ece14e`.
  Upstream checkout: `ref/xbox-build/vol/engine`. Freeze it during diagnosis.
- Renderer: ANGLE/Metal, `config/xbox-angle.lock.json`, cumulative adaptation
  `render-present-v1`. Do not silently fall back to Apple GLES or remove fixes.
- Installed combined app executable SHA256:
  `4e42dc6d4e58ea81cb4ce9cb34817bb4898de715e4b1a9ce804d8d867fe11175`.
- Matching guest ELF SHA256:
  `8fb0112f359467f89526897c3911d36848cc448ad465052583929b7a4ab60a5e`.
- Private disc: `ref/Halo - Combat Evolved (USA).xiso.iso`; extracted maps:
  `ref/xbox-build/data/maps`, 24 maps, build `01.10.12.2276`. Already identified
  and imported; do not redo extraction or seek a different disc without evidence.
- No helpers from the last pass remain running. Last app state: ordinary picker,
  Original graphics. Rediscover app/data paths with `simctl get_app_container`.

## What is actually stuck

1. **One observed late sound crash, not yet a repeatable reproducer.** A diagnostic
   Battle Creek camera sweep completed, then the Simulator crashed around184s.
   A stationary final-camera control survived240s; desktop74 survived246s.
   This does not establish the sweep as cause, normal-gameplay impact, or an
   upstream-versus-HaloPad attribution. The original null dereference is known;
   the earlier sound-lifetime violation is not.
2. **The crash reporter also crashes.** It mistakes a32-bit guest frame pointer
   for a host pointer, recursively faults, and hides the first fault in the OS
   report. Fixing this improves evidence; it does not fix the sound lifetime.
3. **Human acceptance is missing.** Automated touch taps prove routing, not that
   simultaneous move/look/fire feels good. CUA drags were often0–1ms. Real iPad
   graphics/performance and direct-finger controls cannot be accepted in Simulator.
   Normal PC startup also has a still-unaccepted EULA gate; never accept it or
   manufacture a product key on the user's behalf.

The process failure was an overly broad, unbounded loop: repeated small scene
checks, long chronological documents, and no decisive stop condition. Several
visual suspicions proved to be upstream behavior. More passing screenshots are
not the next deliverable. Do not restart research or redesign the working picker.

## Already built; preserve rather than repeat

One combined app has Windows Custom Edition left and Xbox Combat Evolved right,
separate saves/import paths, About and Original/Sharper graphics selection.
Xbox uses the shared HaloPad overlay through an adapter. Default, Southpaw,
Boxer and Green Thumb have varying recorded source/runtime coverage; do not
upgrade that to full human feel acceptance. Normal campaign save/cold reload and
the System Link profile/lobby/Battle Creek/exit route have passed bounded tests.
The second network machine was a stand-in, not another complete game client.

Real build74 update-pin acceptance, backups, Mac/Simulator smoke and restored
adapted preview were exercised. Last Xbox unit suite:206 passing tests; not rerun
for the last two documentation-only passes. Water mip-copy, border sampling and
first-blit fixes exist. Same-revision desktop comparison agrees on22 usable
Battle Creek stepped poses; this is not all-scene or continuous-motion proof.

## New goal to give the next bot

Finish HaloPad's existing two-engine personal-build integration: preserve the
Windows-left/Xbox-right picker, PC behavior, separate saves and HaloPad-owned
shared touch controls; diagnose and fix the concrete remaining Xbox stability
failure; verify the changed build against representative graphics/control/save
regressions; retain the working pinned-update workflow. Keep private game inputs,
translated code and app outputs local, with no IPA or publication. Report the
remaining human/hardware/PC-license acceptance gates explicitly, without calling
the full project complete while required evidence is missing.

Work in the following bounded passes. At each pass report the hypothesis, one
experiment, its result, and the resulting decision. After three focused passes
or roughly45 minutes, give Chris a decision-quality checkpoint rather than
silently expanding the investigation. This is a reporting checkpoint, not a
claim that the full goal has succeeded or permission to pause it autonomously.

### Pass 1: make the failure actionable

- Read the evidence below before building. Verify current branch, hashes and free
  space. Back up real Documents/Library and preserve the installed signed app.
- Replay the failing sweep on fresh copied state at most twice,5 minutes each.
  Preserve both passing and failing outcomes. Use sparse/ring-buffer diagnostics;
  do not accumulate another10GB of full-frame BMPs for a sound investigation.
- Correct test-harness readiness/liveness reporting first if needed: the old
  high-res-HUD log marker did not prove a completed desktop frame; poses0..2 were
  invalid. Its Simulator wrapper also only noticed death at final termination.
  Do not overwrite the original evidence folders or treat a stale PID as live.
- Capture the first invalid looping-sound identifier, its creation/deletion,
  owning sound/channel, update ordering and thread. Trace native source and the
  exact guest translation; do not infer the cause from the final SIGSEGV.
- Separately make fault reporting safe for guest32-bit and invalid frame pointers,
  with a focused test. Do not mask the original guest fault or disable sound.
- Exit with either a reproducible lifetime failure, or a precise narrowed case
  and the next discriminating experiment. Two non-reproductions do not mean fixed.

### Pass 2: fix the cause, not the symptom

- Use evidence to distinguish stale upstream sound ownership, guest translation/
  ABI error, host audio behavior, or an unsupported diagnostic-only path.
- Implement the smallest justified fix. Add a regression that fails before and
  passes after. A null guard is insufficient unless channel retirement/ownership
  semantics are proved correct. Keep diagnostic hardening a separate change.
- Keep upstream imports reproducible: any required guest adaptation gets a narrow
  reviewed source guard, restoration and identity coverage. Never leave an
  undocumented patch in `ref/xbox-build/vol/engine` or merely bless a changed hash.
- Rebuild/install in place only after preserving the outgoing app/output/data.
  Record exact executable/guest identities. Do not upgrade upstream simultaneously.

### Pass 3: verify and hand off

- Re-run the failing path three times beyond its previous failure window, then
  one10-minute normal-menu Battle Creek session on isolated state. A stand-in peer
  is adequate for this host stability test, not full network interoperability.
- Verify menu -> campaign -> shared Fire/Melee/Throw/Swap/Zoom/Pause -> Save and
  Quit -> cold reload. Recheck the PC route without crossing its EULA boundary.
- Run the Xbox unit suite and relevant native overlay tests if input changed.
  Recheck the already-defined a10/b30 material cases only if the fix affects them;
  do not invent more graphics tours. Build74 upkeep need not be rerun unchanged.
- Audit real saves/preferences/PC registry, restore ordinary picker, stop helpers,
  and leave a concise local commit and explicit remaining acceptance list.
- A passing bounded regression supports that fix, not universal graphics/audio
  correctness. If normal play still fails, retain the failed evidence and do not
  call the build ready. If new authority/hardware is essential, ask Chris directly.

## Exact evidence and code pointers

All paths below are relative to the primary checkout.

- Primary failure: `ref/xbox-build/passes/2026-10-03/creek-reference74.7rz74B/`.
  Read `simulator/debug.txt`, `simulator/stderr.log`, `HaloPad-crash.ips`,
  `fault-translation.json`, `trace_fault.py`, `camera_path.py`, `capture.py`.
  `stationary/` is the240s passing control; `desktop/` is official74/Mesa.
  These scripts write into their own folders: copy/adapt to a fresh private pass
  before rerunning. The original camera driver starts both renderers; a Simulator-
  only reproduction should retain the same pose values without requiring Wine.
- Primary assertion: looping-sound datum `0xf98f0000` unused/changed. First fault:
  SIGBUS at guest4; installed `xg_text+0x91024` maps to guest`0x88076870`,
  `ldr w1,[x0,#4]`, in inlined `update_channel_for_looping_sound`/`update_channels`.
- Source: `ref/xbox-build/vol/engine/source/sound/sound_manager.c`:
  `refresh_sound`, `track_loop_track_sound`, `process_looping_sounds`,
  `refresh_sounds`, `update_channel_for_looping_sound`, `sound_render`, `sound_idle`.
  Host mixer: upstream `port/linux/src/dsound_sdl.c`; HaloPad `port/xbox/xg_sdl.c`.
- Reporter: `port/xbox/xg_memory.c`, `report()` and `fault()`. Guest FP
  `0x11013840` is read as host memory at`0x11013848`; SA_NODEFER allows recursion.
  Use exact installed dispatch table/matching ELF; shared intermediate objects
  were not authoritative for the installed translation.
- Preserved accepted-update artifacts:
  `ref/xbox-build/passes/2026-10-03/accept74.D58wri/`.
  Its `HaloPad-before.app` is the adapted working preview; `out-unadapted` is a
  matched Mac baseline. Shared output changes adaptation; verify manifests/hashes.
- Copied campaign profile: `ref/xbox-build/passes/2026-10-03/adapted74.VMMEFa/session`.
  Normal multiplayer evidence: `ref/xbox-build/passes/2026-10-03/menu-match74.LOkrFn/`.
- Build: `scripts/xbox/build-ios.sh`, then normal `scripts/build-ios-app.py` with
  the same renderer/adaptation environment. The latter picks up the local Xbox
  library automatically (no `--xbox` flag). Use `--device-data` when launching,
  and explicitly select the dedicated Simulator; inspect current CLI first.
  Keep normal PC entry, not `--scene tests/halo_app_scene.c`.
  Set `HALOPAD_XBOX_RENDERER=angle-metal` and
  `HALOPAD_XBOX_GUEST_ADAPTATION=render-present-v1` consistently for both steps.
  Existing ANGLE source:
  `/Users/chrissotraidis/.codex/scratch/halopad-angle-xdZB6j/webkit/Source/ThirdParty/ANGLE`.
- Tests: `python3 -m unittest discover -s tests -p 'test_xbox*.py'`;
  `scripts/test-ios-overlay.py` for relevant native overlay changes.
- Network fixture: `scripts/xbox/network-bot.py`, not raw upstream bot (missing
  hardware-ID field). Use only owned existing addresses, no network aliases.
- Detailed historical evidence: `docs/XBOX-SIMULATOR-PASSES.md`, newest first.
  Older STATUS/phase4 paragraphs have stale "next" instructions; this handoff
  supersedes those priorities. Do not read the entire chronology before acting.

## Non-negotiable boundaries

Use the existing checkout. Preserve unrelated work; use apply_patch for edits.
Never uninstall/reset the app to simplify testing. Keep game files, saves,
preferences, PC registry/product key and signing material private and intact.
No physical iPad, external games, upstream contact, push, release, IPA, cleanup of
private evidence, Figma, or automatic upstream upgrade. Do not create another
task/worktree without appropriate user authorization. Do not announce a complete
fix from a build, PID, unit suite, or stationary sample alone.

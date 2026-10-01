# Xbox / edition-picker passes, 2026-10-01

Scope: HaloPad's existing checkout, local personal builds, and the dedicated
**HaloPad Xbox iPad** Simulator (iPadOS 26.5). No physical iPad installation,
IPA creation, publication, or upstream modifications.

## Build-59 baseline

Upstream release [build-59](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-59), commit
`8fb1647e18a68e164529321bdcbe24557801f23b`, was fetched and built.
It fixes a weapon-swap crash and log spam. The candidate is frozen at that exact
commit for this pass rather than following moving HEAD. Build 58 was the earlier
candidate; its private outputs and failed diagnostics remain preserved.
The save-backed update routine passed all six Mac/Simulator cases and moved
the accepted experimental pin from `b47f237d` to `8fb1647e`. This establishes
a repeatable development baseline, not release-ready gameplay. The picker
continues to label Xbox **EXPERIMENTAL** after pin acceptance.

Use `XBOX_REV=8fb1647e18a68e164529321bdcbe24557801f23b` on both Xbox and
HaloPad build commands to reproduce this exact revision independently of a
future pin. Ordinary builds used build 59 at this stage. The app builder
checks the revision and guest/library hashes; mixed or rejected outputs fail closed.

## Build-60 follow-up (current pin)

Upstream [build-60](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-60)
was released at 04:01 UTC on October 1, commit
`bfbac35761335c28aac7a47bf0c578ea37764810`. Its single change pads the glyph
cache to fix text-edge artifacts; it is not a world-geometry fix. After the
build-59 renderer experiments below, this exact candidate passed the save-backed
update routine and became the accepted experimental pin. Ordinary builds now
use build 60 without an override.

Private update evidence: `build60-accept.log`,
`smoke-results/20261001-140820-bfbac357/` and
`simulator-results/20261001-140820-bfbac357/` under `ref/xbox-build/` (the log is
in `passes/2026-10-01/`). All six menu/campaign/scripted-match cases pass:
Mac 2,101 ticks / 16 shots; Simulator 685 ticks / four shots. No presentation
probe or rendering override was enabled. The Xbox menu text is visible and
the Simulator still has world/weapon artifacts. The final app was rebuilt
after acceptance: its normal About panel shows `bfbac357`, not PREVIEW.

A **copy** of the isolated build-59 New001 profile/checkpoint was loaded through
the normal build-60 picker → Xbox → Campaign → New001 → Pillar of Autumn →
Normal path, without a scripted map or forced engine selection. It recognizes
the game in progress and restores the cryo-bay sequence, not the initial ship
cinematic. Evidence: `campaign60-compat.Gh2yDp/checkpoint.png` and logs. This is
one checkpoint across 59→60, not a promise that all old snapshots are compatible.
The original test save and real Simulator saves remain separate.
The real Simulator save folder still matches
`save-backups/20261001-140820-from-8fb1647e/simulator-save` byte-for-byte after
the gates, final in-place installation and manual copy test. The final Windows
card still reaches its existing missing-data setup; no Windows gameplay or
installer provisioning was established. The app is left on the normal picker.
The Python suite is 73 tests, 16 skipped; tree/index and whitespace checks pass.

The serial Mac CPU-texture comparison (`build60-mac-cpu-textures/`) uses the new
binary/guest hash guard, confirms S3TC is disabled, and passes (1,800 ticks /
15 shots). Its captured terrain/weapon view lacks the obvious Simulator cliff
striping. Different viewpoints/timing still prevent a matched fidelity claim.

## Completed changes

- Large Windows Custom Edition / Xbox Combat Evolved cards, with distinct
  multiplayer descriptions and an installed-build panel. Each cold launch
  offers the choice; switching a running engine still requires terminating
  and relaunching the app.
- Dynamic Type, a vertical layout and scrolling at accessibility sizes.
  The maximum accessibility size was exercised with a real swipe to the lower
  edition/action, and the Simulator was restored to its original `large` size.
- Xbox import can return to Editions before an engine starts. Opening and
  canceling Files, then returning, works. Gamepad creation is deferred until
  game startup: import no longer shows ghost controls or keeps a hidden pad polling.
- Quick touch-button presses are retained until the engine polls them.
  Single A clicks advanced Campaign → Profile → Level → Difficulty.
- Short analog gestures now survive until one guest poll. Look samples are
  published on touch events, not only a display tick; cancellation and a hidden
  controller overlay clear unread input. Held input and stronger physical-pad
  merging remain unchanged. A portable C regression tests the actual buffer.
- The Xbox card describes the original campaign without implying accepted
  split-screen/system-link play. About explains experimental status, the lack
  of cross-edition multiplayer, and the separate validation gates.
- A changed revision backs up nonempty Xbox saves before opening them.
  A backup failure prevents startup; isolated test saves cannot change the
  real installation's revision marker.
- Version manifests, old/new UPnP argument compatibility, the newer Android
  user-secret stub, and updated synthetic join-packet support.
- Explicit-UDID Simulator regression runner, fresh per-run save/config folders,
  and Simulator/save-backup gates in the pin updater. Xbox personal device
  packaging stops at the signed app; PC-only packaging is unchanged.

## Evidence and limits

Private evidence is under `ref/xbox-build/passes/2026-10-01/` (ignored).

| Test | Result |
|---|---|
| Build 57, fresh Mac | Menu and a10 opening picture; scripted match 2,130 ticks / 17 shots |
| Build 58, fresh Mac | Menu and a10 opening picture; scripted match 2,108 ticks / 16 shots |
| Build 58, corrected Mac drawable capture | Menu and visible letterboxed campaign cinematic; scripted match 2,100 ticks / 17 shots; all three smoke cases pass |
| Build 57, Simulator | Menu; scripted Blood Gulch match 1,336 ticks / 11 shots; terrain striping and pale weapon-side triangle remain |
| Build 58, Simulator | Menu and scripted match 1,331 ticks / 10 shots; visible terrain striping remains. Campaign still black at 60 seconds; full regression gate fails |
| Build 58, Simulator with presentation fix | All three normal smoke cases pass: menu, visible opening cinematic at 60 seconds, scripted match 938 ticks / 7 shots. No diagnostic overrides |
| Build 59, Mac | All three smoke cases pass with corrected drawable capture: menu, campaign opening, scripted match 2,100 ticks / 17 shots |
| Build 59, pin-update gates | Mac: menu/campaign/match pass, 2,074 ticks / 16 shots. Simulator: all three cases pass, 941 ticks / 6 shots; normal presentation, no probes/overrides. Campaign and match captures visually reviewed; pale match triangles and texture artifacts remain |
| Build 59, real Simulator touch path | One short swipe changes Campaign → Multiplayer; an opposite swipe returns. Created New001, selected Normal, passed the look tutorial, used X to exit the tube, moved slightly and opened pause |
| Build 59, normal save/reload | Save and Quit completes; cold relaunch through edition picker → Campaign → New001 → Pillar of Autumn → Normal recognizes game in progress and reloads the cryo-bay checkpoint, not the initial ship cinematic |
| Old pin, Simulator campaign A/B | Black at 60 seconds, same failure as newer candidate |
| Build 57 campaign diagnostics | Black at 120 seconds; menu-driven entry also black. Game thread samples are in software GL drawing/shader compilation, not proof of a CPU deadlock |
| Picker / import routes | Both edition cards route correctly; Windows reaches its existing missing-data setup; Xbox Files cancel and Editions return work |
| Save preservation | All 39 files in the pre-update Xbox save backup match the independent copy byte-for-byte |
| Build 59 save backup | `save-backups/20261001-133246-from-b47f237d/simulator-save` matches the real Simulator save folder byte-for-byte after isolated gates; no user-save migration compatibility claimed |
| Python / repository checks | 68 tests, 16 skipped; the new portable native-input regression and physical-device SDK syntax check pass; diff whitespace and tree/index guards pass |

The map log says **starting precaching**, not proof of completed load or
campaign gameplay. A nonblack screenshot is only a smoke signal and needs
visual review. An incorrect Mac test capture originally read the source texture
at drawable dimensions and fabricated a lower-left picture with black around it.
The corrected capture reads the drawable; the opening cinematic is correctly
scaled and letterboxed. That earlier capture was not a presentation defect.
Synthetic network machines are not playable peers or human
multiplayer acceptance. This pass did not establish audio quality, split-screen,
controller feel, full campaign progression, physical performance, or hardware networking.
One same-build checkpoint reload does not prove compatibility with older snapshots.

The supplied disc remains `ref/Halo - Combat Evolved (USA).xiso.iso`:
24 extracted retail NTSC maps, build `01.10.12.2276`. The engine's menu
build string `01.01.14.2342` identifies its decompilation lineage, not a
requirement to replace those retail maps with a debug disc.

## Campaign presentation investigation

The opt-in `--render-diagnostics` pass now captures the source and destination
around the presentation blit every ten seconds, with framebuffer completeness,
read errors, viewport and renderer counts. Normal player runs do not enable it.
An installed build-58 campaign pass renders recognizable ship/cinematic frames
in the 640x480 source texture (up to 73% nonblack sampled capture), while the
1376x1032 destination remains black. Both framebuffers are complete and the
reads/blit report no error. The Mac's corresponding source/destination are lit.
This narrowed the failure to the Simulator presentation path. Initial blit
errors on successful menu runs were separate.

Failed approaches are preserved under `campaign-disable-blend/` and
`campaign-disable-cull/`: disabling either state alone does not restore the
destination. `XG_BLIT_PROBE=1` tests opaque/transparent RGBA8 sources, both
filters, both row orientations, texture/drawable targets and culling: all 32
asset-free cases copy the expected orange pixels without errors. It runs before
the guest creates GL state. A temporary state-bisection helper was used
(1 blend, 2 cull, 4 depth, 8 stencil, 16 active texture/sampler 0, 32 program).
It has been replaced by the narrow fix below. `--render-diagnostics` requires actual source/destination captures,
preventing an older installed host from silently appearing to run the new probe.
The full neutral-state mask (63) restored visible campaign frames, confirmed in
the actual Simulator screenshot at 90 seconds. Clearing only the bound program
(32) still failed. Neutralizing texture-unit/sampler 0 alone (16) restored the
source picture at the destination. The production workaround is Simulator-only:
for a final drawable blit, select texture unit 0, temporarily unbind its sampler,
blit, then restore both states. Other framebuffer copies and physical iOS use
the native path unchanged. `XG_PRESENT_RAW_BLIT=1` retains a diagnostic A/B path.
Normal runs do not require flags. They now display the opening cinematic.
The normal full regression is recorded in `simulator-presentation-fixed/`.
Its campaign and match screenshots were visually reviewed. The match frame no
longer has the pronounced horizontal cliff stripes, but viewpoints/timing differ;
it does not establish that all texture/weapon artifacts are resolved. Current
non-test saves also match the independent post-diagnostics snapshot byte-for-byte;
the original 39-file pre-update backup remains intact and matches its reference.
The physical-device SDK syntax check passes, without installing or changing a device.

## Short-gesture and checkpoint investigation

Private evidence: `campaign-controls.8B6vcC/` (before fix),
`campaign59.sXluwg/` (fixed manual path), and `build59-mac-smoke/`.
Final update gates: `ref/xbox-build/smoke-results/20261001-133246-8fb1647e/`
and `ref/xbox-build/simulator-results/20261001-133246-8fb1647e/`.
The manual runs use separate `XG_SAVE` folders and no `init.txt`, scripted bot,
forced engine selection or injected guest actions. `XG_TOUCH_SHOW=1` keeps the
overlay visible without disturbing a Mac controller used by other work.
`XG_TOUCH_TRACE=1` is opt-in input-value logging, disabled in normal runs.

Before the fix, a left-stick gesture published 1 then 0 but the guest never
polled the nonzero value. Right-look gestures could finish before CADisplayLink
published them. The fix retains the latest unread nonzero sample per axis for
one poll, then returns neutral after release; it does not invent a held gesture.
Visible menu navigation and the four-direction look tutorial verify the fix
beyond the isolated buffer test. X exits the tube; a short forward gesture
changes the standing view slightly. Long-distance movement/combat feel remains
untested. Some cryo-bay geometry/texture artifacts remain visible.

Save and Quit writes a normal checkpoint to the isolated profile. After an
actual app restart, the level picker says a game is in progress. Selecting the
same Normal difficulty restores the cryo-bay sequence without repeating the
ship intro. This is the last checkpoint, not a promise to restore the exact
frame where the player quit. `checkpoint-reloaded.png` and reload logs preserve
the result. The new-profile screen's Legendary/The Maw completion labels come
from upstream's debug unlock-all-levels behavior, not completed campaign play.

## Remaining-artifact isolation

Build-59 match diagnostics capture the terrain striping and pale weapon-side
geometry in the 640x480 source as well as the final drawable. They are not
introduced by the final presentation copy. The framebuffers are complete and
later reads/blits report no errors; an initial startup blit error also occurs
on successful runs and does not identify this defect.

Two temporary host-only experiments were tested, then removed:

- Query every `c[0..191]` vertex-constant location at program setup. Across
  122 programs, active locations are consecutive and missing elements are
  trailing only. No interior hole was found to support the suspected constant
  truncation. The isolated match still runs (1,016 ticks / five shots) with
  visible artifacts.
- Replace mapped unsynchronized buffer writes with `glBufferSubData`. The
  diagnostic path is confirmed in the installed host's log. The match runs
  (703 ticks / five shots), but terrain striping and pale geometry remain in
  the source and on screen. This is not an adopted workaround.

Private evidence: `build59-geometry-trace/`, `build59-uniform-trace/` and
`build59-buffer-subdata/`. The Mac reference is not a matched-view comparison.
These checks narrow hypotheses; they do not establish an Apple driver bug or
prove the texture decoder/vertex shader correct.

### Build-60 sampling and equal-depth probes

Private evidence remains in `ref/xbox-build/passes/2026-10-01/`:

| Isolated match | Ticks / shots | Visual result |
| --- | --- | --- |
| `build60-no-anisotropy` | 841 / 6 | Startup confirms anisotropy disabled; cliff stripes remain. |
| `build60-depth-lequal` | 1,092 / 9 | Confirmed EQUAL calls replaced by LEQUAL; stripes remain. |
| `build60-depth-always` | 989 / 7 | EQUAL bypass substantially changes the artifacts, with different camera/timing. Not a rendering fix. |
| `build60-stationary-equal` | 1,205 / 0 | No scripted input/shooting/gathering; fixed camera within this run, visible stripes. |
| `build60-stationary-always` | 1,281 / 0 | Fixed camera within this run, but a different spawn; some stripes disappear and surfaces draw through others. |

These are visually reviewed 640x480 source captures, not only automated smoke
passes. Bypassing EQUAL can overdraw hidden surfaces; it must not become the
normal rendering path. The stationary runs do **not** yet establish a matched
comparison: logged camera positions differ, `(105.0, -162.2, 0.6)` versus
`(105.6, -157.6, 0.8)`. Removing automated movement is useful, but does not fix
the game's randomized spawn selection.

The runner now exposes existing extension hiding with `--render-diagnostics`.
An opt-in, **Simulator-only** host probe accepts `XG_DEPTH_COMPARE=lequal` or
`always`; unset, empty and unknown values leave native depth behavior unchanged.
The runner rejects an unknown diagnostic mode before launch. Normal and physical
player rendering are unchanged. `--stationary-match` requires both `--case match`
and `--render-diagnostics`; it records `stationary-render-diagnostic` in the result.
Its zero-shot result is a rendering check, **not** a combat regression pass. The
default scripted-combat pass still requires at least two shots. Four asset-free
tests cover default input settings, stationary settings and invalid options.

Example, with isolated saves and the dedicated Simulator only:

```sh
XG_DEPTH_COMPARE=always .venv/bin/python scripts/xbox/smoke-simulator.py \
  --device DF51182F-1878-4A54-9AED-CC4AED86BEAB --case match --seconds 65 \
  --render-diagnostics --stationary-match --out ref/xbox-build/passes/depth-probe
```

Next isolate one repeatable view within the **same process**, or replay one
captured asset-free draw/depth pair with identical inputs. Upstream already emits
`invariant gl_Position`, but [GLSL ES 3.00 section 4.6](https://registry.khronos.org/OpenGL/specs/es/3.0/GLSL_ES_Specification_3.00.pdf)
requires matching operations as well as inputs for cross-program invariance.
Algebraically equivalent vertex programs alone do not establish a driver bug.
Keep shader conversion, depth/bias state and the software renderer as separate
hypotheses until that reproduction exists. Do not globally relax depth checks.

### Same-process depth reversal and asset-free controls

`build60-depth-paired/` captures native EQUAL → ALWAYS → restored EQUAL in one
stationary match. Camera position remains `(98.7, -149.7, 0.7)` throughout.
Cliff stripes disappear during the bypass and return on restoration; the base
also shows incorrect surface overdraw during the bypass. All three source
captures were visually reviewed. The run reaches 3,365 ticks with zero shots;
this is a rendering experiment, not combat acceptance. This narrows the defect
to depth-dependent multipass drawing without establishing its precise cause.

`XG_DEPTH_COMPARE=paired` requires `--stationary-match` and rendering diagnostics.
The runner changes a private `presentation.depth-mode` control file. The host
reads it only in this Simulator diagnostic, between complete frames, and
reapplies the requested depth function even if upstream caches GL_EQUAL.
Each phase waits at least 18 seconds, requires a confirmed mode switch and fresh
source/destination PPMs. Complete PPM payload lengths are checked before either
snapshot is saved. Missing, stale or partial captures fail the pass. The local
helper now remains connected for the whole stationary experiment, not a fixed
70 seconds. Normal player and physical-device depth behavior stay unchanged.

`XG_DEPTH_PROBE=1` adds two **asset-free** controls before the guest has GL state:
a slanted perspective triangle in RGBA8 / DEPTH24_STENCIL8 is drawn, then
redrawn with EQUAL within one program and across separate linked programs.
The latter share invariant position code but differ in active varying use.
Both controls cover 6,728 pixels with zero failed pixels and GL error 0.
Evidence: `build60-depth-controls/`. This disproves a general EQUAL failure,
not every precision/invariance failure involving the game's converted shaders.

The hardened runner and probe were repeated together in
`build60-depth-paired-verified/`: 2,890 ticks, all three complete captures,
both controls passing, camera fixed at `(84.9, -161.7, 0.7)`. The same stripe
removal/restoration is visible in that run's separately reviewed images.
Eight asset-free runner tests now cover default/stationary settings, invalid
modes, pair prerequisites, both probe results and stale/missing/partial PPMs.

`XG_DUMP_SHADERS=1`, `HALO_GPU_TRACE=<frame>` and
`HALO_GPU_TRACE_CONSTANTS=1` expose upstream's existing private shader/draw
diagnostics only with `--render-diagnostics`. Frame 240 in
`build60-depth-draw-trace/` identifies terrain VS17 draws with depth writes and
LEQUAL, followed by VS41 EQUAL draws without depth writes; an observed pair has
matching 1,398-index counts and printed c[0..3] values. Their GLSL has the same
dot-based position expressions, with different surrounding instructions/order.
This is a concrete replay target, **not** proof of identical complete inputs.
The trace is valid world-frame evidence, but its original 70-second helper
later disconnects and the final capture is a menu; do not call that final frame
world gameplay. The helper-lifetime correction above prevents this in later
stationary runs. Shader files and constants remain private, outside Git.

### Read-only draw capture and numeric replay

`build60-draw-inputs-matched/` captures original VS17 LEQUAL/depth-write and VS41
EQUAL/no-write draws without relinking or changing their shaders. Both snapshots
are complete, GL error 0, and contain 144 indices. Expanded position bytes total
1,728 per pass and have the same SHA-256:
`f5bd2c45cca3fadcf11e25da05513b6eaa3f77efe265361db44606b483491ade`.
Their c[0..3], c[58..59], viewport scale/offset and screen offset are also
bit-identical, as are viewport, depth range and disabled polygon offset.
Stencil value masks differ (1 versus 3); correctness of that state is not assumed.
The run reaches 2,359 ticks without scripted shots. Its visually reviewed screen
still shows terrain stripes and pale geometry: capture did not resolve the defect.

`XG_CAPTURE_SHADER_DIR` selects the exact private VS17/41 sources and enables
`draw-capture/` only with rendering diagnostics. Buffers are mapped read-only;
COPY_READ binding is restored. The runner rejects incomplete captures. The first
attempt, `build60-draw-inputs/`, targeted the earlier 1,398-index count and missed
the new spawn's draws; its match passed but it produced no valid capture pair.
Selection now uses exact shader bytes, expected depth state and matching count.

`XG_DRAW_REPLAY=<capture root>` replays actual attribute bytes and vertex-uniform
bits before guest GL initialization. Transform feedback changes shader linkage;
pixel constants, textures and depth/stencil rasterization are not replayed.
This is numeric diagnostic evidence, not full original-pipeline fidelity proof.
The first replay (`build60-draw-replay/`) incorrectly used indexed draws during
active transform feedback, yielding GL_INVALID_OPERATION / zero outputs. ES3
forbids that combination; this was a probe error, not evidence of a driver defect.
The corrected helper deindexes every enabled attribute in original index order
and uses DrawArrays. It rejects gl_VertexID-dependent sources.

`build60-draw-replay-expanded/` produces 144 vertices per pass, GL error 0, with
bit-identical coordinates: zero changed vertices, maximum component and NDC-depth
deltas both 0. Output length and finite-number checks are now required by the
runner, alongside the native completion log. The capture and replay modules are
Simulator-only and inactive without their explicit private diagnostic inputs.
All generated shaders, constants and buffers stay under ignored `ref/`.

The hardened runner repeats the replay successfully in
`build60-draw-replay-verified/`, including its required numeric-output checks.
Normal `build60-draw-normal-regression/` menu/campaign/scripted-match gates also
pass (1,277 match ticks / nine shots), with campaign and match frames reviewed.
Python suite: 93 tests run, 16 skipped, no failures. Physical SDK syntax checks
pass; no hardware install occurred. The real save directory remains byte-identical
to the build-60 update backup after reinstall, all diagnostics and About navigation.
Actual About shows `bfbac357` without PREVIEW; Done returns to the normal picker
(`build60-draw-final-picker.png`). Latest release lookup still reports build 60.

Next isolate original-program raster/depth/stencil state for this exact pair,
accounting for replay's changed linkage. Do not infer a driver bug from the
controls or numeric equality, or replace EQUAL with a global tolerance/bypass.

### Depth rasterization beyond transform feedback

`build60-stencil-paired/` compares native stencil → bypass → restored native in
one stationary match, camera `(85.2, -157.8, 0.6)`, 3,007 ticks, no scripted shots.
All three source frames were reviewed; stripes and pale geometry remain. The
temporary stencil toggle was removed from source after the experiment. This
does not establish that all stencil state is correct. The older guest-state
trace reports disabled stencil on its terrain draws, but the later native
capture below observes enabled stencil on both passes: those observations must
not be generalized across frames. Captures now record actual enable flags and
attachment type/bit depth, rather than relying only on function/mask values.

`build60-draw-inputs-large/` uses `XG_CAPTURE_MIN_INDICES=1000` to select a larger
VS17/41 pair rather than the first small draw. Both snapshots contain 1,257
indices, exact projection identity and 15,084 identical position bytes (SHA-256
`5c3ff898384ea4c6ab8e6b6ff6c9dfab094996dbda1a3e1cb3d31415173aaeb2`).
The actual stencil test is enabled for this pair. Its visually reviewed match
still shows stripes and pale geometry (2,411 ticks, no scripted shots).

`XG_DRAW_RASTER=renderbuffer|texture` extends private replay to a 640x480 RGBA8 /
DEPTH24_STENCIL8 target. It links the captured, unmodified vertex and fragment
sources **without transform feedback**, and draws original indexed buffer
layouts. Base LEQUAL writes depth; a same-linked-program EQUAL redraw controls
the baseline; then VS41 EQUAL uses that depth. An ALWAYS reference reuses the
same VS41 program. Defined black 2D/cube textures keep fragment sampling complete;
pixel uniforms retain defaults, stencil/blend/cull/scissor/offset are disabled.
This isolates depth, **not** original scene/texture fidelity or all game state.
Unsupported viewport/depth ranges, incomplete outputs, empty coverage and
fragment sources using discard/FragDepth fail closed.

The first small pair (`build60-draw-raster/`) covers 3,785 pixels with no EQUAL
loss. Larger preliminary texture/renderbuffer runs also preserve all 145,994
base pixels under EQUAL. However, their ALWAYS reference used another linked
program and incomplete textures; texture coverage differed unexpectedly. Those
ALWAYS counts are not reliable driver-defect evidence. The hardened
`build60-draw-raster-defined-texture/` repeat reuses each control program and
binds complete black textures: 145,994 base/EQUAL pixels, zero same-program
missing pixels, zero EQUAL missing pixels, zero ALWAYS coverage differences,
GL error 0. Numeric coordinates remain bit-identical for all 1,257 vertices.
This is a passing concrete shader/depth case; the live scene defect remains.
The hardened renderbuffer repeat (`build60-draw-raster-defined-renderbuffer/`)
produces exactly the same coverage counts and zero losses/differences.

`build60-draw-state-verified/` verifies the expanded live metadata on another
1,755-index pair (1,201 ticks, no scripted shots): actual color/depth attachments
are textures with 8 red / 24 depth bits; depth, stencil, scissor and culling are
enabled; base blend is disabled and second-pass blend enabled. Both captures are
complete with GL error 0 and exact position/projection identity. The reviewed
screen still has stripes and pale geometry. These are observations for that
pair, not assumptions about all draws or a complete scene replay.

Final normal regression (`build60-raster-normal-regression/`) passes menu,
campaign opening and scripted match (1,235 ticks / nine shots), with frames
reviewed. 99 Python tests run, 16 skipped, no failures; physical SDK syntax and
tree/index safety guards pass. The pre-existing frame-0 GL error remains in normal
logs, so no wholly error-free-renderer claim is made. Real Xbox saves still match
the build-60 update backup byte-for-byte. About/Done verifies accepted `bfbac357`
and returns to the normal picker (`build60-raster-final-picker.png`). No physical
install, IPA or publication; the broader goal remains active.

### Live texture state and upload identity (2026-10-01)

Expanded opt-in snapshots record framebuffer/attachment objects, scissor,
blend/cull modes, four sampler bindings/filter/wrap/LOD values, 2D/cube/3D
bindings, mip bounds, texture swizzles and program sampler-unit assignments.
`build60-texture-state/` captures a complete 1,257-index pair with GL error 0
(1,312 ticks). Both passes use draw/read FBO 3, color texture 1, depth texture 2,
and scissor 0/0/640/480. No sampled binding aliases those attachment objects.
This rules out a switched target for this pair, not intervening depth writes.

`XG_CAPTURE_TEXTURES=1` additionally observes 2D level-0 allocations and reads
their pixels using a separate READ framebuffer, restoring read-target/pack state.
It requires shader capture and render diagnostics; buffers/previews stay private
under ignored `ref/`. `build60-texture-pixels/` passes capture validation on a
1,086-index pair (1,297 ticks), with six reviewed texture previews. Texture 83,
256x128, has pronounced bands/atlas-like regions. This is a lead, not a decoder
diagnosis: padding may be legitimate and previews show raw storage without
applying texture swizzles. It is not the same texture as the later random-spawn
capture's 128x256 texture 85.

`build60-texture-upload/` adds CPU references for supported tightly packed RGBA8
uploads, invalidating them after level-0 TexSubImage updates. All supported
references match readbacks byte-for-byte: base textures 86 (4x4) and 85 (128x256),
second-pass textures 102 (256x256), 103 (512x512) and 104 (256x256). Every capture
is complete/error 0; all five unique previews were reviewed. The 1,755-index
pair has exact position identity and the same framebuffer/attachment/scissor
values (1,292 ticks). The reviewed live frame still has terrain stripes and
pale geometry. GPU writes are not tracked by the upload observer; a future
mismatch alone would not establish a driver bug. Cube faces and higher mips are
not read. Capturing bound 2D objects also does not mean every object is actively
sampled by that fragment program.

Next compare Xbox CPU decoding of rectangular textures/lightmap data with an
independent reference, and inspect intervening depth writes/full scene state.
Matching upload/storage bytes does not prove decoding, UVs, mipmaps or sampling
correctness. Neither the texture evidence nor passing depth controls justify a
general EQUAL bypass or a software-driver diagnosis. Normal rendering remains
unchanged; these hooks are opt-in and Simulator-only.

Final `build60-texture-normal-regression/` passes normal menu/campaign-opening/
scripted-match checks (1,347 ticks / 11 shots), with opening and match frames
reviewed. 105 Python tests run, 16 skipped, no failures; physical SDK syntax and
tree/index safety pass. Real Xbox saves remain byte-identical to the update
backup. About shows accepted `bfbac357`, no PREVIEW; Done returns to the normal
picker (`build60-texture-final-picker.png`). Latest upstream release remains
build 60 at 08:47 UTC. No physical install, IPA, push or publication.

### Original Xbox RGB565 bytes independently decoded (2026-10-01)

`XG_CAPTURE_XBOX_TEXTURES=1` extends the opt-in texture capture with original
level-0 RGB565 bytes. The runner resolves `texture_buckets` from the exact
installed ELF after checking its manifest hash. The host reads the arm64_32
cache ABI using bounded, failure-returning `vm_read_overwrite` calls, validates
the matching GL object/dimensions/format/physical-window range and copies source
bytes without calling or writing guest code. The cache ABI is specific to the
reviewed pinned upstream structure, not a universal decoder interface. TexSubImage
invalidates these references. Other formats are reported unsupported, not silently
treated as independently verified. Source reads are Simulator-only.

`scripts/xbox/texture_decode.py` independently constructs Morton addresses from
coordinate bits and expands RGB565 into pre-swizzle BGRA storage. Rectangular
fixtures check the layout documented in
[xemu's reference implementation](https://github.com/xemu-project/xemu/blob/master/hw/xbox/nv2a/pgraph/swizzle.c),
including 8x32 masks and exhaustive 256x128/128x256 address bijections. The oracle
does not reuse upstream's mask/spread code. Fixtures also cover primary colors,
linear row padding, missing/ambiguous cache symbols, malformed captures and an
actual differing-pixel result. Empty/unsupported-only comparisons cannot pass.

`build60-xbox-source/` passes a 65-second stationary match (1,350 ticks, no shots),
with complete/error-0 1,086-index captures and exact position identity. Texture
87 is RGB565, 256x128, swizzled, with 65,536 original bytes: all 32,768 decoded
pixels match GPU storage and the CPU upload, zero differing pixels. Its storage
hash `f4f84c68…946bc5f` is identical to the earlier banded texture 83. Thus the
earlier visual lead is now checked directly, not inferred from another texture.
The preview and live scene were reviewed; stripes and pale geometry remain.

This rules out the tested texture's unswizzle/RGB565 conversion and upload as
the source of differing storage bytes. It does not prove the game produced the
right source bytes, correct UV/sample/shader behavior, other formats, cube faces,
higher mips or all scene draws. Next inspect intervening live depth writes and
remaining sampling/shader state; do not relax EQUAL or declare a driver defect.
The first syntax check used unsupported Simulator `mach_vm.h`; it was replaced
with the available `vm_read_overwrite` API and a full-width-address assertion
before building/running the successful capture.

After hardening the symbol-overflow bound and rebuilding/reinstalling,
`build60-xbox-source-final/` repeats the independent comparison with a different
128x256 RGB565 texture 73: all 32,768 pixels match, zero differences. Both
1,080-index snapshots are complete/error 0 with exact position identity;
1,354 ticks, no scripted shots. Preview and live frame reviewed; artifacts
remain. This adds actual opposite-orientation evidence, not every texture-format
acceptance or a corrected scene.

Final hardened app normal gates (`build60-xbox-source-final-normal/`) pass menu,
campaign opening and scripted match (1,305 ticks / ten shots). Frames reviewed;
113 Python tests, 16 skipped, no failures; Simulator/physical SDK syntax and
tree/index safety pass. About/Done verifies accepted `bfbac357` without PREVIEW
and returns to the normal picker (`build60-xbox-source-final-picker.png`). Real
Xbox saves remain byte-identical to the update backup. Upstream latest is still
build 60 at 09:17 UTC. No physical install, IPA, push or publication.

An auxiliary Mac CPU-texture match (`build59-mac-cpu-textures/`) overlapped the
Simulator match and failed with `WSAEADDRINUSE` before gameplay. It is invalid
rendering evidence: network cases must run serially because they use the same
host port. The subsequent signal does not establish a texture-decoding defect.

The Mac smoke runner now checks a build manifest with executable and guest
hashes before launch, and reports that manifest's revision rather than whichever
checkout happens to exist later. Relative evidence paths are resolved to absolute
paths before being passed to the game. Five asset-free tests cover exact,
changed-image, changed-executable, missing-manifest and missing-revision cases.

## Next focused pass

1. Continue beyond the cryo-bay training to weapon pickup/combat and a later
   checkpoint. Test sustained movement, simultaneous look/fire, weapon switching
   and another cold reload with isolated saves.
2. Use a small reproducible GL/shader case to narrow remaining geometry and
   terrain artifacts. Keep diagnostics out of normal player builds. If this
   proves a remaining software-driver limitation, evaluate a properly built
   Simulator ANGLE/Metal host; do not patch platform tags on Mac binaries.
3. For the next frozen candidate, repeat menu/campaign/match and visible
   navigation/save reload before accepting its pin. Full campaign/save
   progression and physical acceptance remain distinct later gates.

Maintenance policy: check releases regularly (proposed weekly), freeze one
candidate per pass, back up saves, build, test, review frames, then accept.
No automatic upstream executable downloads and no recurring job were created.

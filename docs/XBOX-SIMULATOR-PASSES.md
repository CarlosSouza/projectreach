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

## Build-61 follow-up (current pin)

Upstream [build 61](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-61),
`f8937c6179757774c75f4e7d36de446fabd3dcc8`, published 09:40 UTC on October 1,
adds high-resolution HUD textures. Source review covers PNG embedding, tag lookup,
cache fields and sampler changes. The original-byte diagnostic rejects this
unreviewed cache ABI; normal builds/gameplay are unaffected. The update report
now includes renderer/cache/HUD/embedder changes, not just Android imports.

Before promotion, `build61-candidate-update.log` and
`{smoke,simulator}-results/20261001-185807-f8937c61/` pass all six normal cases:
Mac 2,130 ticks / 16 shots; Simulator 1,324 ticks / 11 shots. Frames reviewed:
HUD/menu visible, terrain stripes/pale geometry remain. Defaults enable the
new HUD and logs recognize all 15 campaign/match HUD bitmaps, without a PNG
decode-failure log. This does not establish every texture's pixel fidelity or
physical performance. The unaccepted app was correctly labeled PREVIEW.

A copy of the earlier build-60 save fixture (originally the build-59 checkpoint)
loads through normal picker → Xbox → Campaign → New001 → Pillar of Autumn →
Normal and restores the cryo-bay/look tutorial, not the ship cinematic. Evidence:
`campaign61-compat.xCtR9E/checkpoint.png` and logs. The real saves and original
fixture remain separate; one checkpoint is not all-save compatibility.

After review, `build61-accept.log` and
`{smoke,simulator}-results/20261001-191048-f8937c61/` repeat all six cases:
Mac 2,130 ticks / 17 shots; Simulator 1,358 ticks / 11 shots. Reviewed match and
Simulator menu/campaign frames retain the same stated fidelity limits. The lock
now accepts exact `f8937c61`; app rebuilt in place without a candidate override.
Backups `20261001-{185807,191048}-from-bfbac357/simulator-save` match the earlier
update backup. Final rebuilt-app normal Simulator gates (`build61-final-normal/`)
pass all three cases (1,348 ticks / nine shots). No physical install, Xbox IPA,
upstream binary download or publication.

Final accepted-app campaign/match frames reviewed; defects remain. About shows
accepted `f8937c61` without PREVIEW; Done returns to the normal picker
(`build61-final-picker.png`). Windows card reaches its existing missing-data
setup (`build61-windows-setup.png`), not Windows gameplay/license acceptance.
Real saves in final container `12A61E04-970B-439E-A567-C0AFA5DD5F54` remain
byte-identical to the acceptance backup after all probes/installs/navigation.
123 tests run, 16 skipped, no failures; both SDK syntax, shell syntax and
tree/index safety pass. Latest upstream remains 61 at 10:24 UTC.

## Build-60 follow-up (previous pin)

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

### Live paired-draw depth observation (2026-10-01)

`XG_CAPTURE_DEPTH=1`, with shader capture and render diagnostics, observes the
native depth texture immediately before/after each selected base/EQUAL draw.
The game texture is sampled, never written: a separate RGBA8 target stores
normalized float32 sample bits using
[texelFetch](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/texelFetch.xhtml)
and [floatBitsToUint](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/floatBitsToInt.xhtml).
These are sampled normalized values, not a raw integer D24 memory dump. The
helper restores program, VAO, draw/read targets, renderbuffer, texture/sampler/
active unit, viewport, color mask, affected enables and pack/unpack state.
Only owned temporary resources are deleted; no original game shader is relinked.
Private outputs remain under ignored ref/.

Each snapshot calibrates its sampler/bit-packing path against an owned 32F depth
texture containing exact binary fractions .25/.5/.75/1. Unsupported targets,
GL/restore errors, failed calibration, incomplete files, invalid/nonfinite
samples, different targets or crossed presentation boundaries fail closed.
The paired draw positions/projection must match, and the base draw must produce
a measurable depth response. An explicit Simulator presentation counter prevents
silently pairing stationary geometry from different frames.

The preliminary `build60-live-depth/` capture (before calibration/frame hardening)
has 1,377 matching indices, complete/error-0 captures, 1,313 ticks, no scripted
shots. It measures 55,587 base depth changes and 216,494 later changes; 35,098
base-changed pixels later become closer, none farther. EQUAL writes no depth.
The live frame still has stripes/pale geometry. This is preliminary observation,
not a same-frame/calibrated causal proof, and the hardened validator intentionally
rejects these older files. Later closer surfaces can be legitimate occlusion;
do not call these writes a renderer bug merely because they occur.

The hardened `build60-live-depth-calibrated/` repeat passes 65 stationary seconds,
1,345 ticks, no scripted shots and a complete/error-0 1,320-index pair with exact
positions/projection. All four observations calibrate and restore without GL
errors, on depth texture 2 in presentation frame 120. The base changes 163,265
pixels; 54,517 pixels change between draws. Of the base-changed pixels, 21,611
later become closer, none farther. EQUAL writes no depth. Live frame reviewed:
stripes/pale geometry remain. This establishes intervening depth changes within
one frame, not incorrect writes or the cause of the stripes. A changed-depth mask
is not total raster coverage: fragments equal to existing depth leave no change.
Next correlate native EQUAL coverage with copied live depth/full pixel state,
distinguishing legitimate occlusion from missing visible terrain. Keep normal
depth semantics unchanged.

Final normal regression `build60-live-depth-normal-regression/` passes menu,
campaign opening and scripted match (1,303 ticks / eight shots); frames reviewed,
artifacts remain. 122 tests run, 16 skipped, no failures; both SDK syntax and
tree/index safety pass. About confirms accepted `bfbac357` without PREVIEW; Done
returns to the picker (`build60-live-depth-final-picker.png`). Rediscovered real
save container `2D21FB1C-C826-4A94-8715-68D38276A18C` matches the update backup
byte-for-byte after final navigation. At 09:53 UTC upstream latest is now
[build 61](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-61)
(`f8937c6179757774c75f4e7d36de446fabd3dcc8`, published 09:40 UTC), adding
high-resolution HUD textures. This appeared during the frozen build-60 pass:
next validate 61 as a separate save-backed candidate before pin promotion.
Its new `hires`/`override` cache fields invalidate the optional original-byte
reader's old ABI. The runner now rejects that diagnostic on unreviewed revisions
before resolving/passing the cache address (123 tests after the guard, 16 skips).
Review the new ABI before re-enabling it on 61; normal update gates are unaffected.

An auxiliary Mac CPU-texture match (`build59-mac-cpu-textures/`) overlapped the
Simulator match and failed with `WSAEADDRINUSE` before gameplay. It is invalid
rendering evidence: network cases must run serially because they use the same
host port. The subsequent signal does not establish a texture-decoding defect.

The Mac smoke runner now checks a build manifest with executable and guest
hashes before launch, and reports that manifest's revision rather than whichever
checkout happens to exist later. Relative evidence paths are resolved to absolute
paths before being passed to the game. Five asset-free tests cover exact,
changed-image, changed-executable, missing-manifest and missing-revision cases.

### Original native-linked pixel/depth replay on build 61 (2026-10-01)

The frozen accepted `f8937c61` pass adds `XG_CAPTURE_NATIVE_PIXELS=1` to
render diagnostics, requiring shader capture and calibrated depth observation.
Immediately before the selected EQUAL draw, it copies live RGBA8 color and
D24S8 depth/stencil into owned, same-format texture-backed targets using
[GL framebuffer blit](https://registry.khronos.org/OpenGL-Refpages/es3.0/html/glBlitFramebuffer.xhtml).
It retains the actual linked program, VAO/index stream, all uniforms, textures,
samplers, viewport, blending, masks, stencil and raster state. EQUAL, ALWAYS and
EQUAL-repeat each start from a fresh copy. Only owned targets are drawn into;
the normal game draw still runs with its original depth function. Saved target,
texture/unpack, depth-function, scissor and readback state are restored.
Active queries, unsupported layouts/formats, GL/restore errors and missing
files fail closed. No original shader is relinked.

The first renderbuffer-backed experiment (`build61-native-pixels/`) is
**rejected**: the replay repeats exactly but differs from actual live color at
1,161 pixels by one/two channel steps. Its identical color-response masks do
not satisfy exact pixel-fidelity acceptance. The next texture-backed run
(`build61-native-textures/`) captures no qualifying pair at the 1,000-index
threshold in its different spawn; also rejected, not a rendering regression.

`build61-native-calibrated/` uses a 144-index minimum and captures a larger
1,944-index pair. All captures are complete/error 0 with exact position/projection
identity, depth texture 2, presentation frame 96. The private depth copy is
independently sampled/calibrated and matches the original pre-EQUAL depth
byte-for-byte. Native EQUAL matches EQUAL-repeat and the actual live draw
byte-for-byte. Stationary match completes 65 seconds, 1,091 ticks, no scripted
shots; original private frames reviewed. Stripes/pale or missing surfaces persist.

The base changes 60,763 depth samples; 176,212 samples change before EQUAL,
including 24,262 base-written pixels becoming closer. EQUAL writes no depth.
Native EQUAL changes color at 36,501 pixels; ALWAYS at 61,643. There are 27,701
color-value differences, including 2,559 at unchanged base depth. Those are
**not** all missing responses: ignoring depth changes ordering among overlapping
triangles too. Direct response-set comparison finds 25,142 ALWAYS-only pixels;
24,262 are at base-written pixels with later closer depth, **zero** at base-written
pixels with unchanged depth. Preview shows ALWAYS painting a large hidden rock
surface across nearer structures. The 880 remaining ALWAYS-only pixels had no
measurable base depth change; that mask is not full raster coverage.

This pair does not prove missing visible terrain, nor that all intervening
occlusion is correct. It closes the original-linkage/full-pixel replay gap for
this draw and supplies a control for the next, stripe-producing material draw.
The strict validator distinguishes changed color values from newly visible
responses; synthetic tests do not establish game/driver acceptance.

Chris's contemporaneous report of physical-iPad graphics/shading coming in and
out of focus remains an unresolved gate. No direct physical observation or
installation occurred here. Simulator menu/save success is not visual parity
with Windows or physical gameplay acceptance.

Final normal `build61-native-normal/` regression passes menu, campaign opening
and scripted match (1,180 ticks / nine shots), with render/input diagnostics
disabled; campaign/match screens reviewed and defects remain. About confirms
accepted `f8937c61` without PREVIEW; Done returns to the normal edition picker.
Real Xbox saves in rediscovered container `509882A5-2652-4667-A5B0-EAB43C24BFE7`
remain byte-identical to the acceptance backup. 132 Python tests, 16 skipped,
no failures; both SDK syntax and tree/index safety pass. No physical install,
IPA, upstream pin change, push or publication.

### One-frame draw timeline localizes wall stripes (2026-10-01)

`XG_CAPTURE_BASE_SKIP` selects a later eligible base batch (0..64); the normal
default remains the first. The runner validates and canonicalizes its value.
`XG_TRACE_COLOR_FRAME` records original before/after color for indexed and
immediate native GL draws in one selected presentation frame. It reuses the
state-restoring color reader, records original program sources and draw state,
and never draws, relinks, changes uniforms or changes the game target. Viewport
filter is 640x480 at origin; overflow beyond 1,024 draws fails closed. It observes
draws, not every clear/blit command. Metadata/state/frame changes, GL errors,
missing sources, unpaired captures and short files reject the timeline. Normal
player launches do not enable these flags.

`build61-color-timeline/` retains exact pin 61 and runs 90 stationary seconds,
2,015 ticks, no scripted shots. Its second eligible VS17/VS41 pair has 984
indices in frame 101 and complete/error-0 depth snapshots with exact input and
projection identity. However native EQUAL-repeat differs at 682 pixels and
actual live color at 9,932: **that native replay is rejected**, and the aggregate
diagnostic reports failure. Do not use its ALWAYS comparison as causal proof.

Independently, all **211** frame-120 original before/after draw pairs pass the
timeline validator, with unchanged recorded state, GL error 0 and framebuffer
3 throughout. Reviewed an RGB-only contact sheet and full-resolution stage
images; normal RGBA previews misleadingly show transparent early alpha-writing
passes, so RGB review does not composite against a white background.

The obvious horizontal bands on the rear walls first appear in the inspected
before/after pair **0119**: program 84, GL_TRIANGLES, 402 indices, GL_EQUAL.
Its pre-draw walls are unstriped; its post-draw walls have the bands. The draw
changes 51,756 color pixels; that count is not a count of bad pixels. Exact
original source matches `vs007_0.glsl` / `ps_0c014f79.glsl` in the existing
shader dump. The earlier 984-index VS41 material pass textures the ground
(draw 0080), not this stripe-producing detail pass. This changes the next
experiment: capture the corresponding **VS17/VS7** batch, preserving original
indices, position/projection constants, pixel state and calibrated live depth.
Prior VS41 invariance/replay controls do not prove VS7 invariance.

Private evidence: `match/color-contact-rgb.png`, `match/color-stages.png`, and
all original binary/JSON/source captures under `match/draw-capture/color-trace/`.
This localizes the symptom's draw; it does not prove why that draw is wrong or
establish a driver bug. Remaining pale/missing geometry and reported physical
focus/shading instability are still open. Normal depth semantics are unchanged.

Final normal `build61-color-normal/` automated gates pass all three cases,
1,254 match ticks / ten shots, with diagnostics disabled. Campaign/match screens
reviewed: the match image still has severe missing terrain exposing background,
so this is **not visual acceptance**. Actual About/Done confirms accepted
`f8937c61` without PREVIEW and leaves the normal picker. Real saves in container
`1A5AEFF0-09B3-47DB-94BD-D8C371C56AEC` remain byte-identical to the acceptance
backup. 140 Python tests, 16 skipped, no failures; both SDK syntax, whitespace
and tree/index safety pass. No physical install, pin update, IPA or publication.

### VS7 capture and coordinate replay (2026-10-01)

The capture runner now accepts `XG_CAPTURE_EQUAL_SHADER=vs007_0.glsl` or
`vs041_0.glsl`, requires the shader directory, and records the selection.
The default is still VS41; normal launches enable neither capture nor replay.
No production shader, depth comparison, texture decoding or upstream pin changes.
App build/install evidence: `docs/artifacts/2026-10-01/G3/ios-app-20261001T113653Z`;
private library/app logs are `build61-vs7-library.log` / `build61-vs7-app.log`.

Three stationary captures under the same frozen build 61:

- `build61-vs7-live/`: 228 indices, frame 104. Position/projection identity and
  depth controls pass; 23,876 base-written pixels have later closer depth, none
  farther. Native replay repeats and matches the live draw exactly, but both
  EQUAL and ALWAYS have zero color response. **Inconclusive/rejected.**
- `build61-vs7-first/`: 531 indices, frame 108. Identity/depth controls pass;
  11,202 base-written pixels have later closer depth, none farther. Exact repeat
  and live color, but again zero EQUAL/ALWAYS response. **Inconclusive/rejected.**
- `build61-vs7-second/`: 1,686 indices, frame 103, one eligible base skipped.
  Identity/depth controls pass; 17,649 base-written pixels have later closer
  depth, none farther. Native repeat and actual live color each differ from the
  cloned EQUAL draw at 43 pixels. **Rejected exact-fidelity control.** Do not
  interpret its ALWAYS comparison as causal proof of missing visible terrain.

All three EQUAL draws leave depth unchanged. These sampled VS7 materials are
not established as the previously observed stripe-producing wall draw. A
shader-name match is not a material/visibility match. The first capture uses
16x16 textures on units 0/1; that observation and zero response do not alone
prove why the draw is inactive.

`build61-vs7-clip/` replays the saved 1,686-index pair on the live Simulator
using transform feedback before guest GL initialization. Menu smoke and replay
pass: all vertices bit-identical, changed vertices 0, component/depth deltas 0.
Transform feedback changes shader linkage and does not retain native pixel
behavior. It is evidence against a simple coordinate mismatch in that replay,
not original native invariance, a driver-bug finding or a visual fix. No isolated
raster proof was attempted: this pixel shader's discard path is rejected by the
existing simplified raster guard. Keep those guards strict.

Next correlate a visibly stripe-producing VS7 draw's original color response
with its material textures/alpha inputs, then repeat the exact native controls.
Do not keep sampling arbitrary batches solely because they use the same shader.
Chris's physical-iPad shading/focus report remains unresolved; this pass has no
physical observation/install, pin update, IPA or publication.

Final normal `build61-vs7-normal/` regression passes menu, campaign opening and
scripted match (1,261 ticks / nine shots), with diagnostics disabled. Reviewed
campaign/match images: severe wall banding remains, so this is not visual
acceptance. Actual About/Done confirms accepted `f8937c61`, no PREVIEW, and
returns to the normal Windows/Xbox picker. Real saves in rediscovered container
`DA23CE8C-1C94-4472-9788-C9AA87C62AA2` match the acceptance backup byte-for-byte.
141 Python tests, 16 skipped, no failures; both SDK syntax, whitespace and
tree/index safety pass. Goal active; this pass is diagnostic progress only.

### Original material inputs retained at the visible stripe draw (2026-10-01)

`XG_TRACE_MATERIALS=1` requires the existing color-frame and texture-capture
flags. It matches exact VS7/`ps_0c014f79` source, rather than selecting arbitrary
shader-name batches. Separate before/after folders retain level-0 bytes for
the two active 2D samplers, actual sampler/texture addressing/filter/swizzle
state, alpha/combiner/UV constants and all active vertex constants. The observer
reads existing state; it never relinks, changes uniforms or draws to the game
target. Optimized-out uniforms are omitted, not invented. Reuses the existing
texture reader. Validation rejects missing materials, uniforms or sampler stages,
nonfinite uniform bits, incomplete/cross-frame/error captures, state changes,
short/outside-path files, inconsistent upload comparisons and texture changes.
Normal launches do not enable it. Unsupported cube/mip behavior is not validated.

`build61-material-timeline/`: frozen accepted 61, 90 stationary seconds,
2,083 ticks, no scripted shots, all automated diagnostic checks pass. Original
frame 120 has **211 draws / 17 matching material pairs**, all with unchanged
recorded uniforms and level-0 bytes. Reviewed RGB before/after **draw 117**:
program 82, 402 indices, EQUAL; it changes the rear walls from smooth/unstriped
shading to obvious bands (51,756 changed color pixels, not proven bad pixels).
Both active textures (objects 116/117) are 16x16, sampled linearly with clamp
to edge, base/max level 0; before/after readbacks match CPU uploads exactly.
They are gradient textures, not evidence of missing placeholder assets.
Alpha reference is 0. Recorded combiner inputs make the second texture's
stage-0 alpha term zero for this draw; this observation is not a GPU alpha/UV
or native-depth proof. Texture upload changes during this draw are excluded
for these samples; incorrect interpolation, shader semantics or depth remain open.

Added diagnostic `XG_CAPTURE_INDEX_COUNT` (3..100000) to constrain paired
capture, keeping its original default unrestricted. Counts alone do not identify
a material or a repeatable scene. `build61-material-402/` uses the filter plus
native/depth/material flags, 90 seconds, 2,034 ticks, no scripted shots. Its
different spawn never draws the requested 402-index pair, so paired/depth/native
captures are missing and the aggregate **fails**. Do not retry random spawns as
a substitute for a matched fixture. Independently, its original frame-120 trace
passes with **294 draws / 22 unchanged material pairs**. Reviewed draw 176,
1,377 indices, EQUAL: smooth walls become banded (12,988 changed color pixels).
The symptom follows the material pass in both views, but no new full native
pixel/depth proof was obtained.

Both logs identify OpenGL ES 3.0 APPLE-23.1.1 on Apple Software Renderer. The
physical Windows edition uses a different rendering route; no causal link to
Chris's physical focus/shading report is established. Next either attach native
controls to the visible material draw in its own trace frame, or test a properly
built Simulator ANGLE/Metal candidate without treating it as a proven driver fix.
Do not retag a Mac library as Simulator code or change normal EQUAL semantics.

Source build/install evidence: `ios-app-20261001T120429Z` and final filtered
app `ios-app-20261001T120952Z` under `docs/artifacts/2026-10-01/G3/`; library/app
logs have matching `build61-material[-count]-` prefixes in private pass evidence.

Final normal `build61-material-normal/`: automated menu/campaign/match gates
pass, 1,295 match ticks / nine shots, no render/input diagnostics. Campaign and
match screens reviewed; severe banding and pale/missing terrain remain. This is
not visual acceptance. Actual About/Done confirms accepted `f8937c61`, no
PREVIEW, and leaves the normal Windows/Xbox picker. Real saves in rediscovered
container `647C8BF3-6DC3-48A5-81A2-89654BF1AEE5` remain byte-identical to the
acceptance backup. 150 tests, 16 skipped, no failures; both SDK syntax and
whitespace/tree/index safety pass. Upstream checkout stays clean; no physical
install, pin promotion, IPA, push or publication. Goal active.

## Historical next-pass plan (build 61; superseded by later passes)

0. Keep accepted build 61 (`f8937c61`) frozen for the next diagnostic pass;
   retain build-60 evidence under its original identity. Original-byte capture
   remains unavailable on 61 until its changed cache ABI is reviewed/adapted.
1. Continue beyond the cryo-bay training to weapon pickup/combat and a later
   checkpoint. Test sustained movement, simultaneous look/fire, weapon switching
   and another cold reload with isolated saves.
2. Material inputs are now retained at the visibly stripe-producing VS7 draw.
   Attach native controls to that original draw within its own frame; counts
   and spawns do not repeat reliably. VS41 is different. Retain
   the new exact native repeat/live-color and calibrated copied-depth controls.
   Correlate color-response changes with copied live depth. Determine whether closer intervening
   surfaces legitimately occlude it or visible terrain fails EQUAL. Keep
   diagnostics out of normal player builds. If this
   proves a remaining software-driver limitation, evaluate a properly built
   Simulator ANGLE/Metal host; do not patch platform tags on Mac binaries.
3. For the next frozen candidate, repeat menu/campaign/match and visible
   navigation/save reload before accepting its pin. Full campaign/save
   progression and physical acceptance remain distinct later gates.

Maintenance policy: check releases regularly (proposed weekly), freeze one
candidate per pass, back up saves, build, test, review frames, then accept.
No automatic upstream executable downloads and no recurring job were created.

### Real Simulator ANGLE/Metal candidate and texture-swizzle control (2026-10-01)

Changed the experiment after the original Apple material observer: build a real
Simulator backend, not a retagged Mac library. Xbox guest remains accepted build
61, `f8937c6179757774c75f4e7d36de446fabd3dcc8`; no pin promotion. New optional
`HALOPAD_XBOX_RENDERER=angle-metal` uses EGL/CAMetalLayer and does not link
Apple OpenGLES. Apple/physical defaults remain unchanged. The combined archive
and manifest have a separate `iphonesimulator-angle` directory. Packaging refuses
missing/stale/mismatched candidate inputs or a physical target, and always labels
this renderer PREVIEW, independently of guest revision.

Source: sparse official WebKit checkout, revision
`a1fb7ce122d0cd99f7d6cc82775f02565e266ece`, vendored ANGLE `eb725ace1839`,
position 28912. No edits to either third-party checkout. The small wrapper uses
WebKit's maintained compiler/GLES/Metal source lists (342 objects) with the actual
iPhoneSimulator SDK 27.0. Standalone probe platform readback is IOSSIMULATOR.
Live renderer is `ANGLE Metal Renderer: Apple iOS simulator GPU` on iPadOS 26.5.
See [XBOX-ENGINE.md](XBOX-ENGINE.md) for the reproducible source-build/probe commands.

Private evidence root: `ref/xbox-build/passes/2026-10-01/`:

| Run | Automated result | Reviewed image / limits |
|---|---|---|
| `build61-angle-normal` | Menu fails black ten-second dump; campaign and match pass (1,536 ticks / 12 shots) | Fifteen-second menu screenshot is visible. Match lacks pronounced wall bands but terrain is blue / sky orange; not accepted. |
| `build61-angle-menu30` | Menu, renderer and presentation gates pass | Later dumps contain the menu. Preserve initial failure; allow 30 seconds for cold ANGLE menu capture, with image gate unchanged. |
| `build61-angle-stationary` | 45 seconds, 930 ticks / zero shots; presentation passes | Bands absent in reviewed view, but red/blue reversal remains. |
| `build61-angle-final` | All normal cases pass, 1,530 ticks / 12 shots | Still the uncorrected-color backend. Automated pass is not visual acceptance. |
| `build61-angle-swizzle` | All normal cases pass, 1,530 ticks / 12 shots | Reviewed menu/campaign/match: restored colors, no pronounced cliff bands in sampled views. |
| `build61-angle-swizzle-stationary` | 45 seconds, 933 ticks / zero shots; renderer/presentation gates pass | Wide base/cliff view has restored colors and lacks pronounced horizontal bands. Not a matched cross-backend camera or full motion acceptance. |

The asset-free `tests/xbox_angle_probe.m` now distinguishes depth, blit and texture
sampling. Both equal-depth controls cover 1,352 pixels with zero failures / GL
error 0. Blitting red texture storage stays `(204,26,13,255)` with sampling swizzle
off/on: final blit is not the channel-swap cause. Sampling that texture should
become `(13,26,204,255)` when R/B swizzle is enabled. Default ANGLE returns the
original red value instead, without a GL error; the probe correctly exits 6.
Upstream `DisplayMtl` explicitly disables `hasTextureSwizzle` on Simulator.
An EGL enabled-feature override passes all controls on this Mac/runtime, including
the expected blue sample. The opt-in engine candidate enables this one feature;
the source pin records it. Final probe links the same archive used by the app and
exits 0 (`angle-probe-final.log`). No global color postprocess or relaxed depth.

GL-call checks retain one startup `0x502` after `glBlitFramebuffer`. Subsequent
logged source/destination reads have prior/read error 0 and complete targets.
Do not call this a completely error-free backend; the startup blit remains to be
localized. Shader compilation/cold loading and unmatched scene timing also
remain limits. The synthetic sampling failure is established; it does not prove
the precise Apple wall-band cause or any physical Windows/Metal defect.

Final app evidence: `docs/artifacts/2026-10-01/G3/ios-app-20261001T130852Z`.
The dedicated HaloPad Xbox iPad Simulator remains on its ordinary launch picker,
with Xbox **PREVIEW**. Actual About identifies `f8937c61` with validation incomplete;
Done returns to both edition controls. Real saves in rediscovered container
`6363645B-5CB1-48F7-A108-B6E3D2810148` match the existing acceptance backup
byte-for-byte. Accepted Apple app is preserved at private
`angle-apple-reference/HaloPad.app` under this pass root for in-place rollback.
Temporary ANGLE dependency checkout/build remains outside GitHub at
`/Users/chrissotraidis/.codex/scratch/halopad-angle-xdZB6j/`; retained for rebuilds,
not an isolated HaloPad branch or a published artifact.

157 tests, 16 skipped, no failures. Apple Simulator/physical and ANGLE Simulator
syntax checks pass; ANGLE physical build rejects before guest preparation.
Tree/index safety and whitespace checks pass. No physical install, pairing
change, IPA, push, publication or recurring job. Goal stays active. Next compare
an isolated copied checkpoint/fixed scene and motion, verify touch/cold reload
and later assets, then decide whether to adopt the renderer. Physical iPad
shading/focus instability remains a separate unresolved acceptance gate.

## 2026-10-01 — same copied checkpoint, renderer comparison and preview touch

Private evidence: `ref/xbox-build/passes/2026-10-01/angle-checkpoint.B09wUK/`.
Separate `apple/save` and `metal/save` copies of the existing build-61-compatible
fixture; shared read-only map symlinks, no init script, bot, forced edition or
injected guest actions. Both load normally via picker → Xbox → Campaign →
New001 → Pillar of Autumn (game in progress) → Normal. Both use the frozen guest
61 and 640x480 guest rendering on the same dedicated iPadOS 26.5 Simulator.
Before any look/movement, `apple/checkpoint.png` has large black floor polygons
at bottom-left and visible bands in the upstairs walls; `metal/checkpoint.png`
draws the floor and lacks those pronounced bands. Corresponding PPM dumps are
preserved. Static room/window/cryo edges align; NPC animations and tutorial
prompt timing differ. This is stronger than unmatched spawns, but not a
frame-synchronized raster comparison, exact cause or full visual acceptance.

Restored the candidate app in place from the preserved `candidate-app/HaloPad.app`.
Normal preview menus reload the copy. Actual touch drags publish and the guest
polls right/left/up/down look values; the tutorial advances to X tube exit.
X exits the tube; the movement tutorial appears. A finite forward drag publishes
and polls move axis 1 at -1 and releases to zero. Screenshots show the modest
view displacement. Pause navigation → Save and Quit shows Saving last checkpoint
and returns to the ordinary main menu. A fresh process (PID 52855, preceding
touch run 38151) reloads via the same normal menu flow into the cryo-bay look
tutorial (`metal/cold-checkpoint.png`), not the ship introduction. It restores
the last checkpoint inside the tube, not the unsaved exit position.

At the untouched cross-backend capture, both copied `save/z/savegame.bin` files
have SHA-256
`7eb537a5bb0599d3e2d05308a9b12688933b34cb28443113c90b5d8c2f2df963`.
Source/Apple still retain it. The Metal copy changes on the fresh load at 22:49:04
to `24c2fb84c94418d0d9c4993489ee608097926e00f32ee1df35147690617e22e5`.
That write alone does not prove creating a later checkpoint. Short sequential drags do not
prove sustained or simultaneous two-thumb play, fire/weapon interaction, audio
or later campaign progression. No normal-depth changes or speculative renderer
patches. The startup blit error remains. Real Simulator saves in rediscovered
`7C9D2071-CFA5-48FE-9723-67DC6DACEA4C` match the acceptance backup byte-for-byte.
Physical iPad Windows/Metal shading/focus report remains unresolved; no physical
install, guest pin promotion, IPA, push or publication. Candidate remains PREVIEW.
Final ordinary picker/About/Done verified, build `f8937c61` and incomplete
validation shown. Both edition choices return. 98 Xbox unit tests pass; whitespace
and tree/index safety pass. Both third-party source checkouts remain clean. This
follow-up adds evidence/documentation only; no engine or UI source changes.

## 2026-10-01 — later campaign, fire/swap/reload and bounded scripted coverage

Private evidence: `ref/xbox-build/passes/2026-10-01/angle-later-campaign.Yu2u87/`.
Same installed ANGLE/native-swizzle PREVIEW, frozen guest `f8937c61`, dedicated
iPadOS 26.5 Simulator. Full separate copy of `campaign61-compat.xCtR9E/save`,
read-only maps, ordinary picker/Campaign/New001/Halo/Normal flow. No init script,
bot, forced edition or injected guest actions in this normal-menu run. First-person
play reaches the escape pod (`halo-pod-before.png`). Two actual RT taps reduce
the rifle display 60→58; Y switches to the pistol (064 reserve). Eighteen finite
forward drags displace the view modestly, not sustained human walking. Actual
pause menu → Save and Quit shows Saving last checkpoint, then the main menu
(`after-save-quit.png`). Fresh process PID 64259 (preceding run 60384) follows
normal menus: Halo explicitly says game in progress, then restores the pod
checkpoint (`cold-checkpoint.png`), rifle 60, not the unsaved shots/pistol.
This is a later-map checkpoint restore, not proof of reaching a new checkpoint
through campaign progression. Copied save hash after initial a30 load/Save and
Quit is `502640dc2d11ae2ae0f101ce29c1b81d15fb7c1f9dc21ae0647964feb308287c`;
fresh load changes it to `384c0c0aab8eb4e0dd4cc4951d6e60f34bd6441128d43a441f8ab7aded657a28`.

The smoke runner now supports a10/a30, validates the actually requested map and
retains the original a10 result field. Explicit `--scripted-campaign` requires
`--case campaign --render-diagnostics`; ordinary menu/campaign runs clear
inherited bot/network-test settings. Result input mode distinguishes automation
from human controls. A separate fresh-save 90-second a30 `bot:7` pass succeeds:
ANGLE identity, lit fraction 0.941, both presentation captures, no signal report.
`scripted/campaign/motion-middle.png` and `screen.png` show changed outdoor valley
views, rifle shots/reload and grenade changes. Cliffs, trees, terrain and ring
lack the pronounced earlier bands in these sampled views. Initial-map script
and diagnostic inventory are not the normal pod checkpoint flow; no human-control,
continuous temporal stability, full mission or audio acceptance. Startup GL
`0x502` remains; later presentation observations report prior/read error 0.

Normal no-render-diagnostic regression (`normal/`) also passes menu 30 seconds,
a10 60 seconds and scripted-match 65 seconds / 1,530 ticks / 12 shots. Reviewed
campaign cinematic and Blood Gulch images retained. Ordinary launch picker
restored, Xbox PREVIEW, both edition choices present. No app rebuild/install or
UI/renderer source change in this pass. 103 Xbox unit tests pass, whitespace and
tree/index safety pass. Real saves in rediscovered container
`7C9D2071-CFA5-48FE-9723-67DC6DACEA4C` still match the acceptance backup byte-for-byte.
Both upstream and ANGLE checkouts remain clean; no physical changes, IPA, push
or publication. Physical iPad shading/focus report remains unresolved.

Live GitHub check found upstream four commits ahead at
[`c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9`](https://github.com/cybersecurity/halo-ce-universal/commit/c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9),
committed 2026-10-01 13:21:22 UTC. Read-only comparison: broader high-res HUD and
sniper assets, CRC-guarded replacement, coverage-alpha meter shaders and flat
widescreen UI fills. Source estimates up to 69 textures / about 225 MB with mips
versus 15 / about 63 MB, though only a subset is used at once. This is an upstream
estimate, not measured HaloPad memory. Texture-description ABI changes again.
Next save-backed candidate update must test clean embedding/translation/build,
memory, scopes/meters/pause backgrounds, normal menu/checkpoint reload and
rendered motion. Keep the tested guest/renderer frozen until candidate gates
pass; newest commit is not installed or promoted by this pass. Goal stays active.

## 2026-10-01/02 — upstream build 64 candidate and normal HUD/checkpoint review

Frozen candidate `c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9` (upstream tag
`build-64`), four commits after the accepted build 61. Private evidence:
`ref/xbox-build/passes/2026-10-01/upstream-c55.foFBTU/`. Saved the outgoing
ANGLE/build-61 app and the candidate app separately for in-place rollback.
Full real-save copy verified before the candidate update. The update routine
also backs up Mac/Simulator saves at `save-backups/20261001-233228-from-f8937c61`.
Read-only review covers embedded assets, CRC-guarded substitutions, coverage-alpha
shader keys and the newly changed texture-description ABI. Keep the original
Xbox texture-cache byte reader disabled; an explicit build-64 regression case
now verifies rejection of that unreviewed ABI. No speculative normal-depth fix.

Candidate rebuild succeeds: 684,271 translated instructions, 192 imports,
98 GLES imports. Mac menu/campaign/match pass (last match tick 2,102, 16 shots).
Simulator normal menu 30 seconds, a10 60 seconds and scripted-match 65 seconds
pass (1,536 ticks, 12 shots). Campaign has no scripted input or rendering
diagnostics. ANGLE/native-swizzle identity is verified. Reviewed campaign and
Blood Gulch captures retain restored colors and lack the pronounced earlier
bands in these sampled views. Source-build configuration/link warnings and the
existing startup GL blit `0x502` are not hidden by the passing gates.

Normal candidate process 82876 uses a full separate copy of the build-61 a30
checkpoint, no init/bot/forced edition. Actual picker → Campaign → New001 → Halo
(game in progress) → Normal restores first-person pod play. Actual RT taps reduce
rifle 60→58; Y swaps to the pistol; Zoom reaches its circular 2x scope. Radar,
ammo, health/shield meters and pause panel draw in the reviewed samples. Save and
Quit shows Saving last checkpoint and returns to the main menu. Fresh process
98213 takes the same normal menu path and restores the pod with rifle 60, not
the unsaved shots/pistol. This narrow fixture result is not general snapshot
compatibility, a newly reached checkpoint or full campaign progression. The
copied save hash starts at `384c0c0aab8eb4e0dd4cc4951d6e60f34bd6441128d43a441f8ab7aded657a28`,
becomes `ed2fc0da61363e30b069f82ee3cb51b4378a11d627d1a9266a638d444f23b8d7`
on first candidate load (unchanged by Save and Quit), then
`569b949ecaa03a3ae76d4e6b59addfcf41de89da0076342cd307f267ec2a0103` on cold load.
The real-save container is not this fixture.

HUD logging maps 34/69 menu bitmaps and 69/69 a30 bitmaps. `top` reports 145M
MEM/RSIZE in menu and 172M in the pod; separate `ps` RSS samples are 377,200 and
419,120 KiB. These different metrics are not interchangeable, not a same-scene
build-61 comparison, and not physical iPad memory acceptance. `vmmap` itself
fails with signal 10; no usable physical-footprint result from it, and no guest
crash inferred from that diagnostic failure. Sniper ladder/fringes, sustained
human controls, temporal fidelity and audio remain unverified. Physical iPad
Windows/Metal shading/focus report remains open; these Xbox Simulator samples
do not fix or diagnose it.

Guarded acceptance rerun backs up saves again at
`save-backups/20261002-000148-from-f8937c61`. Mac menu/a10/match and Simulator
menu/a10/match all pass again (Simulator 1,530 ticks / 12 shots). Accepted
experimental guest pin moves to build 64, dated 2026-10-02; ANGLE source pin and
opt-in PREVIEW remain unchanged. Rebuilt the default Apple library against the
same guest hash `102885c274aa95771be8672a65ba16fa88a52d9f4d3889310072e214aabfdaaa`
without installing it or claiming that backend's known rendering defects fixed.
Separate 90-second a30 scripted rendering diagnostic passes with ANGLE identity,
lit fraction 0.935 and presentation captures. Reviewed final outdoor valley
image draws terrain, trees, ring and updated HUD without the earlier pronounced
bands. This automated motion sample is not sustained human input or temporal
fidelity acceptance. Ordinary accepted-pin app packaging/launch and final save
readback are checked separately below.

Accepted-pin one-app rebuild installs in place on the dedicated Simulator;
ordinary launch shows both editions. About reports `c55e4e2b` and incomplete
validation, Done returns both choices. Private accepted app copy and picker
capture retained; no test environment overrides on the final launch. Real saves
in rediscovered container `A7EB6FDF-9D71-4EEE-8003-EC060348128D` match both the
pre-pass full copy and original `20261001-191048-from-bfbac357` acceptance backup
byte-for-byte. Upstream and ANGLE checkouts remain clean. 103 Xbox unit tests,
whitespace and tree/index safety pass. No physical installs, IPA, publication,
push or cleanup. The broader goal remains active.

## 2026-10-02 — build 64 a50 recording, sniper scope and normal save/reload

Private evidence: `ref/xbox-build/passes/2026-10-02/temporal64.wTBR3I/`.
No app rebuild/install or renderer change. Installed guest hash remains
`102885c274aa95771be8672a65ba16fa88a52d9f4d3889310072e214aabfdaaa`.
Upstream and independently pinned ANGLE checkouts are clean; live remote HEAD
still resolves to accepted build 64 / `c55e4e2b`. GitHub web/API reads failed,
so that freshness check uses `git ls-remote`, not a cached search result.

The smoke runner now accepts a50 alongside a10/a30, with map-specific targeted
case validation and regression coverage. 90-second `--case campaign
--campaign-map a50 --scripted-campaign --render-diagnostics` passes: ANGLE
identity, lit fraction 0.577, requested map and presentation captures, 69/69 HUD
replacements. No signal report; startup GL blit `0x502` remains, later
presentation observations report prior/read error 0. `a50-motion.mov` is valid
152.682-second H.264; it includes picker/loading and post-run Home Screen time,
not 152 seconds of gameplay. No audio stream. This is primarily upstream bot
input; a manual Zoom tap near termination also appears in the recording.
Reviewed sparse frames, a 90–120-second 1 fps contact sheet and a 105–107-second
12 fps sheet show night terrain, NPC action, fire/reload and shield-damage
flashes. The final Home Screen follows bounded runner termination, not an
observed guest crash. No earlier pronounced bands in these sampled views;
no continuous temporal fidelity, reference match or physical graphics claim.

Separate `sniper/` fixture starts from a full copied a30 profile with only a50
snapshot/last-solo files replaced. This partial mixed-map fixture did NOT make
the normal menu recognize a50 continuation. Do not use that as compatibility
evidence. Instead actual picker → Campaign → New001 → Truth and Reconciliation
→ Normal starts normal new-level first-person play with four loaded sniper
rounds, 64 reserve and four grenades, no init/bot/network-test inputs. Actual
Zoom taps reach 2x then 10x; LB toggles night vision, observed fully green after
the transition. Reviewed ladder and scope border have no obvious missing
sections; views are mostly aimed at the ground, not sustained panning or full
fringe/meter acceptance. `sniper-controls.mov` is valid 273.873-second H.264,
video only, largely static; this is not a 274-second human-movement gate.

Actual pause → Save and Quit shows Saving last checkpoint and returns to the
main menu. Fresh process 67566 follows the normal picker/profile path; Truth
and Reconciliation now says game in progress. Choosing Normal restores its
opening checkpoint with four loaded rounds, full shield and unzoomed/night
vision off, not the unsaved scope state. `sniper/cold-reload.png` retained.
Snapshot hash after saving is
`6e9fa68842c0c84d2db9117d257a36afd72e7a4a507733a3dd0d1e0753b53065`;
after cold load it is
`0ed8c68db8b463fbeb7827dd9851ada50eb1ac234202006a858bd4459ce96fa8`.
This proves this normal opening-checkpoint fixture path, not reaching a later
checkpoint, completing the mission or general cross-pin save compatibility.

103 Xbox unit tests pass. Real Simulator save directory in container
`A7EB6FDF-9D71-4EEE-8003-EC060348128D` remains byte-identical to this pass's full
pre-test copy and the original acceptance backup. Final launch 79496 has no
test overrides: both edition choices present, About reports `c55e4e2b` and
incomplete validation, Done restores both choices. Whitespace and tree/index
safety pass. No physical
device change, IPA, push, publication or cleanup. Physical Windows/Metal
shading/focus report is still open. Next useful fidelity gate needs a continuous
moving-view/reference comparison, not another static screenshot.

## 2026-10-02 — bounded output-callback audio coverage

Private evidence: `ref/xbox-build/passes/2026-10-02/audio64.U2hPbE/`. Previous
Simulator recordings contain video only. Added an explicitly opt-in,
Simulator-only diagnostic, not an audio-output redesign: preallocate at setup,
skip ten seconds of callback frames, copy the next four seconds (including
zero-filled starvation), freeze the buffer with a release/acquire handoff.
The game thread writes samples/metadata, never the real-time callback. This
one diagnostic write can affect its frame; no performance acceptance from it.
No microphone/system/other-app audio captured. Normal physical compilation
excludes the capture; iPhoneOS syntax check passes, not a physical build/install.

Six new tests exercise the actual C helper with address/undefined sanitizers,
startup skipping, bounded length, partial starvation and completion freezing,
plus analysis rejection of missing, truncated, invalid, nonfinite and silent
captures. The analyzer reports range excursions and starvation; its narrow
signal gate does not claim deadline timing or general audio quality. Full Xbox
suite: 109 passing tests. Default runner explicitly clears inherited capture
settings; only `--audio-diagnostics` enables it.

Guest stays build 64 / `c55e4e2b`, SHA
`102885c274aa95771be8672a65ba16fa88a52d9f4d3889310072e214aabfdaaa`;
renderer pin and native swizzle remain unchanged. Updated Simulator-only host
library SHA `cdcda2166b84cac16a434bb88f8cbbc01b5ef1bff4fef07c0596384d49f7ad97`.
Source-built one-app candidate copied into this pass and installed in place
after a full real-save backup; no physical app change or Xbox IPA. Existing
profile-guidance and empty-object link warnings remain; startup GL `0x502`
remains. The outgoing accepted app is retained in the previous private pass.

Menu 35-second audio diagnostic passes: 192,000 frames / four seconds at
48 kHz stereo, finite/non-silent, RMS 0.180481, peak 0.845015, zero counted
starvation frames and zero samples outside ±1. Normal-input a50 launch
60-second diagnostic passes: same format/length, RMS 0.106581, peak 0.609333,
zero starvation/range excursions. This is output-callback signal delivery in
those windows, not listening, per-effect correctness, wall-clock deadline
stability, speaker quality, audio/video sync or physical acceptance. a50 first-
person image retained; no scripted input, normal-menu/save claim from this
init-map smoke. Metadata/audio files remain ignored and private.

Normal no-capture regression also passes menu 30 seconds, a10 60 seconds and
scripted-match 65 seconds (1,560 ticks, 13 shots); result explicitly labels
`audio_diagnostics: false`, and no capture files exist in those folders. The
final ordinary launch 10126 shows both editions; About still reports `c55e4e2b`
and incomplete validation, Done returns both choices. Real saves in rediscovered
container `074F7EDF-452F-46FF-98FB-938B21C0F0B4` match both the pre-install full
copy and original acceptance backup byte-for-byte. Whitespace and tree/index
safety pass. Upstream/ANGLE checkouts remain clean.
No physical install, publication, push or cleanup. Goal remains active.

Separate read-only PC inspection finds `MIPMAPLODBIAS` explicitly degraded and
ignored in `port/runtime/halopad_d3d9_draw.c` (sampler mapping); the Metal sampler
only carries a minimum LOD clamp. No matching nonzero-use observation in the
checked physical logs. This is a source fidelity gap to test, NOT the established
cause of Chris's shading/focus report. Xbox ANGLE improvements do not cover that
PC/Metal shader-sampling path. Confirm actual affected edition/scene and match
moving-view/reference evidence before any rendering fix claim.

## 2026-10-02 — PC sampler-state lead and macOS host regression

Private pass `ref/xbox-build/passes/2026-10-02/pc-lod.c5yJyx/`. This is a separate
PC/Metal component check, not an Xbox renderer or physical iPad test. The first
native PC link fails with missing `halopad_host_post_input`, now referenced by
shared touch binding cancellation. AppKit and Halo already deliver native
events on the same main thread. Added the missing Mac entry point using that
existing synchronous delivery, not a no-op stub or another event queue. Header
clarifies this is not a Mac cross-thread API. UIKit's queued path is unchanged.
The native DirectInput test now uses the actual post-input API on Mac as well
as iOS, rather than bypassing it with its local direct-delivery helper.

Fresh-state Blood Gulch component runs after the fix both exit 0 / no trap,
330 presents, map loaded and nonblank capture. It uses original guest startup
systems plus console init, not the ordinary licensed WinMain/menu route, and
has no human input or audio device. Full source outputs are at
`docs/artifacts/2026-10-02/G3/core-arm64-apple-macosx14.0.0-20261001T162406Z/`
and `…-20261001T162729Z/`. Do not use the runner's truncated console tail as the
complete trace. Reviewed frame 300 shows cliff/base/weapon/HUD first-person
play; screenshot is not temporal or reference-fidelity acceptance. The existing
1,165 ms startup frame gap reports zero shader/pipeline/texture work; no
causal relation to the visual complaint from it.

Added sampler stage/texture, mag/min/mip filter, typed/raw LOD bias, minimum LOD
and anisotropy fields only inside existing `HALOPAD_TRACE_DRAWS=first:last`.
Frames 300–301 in the second run contain 248 draw requests and 932 bound-sampler
observations, all bias 0 / `00000000`, minimum LOD 0. Bound includes unused/stale
stages; a draw request trace precedes shader/pipeline acceptance, so counts are
not GPU-submission counts. No nonzero-bias warning in the complete initial run.
The ignored-bias source gap remains real, but is inactive in these sampled
states and is not the established cause of Chris's iPad shading/focus report.
No texture-sampling, depth-test, shader or capability workaround adopted.

Read-only reference review: Direct3D's sampler enum defines bias and its zero
default; Apple's Metal sampling specification supports shader-side bias and
implicit fragment derivatives. This informs a possible implementation if
actual nonzero use is found, not a fix applied here. Official references:
[Microsoft sampler states](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dsamplerstatetype),
[Apple MSL specification](https://developer.apple.com/metal/Metal-Shading-Language-Specification.pdf),
pages 239/248. The web reader rejects the large PDF; bundled pypdf text and a
rendered relevant page were inspected locally. Download/render remain private.

Native input run `…-20261001T162857Z/` exits 0, 264 checks / zero failures through
the restored actual host API. Synthetic keyboard/mouse/gamepad/state tests are
not real controller acceptance. Fresh state folders remain separate from real
Mac/Simulator saves. One-app Simulator compilation passes; no install. All 109
Xbox tests, whitespace and current tree/index safety pass. Existing dedicated
Simulator saves in container `074F7EDF-452F-46FF-98FB-938B21C0F0B4` still match the
previous full real-save copy. Upstream/ANGLE checkouts remain clean. No app install,
physical changes, IPA, public artifact, push or cleanup. Goal remains active.

### PC mip controls and a confirmed texture-lifetime defect (2026-10-02)

Private pass evidence: `ref/xbox-build/passes/2026-10-02/pc-mips.i3W3Ez/`.
Remote HEAD still matches accepted experimental Xbox build 64, `c55e4e2b`;
neither guest nor renderer pins change. This pass checks the separate PC D3D9/
Metal route. Chris's affected physical edition/map remains unconfirmed.

PC Blood Gulch component run on the dedicated iPad Simulator, artifact suffix
`core-arm64-apple-ios17.0-simulator-20261001T164557Z`, exits 0 with 330 presents,
the map loaded, no dialog/trap, and a reviewed nonblank first-person frame.
Like the Mac component test, it starts systems and invokes main via a console
script, not the normal licensed WinMain entry; no human input/audio. Opt-in draw
tracing now includes texture dimensions/type/format/mip count/pool/resource LOD;
the final version also includes the dirty-level mask. Frames 300–301 have 248
draw requests / 932 bound-resource observations, all sampler bias, minimum LOD
and resource LOD zero. Mip counts span 1–11. Bound observations include stale/
unused stages and are not GPU submission counts. No nonzero LOD workaround.

Added native GPU tests with four solid-colour mip levels and actual fragment
derivatives. NONE, POINT at LOD 1.25/1.75, LINEAR at 1/1.5/2, and MAXMIPLEVEL
clamping all pass on both platforms. Compute-test kernels' forced level(0) are
not used. Fractional blending allows one UNORM rounding unit per channel.

Source inspection found that texture `replaceRegion` updates the same shared
Metal object immediately, although earlier draws execute later at Present.
The exact two-draw regression reproduces it on both platforms: draw blue on the
left, change level 2 to yellow, draw right, then Present. **Before the fix the
left is incorrectly yellow**; the right is correctly yellow. Failures retained:
Simulator `…-20261001T165027Z`, Mac `…-20261001T165135Z`.

Fix: only when CPU-backed texture data changes, allocate a replacement and
upload all retained levels/faces, preserving the old object for pending draws.
Clean textures are reused. Render targets retain their separate GPU path.
The normal retained-reference command buffer is confirmed in the installed
Metal SDK header and existing `halopad_metal.m`; no extra synchronization queue
or per-frame global stall. Full-chain reupload can cost more for partial mip
updates; optimize only with evidence, without reintroducing the hazard.
Fixed Simulator `…-20261001T165252Z` and Mac `…-20261001T165514Z` each pass **260
native D3D9 checks**, including old-blue/new-yellow output and unchanged red/
green lower mips. Metal validation enabled; no trap/validation error.

Post-fix Simulator Blood Gulch `…-20261001T165357Z` again exits 0 at 330 presents,
and the frame is reviewed. There are no dirty managed-texture observations in
frames 300–301 (556 managed bindings). Dirty default-pool observations include
GPU render targets, not evidence of CPU uploads. Thus this stationary view does
not establish the texture hazard as the cause of Chris's physical flicker.
Frames differ in animation timing; no pixel-identical or temporal-fidelity claim.
The remaining 1,186 ms startup gap reports zero shader/pipeline/texture work.

One-app ANGLE-preview Simulator compilation passes, without installation.
The rebuilt `HaloPad.app/HaloPad` executable SHA-256 is
`11693a936c8de081727519601ee9afb42ef2899ba6ee4a653a347b8cbf356b59`;
this private generated output is mutable, while native run result files retain
their individual executable identities.
109 Xbox tests, whitespace/tree/index guards pass. Dedicated real save container
`074F7EDF-452F-46FF-98FB-938B21C0F0B4` still matches the audio pass's full save copy.
Upstream remains clean. No physical install, save migration, IPA, push, release
or cleanup. Next reproduce a moving affected scene/reference and correlate real
texture updates before claiming the iPad graphics report fixed. Goal active.

### Consecutive PC pan frames through host input on Simulator (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/pc-motion.FbhdY2/`.
Previous goal pass made progress by correcting the verified texture-ordering
defect; no blocked audit applies. This follow-up narrows the moving-view evidence
gap without substituting a PC component run for the Xbox/physical acceptance gates.

`halo_play_test.c` now posts scripted W/mouse/button events through the actual
host API: iOS queue/pump, Mac same-thread dispatch. It still uses the existing
console-script/systems/main component route, not licensed WinMain, real human
touch or a physical controller. Ordinary Simulator run
`core-arm64-apple-ios17.0-simulator-20261001T170346Z` and Mac
`core-arm64-apple-macosx14.0.0-20261001T170730Z` each pass 16 checks, quit normally
at 300 presents, and write only the original firing frame. Simulator walking
moves +7.336 x units, stop check passes, pan turns 0→−24.9 degrees, firing drops
60→48 rounds. Native input tests remain scripted, not human acceptance.

Exact `HALOPAD_TEST_CAPTURE_MOTION=1` enables bounded test-only readback of
frames 230–259, with frame/file and finite guest position/look metadata in stdout.
The full set is required by a conditional native assertion; file write/close
failure prevents a frame from counting. Existing firing snapshot now checks its
write success too. No changes to production app capture/input flags. Repeated
RGB writes use one packed buffer per image rather than a fwrite per pixel.

Diagnostic Simulator `…-20261001T170441Z` passes all 17 checks. All 30 RGB images
validate at 800x600, with contiguous frame labels and finite poses. Player position
is unchanged during the pan; yaw steps range −0.8315905…−0.8315792 degrees.
Reviewed six-view contact sheet and full-resolution pair 234/235 (largest adjacent
RGB difference): near cliff/ground/weapon remain rendered, without an obvious
frame-wide brightness jump or pronounced previous Xbox-style bands. ROI y65–549
mean RGB runs 20.899→20.424; adjacent mean absolute differences range 2.179–2.673
on the 0–255 channel scale. These simple metrics are observations, not a visual
fidelity gate; local shading/detail errors and differences from Windows remain open.

Opt-in draw trace across that same 30-frame interval has 3,134 requests / 11,966
bound-sampler/resource observations: zero bias, zero resource LOD, no dirty
managed bindings. Stale/unused bound stages and GPU render targets are included;
these are not GPU submission or CPU-upload counts. Thus no link to the recently
fixed texture-update hazard is established in this pan either. No matching
original Windows moving-view reference was obtained in this pass.

`pan-frame-replay.mp4` is H.264, 800x600, exactly 30 frames / 1 second at an
assigned 30 fps; it is a frame replay, **not original elapsed gameplay timing**.
Readback, traces and file I/O perturb pacing. No audio/sync/FPS claims. RGB originals,
contact sheet, reviewed pair and replay stay private/ignored.

109 Xbox tests and whitespace/tree/index safety pass. Dedicated real Simulator
save directory still matches the prior full audio-pass copy; no app install,
physical operation, pin change, IPA, public artifacts, push or cleanup.
Goal remains active. Next prioritize a matched affected moving scene/reference,
and the remaining actual Xbox checkpoint/control/physical gates; do not keep
repeating this now-covered stationary-position cliff pan as hardware proof.

### Newly reached Xbox checkpoint through normal menus (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/checkpoint64.sr7KEQ/`.
Installed build-64 ANGLE/Metal preview remains unchanged; no new app install.
Full APFS-cloned save snapshots isolate the test from the real Simulator saves.
Source fixture is the accepted pin pass's `upstream-c55.foFBTU/checkpoint/save`;
`before-save/` preserves it and `after-save-quit/` preserves the first completed
Save and Quit result before the cold reload.

Actual edition picker → Xbox → Campaign → New001 → Halo (game in progress) →
Normal resumes inside the escape pod: 60 loaded / 120 reserve, no grenades.
Finite touch drags on the left movement region advance out of the pod, across
the crash-site bodies and into the grass; ammunition/grenade pickups are visible.
The last pre-quit HUD shows 60 loaded / 425 reserve and two grenades. These are
short CUA drags with input returning to neutral between gestures, not a sustained
hold or simultaneous two-thumb test. A finite right-region look gesture is also
used. No `init.txt`, bot, network-test sequence, direct map load or injected guest
movement is used. `outside-pod.png` retains an intermediate outside view.

Pause → Save and Quit is selected only after observing each menu step. An initial
batch of three down drags coalesces to one step; subsequent individually observed
steps reach Save and Quit without activating Revert or Restart. Saving finishes
and the Xbox main menu is visibly restored before terminating the process.
The 16,777,216-byte `z/savegame.bin` SHA-256 changes from
`569b949ecaa03a3ae76d4e6b59addfcf41de89da0076342cd307f267ec2a0103` to
`ee33ca3d2b22261beb49cdca25972a88a5b662ee5178a97a0a5335631f6d5500`.
Hash change alone is not the acceptance gate.

Cold launch PID 95137 again uses the actual picker and Campaign/New001/Halo/Normal
menus. First person restores **outside the pod**, with the valley, nearby bodies
and medkit visible: 60 loaded / 120 reserve and **one grenade**, full shields and
health. `cold-resume-outside-pod.png` preserves this actual Simulator frame.
This is a genuinely later checkpoint than the original inside-pod state; it
does not restore the subsequent unsaved ammunition/second grenade pickups.
The live isolated save changes again during continuation to
`416e8fd8b69e11426a429fa3af4ae18862605eb84537d7be772995bc92dd001b`;
the pre-cold snapshot remains preserved. The profile card's older Legendary/The
Maw metadata differs from the recognized Halo/Normal continuation, as in the
source fixture; no profile-metadata migration or general save-compatibility claim.

This closes bounded early new-checkpoint progression, not campaign completion,
continuous movement, simultaneous controls, physical performance or graphics
fidelity. Lighting changes between pod and outdoors are not compared with an
original reference. Chris's physical iPad shading/focus complaint remains open;
affected edition/map is still unconfirmed. Next prioritize that matched moving
scene/reference and sustained controls, not another opening-checkpoint reload.
Goal remains active; no physical operations, pin change, IPA, push or release.

The second Save and Quit also finishes at the Xbox main menu before restoring
the ordinary picker with no data/save/test overrides. `after-cold-save-quit/`
preserves the full final isolated save; its checkpoint hash matches the live
post-reload hash above. Full recursive comparison confirms the dedicated real
Simulator save directory still matches `audio64.U2hPbE/real-save-before`.
109 Xbox Python tests, whitespace and current-tree/index safety guards pass.
Only evidence/status documentation changes in this pass; private copies and
screenshots remain ignored. No cleanup or publication.

### PC Battle Creek moving-view and texture-upload classification (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/pc-battlecreek.HMpnzY/`.
The preceding goal turn made progress with a genuinely later Xbox checkpoint.
This pass expands investigation of the unresolved physical shading/focus report,
not Xbox renderer acceptance. A temporary `halo_play_test.c` map switch to
beavercreek loads the map but creates no player: Simulator
`core-arm64-apple-ios17.0-simulator-20261001T174800Z` fails five checks, including
the absent player and zero motion frames. That source experiment is reverted;
no map-only screenshot is counted as gameplay.

Use the existing `halo_host_test.c` local route instead: Multiplayer, new profile,
Create Game/LAN, Battle Creek, Slayer, Start Game. All scripted keyboard/mouse/
button actions now use `halopad_host_post_input` (Simulator queue/pump; Mac
same-thread dispatch). It remains a component systems/main route, not normal
licensed WinMain, human touch, Bluetooth input or Internet play. Fresh state
keeps profiles separate. No script/guest-state shortcut starts the match.

Exact `HALOPAD_TEST_CAPTURE_MOTION=1` saves 30 consecutive frames during the
existing downward mouse sweep, with actual frame/file/position/look metadata.
A conditional native assertion requires all 30. Image allocation/open/write/
close failures count as failures, including the original seven snapshots;
RGB is written as one packed image instead of per-pixel fwrite. No production
capture/input changes. Readback and logging perturb pacing, so no FPS/sync claim.

Simulator `…-20261001T175127Z`: all 20 checks pass, 3,637 presents, profile New001,
fire/melee/grenade damage/death/respawn/weapon pickup and the existing manual
DirectSound mix checks. Frames 3141–3170 are all valid RGB 800x600 with finite
poses and unchanged position; angular increments 4.343524–4.343598 degrees.
Reviewed contact sheet and full-resolution largest adjacent pair 3146/3147 show
a downward sweep across the base ramp/floor. ROI y65–549 RGB means span
56.009–95.558; adjacent mean absolute differences 3.704–13.900 on a 0–255 scale.
These statistics do not establish correct detail, lighting or temporal fidelity.

Ordinary Mac `core-arm64-apple-macosx14.0.0-20261001T175417Z` passes all 19 checks
at 3,716 presents. Trace 3141:3170 contains 3,245 draw requests / 9,770 bound
observations, all bias/minimum/resource LOD zero, no dirty managed bindings.
Ordinary traced Simulator `…-20261001T175720Z` reaches 6,400 presents but fails
four later assertions: death, respawn, ensuing pickup and death-associated audio.
First frag damage/fire/melee/look/menu/profile pass. No death occurs; do not call
that run passed or blame tracing/host input without a controlled experiment.
Its moving interval has 3,179 requests / 11,326 observations, three dirty managed
bindings (16x16 single-level, 256x256 nine-level, 1024x256 eleven-level), again
zero bias/minimum/resource LOD. A dirty flag alone cannot distinguish a first
upload from changing an existing texture. Bound stages may be stale or unused,
and are not GPU-submission or actual-upload counts.

Add only `native 0/1` to opt-in texture trace, indicating whether a Metal object
exists **before that draw request's upload**. Do not log raw pointers or change
sampling/upload behavior. Mac native D3D9 run `…-20261001T180500Z` passes all
260 checks; trace includes initial dirty/native-0 textures and the exact
managed eight-by-eight four-mip rewrite with dirty-0004/native-1, validating the
distinction. Ordinary untraced Simulator host `…-20261001T180343Z` passes all 19
checks and writes only the original seven snapshots, no motion sequence.

Combined Simulator capture/trace `…-20261001T180726Z` passes all 20 checks and
retains all 30 frames. Its stationary player position is −3.201,17.327,−0.217,
unlike the first capture's 24.507,10.438,−1.356: random Slayer spawns select
different surfaces, not a matched old/new rendering comparison. Reviewed six
views and full pair 3155/3156 show **strong high-frequency grain on outdoor
ground** during the downward sweep. This is a concrete material/detail-sampling
case, not confirmed reproduction of Chris's physical complaint or proof that it
is incorrect against the original game. ROI RGB means 52.774–113.220, adjacent
differences 15.541–44.572; camera steps 4.343525–4.343593 degrees. Trace has
3,173 requests / 9,299 observations (9,239 managed), all bias/minimum/resource
LOD zero and zero dirty managed textures. All observations include the new
native flag. The three dirty textures in the earlier failed run remain
unclassified: do not retroactively label them first uploads. No original
Windows reference or physical capture is available for these views.

The private `combined/` contact sheet, largest pair and analysis JSON retain the
grainy case; all raw frames/logs/audio remain in ignored per-run evidence.
Next isolate its ground-material draw and actual mip/detail sampling and obtain
a matched original view. Do not substitute another generic mip test or green
gameplay run for that comparison. The separate physical report and sustained
two-thumb/controller gates remain open. Read-only iPad app metadata confirms
HaloPad 0.1/build 1 but cannot identify its renderer revision; no device launch,
install, pairing, filesystem mutation or gameplay claim follows from it.

109 Xbox tests, five input guards, whitespace/tree/index safety pass. The
ordinary two-edition picker is restored on the dedicated Simulator without test
overrides. Full real Xbox-save comparison still matches the audio pass backup.
No one-app install, pin change, physical mutation, IPA, public artifacts, push,
release or cleanup. Goal remains active.

### PC Battle Creek ground-material isolation (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/pc-material.lkWYNE/`.
This is new localization evidence for the prior grainy-ground case, not a fix
or a confirmed reproduction of Chris's physical shading/focus report.

Add a native test callback after `halopad_metal_draw`; ordinary apps leave it
NULL. The existing draw trace and callback share request numbering, and skipped
requests do not invoke it. Exact `HALOPAD_TEST_CAPTURE_DRAWS=<frame>` opts the
host component test into one frame, bounded to 1–6400 and at most 256 requests.
It reads the current texture-backed render attachment, not the back buffer,
with bounded BGRA dimensions, packed RGB output and checked file writes.
Failures count; the conditional native assertion requires at least one image.
A separate analyzer checks all expected filenames, sizes, logged request IDs
and the complete 93-image sequence. Readbacks commit and wait on pending work,
changing submission timing. This cannot establish unmodified frame ordering,
texture lifetime or performance. Production upload/filtering is unchanged.

Simulator `core-arm64-apple-ios17.0-simulator-20261001T182503Z`, fresh LAN-menu
Battle Creek/Slayer route, passes all 21 checks and retains all 30 motion frames.
At frame 3155, position is 2.34679008,19.3775997,−0.216702014 and look is
0.00151171628,0.487706095,−0.873006582. It is another outdoor grass/dirt spawn,
not the prior pass's exact camera. The cumulative images span requests 0–92,
including 800x600 world rendering and a 64x64 intermediate. Reviewed contact
sheet and full-resolution requests 21/92 retain grainy ground before and after
HUD/presentation. Request 15 first adds a nearly white world base; request 21
adds colored ground over 454,946 pixels (mean absolute RGB change 157.05574).
This localizes the appearance to the world-material pass in this instrumented
frame; the picker/final copy is not where that appearance first enters.

Request 21: VS `010441b0`, PS `01046420`, target `012426d0`, indexed triangle
list, 23 primitives, viewport 800x600, depth test EQUAL/no depth write. Texture
stages 0–3 are respectively 512x512 DXT2/10 levels, 256x256 DXT1/9 levels,
512x512 DXT1/10 levels, 256x256 DXT1/9 levels. All four have min/mag/mip filters
2/2/2, anisotropy 1, zero bias/minimum/resource LOD, clean managed storage and
an existing native object. The program actually samples all four stages;
these are not merely stale bound slots. This proves neither valid mip contents
nor correct derivative-based level selection while moving.

Existing opt-in shader dumps now log successful guest-ID/file mappings and
check write/close results. Separate Simulator run `…-20261001T183346Z` passes
20 checks and records all 30 motion images without per-draw readback. Its
random spawn is inside the base (21.9461994,13.6329212,−1.35571635); frame 3155
does not draw PS `01046420`. The same startup shader IDs are created and mapped
there, so bytecode identities are from this separate run, not captured from
the exact first run's ground draw. VS `010441b0` maps to a 60-token vs_1_1
program (SHA256 prefix `376340866658`); PS `01046420` maps to a 40-token ps_1_1
program (`026e77794dbc`). Sizes and full hashes verify. The vertex program
scales its coordinates for four stages; the pixel program blends/multiplies
their colors/alpha with co-issued instructions and RGB doubling. All original
shader bytes, disassembly and generated diagnostic files remain private.

Reuse `scripts/shader-diff.py`'s independent interpreter and `tools/shader_run.m`
for these two programs only, 4,096 synthetic cases each, seed 27. The first
attempt fails Metal validation: the existing runner passes a 4,432-byte vertex
constant block through `setBytes`, above its 4,096-byte limit. Preserve that
failed attempt; replace inline constant data with a normal Metal buffer in the
test runner, not the production renderer. Both shaders then have zero bad
cases: maximum error 0.0000009536743 (VS), 0.00073337555 (PS), within the existing
tolerance without relaxation. This is level-zero synthetic sampling with no
projection, not actual raster/mip/game-reference validation. Do not turn this
arithmetic result into a visual acceptance claim or disable projection based
only on programmable vertex-shader use.

Final-source ordinary Mac native D3D9 `…-20261001T184235Z` passes 260 checks.
Ordinary Simulator host `…-20261001T184331Z` passes 19 checks at 3,630 presents
and writes only the seven original images: no draw/motion/shader diagnostics.
Invalid capture-frame values 0, 6401 and nonnumeric input each exit 2 before
guest initialization. 109 Xbox Python tests, five input guards and whitespace/
tree/index safety pass. Normal picker is restored (PID 34362) and its screenshot
reviewed. Full real Xbox-save comparison matches `audio64.U2hPbE/real-save-before`.
No one-app build/install, upstream pin change, physical operation, IPA,
publication, push or cleanup. Goal remains active.

Next discriminating pass: inspect the isolated ground material's actual UV/
detail scaling and mip texels/selection across moving views, with a matched
original-PC reference if available. A green arithmetic or gameplay test cannot
replace that comparison. Avoid speculative sharpening, blur, anisotropy or LOD
workarounds; the physical report's edition/map and original view remain unknown.

### PC live mip capture and calibrated readback correction (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/pc-material-mips.7BcfA9/`.
The preceding goal turn made progress by isolating the ground material. This
pass inspects its real inputs; it is not original-PC or physical-iPad acceptance.

A private wrapper reuses the existing host-menu/gameplay harness and its native
draw hook. It matches the actual pixel-program tokens, not a hardcoded guest ID,
and captures only the first match during frames 3141–3170. Bounded managed 2D
textures retain raw CPU blocks and uploaded native texels for all four stages;
the live vertex program, declaration, bounded streams and constants stay private.
The initial wrapper fails compilation on two untyped COM-state dereferences;
the corrected source assigns a `res *` first. No game execution follows that
failed build, and a private note preserves the failure.

Simulator `core-arm64-apple-ios17.0-simulator-20261001T185619Z` captures frame
3141/request 70, VS `010441b0` and PS `01046420`, with BC support false. Both
program hashes match the preceding arithmetic check exactly. Four 2D stages
have 10/9/10/9 levels, trilinear filters, zero bias/minimum/resource LOD, clean
storage and texture-transform flags zero. Actual c10 is (100,100,60,60), c11
(1,0,12,1), c12 (0,1,12,−1): the program requests 100x/60x detail and 12x fourth
stage scaling. Vertex input v4 is FLOAT2 TEXCOORD0 at offset 48 in a 56-byte
stream. These are bound inputs, not an exact indexed primitive replay or measured
fragment derivatives. Correct the prior stage-zero label to **DXT2**, FourCC
`0x32545844`, not DXT3; [Microsoft's format enumeration](https://learn.microsoft.com/en-us/windows/win32/direct3d9/d3dformat)
defines the FourCC. Its stored BC2 explicit-alpha blocks are compared without
unpremultiplying, not treated as a blend-semantics reference.

The initial independent [Pillow BC decoder](https://pillow.readthedocs.io/en/stable/handbook/image-file-formats.html#dds)
comparison agrees at level zero but reports large lower-level differences.
Reject those lower-level native images: all four level-one images are exact
level-zero top-left crops. A separate asset-free BGRA four-color/four-mip native
probe confirms that this Simulator's **short** `getBytes` selector returns red
level-zero data at all levels while explicit shader sampling returns the intended
red/green/blue/white. The full selector returns all four correct levels. On Mac,
short/full reads and shader samples all agree. The first probe's compute dispatch
aborts on unsupported nonuniform threadgroups; uniform dispatch then calibrates
the samples. Simulator probe exit 1 is the intentional old-selector negative
control: three short-read failures, zero full-read or shader-sample failures.
This does not prove an upload defect or that mip selection in the game is wrong.

Add eight native D3D9 assertions for this existing readback helper: exact texels
and untouched row padding at all four levels of the actual uploaded managed
texture. Before correction, Simulator `…-20261001T190609Z` fails exactly the
three nonzero-mip texel checks; all padding checks pass. Change only the helper
to the full selector with explicit bytes-per-image, 3D region depth one and
slice zero. No upload, filtering, shader or game-content changes. Afterwards
Simulator `…-20261001T190737Z` and Mac `…-20261001T190738Z` pass all 268 checks
with Metal validation. Preserve the rejected first mip analysis and readbacks.

Corrected actual-material Simulator `…-20261001T190738Z` captures frame 3141/
request 25. `corrected-readback/` contains all 38 CPU/native mip pairs, 873,812
texels: independent decoding differs by at most one RGB unit, alpha exact,
zero pixels exceeding two RGB units. Both shader hashes and the same detail
constants verify in this capture, with projection disabled. The reviewed mip
contact sheet now shows genuinely downsampled levels, not crops. This checks
stored texels, not actual moving raster derivatives or an original driver.

The first full gameplay run reaches 6,400 presents but fails five later checks:
grenade damage, death, respawn, ensuing weapon pickup and death-associated audio.
Health/shields never fall below 1/1. Motion and material capture succeed, but
the run is not passed. The corrected full run passes all 21 checks, including
the original gameplay/audio checks, 30 motion frames and requested material.
Random spawns differ; do not blame or credit the helper for gameplay outcomes.
Corrected frame 3155 has exactly the same recorded position/look as the earlier
`…-20261001T180726Z` dirt view. Ground ROI x50/y150/w400/h250 is identical at all
100,000 pixels (mean/max RGB difference zero). Thus visible grain persists and
this correction must not be called a rendering/focus fix. Timings/HUD may differ;
the comparison is not synchronized original-PC or physical fidelity.

109 Xbox Python tests, five input guards, whitespace and tree/index safety pass.
Ordinary two-edition picker restored, no overrides (PID 47787); real Xbox saves
still match `audio64.U2hPbE/real-save-before`. No one-app install, upstream change,
physical operation, IPA, publication, push or cleanup. Goal remains active.
Next investigate the calibrated material's actual fragment LOD selection against
a matched original view and verify the latest-source integrated Simulator app.
Do not infer corruption from uncalibrated readbacks or alter detail scales as a
visual workaround. Physical shading/focus, sustained controls and device renderer
acceptance remain open.

### Current-source one-app integration (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/integrated-current.Uyb7ck/`.
Rebuild from source `9888782`, normal `halopad_core_run` PC entry, no development
scene. Xbox remains frozen build 64; ANGLE/Metal is the Simulator-only preview.
Before the in-place install, terminate HaloPad and preserve the outgoing installed
app, generated app, Documents (2.4 GB) and Library (15 MB) as private APFS copies.
Documents/Library compare exactly before installation; outgoing strict signing
passes. The new candidate and installed HaloPad binary both have SHA-256
`db16569da62c6e5c88c6adfad6842645efa884550b6abad25af7c54d25cadd73`.
Strict candidate signing passes. Simulator installation changes container UUIDs;
rediscover them rather than trusting the previous paths. Documents still compare
exactly immediately after installation; Library initially differs only in OS
SplashBoard snapshot names.

Dedicated iPad Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, iPadOS 26.5:
actual picker cards, About these builds and Done work. Windows correctly opens
import because this Simulator previously had no PC installation. Xbox opens its
main menu. Controller detection reports connected and normally hides touch;
a campaign-text tap does nothing. Relaunch the isolated Xbox session with
`XG_TOUCH_SHOW=1`, retaining separate `XG_DATA`/`XG_SAVE` and cleared bot/network
test flags. This is diagnostic overlay visibility, not physical controller proof.
Using actual virtual A: Campaign → New001 → Halo → Normal restores the existing
outdoor checkpoint copied from `checkpoint64.sr7KEQ/after-cold-save-quit`.
RT decreases rifle ammunition 60→58, Y changes to pistol, Zoom renders 2x scope
and toggles back. Start and three separate down-stick drags select Save and Quit;
observe Saving finish and return to the Xbox main menu. No new progression or
sustained/multi-touch/audio-quality acceptance is claimed. Scope capture stays
private as `xbox-scope.png`.

Ordinary cold relaunch, no overrides, restores both edition cards. The unchanged
older handoff package fails current-build preparation verification with
`Unexpected number of archive entries`; do not weaken the verifier or modify
the handoff kit. Prepare a matching package from existing private
`ref/inputs/custom-original`: 89 entries, core identity
`e0dc0256d47486c442bad3d3c789b267c753c9c021810c64c721f21949832bce`, ZIP SHA-256
`7563091a776d3496b5a7608aa7f5d4d82927359b2ced7133724e44dd8d0e5a63`.
Copy it into Simulator Documents and use the actual package picker:
On My iPad → HaloPad → current-ce.halopad.zip. The initial button tap during
startup verification is disabled; it opens once that check finishes. Import
publishes exactly 78 stock files; independent size/hash readback matches all
78 signed records with no extra files. Normal PC entry then presents the original
Custom Edition EULA. **Leave I Accept untouched and request user confirmation.**
Capture `pc-license-unaccepted.png`; neither PC main-menu/gameplay acceptance nor
the later product-ID gate has been exercised in this installed candidate.

The entire real Documents/Halo Xbox tree remains identical to the pre-install
backup; only the isolated campaign saves are used. PC import adds its folder and
retained ZIP. Library adds normal PC startup state and changes Metal/SplashBoard
caches. No preference keys are lost or added; only `HaloPadLastEngine` changes,
consistent with selecting PC after Xbox. Do not claim the whole Library is now
unchanged. 109 Xbox tests and five input guards pass. No runtime source change,
pin promotion, physical-device operation, IPA, publication, push or cleanup.
Goal remains active: confirmation is needed for this normal PC startup path;
matched moving-scene fragment LOD/shading and physical-iPad fidelity remain open.

### PC exact-material raster mip sampling (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/pc-raster-lod.RfwgiV/`.
The previous turn rebuilt the integrated app and verified both launch routes
through Xbox continuation and PC import/EULA; it was progress, not a no-progress
blocker audit. Leave the installed PC EULA unaccepted pending confirmation and
do not operate the physical iPad shared with BlueWake.

Add a nullable native test hook after the actual D3D9 draw encodes, exposing its
borrowed pipeline/draw descriptors only during the callback. No new app setting,
automatic instrumentation, shader/filter change or game-content alteration.
Two native regressions inspect seven real *UP mip draws: callback count and
exact bound descriptors. Simulator `…-20261001T200509Z` and Mac
`…-20261001T201013Z` pass all 270 native assertions with Metal validation.

Direct fragment mip queries require appropriate GPU families per
[Apple's LOD-query documentation](https://developer.apple.com/documentation/metal/predicting-which-mips-the-gpu-samples-with-level-of-detail-queries).
An independent native capability probe reports Apple7=false, Mac2=false and
32-bit-float filtering=false on this Simulator. **Do not use direct query modes.**
Instead create matching-size/mip-count RGBA16Float textures whose constant red
value is the mip index. Original trilinear samplers interpolate these markers;
RGBA32Float output/readback retains the sampled numeric level. An asset-free
fragment probe calibrates six known footprints, levels 0/1/1.5/2/3/4: all 384
fragments pass on each of Simulator and Mac, integer levels exact, fractional
marker error at most 0.00390625. Analytic derivative calculation is exact in
this simple calibration. This is not a Windows-driver reference.

The private wrapper retains the existing menu/LAN Slayer/audio/motion harness.
It reuses the **exact encoded indexed geometry, buffers, offsets, base vertex,
vertex shader, constants, four sampling coordinate expressions and samplers**.
Independent offscreen probes disable depth/occlusion and blend; they do not
establish original visibility, draw order, frame timings or physical fidelity.
Original color is read before/after and must remain byte-identical.

First compilation fails on hp_bound w/h names; correct to width/height, no game
run from that failed link. First live run `…-20261001T200158Z` exceeds its 180 s
deadline and fails five later combat/death-associated audio checks plus two
zero-coverage probes. Its first frame-3150 matching shader draw is fully clipped.
Preserve that failed source/executable/output; do not declare the whole run passed.
Revised capture matches **all four raw level-zero texture payloads** against the
accepted previous material, not shader tokens alone, and tries another exact-
material draw in the same frame if coverage is zero. Keep all gameplay checks;
extend only the execution deadline to 360 s. The analysis script initially lacks
Pillow in the repo venv; use the bundled workspace Python, no package installation.

Corrected `…-20261001T201011Z` exits 0, passes all 25 assertions, includes all 30
motion frames, and captures frames 3150/3155/3160 at requests 49/35/31. A clipped
request 41 is explicitly skipped at frame 3150. All four texture identities stay
the same across the accepted captures; zero projection, trilinear, zero LOD bias/
minimum/resource LOD. Coverage is 467,322 / 480,000 / 480,000 pixels, with finite
four-stage results and no original-color changes, including the skipped attempt.
This is 1,427,322 independent raster pixels, 5,709,288 stage samples.

Marker stage 1/2 maxima at frame 3150 are 4.01171875 / 4.26953125, falling to
0.80078125 / 1.05859375 at 3155 and zero at 3160 as the view points down. Stage
zero stays at mip zero; stage three peaks at 0.94921875 then falls to zero.
Compare against log2 of the maximum x/y texture-coordinate derivative length,
clamped to each texture's valid mip range. Largest absolute residual is
0.05881428 levels; 288 stage-two samples at frame 3150 exceed 0.05, all around
the level-zero transition (y522–535, measured 0.05078125–0.08984375).
Retain these residuals rather than declaring exact arithmetic equivalence.
The reviewed fixed-scale contact sheet shows distance-dependent interpolation;
the original target still shows grainy ground. This argues against a **gross
mip-selection defect in this sampled PC material**, not against the user's
physical shading/focus report and not proof of original-driver correctness.

109 Xbox tests, five input guards, whitespace and current-tree/index safety pass.
Installed binary remains `db16569d…cadd73`; real Documents/Halo Xbox is identical
to the integration backup. No test processes remain. No install, pin promotion,
physical operation, IPA, publication, push or cleanup. Goal remains active.
Next obtain a matched affected original-driver view and compare material/lighting,
UV interpolation and sampler behavior; do not reduce detail scales or force mip
levels as a visual workaround. PC EULA confirmation and physical acceptance remain
separate gates; the diagnostic hook is inactive in normal apps.

### Native disc-import validation and stale-library gate (2026-10-02)

Private pass: `ref/xbox-build/passes/2026-10-02/disc-import.1pB6xv/`.
Work the independent fresh-install flow while the PC EULA remains unaccepted
and the physical iPad is shared with BlueWake. No physical operation or visual
fix. The previous raster pass was progress, not an unchanged blocker audit.

The old native extractor trusts directory names, silently skips malformed tree
nodes and writes into a reused `maps.partial` folder. Add bounded validation
before any destination write: 1 MiB directories, 256 entries, cycle/repeated-node
and case-alias rejection, safe path components, image extents, maps/XBE entry
types and XBEH magic. Maps must have canonical ASCII `.map` filenames, version-5
daeh/toof headers and a common terminated build string; bound each to 512 MiB and
total to 3 GiB. Keep the compressed header's logical length separate from its
physical disc extent: ui.map's physical 14,145,536 bytes differ from header
33,582,080. Do not reject Chris's valid compressed maps for that difference.
Clear both result strings on entry; the UI previously formed NSStrings from
an uninitialized success-path error buffer. Reset retry progress to zero.

Create unique `maps.import-XXXXXX`, write only new fd-relative files with
O_EXCL/O_NOFOLLOW, check close errors, and use Darwin's exclusive rename into
`maps`. Refuse an existing destination maps file/directory/link and a final-root
symlink. A competing maps directory appearing during copy is preserved; retain
the unfinished stage. No old partial folder or save is reused/deleted. This
does not authenticate game payloads, establish a complete stock inventory or
make the upstream game's map loader safe for arbitrary malicious payloads.
The Python reference extractor is unchanged; restrict it to trusted own input.

Seventeen Darwin tests exercise inert fixtures, including valid/canonical names,
unsafe names, aliases/cycles/bounds, invalid later map headers/mixed builds,
existing files/directories/links, retained old stages/saves, publication race and
source truncation followed by retry. The initial fixture accidentally adds a
NUL to the 20-byte descriptor magic; successful cases expose the error. Correct
the fixture before accepting negative-case results. Final 17 tests pass.
Current strict-warning ASan/UBSan runner passes 104 retained cases (four named
fixtures plus 100 deterministic bounded metadata/header mutations), with no
sanitizer findings. This is not exhaustive fuzzing. A real-disc ASan/UBSan copy
also exits 0; all 24 size/hash records match `ref/xbox-build/data/maps`.

Preserve outgoing installed/generated apps, Xbox library and Simulator
Documents/Library using APFS clones. First integrated candidate executable
`8f3c1171…510a5fa` rebuilds the main app but links the older Xbox archive. Actual
Files selection of `Invalid Xbox fixture.iso` is wrongly accepted: ui.map is
published and `../a10.map` escapes the old partial folder into the **isolated**
test root. Do not count that UI attempt as a pass. It reaches a black scene,
not gameplay. Real game files remain identical. Retain this failed candidate,
logs and isolated roots; no cleanup.

Add a shared local-source identity to Xbox library creation and main-app
packaging: hashes of native Xbox source/header/assembly files and relevant
prepare/compiler/generator/build scripts. Missing or changed identities now fail
closed. Two regression tests cover missing metadata and an older disc extractor;
an actual main-app build rejects the outgoing archive. Rebuild through the
normal frozen-pin library script, not a manifest edit. ANGLE checkout is clean
and unchanged; guest remains `102885c2…bfdaaa`, new library
`f0805929f4f4e34f1cd10afcafc6f8c663a8abe8a13b95d0ef52bf28be2fc83a`.
The GitHub commit-page request fails; read-only official API refresh returns
[c55e4e2b](https://github.com/cybersecurity/halo-ce-universal/commit/c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9),
still the newest commit. No upstream pin promotion.

Corrected integrated executable SHA-256:
`5ea1369de8579e0314a02fbd89f5336ce278dc6d5e6a4b076f39c2f555768df9`.
Normal PC entry, ANGLE/Metal PREVIEW, build 64. In-place install into dedicated
iPad Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, iPadOS 26.5. A first launch
using relative stdout/stderr filenames is denied; use absolute paths, then launch
works. All Xbox test operations use fresh `ui-checked`/`ui-checked-save` overrides
and cleared bot/online/UPnP/clipboard-join flags; real Xbox saves never open.

Actual UI: Xbox card → Choose Disc Image → Files/HaloPad → malformed fixture.
Corrected extractor rejects it before even creating the isolated destination;
error appears, Choose and Editions re-enable, no touch pad is created. Editions
returns both cards. Re-enter import and select the real `Halo USA Xbox.iso`.
It reaches the Xbox main menu; no development import flag or synthetic input.
All 24 imported map sizes/hashes independently match the reference, no missing
or additional maps. The copied/source disc hashes both equal
`bbed30485a34fb971687648f31a8b50467c92d7ef296e3f97bad51372db1b012`;
disc is 3,728,867,328 bytes, partition zero, maps build 01.10.12.2276, XBE hash
`ed3a8e962351ad6c4b3b620768fb6a0bda658963390252439ea036d5ede3a3ac`.

Actual virtual A selects Campaign. Fresh New001 name/Done, profile saving and
Normal selection load a10's opening cinematic from the newly imported maps.
The no-existing-profile path briefly logs `event handler function failed`;
default profile creation nevertheless finishes. Cold launch changes the card to
Play Xbox, skips import and reaches the main menu. Campaign shows the saved
New001 profile, but its summary says **The Maw / Legendary**, despite this fresh
a10 run. Retain `cold-new001-profile.png`; cause/progression semantics are not
established, and no later checkpoint/complete gameplay acceptance is claimed.

Ordinary launch with no Xbox overrides restores both real-installation cards.
Windows verifies its existing files and reopens the original unaccepted EULA;
do not accept or fake product identity. Final ordinary launch is left at the
edition picker. Independent readback preserves all 114 real Xbox files and all
PC installation/package bytes. Documents only adds the real-disc clone and
inert fixture, with the normal HaloPad log changed. Library changes Metal and
SplashBoard caches and scene-session metadata. Only HaloPadLastEngine changes
during the Xbox tests; selecting Windows restores its original value, so the
final preference dictionary exactly matches the backup, with no keys added/lost.
PC registry remains unchanged. Retain both full backups.

128 Xbox tests and five input guards pass; package suite runs 13 checks and skips
16 native checks (not 29 executed passes). Whitespace and current tree/index
safety pass. No physical operation, pin update, Xbox IPA, push, publication or
cleanup. Goal remains active. Next isolate the fresh-profile summary anomaly,
keep normal PC license confirmation separate, and resume matched moving-scene
rendering/reference work. The user's physical shading/focus complaint is still
unresolved; this pass only establishes safer import/build behavior.

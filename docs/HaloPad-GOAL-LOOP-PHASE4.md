# HaloPad goal loop — shared controls and Simulator compatibility

Written 2026-10-02 from Chris's latest direction. This supersedes phase 3 as
the operating loop. Work only on this Mac and the dedicated HaloPad Simulator;
the physical iPad is unavailable today. Continue implementation and investigation
without waiting for hardware or direct-finger feedback. Preserve existing inputs,
saves and unrelated work. Personal Xbox builds remain local `.app` outputs.

## Intended result

One HaloPad app opens with Windows Custom Edition on the left and Xbox Combat
Evolved on the right. HaloPad owns a consistent touch interface, layout settings
and lifecycle behavior across both engines. Each engine receives those actions
through a small adapter. The Xbox engine is an independently pinned dependency;
upstream updates must not replace HaloPad's controls or UI.

## Ordered goals

1. **Shared input interface.** Reuse the existing PC overlay for Xbox: matching
   action labels/icons, move/look sticks, drag-to-aim while firing, touch settings,
   layout editing and handedness. Keep engine-specific menus and input mapping
   behind the adapter. Verify both routing paths, alias-button ownership, short
   taps, cancellation and independent move/look/fire. Exercise actual menus and
   gameplay on the dedicated Simulator before claiming integration.
2. **Texture reproduction.** Keep the guest and renderer pinned while capturing
   repeatable a10 and outdoor a30 views with isolated saves. Record which surfaces
   fail, render resolution, texture formats, filtering, depth and shader state.
   Review source from other iOS ports for specific hypotheses. Implement only a
   change with a reproducible before/after result; check colors, UI and depth too.
3. **Upstream maintenance.** Review the latest release at the start of each update
   pass and freeze its commit for that pass. Review guest ABI/import, renderer,
   generated assets and save changes. Use the existing candidate build and backup
   workflow; accept only after menu/campaign/match and shared-control regressions.
   Keep the accepted guest lock separate from the ANGLE lock and HaloPad source.
   Preserve the prior candidate, saves and rejection evidence for rollback.
4. **Product integration.** Keep the launch choice clear and consistent, Windows
   left/Xbox right. Test About/Done, import/back, settings persistence, relaunch,
   menu navigation and switching by restarting the app. Make engine-specific
   features explicit without exposing unnecessary implementation details.
5. **Simulator acceptance.** Complete a normal-menu campaign control/save/reload
   sequence and a bounded local match. Check touch target sizes and layout on
   tablet and phone dimensions, interrupted touches, same-build save reload,
   sustained moving-image rendering and available audio diagnostics. Hardware
   feel/performance remains a later acceptance task and does not block this loop.

## Pass discipline

For each pass, name the problem, choose the smallest useful experiment, implement,
verify, and update STATUS/JOURNAL with evidence and the next unmet goal. Keep
control work and guest upgrades in separate checkpoints so failures are traceable.
Use the existing checkout and explicitly target Simulator
`DF51182F-1878-4A54-9AED-CC4AED86BEAB`. Preserve app/data before replacement;
use copied test saves. Do not repeat completed checks without a changed input or
new concern. Research ends in a concrete experiment or a documented rejection.

Initial findings: the two current touch overlays are both HaloPad-owned; Xbox's
floating-stick/velocity-look implementation is separate from the richer PC
overlay. Upstream build 66 (`f2ba71d9`) is current at this pass's initial check;
accepted build 64 remains the control-work baseline. Build 65/66 introduce
high-resolution fonts/titles and multiplayer player names, requiring renderer/
asset review before promotion. Candidate iOS references are NicholasDominici's
`halo-ce-ios` and zimm3rmann's `halo-ce-ios-macos`; implementation claims require
source inspection and local reproduction.

## First-pass checkpoint

- Shared overlay is implemented in `94dfd39`, with 128 native assertions and
  143 Xbox tests. Normal-menu copied a30 checkpoint verifies movement, look/fire,
  swap, zoom, pause, settings, quit and cold reload. PC defaults remain intact;
  actual PC gameplay and sustained human multi-touch are not newly accepted.
- Unmerged upstream anisotropy PR 35 was tested separately at 1x and 16x on the
  same checkpoint. More ground detail is visible at 16x, but considerable blur
  remains at 640x480. Preserve the experiment, not as an accepted release pin.
- Menu-clarity pass completed: A/B/X/Y badges preserve shared gameplay controls,
  with a guide explicitly disclosing the Default profile requirement. 134 native
  assertions and 145 Xbox tests pass; normal Simulator menus, copied checkpoint,
  fire/drag, swap, scope and save/quit verified. Non-default profile adaptation
  and sustained human multi-touch remain open. Next graphics pass: test real render-target
  scaling, including depth, scopes and HUD, then isolate remaining material and
  temporal defects. Do not label these unresolved issues fixed by the overlay.

Rejected shortcut for menu state: desktop `halo_ui_pointer_update` drives SDL
relative-mouse capture, but the Android guest compiles a no-op implementation.
Do not infer guest menu visibility from our existing relative-mouse import.
A default-binding A/B label hint is simpler than pretending that callback is an
authoritative menu signal. Any actual menu-state bridge needs a separately
validated, versioned guest interface rather than hard-coded memory offsets.

## Resolution experiment checkpoint

Private build-66 adaptation `97b45239` enables 2x existing render targets while
retaining the logical 480-line layout. Actual source/viewport readback is 1280x960
versus 640x480; matching a30 and scoped views show sharper edges/HUD but continued
ground softness. Accepted pin/app restored after the experiment. Next: make the
small guest adaptation reproducible with a separate identity and strict source
preconditions, extend depth diagnostics beyond 640x480, and test effects/match
before enabling a default. Android atomic visibility counts omit scale correction;
the tested ES3.0 fallback cannot validate that path. Do not merge resolution and
filtering hypotheses into one unexplained graphics fix.

Depth follow-up: the opt-in observer now queries the real attachment extent and
captures calibrated full-sized depth. Selected 1280x960 and 640x480 terrain pairs
pass same-frame position/projection, GL/restoration and responsive-base checks;
EQUAL writes no depth. 151 Xbox tests pass. Native color/synthetic raster replay
is still restricted to 640x480. Next gate is reproducible adaptation identity
and visibility-effects/broader scene validation, not repeating the completed
selected-pair depth check or claiming full graphics acceptance from it.

Adaptation follow-up: `render-scale-v1` now reproduces the private experiment
from accepted build 66 without a maintained upstream branch. Source checks,
restoration, separate metadata and opt-in packaging are tested; ordinary rebuild
reproduces the accepted guest hash. Actual copied a30/controls verify the packaged
candidate. 167 Xbox tests and 26 Simulator save-helper assertions pass. Save
backup identity now includes guest hash, covering same-pin adaptation/rollback.
Default app restored. Next pass should inspect visibility effects, transparent
materials and moving scene artifacts, not repeat adaptation/depth plumbing.

Filtering follow-up: `render-quality-v1` retains accepted build 66 and adds
opt-in bounded world filtering. Same-view 1x/4x/16x comparisons at 1280x960 show
more ground detail at 4x/16x but continued slope softness; 170 tests pass and
shared gameplay actions/scope/pause respond. Defaults remain unchanged. Next:
turn validated resolution/filtering choices into one small pre-launch Xbox
quality setting while retaining the original mode, then broaden visual checks.
Do not claim the ES3.0 boolean flare-occlusion fallback or atomic-count scaling
issue solved by texture filtering, or repeat the same checkpoint comparisons.

Quality UI follow-up: adapted combined builds now expose Original/Sharper
(Preview), Original default. Actual Simulator UI verifies both persisted choices,
Cancel, About/Done and copied a30 startup at the intended dimensions/filtering,
without quality environment overrides. Windows routing stops at its license
screen, left untouched. 170 Xbox tests and 35 native launch/save/quality checks
pass. Preview remains installed with Original selected; all original saves and
PC registry preserved. The setting is complete; next isolate an effects/visibility
or different-scene defect. Do not rerun this same picker/AF pass or treat a sharper
image as resolution of the user's shading/popping report.

Visibility observer follow-up: 172 tests and actual bounded query logs confirm
the ES3.0 boolean path is active. Backend source also uses boolean mode; scaling
the result cannot recover coverage. The camera sweep did not isolate a partially
covered flare, so there is no user-defect reproduction or graphics fix yet.
Next isolate one flare draw/area and occluder before implementing counted
visibility; do not conflate this with general texture blur or rerun an aggregate
query trace as if it established flare attribution. Observer is off by default.

Blood Gulch follow-up: the stationary match passes 180 seconds/tick 5010 and
manual background free-look locates a visible sun flare without firing. Small
reversible drags reproduce edge-visible / outside-view / edge-visible states.
This supplies a concrete scene/gesture route, not proof of bad brightness or
world-geometry occlusion. Projected query area is not viewport-clamped in source;
the visible corona is not necessarily that area. Next correlate this one test's
rectangle/result with matched edge views or a reference before changing counting.
No new runtime fix or pin import; the two reviewed iOS fork heads are unchanged.

Rectangle correlation follow-up: new opt-in CPU-only observer and actual sun
edge views now establish the gap: at most 928/3364 pixels (27.6%) can be visible,
yet the returned boolean makes the guest target 255; fully outside returns zero.
174 tests/build/five-minute smoke pass. This supersedes the need to isolate that
rectangle again. Next test an explicitly counted backend/guest capability with
cross-render-pass accumulation and scale normalization. Keep it separately
identified/opt-in and preserve GLES boolean semantics. CPU viewport clipping is
not a general occlusion fix. No claim of fixing broader texture/shading defects.

Counted backend follow-up: isolated generated ANGLE candidate enables Counting
and sums across passes; private bridge returns counts while standard GLES stays
boolean. 56 actual GPU cases pass across both verified accumulation paths,
including 928 pixels for the measured edge rectangle, depth hiding, query reuse
and larger targets. 177 Python tests pass. No HaloPad install or guest change.
Next integrate the explicit private capability, separately fingerprint backend
and guest, normalize scaled counts, then validate the actual sun view and rollback.
Do not repeat these same synthetic tests in lieu of game integration, and do not
mark the visual defect fixed from this backend-only evidence.

Counted guest integration follow-up: explicit `render-visibility-v1` now pairs
the private query token, normalized guest result and counted ANGLE identity.
181 Xbox tests and 36 native Simulator launch/save/quality checks pass. Actual
Blood Gulch sun-edge views return full/partial/zero counts and reflections fade
and return. At Sharper, the narrow slice returns 1624 raw samples versus 406 at
Original; exact compiled guest normalization passes. This is the specific
coverage-loss fix, not resolution of broader texture/shading defects. Do not
repeat aggregate tracing or backend-only fixtures as the next experiment.
Next test world-geometry occlusion in game and a different effects/material
scene, with a matched reference where available. Keep this Simulator candidate
opt-in; hardware/MSAA/performance and sustained multi-finger touch remain open.

Geometry/night-scene follow-up: same installed candidate, no source change.
Stationary Blood Gulch passes 360 seconds/tick 10410, zero shots. Fully
in-viewport light rectangles return 20/25 visible, 0/36 behind the weapon,
25/25 revealed and 30/30 near its silhouette. This is foreground-depth evidence,
not a BSP-wall transition; the small light-core query is not the entire glow.
a50 passes 120/300-second smoke runs. Manual run then verifies shared Zoom
2x/10x/unscoped and Light night vision on/off. Debug saves lack a named player
profile, so normal-menu save/resume is not accepted. Short CUA stick drags do
not sustain movement; do not alter correct release behavior for automation.
Returned to Original-quality picker; original saves/PC registry/preferences
preserved. Next choose a distinct material or wall-occlusion scene with matched
reference where available, not another sun-edge/sniper-button repeat. Full goal
remains active; no hardware access or promotion to graphics/touch acceptance.

Beach follow-up: b30 is now a tested harness choice; actual 180-second run passes
and manual free-look exposes ocean, shoreline bands and translucent exhaust.
No definite new defect isolated. Next inspect generated water ripple mip levels
and composite sampler state in that scene, or obtain matched reference evidence,
before changing rendering. 181 Xbox tests pass; state preserved; runtime/pins
unchanged. Do not count visible water as complete material-fidelity acceptance.

Mip follow-up: bounded actual GL readbacks find all four levels of the likely
water ripple composite complete and changing at frames 120/180. Sampler state
is trilinear/linear/repeat. First generation-only observer did not fire because
a fully copied chain need not generate a tail; final observer captures bound
render-target chains before draws. 184 tests and 120-second b30 pass; off-menu
passes with no captures. Checked GL state restores, no readback errors. No new
renderer fix. Next inspect consuming shader/material behavior or a reference,
not another mip-existence pass. Normal-menu save/reload remains separately open.

Consumer trace follow-up: existing diagnostics identify the actual water
reflection draw (VS24, 198 indices, modes 64621), ripple target and cubemap.
Generated reflection expression matches the published NVIDIA equation, with a
zero-denominator guard. This is not Xbox precision/orientation/final-pixel proof;
no fix is justified. 90-second b30 passes, state preserved, runtime unchanged.
Next matched reference/material input-output comparison if a visible defect is
isolated; do not repeat existence tracing. Normal-menu save/reload is also a
useful independent acceptance task rather than adding speculative graphics fixes.

Normal-menu follow-up: current counted candidate loads copied build-64 New001
a30 checkpoint, accepts shared fire/look/pause/menu input, completes Save and
Quit, and restores the last checkpoint in a cold process through normal menus.
Evidence: `normal-save66.vLJy5R`. This closes the bounded existing-profile
save/reload gate above; fresh-profile creation, newly reached progression and
general cross-version compatibility are not established. No runtime change.
Original app data preserved except known log. Next return to an actual material
comparison or another unmet control/update gate, not repeated checkpoint reload.

Upstream follow-up: freeze build 73 `d1c7243c` as a separate candidate; accepted
lock remains 66. Full combined counted ANGLE Simulator app builds/installs and
passes menu/a10/scripted-match smoke plus copied build-66 normal-menu save/quit/
cold reload. Original state preserved; rollback app/output/data retained in
`upstream73.NGS3eC`. Protocol moves 9 -> 10. New scoreboard pagination requires
wheel/Page Up/Down inputs absent from our touch adapter. Next reproduce that
overflow/display path and add a minimal touch bridge if confirmed. Separately
compare the captured transient cyan shield-HUD line against build 66 before
attributing it. No new rendering fix or pin promotion. Unadapted Mac/Simulator
acceptance remains required for promotion; do not repeat completed save tests
without another change. Candidate 73 is left installed at the ordinary picker.

Scoreboard follow-up: optional shared-overlay drag hook plus a small Xbox paging
queue implemented without upstream changes. 185 Xbox tests/141 overlay assertions
pass; real drag produces paired Page Down events and no camera displacement.
Fire still works. Visual scoreboard response is NOT established by retained
video/sampled frames, and two players do not exercise overflow. Next discriminate
BACK consumption/fade timing from new guest font/layout/rendering failure before
calling pagination accepted. Do not repeat queue-only checks as end-to-end proof.
Current installed candidate `39f06f77…7198e`; accepted lock 66 unchanged. Original
state preserved and ordinary picker restored. Cyan HUD comparison still pending.

Scoreboard observation follow-up: read-only guest probe on unchanged73 proves
actual automated drag reaches Back and scoreboard-open path. It lasts only one
rendered sample, fade0.0814 / effective opacity~0.85%, then closes. This narrows
the previous video failure to a short-gesture/capture limitation, not proven
font/layout or Back-loss failure. Final pixels and overflow still unaccepted.
Keep hold semantics unchanged. Next prioritize the captured cyan HUD/material
comparison; avoid another queue-only or short-drag loop. LLDB must ignore the
renderer write-watch EXC_BAD_ACCESS before attachment, and must be detached
after sampling. Private probe/addresses are exact-build diagnostics, not an ABI.

Cyan streak follow-up (October3): it reproduces with high-res HUD both on and
off (guest off flag verified), and follows the world under camera rotation rather
than the stationary shield. The private a30 map contains a beam-emitter device,
shader and effects consistent with the traveling blue pulse. Retire the HUD
replacement hypothesis; do not "fix" it by hiding a world effect. Videos and
positive-control detector retained in `hud73.hWLeWC`. Exact effect fidelity is
not established; next obtain a matched pinned upstream desktop material/effect
reference before changing shader behavior. No more repeated HUD toggle passes
without a new discriminating hypothesis. Original state preserved,73 candidate
unchanged, picker restored; goal and broader graphics/controls gates stay open.

Independent-reference follow-up: upstream73's unmodified Windows Xbox-port
binary now renders menu/b30 through a private Wine9 + Mesa26.2.3 llvmpipe setup,
without HaloPad translation or ANGLE. Actual corresponding landing views show
broad ground softness and distant waterfall blockiness on both paths. Regional
differences remain, so no pixel/temporal fidelity gate is closed. Original
Simulator state preserved after180-second pass; accepted66/adapted73 unchanged.
Next reuse this proven independent path for a moving water/effect comparison;
do not rebuild the Mac Android wrapper and call it an independent reference,
repeat this static view, or dismiss all reported defects as upstream behavior.

Water fix follow-up: the moving desktop/Simulator comparison locates genuinely
missing water reflections on ES. Its mip-copy fallback resets framebuffer0 after
the current draw target was bound. New opt-in `render-water-v1` restores real
read/draw framebuffer and scissor state, retaining cache invalidation. Actual
reference/before/after sequences and ordinary app screenshots show restored
animated reflections;188 Xbox tests,37 helper checks and menu/a10/match pass.
Installed candidate `aa0c46d8…7077d`, source73/accepted66 unchanged. Next exact-
candidate normal controls/save reload and Sharper water/depth, then another
material defect. Do not repeat the previous mip-existence/HUD tests or treat
this narrow Simulator correction as all graphics/hardware acceptance.

Water candidate acceptance follow-up: unchanged `aa0c46d8…7077d` passes the
normal-menu copied checkpoint, shared fire/look/swap/2x scope/pause/save-quit and
cold checkpoint reload. UI-selected Sharper persists into fresh b30:1280x960,
effective4x filtering and restored water in actual composited screenshots;
foreground cabin/marines/Pelicans occlude the sampled scene normally. This is
bounded regression evidence, not exhaustive depth, fresh progression or sustained
multi-touch acceptance. Original quality restored, real state preserved, no
pin/product change. Next different moving material/effect against independent
desktop or an unmet product/control gate; do not repeat this completed water/
checkpoint check without changed inputs. Evidence `water-accept73.wCglTA`.

Shared settings follow-up:153 native assertions/90 layout combinations pass,
including12 new registered-handler, cancellation and cross-adapter persistence
checks. Actual Simulator UI mirrors handedness; left Fire and background Look
work. The helper's saved-position fixture is not a real drag test, and the short
automated mirrored LOOK-stick gesture shows no observable rotation. Keep physical
multi-touch and non-default profiles open. Restore original preference absence
after UI-off creates an explicit false key; cold picker and full save/PC audit
pass. No runtime/pin change. Evidence `shared-settings73.ugJORz`. Next compare a
different moving material/effect against desktop; do not repeat these unchanged
settings or checkpoint tests absent a new concern.

Bridge comparison follow-up: a10 exterior broadly matches desktop, but Simulator
has long black bridge-floor bands absent in desktop. Actual per-draw before/after
locates shadow projection; a matched material capture shows a128x128 single-level
linear shadow with bright edge texels and CLAMP_TO_EDGE instead of requested
BORDER. Both shadow projection and convolution depend on BORDER; ANGLE Metal
does not expose it. Next implement/test faithful border sampling as a separately
identified candidate, preserving half-texel blending and legitimate shadows.
Do not change EQUAL or hide shadows. Full mip/cube support requires its own proof.
Use `effects-reference73.xzSmz9` captures for the A/B; fixed frame numbers do not
guarantee the same cinematic camera. Original state restored; no runtime/pin change.

Border-fix follow-up: new guarded `render-border-v1` candidate emulates actual
border color/half-texel blending for single-level 2D point/linear textures, leaving
unsupported mip/cube/3D paths unchanged. Matched bridge bands disappear while
nearby character shadow remains. Original water, exterior glow, menu and local
match remain visible.194 Xbox tests/38 launch-save-quality checks pass; full build
and in-place install verified, app `dc469db1…5997d`. Real state preserved and
ordinary Original picker restored. Evidence `border-fix73.zpl1rD`; upstream73
and accepted66 unchanged. Next exact-candidate normal controls/save/cold reload
and Sharper regression, then a distinct unmet material/control gate. Do not use
prior water-build acceptance to close these changed-build gates.

Border-candidate regression follow-up: unchanged `dc469db1…5997d` passes normal
menus, copied a30 checkpoint, Fire/Look/Swap/Zoom/Pause, Save and Quit and cold
reload. UI-selected Sharper persists into b30 and a10 at1280x960/effective4x;
water detail remains and bridge bands are absent. No full parity or human touch
claim. Real state preserved, Original restored; evidence `border-accept73.vtqWlq`.
Next reproduce non-default button-profile behavior using a copy, then identify
an explicit mapping/menu-state boundary. Fixed default SDL mappings and upstream
five button/four stick presets are verified source facts, not yet a new runtime
reproduction. Preserve physical controller preferences; no silent profile reset
or guessed guest offsets. Do not repeat these unchanged regression checks.

Non-default profile follow-up: copied Southpaw buttons reproduce reversed Fire/
Throw through actual menus and gameplay (grenade1->0 for Fire; rifle60->59 for
Throw). The real data audit passes, ordinary Original picker restored; same
candidate/pins. Evidence `profile73.YKiZSk`, including retained changed test
profile. Next implement/test a guarded, versioned mapping/menu-context bridge,
not another reproduction. Source candidates are resolved input preferences and
`ui_widgets_active()` before device-state polling; insertion timing/startup still
need validation. Touch-only normalization must preserve raw menu A/B, physical
controller preferences and held-input release across transitions. Keyboard
emulation is also raw Xbox input, so it does not solve the mismatch. Follow the
concrete gates in XBOX-SIMULATOR-PASSES. No fix or new unit suite claimed yet;
broader rendering and human multi-touch remain open. Goal active.

Input bridge implementation checkpoint: `shared-input-v1` adds a guarded,
versioned resolved mapping/menu import and normalizes touch only.198 Xbox tests,
153 native overlay assertions and39 launch/save/quality checks pass. Actual
copied Southpaw profile now fires with Fire and throws with Throw; normal menus,
Save and Quit and cold checkpoint reload pass. Installed app `48f118f3…213e`,
guest `652fbebb…de17`; upstream73/accepted66 unchanged. Real state preserved,
ordinary Original picker restored. Evidence `input-bridge73.BvdqSP`. Next test a
different A/B-binding preset and Default on the new candidate, then exact-build
water/border/local-match regression. Unit coverage of all presets is not actual
all-preset movement/feel acceptance. No hardware or broader fidelity claim.

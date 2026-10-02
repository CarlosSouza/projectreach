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

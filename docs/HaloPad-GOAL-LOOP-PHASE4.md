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

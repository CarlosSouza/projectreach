# Xbox / edition-picker passes, 2026-10-01

Scope: HaloPad's existing checkout, local personal builds, and the dedicated
**HaloPad Xbox iPad** Simulator (iPadOS 26.5). No physical iPad installation,
IPA creation, publication, or upstream modifications.

## Build decision

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
future pin. Ordinary builds now use build 59 without an override. The app builder
checks the revision and guest/library hashes; mixed or rejected outputs fail closed.

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

# Xbox / edition-picker passes, 2026-10-01

## Southpaw thumbsticks and gesture duration (2026-10-03)

Previous pass is progress: exact-candidate Original rendering/local match pass.
Same installed `48f118f3…213e`, guest `652fbebb…de17`, shared-input-v1; accepted66
and upstream73 unchanged. No app rebuild. Private evidence:
`ref/xbox-build/passes/2026-10-03/sticks73.gpETY0/`.

Full real-container backup precedes testing. `session/save` is a copy of the
prior border-acceptance checkpoint, with private map links and launch-only
network/update suppression. No forced edition/init, scripted input or edited
profile bytes. PID86046: picker -> Xbox -> Settings/New001/Controller Setup;
select **Southpaw thumbsticks**, keeping Default buttons. Accept/Save Changes
through genuine menus, then Campaign/New001/Halo/Normal. Context reports
`76513240:fedc`, sticks1, valid1. This is different from Southpaw **buttons**
tested earlier. `southpaw-profile` preserves the genuine saved test profile.
Background swipe visibly rotates toward the pod wall (`southpaw-before.png`,
`southpaw-look.png`). Short Move drags show no clear displacement.

PID90343 cold-launches the same saved Southpaw profile through normal menus.
Existing `HALOPAD_TRACE_TOUCH=1`/`XG_TOUCH_TRACE=1` diagnostics show:

- Move forward began/moved/ended at418682.266; canonical Y-1 reaches guest
  axis3 as-1 and then0, as Southpaw requires.
- Move right began/moved/ended at418928.276; canonical X+1 reaches guest
  axis2 as+1 and then0. Both confirm routing, not sustained locomotion.
- LOOK began418906.454 and moved/ended418906.455. The display-link-driven held
  look path has no meaningful hold interval; no visible held-aiming pass.
- Shared Fire reduces rifle60->59 with grenade1 unchanged; trace axis5 press/
  release agrees. The earlier PID90069 attempt used the wrong trace-variable
  name and never left the picker; it is not gameplay/trace evidence.

Do not extend input lifetime or alter the actual control response to accommodate
these0–1ms automated drags. Native regression coverage now checks60 repeated
polls without new touch events under all five button/four stick combinations,
unchanged context, release, one-poll short drags and held-axis suppression across
preset changes until release.199 Xbox tests pass, including the sanitized C
mapping harness. This is state-machine coverage, not human touch-feel acceptance.

Final ordinary Original picker PID92696 verified visually/AX. Installed SHA
unchanged.196 real Documents differ only by app log; Library only four Metal
cache files. Preferences/keyset and PC registry exact. No hardware, cleanup,
IPA, publication or pin promotion. Disk4.5GiB free at start; only small captures.
Next exact-build Sharper graphics regression. Sustained/multi-touch feel,
legacy-stick runtime behavior and broad fidelity remain open; repeating the
same zero-duration Move/LOOK gesture is not a useful next pass.

## Shared-input Original rendering regression (2026-10-03)

Previous pass is progress: Jumpy/Default controls run correctly. Unchanged
installed app `48f118f3f2cb690c29a8978c8f213644c45ddfc0400cfdf26495add966d1213e`
is verified before/after; guest `652fbebb…de17`, shared-input recipe and
accepted66 unchanged. Read-only `git ls-remote` confirms upstream main remains
`d1c7243cb20eab4488efa1266e259b1f4d5240f6`; nested source is clean. No new import.
Private evidence: `ref/xbox-build/passes/2026-10-03/input-render73.wmTYV3/`.

Full real-container backup precedes testing. Independent scene data/save roots,
trusted map links and launch-only suppression of online/clipboard/UPnP/update
checks. Campaigns use explicit init maps with no scripted player input: these
are rendering diagnostics, not normal-menu or human gameplay acceptance.
Original quality is preserved, actual640x480/effective1x filtering. Capture
counts include loading and must not be interpreted as FPS.

- b30 PID76028:100.858s,121 BMPs/20 composited screenshots. `b30/screen-04.png`
  retains detailed reflective/ripple water behind the weapon and Pelican cabin;
  screen07 shows the beach/enemies/effects with ordinary foreground occlusion.
- a10 PID77037:180.696s,134 BMPs/36 composited screenshots. Approximate camera
  ranking selects `frame03792.bmp` for the prior bridge02832/desktop01336 view.
  `bridge-comparison.png` visibly retains the removal of long black floor bands.
  Exterior `frame01536.bmp` retains bright engine glow; camera/ship position
  differs, so this is not pixel parity. Reference desktop is the pinned Xbox
  port, not HaloPad's Windows CE engine. The first comparison attempt lacked
  Pillow in `.venv`; system `python3` runs the same helper successfully.
- 65-second local-match smoke passes: last_tick1533,12 scripted shots, lit0.980,
  expected ANGLE Metal renderer. `match/result.json` and `match/match/screen.png`
  retained. Native capture shows textured terrain/base, weapon and HUD. The
  stand-in network machine is not a second playable client or online acceptance.

Campaign sampled frame/presentation checks report error0 and complete FBOs.
Match retains the known startup `glBlitFramebuffer`0x502 diagnostic, despite
frame0/1/2/120 subsequently reporting0. No blanket error-free claim.

Small test-harness change: all smoke cases now include `HALO_UPDATE_AUTO=false`
in their isolated launch environment; upstream's generated default is true.
This explicitly excludes update prompts without changing saved configuration,
image/progression gates or the installed app.31 focused diagnostics tests and
199 Xbox tests pass; native overlay/launch suites are not newly rerun.

Final ordinary Original picker PID81615 verified through actual UI.196 real
Documents differ only by app log; Library changes only four Metal cache files.
Preferences/keyset/PC registry exact. No hardware, IPA, publication or pin
promotion. Disk was5.3GiB free during capture; no cleanup or rebuild performed.
Next distinct gate: alternate-thumbstick runtime move/look semantics. Sharper on
this exact executable, full preset feel and broader temporal fidelity remain
unaccepted. Do not repeat unchanged Original captures without a new concern.

## Jumpy and Default runtime regression (2026-10-03)

Previous pass is progress: the touch-only mapping bridge fixes Southpaw.
This pass tests an A/trigger-changing preset and the common Default setup on the
same installed `shared-input-v1` candidate, without rebuilding. Installed app SHA
`48f118f3f2cb690c29a8978c8f213644c45ddfc0400cfdf26495add966d1213e` verified before
and after. Guest/recipe/upstream73/accepted66 identities remain those below.
Private evidence: `ref/xbox-build/passes/2026-10-03/presets73.WnjD7M/`.

Full real-container `data-before` backup precedes testing. Separate copied
checkpoint/profile trees isolate Jumpy and Default; only genuine settings menus
change profile bindings. Launch-only network/update suppression remains in use.
No forced edition/init, scripted game input, profile-byte edits or hardware.

PID61051: normal picker -> Xbox -> Settings -> New001 -> Controller Setup.
Thumbsticks remain Default; select **Jumpy** buttons, visibly assigning Jump to
left trigger and Throw Grenade to A. Accept and Save Changes through the ordinary
menus. Return to Campaign/New001/Halo/Normal and load the outside-pod checkpoint.
The host context reports `70513246:fedc`, sticks0, valid1 in menus and gameplay.
Shared Jump leaves grenade1 intact; Throw consumes it1->0 with rifle60 unchanged;
Fire subsequently changes rifle60->59. `jumpy-save` preserves the selected profile.

`jumpy.mp4` retains the initial sequence (139.583s). A second isolated Jump-only
recording, `jump-short.mp4` (68.043s), shows a brief viewpoint rise and return;
`jump-detail.png` samples18.5..22s. A nearby fragmentation grenade is picked up
afterward, so its0->1 counter change is not a throw or extra-input claim. Capture
duration includes idle/tool latency, not sustained play. These observations do
not establish movement feel, simultaneous fingers, new checkpoint progression,
all alternate presets or cold reload of the Jumpy profile.

PID70320: fresh isolated Default copy from the previous border regression,
normal menus load the same checkpoint. Context `76513240:fedc`, sticks0, valid1.
Fire changes rifle60->59 while grenade1 stays; Throw changes grenade1->0; Swap
selects the pistol (reserve64); background drag changes view; Pause opens the
ordinary guest menu and context returns to menu1. No new Save and Quit/cold-load
claim here; the preceding Southpaw pass covers those on this executable.

Final ordinary Original picker PID73921. Readback:196 real Documents files,
no additions/removals, only expected app-log change. Library differs only in
`Saved Application State/dev.halopad.HaloPad.savedState/KnownSceneSessions/data.data`.
Preference keys/values and PC registry are exact. Audit JSON and stderr retained.
No runtime edits, build, new unit-suite run, pin promotion, IPA or publication.

Next exact-candidate water/border/local-match regression. Boxer/Green Thumb,
alternate stick runtime behavior, sustained human multi-touch and broad graphics
fidelity remain open. Prior candidate rendering results do not close this gate.

## Southpaw touch mapping bridge (2026-10-03)

Previous pass is progress: a copied Southpaw button profile reverses Fire/Throw.
This pass implements and verifies the paired guest/host fix, keeping upstream73
`d1c7243cb20eab4488efa1266e259b1f4d5240f6` and accepted66 unchanged. Private
evidence: `ref/xbox-build/passes/2026-10-03/input-bridge73.BvdqSP/`.

### Boundary and safeguards

Opt-in `shared-input-v1` includes the previous border/water/quality adaptations.
`scripts/xbox/profile_input.py` guards exact input source/import-list hashes and
adds a scalar `host_halopad_input_context_v1` call before device-state polling.
It reads resolved controller-zero preferences and `ui_widgets_active()`, passing
12 packed button bindings, stick layout and menu state. No guest pointers,
hard-coded memory offsets, saved-profile writes or proprietary source copies.
The four-file adaptation transaction restores upstream files on success/failure
and preserves concurrent edits. A changed upstream source requires review.

`xg_profile_input.h` translates the canonical HaloPad touch pad to the selected
guest bindings, leaves menu controls raw, and inversely routes movement axes.
Context transitions discard queued pad input and suppress previously held
buttons/axes until release; repeated identical context preserves pending taps.
Invalid context suppresses touch-pad output. Relative mouse look is separate;
this is not a claim that invalid context suppresses every input channel. Physical
controller merging is unchanged. Legacy diagonal response remains guest behavior.
Old/unadapted guests retain the existing default mapping and controls warning;
paired candidates show the new touch/profile explanation.

Build: counted ANGLE/Metal Simulator, adaptation recipe
`71781a2a250a1e868243a461edc51127b548307149673ab05179d986e09f95fd`.
Guest SHA `652fbebb435c433dcf9357096a5431092af0316ab9f011f7f892d5b98a67de17`.
Signed app SHA `48f118f3f2cb690c29a8978c8f213644c45ddfc0400cfdf26495add966d1213e`,
verified again from the installed bundle. Backups: full `data-before`, cloned
`app-before` and `out-before`; in-place install only. Nested upstream ends clean.

### Tests and actual Simulator result

- 198 Xbox Python tests pass, including ASan/UBSan native helper tests for all
  five button maps and four stick layouts, menu controls, alias ownership,
  short taps, held-context transitions, invalid data and transaction restoration.
- 153 native overlay assertions and 39 native launch/save/quality checks pass.
  Evidence: `docs/artifacts/2026-10-02/G9/overlay-20261002T193129Z` and
  `generated/xbox-launch-tests/20261002T192505785292Z`.
- PID52279 loads the exact copied Southpaw reproduction profile by ordinary
  picker/Campaign/New001/Halo/Normal navigation, without forced edition/init or
  scripted input. Fire changes rifle60->59 with grenade1 unchanged; Throw changes
  grenade1->0 with rifle59 unchanged. `southpaw-fire.png`, `southpaw-throw.png`.
- Pause, individually observed Move-down gestures and A select Save and Quit;
  actual main menu appears. Resulting isolated save tree retained in
  `after-save-quit`.
- The overlay suite relaunches the app, so PID57563 is the subsequent deliberate
  cold launch using isolated saves. Ordinary menus reload the outside-pod
  checkpoint; Fire again changes rifle60->59 without consuming grenade1.
  `cold-fire.png` and `cold-stderr.log` retain evidence. This reloads an existing
  checkpoint, not newly advanced campaign progress. The profile card's Default
  label reflects neither proof nor reset of Southpaw button settings.
- Logs show mapping `67513240:fedc`, sticks0, valid1, first menu1 then gameplay0;
  the earlier run also returns to menu1 on Pause. Guest preferences remain
  alternate while touch actions keep their labels' meanings.

Final ordinary Original-quality picker PID58269. Real196 Documents have no
additions/removals and only the expected app-log change. Library differs only in
two replaced SplashBoard snapshot files; preferences/keyset and PC registry are
exact. No physical device, IPA, push or accepted-pin promotion.

Next: actual alternative A/B-binding preset and default regression on this exact
candidate, followed by exact-build water/border/local-match checks. All-preset
runtime acceptance, legacy movement feel, sustained human multi-touch, hardware
and broad rendering fidelity remain open. Goal stays active.

## Southpaw touch mismatch reproduced (2026-10-03)

Previous turn is progress: exact border-candidate regressions passed. This pass
reproduces a different unmet control gate, without rebuilding. Installed app SHA
`dc469db18505a1a254502eed6d3427027003857a7a84b530085e226e3bf5997d` verified again;
upstream73/accepted66 and `render-border-v1` unchanged. Private evidence:
`ref/xbox-build/passes/2026-10-03/profile73.YKiZSk/`. Back up the real container,
copy `border-accept73.vtqWlq/after-save-quit` into isolated session/save and link
trusted maps. No forced edition, init script or scripted input; online, clipboard
joins, UPnP and auto-update disabled for the isolated run.

PID32958: normal picker -> Xbox -> Settings -> New001 -> Controller Setup.
Leave thumbsticks Default; change **Button Settings to Southpaw** through the
shared Move control. The visible diagram exchanges Fire Weapon and Throw Grenade
between triggers. A accepts, Move selects Save Changes, A saves. Return normally
to Campaign -> New001 -> Halo in-progress -> Normal. The profile card still says
Controls Default; do not use that card as proof of the button preset.

At the same outside-pod checkpoint, rifle60/120 and one grenade:

- Tap shared **Fire**: grenade1->0, rifle remains60. Throw animation observed.
- Tap shared **Throw**: rifle60->59, grenade remains0.
- Pause still opens the normal guest menu.

Actual Simulator screenshots `01-before-fire.png`, `02-fire-threw-grenade.png`
and `03-throw-fired-rifle.png` retain the HUD counters. This is a reproduced
semantic control defect, not a fix or a direct-finger feel assessment. Copy the
resulting test save tree to `after-southpaw-repro`; its profile SHA is
`59fadcd6ba046465a9f1c3d22543d7e82d31cff729f407b77e6cb6c05e82b255`, versus
original copied profile `fd7317882931657e03e9d277e0e3ee585cdc6690f17df3435d9a5dba6926396b`.
The copied campaign checkpoint stays `9162fda3…1e69`. No fresh progression or
cold-load acceptance of the changed profile is claimed.

### Source boundary and next implementation

`port/xbox/xg_overlay_input.h` maps shared actions to fixed default SDL controls.
Upstream `source/interface/player_ui.c:set_local_player_controls_from_player_profile`
resolves five button presets and four stick presets, then calls
`input_abstraction_update_local_player_preferences`. The actual twelve-entry
mapping is authoritative; do not infer it from profile names or save offsets.
`input_abstraction_update` subsequently applies that mapping to raw pad input.
The Linux keyboard path also synthesizes raw Xbox input, so sending keyboard
events instead is not a semantic bypass.

Source-verified candidate boundary: `input_frame_begin` calls
`input_get_device_states` before abstraction/UI processing. The main loop calls
it before `input_update` and `input_abstraction_update`.
`input_abstraction_get_local_player_preferences(0, ...)` copies controller0's
resolved preferences; `ui_widgets_active()` reports initialized active widgets.
These are candidate inputs to a guarded, versioned guest-to-host context call,
not a runtime-validated bridge yet. Do not reuse Android's no-op relative-mouse
callback as a menu signal. Review startup/loading and secondary frame-begin
call sites before choosing the final insertion point.

Next pass implements the smallest paired bridge with source hashes and a new
adaptation identity, leaving the accepted pin and current graphics recipe intact:

1. Publish menu/game context and resolved mapping before the relevant input poll.
   Validate version, ranges and mapping; reject unsupported context explicitly.
2. Normalize **touch only** to the desired gameplay actions before merging real
   controllers. Preserve raw A/B/X/Y and Move navigation while menus are active.
   Never rewrite profile settings or remap hardware/keyboard globally.
3. Clear queued input and quarantine held touches until release on context or
   mapping changes. Clearing only the host buffer is insufficient: the overlay
   retains held action bits and could reassert them on a later unrelated event.
4. Test all five button maps, aliases, all four stick layouts, invalid contexts,
   short taps, cancellation and menu transitions before installing. Stick layouts
   include nonlinear legacy diagonal processing; a byte permutation alone is not
   movement-fidelity acceptance. Existing relative touch aim needs no speculative
   controller-sensitivity rewrite.
5. Reuse this exact copied Southpaw profile for a before/after Fire/Throw test,
   normal menus and cold reload. Then another non-default A/B layout and Default
   regression. Do not repeat the broken-build repro or promote a pin instead.

Restore ordinary launch with no test environment, PID42922; picker shows Original.
Readback:196 real Documents files, only normal app log changed; Library byte-exact,
preference dictionary and PC registry exact. Nested upstream clean, installed
executable unchanged. No runtime edits, unit reruns, hardware, IPA or publication.
Goal remains active; graphics fidelity and sustained human multi-touch stay open.

## Border candidate controls, saves and Sharper (2026-10-03)

Previous turn is progress: guarded border sampling fixes the traced bridge bands.
This pass verifies the unchanged installed executable
`dc469db18505a1a254502eed6d3427027003857a7a84b530085e226e3bf5997d`, guest
`2d03ab18…b6b6`, upstream73 and accepted66. No rebuild or source/runtime change.
Private `ref/xbox-build/passes/2026-10-03/border-accept73.vtqWlq/`; full real
container backup before testing. Copy `water-accept73.wCglTA/after-save-quit`
into independent session/save, link trusted maps. Normal launch uses no forced
edition, init script or scripted player input; public network/update disabled.

PID18874: actual picker -> Xbox -> Campaign -> New001 -> Halo in-progress ->
Normal loads outside-pod checkpoint, rifle60/120 and one grenade. Actual Fire
consumes one round; background Look drag turns the view; Swap selects pistol;
Zoom shows2x scope (`zoom.png`), second Zoom exits; Pause opens guest menu.
Three individually observed Move-stick-down drags select Save and Quit. Shared
A saves and returns to main menu (`save-quit.png`). Copy isolated resulting tree
before cold launch. Copied input checkpoint SHA
`927818a3dbab2f8c5102463f064f635144e52c75b3c7aa6264f13899422efeb9`;
after-save checkpoint SHA
`9162fda32d717d53c501d127a775fc0a181eae744556bacc614a60afb9541e69`.

Cold PID22487 follows the same normal-menu route and restores outside-pod
location, rifle60/120 and one grenade (`cold-reload.png`). Checkpoint semantics,
not restoration of unsaved weapon/camera changes or new progression. Default
profile only; pointer gestures do not prove simultaneous or sustained touch.
Known upstream profile card still says The Maw/Legendary. Normal-menu startup
frame0 records0x502; frames1/2/120 record0. Retain the known startup diagnostic,
not a blanket error-free claim; no shader compile/link failure observed.

Actual quality UI selects Sharper; fresh isolated b30 PID24634 uses persisted
choice without resolution/filter environment overrides. Completes100.8045s,
210 BMPs/20 native screenshots. Actual1280x960 and effective4x filtering;
sampled GL checks0 and complete framebuffer. Native screen04 visibly shows
detailed water behind the weapon/Pelican cabin; screen07 shows beach, Pelican,
enemies and effects behind the ordinary foreground. No broad depth/fidelity
claim. Fresh ordinary picker PID26212 AX-confirms Sharper persistence.

Additional a10 Sharper PID26491 completes181.2334s,246 BMPs/36 native screenshots.
Actual1280x960/effective4x, sampled errors0. Coarse camera ranking yields
bridge03528 versus old Original02832 and desktop01336 (`bridge-comparison.png`).
Sharper framing is wider/slightly different; no pixel-parity claim. Long black
bands remain absent in the corresponding floor view and native screen15 shows
the bridge floor without the extended bands. Candidate BMP is downsampled only
in the comparison sheet; original1280x960 capture retained. Capture counts
include loading and are not FPS. Independent desktop is the Xbox port, not the
HaloPad Windows CE engine. No new PC gameplay acceptance.

Restore Original through actual UI, cold ordinary picker PID28582. All196 real
Documents files unchanged except normal log; preference keys/values and PC
registry exact. Library only Metal caches/plist encoding differ. No hardware,
accepted-pin promotion, IPA, publication or cleanup; app hash unchanged.

Next distinct control gate: reproduce a non-default guest button profile on a
copy. Current `xg_overlay_input.h` maps named actions to fixed default SDL
buttons; upstream `player_ui.c:set_local_player_controls_from_player_profile`
changes game-action mappings for five button/four joystick presets. This source
fact does not establish the runtime symptom yet. Investigate an explicit guest
mapping/menu-state boundary, preserving actual controller/profile settings and
default A/B menu navigation; do not silently rewrite a player's profile or guess
guest offsets. Broad fidelity and human multi-touch remain open; goal active.

## Bridge border-sampling fix (2026-10-03)

Previous turn is progress: draw/material capture isolated the bridge bands to
requested BORDER being substituted with edge clamp. Private evidence:
`ref/xbox-build/passes/2026-10-03/border-fix73.zpl1rD/`. Complete real-data, prior
app and generated-output backups retained before in-place Simulator installation.
Only dedicated Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB`; no hardware.

New opt-in `render-border-v1` inherits water/quality/counted visibility and adds a
guarded shader helper plus per-draw border state. Eligibility: no native border
extension, single-level 2D, matching point/linear min and mag, no high-res texture
replacement. Linear coverage includes the half-texel fringe and corner product;
point uses the texture domain. Both use actual border color. Non-border axes
retain ordinary sampling. No approximation for mipmapped, anisotropic, mixed
min/mag, cube or 3D paths; those remain unchanged. Uniforms update before the
ordinary-uniform serial early return to avoid stale per-draw masks. Do not claim
full border emulation or change depth/shadow visibility as a workaround.

Separate recipe SHA `eaa7d81b13ea041db40add5fcca82a628a22b6ce5073719c79979a8091f9fed6`.
Renderer SHA `5c8c132048b1efaa57d322b9c8a0ef65df07c1755df653c0f1a178ce96831cc6`;
new shader-generator guard `504d1d854e97612db322ce8e0f8bf89749ee63352067dfe190f3f1528b3dc054`.
Both source inputs/anchors must match before any mutation; temporary changes
restore after build/failure, preserving concurrent edits for manual review.
Old water recipe identity remains unchanged. New module enters runtime provenance.
Guest SHA `2d03ab1836a4aa687ac70062776966e7a2dcf795769297c5aa50013a39aea6b6`;
installed executable SHA `dc469db18505a1a254502eed6d3427027003857a7a84b530085e226e3bf5997d`.
Upstream candidate73 remains `d1c7243cb20eab4488efa1266e259b1f4d5240f6`;
accepted66 lock unchanged. Nested checkout clean after restoration.

194 Xbox tests pass in27.803s;38 native launch/save/quality checks pass in
`generated/xbox-launch-tests/20261002T175534413432Z/`. New tests exercise unique
anchors, strict identity, two-source restoration/concurrent edits, compiled C
eligibility with ASan/UBSan, and an independent bilinear-border numeric oracle
with arbitrary texels, nonzero color and axis/corner cases. The oracle is a CPU
math check, not GPU pixel equivalence. Actual candidate shader compiles/renders.
Full guest/library/combined app builds and strict codesign verification succeed;
known upstream availability/libtool warnings retained in logs.

a10 PID6415 completes181.4469s:474 BMPs and18 native screenshots, fresh isolated
save, no optional draw observers. Counts include loading, not FPS. Camera-matched
`bridge-comparison.png` uses prior desktop1336 / old Simulator2832 / new3312:
the long floor bands disappear. Small floor box440:635,265:340 has RGB-mean<8 at
5 desktop /936 old /8 new pixels, a local diagnostic only, not pixel parity.
Nearby new3204 (`preserved-shadow.png`) retains the localized full character
shadow. Native screen09 shows ordinary bridge corridor without the bands.
Exterior engine glow remains in the corresponding comparison. Slight camera/
animation differences persist; this is bounded defect acceptance, not fidelity.

b30 Original PID8160 completes101.0809s:175 BMPs/20 native screenshots. Native
screen04 shows restored detailed blue/ripple water behind the Pelicans; screen07
shows ordinary cabin/marine/beach foreground. Sampled presentation checks report
complete framebuffers and error0,640x480 source/filter1; not every-frame proof.
65-second scripted local match passes (tick1532,12 shots, lit0.967), visually
reviewed native screen retains world, weapon, HUD and shared controls.30-second
menu passes (lit0.633). Bot smoke is not human-control/network acceptance.

Cold ordinary picker PID11624, Original quality restored/unchanged. Real196
Documents files remain except normal log changes; preference dictionary and PC
registry exact. Library changes only Metal caches and OS snapshots/scene state.
No IPA, publication, accepted-pin promotion or cleanup. Backups remain available.
Next normal-menu shared controls/save-and-quit/cold checkpoint reload and Sharper
on this exact executable. Prior water-candidate acceptance cannot substitute for
those regressions. Full material fidelity, non-default controller profiles,
sustained human multi-touch and hardware remain open; goal active.

## Bridge shadow-band localization (2026-10-03)

Previous turn is progress: shared settings contracts verified. New private
`ref/xbox-build/passes/2026-10-03/effects-reference73.xzSmz9/`, full real-data backup,
fresh isolated saves/maps links. Same installed water73 app `aa0c46d8…7077d`,
accepted66 unchanged, and the independent official desktop73/Wine9/Mesa reference.
No player input or public network. a10 init script starts the opening cinematic.
Desktop PID66317 exits0 after187.80s/180 BMPs; Simulator PID66346 completes181.46s,
410 BMPs plus18 native screenshots. Counts include loading, not FPS. Match camera
geometry, not elapsed time or equal frame numbers. Coarse pair ranking excludes
black frames;131 desktop/375 Simulator nonblack frames retained. No fidelity
threshold claimed. Initial analysis venv lacks Pillow; use bundled runtime.

Exterior engine glow/hull lighting/starfield broadly agree. Interior bridge
desktop1336/Simulator2832 reveals long black floor bands only on Simulator
(`desktop-bridge.png`, `simulator-bridge.png`, `pairs-2.png`). Camera/animation
positions differ slightly; the absent-vs-present bands remain a concrete defect.

Existing opt-in color timeline, no app changes: trace PID77945 completes181.53s.
Frame2832 has235 complete before/after draw pairs, no reported GL errors. Draw0091,
program212,1062 indices, EQUAL introduces a floor band. Its vertex shader bytes
match `vs030_0.glsl` (SHA `2edbb969c7a8cdf15e147d8ce038a142b2468034e065d3661826bfbec58ddc61`).
Guest trace identifies shadow projection with128x128 render-target texture and
16x16 corner fade; source shadow pass requests BORDER for stage0 U/V. This is
not the later program25 lightmap draw, which has a larger overall darkening count.
Correct an initial diagnostic vertical flip before ranking/inspecting snapshots;
the game's offscreen target pixels already have the image orientation used here.

Reuse the exact-source material observer with private selector aliases: its
historical `vs007_0.glsl`/`ps_0c014f79.glsl` filenames contain the captured program212
sources, **not** claims that those historical shader IDs are the shadow program.
Actual shader dumps remain in a separate directory. First material PID86772,
181.23s/428 BMPs, captures an earlier close-up at frame2832; fourteen complete
shadow draws, no floor-band proof in that view. Do not infer matched scene from
a fixed frame number. Its camera correspondence occurs near frame3024 instead.

Follow-up material2 PID89069,181.35s/442 BMPs: frame3100 has217 complete color
pairs and25 complete shadow-material draws. Draw0073/program215/1068 indices
visibly adds a black line across the floor (`material2-0073-before/after.png`).
692 pixels in the diagnostic floor box darken by more than half; this count is
localization, not correctness. Actual stage0 texture157 is128x128, maxlevel0,
linear min/mag (`0x2601`), U/V CLAMP_TO_EDGE (`0x812f`); seven edge samples have
nonzero RGB, up to255 (`material2-0073-texture0.png`). Stage1 is16x16 corner fade.
Both projection and convolution request BORDER in upstream shadows source.
ES address_mode substitutes edge clamp when border_clamp is false; startup
reports0, and pinned ANGLE Metal explicitly leaves textureBorderClampOES disabled.
These observations strongly support edge extension as the mechanism. A corrected
sampling A/B is still required to establish the fix and preserve real shadows.

Next implement a separately identified, guarded border-sampling candidate; handle
the actual single-level linear shadow/convolution paths and half-texel boundary
filtering faithfully. Do not hardcode scene geometry, remove shadows, alter EQUAL,
advertise unsupported capabilities, or claim full mip/cube border emulation from
a shadow-only test. Verify the captured bands disappear, legitimate shadows remain,
then regress water, menus, controls/saves and quality modes on the changed build.

All captures finished. Ordinary picker PID90385/Original visually and AX verified;
all196 Documents retained except log, Library only Metal cache/OS scene state,
preferences and PC registry exact. App hash unchanged and upstream clean. No
runtime edit/rebuild/install/unit rerun, pin promotion, hardware, IPA or publication.
Goal active; this pass yields a specific renderer defect and fix target.

## Shared settings and layout contracts (2026-10-03)

Previous turn is progress: water-candidate controls/save/Sharper checks passed.
This pass covers an unmet shared-settings regression contract, without changing
runtime code, installed app, guest73, water recipe or accepted66. Private evidence:
`ref/xbox-build/passes/2026-10-03/shared-settings73.ugJORz/`. Full real container
backup precedes testing; isolated session links trusted maps and copies the known
`normal-save66.vLJy5R/after-save-quit` checkpoint. Forced Xbox edition, then actual
Campaign/New001/Halo/Normal menu route; no init script or public networking.

`tests/halo_overlay_test.m` adds12 checks: settings opening cancels Xbox holds;
registered settings handlers remain wired; handedness mirrors identities; shared
Look Speed affects Xbox counts; editing cancels gameplay; resize/selection do not
play the game; a fresh PC overlay reads the same position, scale and sensitivity;
phone layout keys do not overwrite tablet; fresh Fire works after editing.
The helper has no UIApplication event loop: invoking UIControl events did not
dispatch handlers, so tests explicitly verify registration and invoke the real
handlers. A driveControl test hook also bypasses the touch editor guard and is
not used to claim editor touch suppression. UISlider's near-2 float is compared
with tolerance, with integer mouse counts derived from its actual value. Saved
position is an explicit persistence fixture, not a claim of real drag delivery.
An intermediate diagnostic compile error was corrected before the final run.
Final native run153 PASS/0 failures,90 layout combinations/0 failures:
`docs/artifacts/2026-10-02/G9/overlay-20261002T170146Z/` (UTC date).

Actual installed app PID39747: native Menu/Controls/Look Speed & Touch Settings
opens the shared panel. Left-handed switch changes both clusters; left Fire
consumes a rifle round60->59 and background left-side swipe visibly rotates the
camera (`left-handed-gameplay.png`). Short automated left LOOK-stick drag has no
visible rotation; short gesture duration is a possible cause, not established
here. Sustained mirrored-stick behavior remains unaccepted.
Generic switch AX click is ambiguous; coordinate clicks sometimes do not toggle.
A visible switch drag reliably restores off, AX confirms0, and Done closes the
panel. No real sensitivity or custom-layout edits made. This does not establish
physical multi-touch, actual cross-engine gameplay or non-default guest profiles.

UI-off writes an explicit false preference where the original key was absent.
Audit catches that difference; terminate app and remove only the newly created
`HaloPad.leftHanded` key through Simulator defaults against its exact container
preference domain. Cold picker PID58756: Windows left/Xbox right, Original quality,
no launch overrides. Readback shows identical preference keys/values, identical
PC registry, all196 Documents retained with only normal log changes. Library
otherwise changes plist encoding and OS scene state. Backup retained. Installed
executable SHA remains `aa0c46d8ebf733fa5d208e7a2df5703f57b81b36242e21a8554737981667077d`.
No build/install, hardware, IPA, publication or pin promotion. Next different
moving material/effect comparison; full goal remains active.

## Water candidate controls, saves and Sharper (2026-10-03)

Previous turn is progress: `render-water-v1` fixes the missing water layer.
This pass tests that exact installed app `aa0c46d8…7077d` without rebuilding or
changing source73, guest `83dd49b6…45c5`, counted ANGLE or accepted lock66.
Private evidence: `ref/xbox-build/passes/2026-10-03/water-accept73.wCglTA/`.
Full real container backup precedes testing. Normal-menu session uses linked
trusted maps and a separate copy of `normal-save66.vLJy5R/after-save-quit`;
no init script, forced edition, scripted player input or public network.

PID82463: actual picker -> Xbox -> Campaign -> New001 -> Halo (game in progress)
-> Normal restores outside-pod a30 checkpoint, rifle60/120 and one grenade.
Shared Fire consumes one rifle round, background Look drag visibly rotates the
camera, Swap selects pistol, Zoom produces the2x scope (`zoom.png`), second Zoom
unscopes, and Pause opens the guest menu. Three individually observed Move-stick
down gestures select Save and Quit; shared A returns to main menu (`save-quit.png`).
Copy resulting isolated save tree before cold launch. Checkpoint SHA256 before
run `050d594383ed1e8f718e0050c4ad75916f3fa68f2e9a2b62fddcaf72125fa6de`,
after Save and Quit `b67a65d2dfca0e6ea91cf30be29caef3837e82cc0609fea89e89690ebabb837b`.

Cold PID99163: same normal picker/menu route restores the outside-pod checkpoint
with original camera and rifle60/120 (`cold-reload.png`). This is checkpoint
semantics, not restoration of unsaved aiming/weapon state. Reloaded file becomes
`927818a3dbab2f8c5102463f064f635144e52c75b3c7aa6264f13899422efeb9` as the
running game rewrites it; byte identity is not the save-acceptance criterion.
No fresh profile, new progression or all-version save claim. The known upstream
unlock-all profile card still says The Maw/Legendary. Device Hub briefly stalls
its AX observations and its first Xbox-card click does not dispatch (live process
remains idle at picker, no Xbox startup log). Clicking the visible card by
coordinates starts normally; no app restart/fix needed for that viewer hiccup.

Return to normal picker and choose Sharper (Preview) in the actual quality UI.
New isolated b30 process PID4246 uses that persisted choice, with no resolution
or filtering environment override. `sharper-capture.py` captures101.55 seconds,
20 native Simulator screenshots and172 upstream BMPs (includes loading, not an
FPS measurement), then terminates normally. Actual source1280x960, effective4x
world filtering, complete framebuffer0x8cd5 and sampled GL errors0. The saved
choice is also AX-confirmed Sharper after a subsequent fresh picker launch.
`sharper/screen-04.png` visibly shows detailed blue water behind the two Pelicans;
`screen-08.png` shows the cabin/marine in front of the landing terrain. No
foreground-through-water artifact in these samples. This is a bounded visible
occlusion regression, not exhaustive depth correctness or exact desktop parity.
Remaining ground softness is still visible; do not label Sharper a texture cure.

Restore Original through UI; final picker PID16731 and `final-picker.png`.
All196 original Documents files retained; only the normal log changes. Library
changes only OS KnownSceneSessions state. Preferences and PC registry are exact;
the installed executable hash is unchanged. No real save marker advance, app
replacement, hardware, IPA or publication. No unit rerun for unchanged product
code. Next pursue a different moving material/effect with the now-proven desktop
reference, or an unmet product/control gate; do not repeat this checkpoint or
water regression without a changed build or new concern. Full goal stays active.

## Water mip-copy state fix (2026-10-03)

Previous turn is progress: independent upstream73 desktop reference works.
Private evidence `ref/xbox-build/passes/2026-10-03/water-reference73.eG38SP/`.
Same official Windows Xbox-port binary, Wine9 and Mesa26.2.3 software renderer
as `desktop73.Fi7gKD`; same maps, Original640x480, default interpolation/HUD,
no player input. New isolated desktop/Simulator data and saves. Full real app
data, prior app and prior output tree are copied before candidate replacement.

Capture the b30 approach with upstream's existing screenshot facility. Desktop
PID59386:154.81 seconds/294 BMPs, interval4, exit0. Before-fix Simulator PID59420:
91.69 seconds/361 BMPs, interval12. These counts include loading and are not
performance measurements. `match.py` ranks coarse image correspondences only;
manual inspection confirms the relevant views, not exact camera/time identity.
Desktop96/Simulator216 show the island approach; desktop276/Simulator792 show
water from inside the Pelican. In both, desktop has detailed blue reflections
and Simulator has a nearly uniform green surface. This is a concrete rendering
defect, unlike the earlier broad static terrain similarity.

Source cause: `prepare_draw` binds targets and applies raster state **before**
`bind_textures`. The active ES3.0 path logs `copy image 0`. When assembling the
water mip composite, `copy_level_by_blit` binds temporary read/draw framebuffers,
disables scissor, and finishes by binding framebuffer0. Invalidating the state
cache only repairs a later draw; the current reflection draw has already lost
its target. Desktop uses `glCopyImageSubData` and avoids this fallback. Earlier
mip completeness/readback tests were correct but did not establish that the
subsequent water draw still targeted the game's back buffer.

New separately identified `render-water-v1` layers a narrow state-preservation
recipe on the existing visibility/quality adaptation. Read back read framebuffer,
draw framebuffer and scissor enable before the copy; restore all three afterward,
then retain the existing cache invalidation. No shader math, texture filtering,
map bytes, controls or upstream pin change. Full input SHA and unique anchors
guard application; upstream source is clean again after Ninja. Old visibility
recipe identity remains byte-for-byte unchanged. New recipe SHA256
`ab7a4178dd3ab4363cc5ab7203e84b5acebecf6ed5af2c1e26c2c30a12302db2`.
Build/packaging require matching counted ANGLE identity and Simulator SDK; the
new recipe retains Original/Sharper UI support and is not a hardware default.

188 Xbox tests pass, including exact inserted C state-restoration fragments
under ASan/UBSan (same/distinct framebuffer bindings, scissor on/off, four levels),
anchor rejection, separate identity and packaging rejection without the matching
backend/platform. Launch-helper link initially exposes a missing inert stub for
the previous scoreboard bridge; add an aborting stub, preserving the fixture's
no-game rule. The corrected suite passes37 actual Simulator helper checks at
`generated/xbox-launch-tests/20261002T155744884409Z`. Full guest/library/app build
and strict codesign validation pass; known8 availability/libtool warnings remain.

Candidate installed in place: executable SHA256
`aa0c46d8ebf733fa5d208e7a2df5703f57b81b36242e21a8554737981667077d`, guest
`83dd49b694ad8425fae2a561143c3d55617e2ef064a51d4f7d3075ab34b045c5`.
PID65491 runs91.42 seconds/405 BMPs. `candidate180.png` and `candidate792.png`
restore reflections/ripple detail in both reference views. In a fixed water-only
region x0..279/y270..409, horizontal RGB variation rises from0.005 before to2.66
after, versus3.05 desktop (0..255 channel units). Adjacent captured sample changes
rise from0.09 to6.73, versus7.07 desktop. These characterize restored spatial and
temporal detail, **not exact phase/fidelity or performance acceptance**.

Observer-off follow-up PID69271: no optional GL trace, upstream bitmap screenshots
or scripted input;55-second bounded run, native Simulator screenshots every5s.
`clean/screen-02.png` and `screen-04.png` visibly show restored ocean detail in
the actual composited app with shared controls. This rejects a fix visible only
through the bitmap diagnostic. Default built-in startup frame probes still run.
Separate normal regression passes menu30s, a10 60s/three nonblack late samples,
and Blood Gulch65s/tick1530/12 scripted shots with one stand-in peer, not a second
human game client. All run handles complete. Normal picker is restored and real
state is read back after the in-place install; see this pass's audit JSON.
Final picker PID78201 is AX-verified at Original quality. All196 original
Documents files remain, only the known log changes. Preferences and PC registry
are exact. Library differences are Metal cache, OS scene state and replaced
SplashBoard snapshots only. No real save-revision marker is advanced by the
isolated data/save overrides.

Accepted lock remains66, guest source frozen73, no hardware/IPA/publication.
This fixes the reproduced missing-water layer on Simulator; it does not close
all texture/shading, normal-menu save acceptance on this exact candidate,
sustained shared controls, unadapted73 update gates or physical-device gates.
Next verify this candidate's ordinary controls/save reload and Sharper water/depth
path, then pursue a different reproduced material defect. Do not repeat missing-
mip existence or HUD toggles; no longer describe this water issue as unlocated.

## Independent desktop reference (2026-10-03)

Previous turn is progress: it retired the replacement-HUD hypothesis. This pass
establishes a separate desktop renderer instead of treating the translated
Android guest's Mac wrapper as an independent reference. Private evidence:
`ref/xbox-build/passes/2026-10-03/desktop73.Fi7gKD/`.

The existing Mac executable manifest is build66 while the shared guest output
is adapted73; do not run that stale pair or overwrite output just for this
comparison. Upstream's desktop targets are 32-bit x86 Linux/Windows/OpenGL4.5,
not native macOS. Docker's daemon socket is absent; no VM/service is started.
Existing Wine9 is available. Use a **new private Wine prefix**, local per-app
Mesa DLLs and upstream's official Windows release. No system driver install,
default Wine-prefix change, upstream source patch, or HaloPad rebuild occurs.

Provenance (release download hashes independently match GitHub asset digests):

- [Upstream build73](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-73),
  commit `d1c7243cb20eab4488efa1266e259b1f4d5240f6`.
  `halo-windows-release.zip` SHA256
  `5944157726fa5dd9c60e697e07efb7ceaa8bd3f2cb2a0afec50c366dc210517a`;
  extracted, unmodified `halo.exe`
  `3f0e2355332fb36f913ba0ca6d2c52832ffb17f8d183d58ed22b91a51db033c6`.
- [Mesa Windows package26.2.3](https://github.com/pal1000/mesa-dist-win/releases/tag/26.2.3),
  MSVC archive SHA256
  `3f3613adb43cfd0f2e665ce2400b130c275f0b3317cb3a05566320a3a67589ed`.
  Only x86 `opengl32.dll` and `libgallium_wgl.dll` are deployed beside halo.exe.
  Actual context: OpenGL4.6 Core, Mesa26.2.3 (`afe29290a0`), llvmpipe
  LLVM23.1.2/128bits, four software worker threads. This is an independent
  software-renderer comparison, **not hardware/performance or original-Xbox proof**.

`wine-menu.log` and `menu.png` verify real menu rendering, not merely launch.
`run-reference.py b30` starts with an allowlisted shell environment, an isolated
data/save root and the same trusted maps. Online/clipboard join/UPnP/auto-update
are disabled. Windowed640x480, default HUD, no scripted player input. Upstream
`init.txt` requests `map_name levels\b30\b30`; screenshot interval120 frames.
PID38855 exits0 at244.39 wall seconds with16 BMP samples, using upstream's
240-second exit setting. The private runner has a300-second watchdog scoped to
this prefix only. Do not rerun into the same data/frame folders: it refuses
existing directories; use another named pass when changing inputs.

Before Simulator launch, copy the full real app-data container to `data-before`.
Run the unchanged installed candidate with:

```sh
.venv/bin/python scripts/xbox/smoke-simulator.py \
  --device DF51182F-1878-4A54-9AED-CC4AED86BEAB \
  --out ref/xbox-build/passes/2026-10-03/desktop73.Fi7gKD/simulator \
  --case campaign --campaign-map b30 --seconds 180 --render-diagnostics
```

Pass: correct ANGLE renderer, b30 load, four late nonblack samples, presentation
capture, no scripted input or reported signal. This bounded smoke gate does not
grade pixels. Installed app remains `39f06f77…7198e`, adapted73 guest unchanged,
accepted lock66. Both sources draw640x480; compare source targets, not the larger
Simulator screen or touch UI. `sim-upright.png` vertically flips the raw GL source
readback for viewing; this is a diagnostic orientation conversion, not a runtime
fix. `b30-landed.png` is upstream frame1080. The landing camera is visually
aligned, **not verified identical matrices or synchronized simulation**.

Both images show the broad blurry ground bands/detail transition and blocky
distant waterfall. Neither symptom alone establishes a HaloPad/iOS regression.
`compare.py` records regional mean absolute RGB differences on the0..255 scale:
left cliff3.19, left ground5.91, ground detail9.94; animated waterfall6.92.
These are measurements without an acceptance threshold, not a claim of exact
parity. Actor positions, radar, clouds/waterfall animation, and subtle sampling
differences are not time-matched. Do not dismiss the user's temporal shading,
focus, water or other-scene complaints from this stationary view.

Both run handles complete. Normal app PID55014 returns to picker; AX verifies
Windows left, Xbox right, Original quality. Audit retains all196 Documents
files, only normal app log changes; Library is byte-identical, preferences and
PC registry exact. No reinstall, shader change, unit rerun for unchanged runtime,
hardware, IPA, pin promotion or publication. Next use the now-working independent
reference for matched moving water/effects; do not repeat the same static landing
or infer all graphics accepted. Unadapted73 update gates remain separate.

## Cyan streak isolation (2026-10-03)

Previous turn yielded guest Back/fade evidence (progress). This pass returns to
the reported graphics issues. Private evidence is
`ref/xbox-build/passes/2026-10-03/hud73.hWLeWC/`. Same installed73 executable
`39f06f77…7198e` and guest `109a9789…bb4a3`; accepted66 unchanged. No rebuild,
install or product-source change. Full real app-data backup completes before
test launch. Two independent copies of build66's preserved checkpoint go into
`save/` and `save-low/`; never select or modify original app saves.

Launch with `HALO_HIGH_RES_HUD=true` (PID16536), then false (PID19288); each
uses the normal Campaign/New001/Halo-in-progress/Normal menu route. Same
Original640x480 quality and counted ANGLE renderer. Fire60->59 and background
Look `[1540,790] -> [1480,790]` exercised in both. Network options off. Both
recordings show the recurring cyan streak. `high-at50.png` reproduces its
apparent alignment beside the shield, including the bright traveling pulse.
In the unturned low-HUD scene the pulse is instead near the upper center, away
from the stationary shield. After turning, it aligns near the shield again.
Retained `low-world-streak.png` / `low-turned-streak.png` are frames9/6041.

The exact guest symbol `hud_hires_override_find.hud_enabled` is0 in the second
run, titles_enabled1 (`low-hud-readback.log`). This verifies effective config,
not just launch intent; high-res asset registration logs alone do not prove
replacement usage. Probe reads memory only, ignores intentional write-watch
faults before attachment, and detaches in the same batch. No guest mutation.

`check-line.py` validates a narrow cyan-column detector against the original
artifact screenshot (140/140 rows at native x2047). Video output uses passthrough
timestamps, avoiding ffmpeg's initial duplicate-frame resampling. High recording:
71.653s /331130760 bytes,5767 decoded frames,347 detections in the original
above-shield strip. Low recording:142.952s /670881687 bytes,11407 decoded frames,
790 detections in that strip. A second strip near center finds769, with peak
native x1256 versus x2052 after turning. Counts are encoded video frames, **not
game FPS**, and this narrow detector is not a general rendering verifier.

Read-only map parsing finds3215 tags including
`levels\\a30\\devices\\beam emitter\\beam emitter` (mach), its transparent
beam shader, light/lens, beam effect and beam-smoke particle. This supports a
world beam-emitter hypothesis; exact live draw attribution/reference fidelity
is not yet proved. Community descriptions of the game's skyward beams provide
[corroborating context](https://www.halopedia.org/Beam_emitter), not renderer
acceptance. **Retire the HUD-replacement/failed-shield-draw hypothesis**: the
effect is camera-relative and survives verified original bitmap mode. Do not
remove it or change HUD sampling merely to hide it. The earlier label "cyan HUD
artifact" described screen proximity, not established origin.

Recordings stopped and debugger detached. Ordinary launch PID28067 restores
Original-quality picker (AX verified). All196 Documents files retained except
known log; Library changes only OS KnownSceneSessions state, preferences and PC
registry identical. No unit rerun for unchanged runtime. No hardware, IPA or
publication. Next compare a pinned upstream desktop material/effect view against
the Simulator before another shader change; neither overall texture fidelity
nor candidate73 promotion is accepted by this experiment.

## Build 73 scoreboard read-only observation (2026-10-02)

Private evidence: `ref/xbox-build/passes/2026-10-02/scoreboard-observe73.0c6lPL`.
Installed app remains `39f06f77…7198e`, build73/counting adaptation unchanged;
accepted lock remains66. No rebuild, install, source patch or hardware access.
Separate test save/data root and original app-data copy retained. Simulator only.

Source tracing identifies Back as guest binary button13, fade timer at verified
`game_engine_globals` offset0x14, and frame duration at verified `main_globals`
offset0x28. Private `observe.py` uses this exact guest ELF's symbols and reads
memory at host swap only; it is not a supported cross-version runtime ABI.
It never writes guest state or synthesizes input. LLDB must set
`platform.plugin.darwin.ignored-exceptions EXC_BAD_ACCESS` **before attachment**:
renderer write-watch faults are intentional. The first attempt paused networking
and the second initially intercepted a write-watch during loading. Both are
diagnostic perturbations, not accepted crash/performance evidence. Detach, set
the option before reattaching, and auto-continue resolves observation.

Actual CUA Scoreboard drag `[1315,274] -> [1315,680]` in local two-player Blood
Gulch, PID12988: observations963/964/965 show host Back then guest Back1,
suppression0, fade0.081401996 and scoreboard_open1, followed by Back0/fade0/open0.
Guest frame duration at the open sample is0.040701 seconds. Upstream converts
fade to alpha with `pow(fade,1.9)`, about0.0085 opacity here. Host Back is sampled
held once, then released; two Page Down press/release pairs are logged. The prior
video's sparse sampling is therefore not proof of a missing scoreboard. This
run establishes input delivery, advancing fade and passage through font/height
guards into the scoreboard-open path, **not final pixels or overflow paging**.
Debugger sample timing is not uninstrumented frame-rate/gesture-duration proof.
Keep held-control semantics unchanged; do not add a toggle merely for CUA.

Both helpers complete exit0. Delete breakpoint and detach debugger before final
ordinary launch PID14557; Original-quality picker visually verified. All196
Documents files retained, only HaloPad log changed. Library only OS
KnownSceneSessions state changed; preferences dictionary and PC registry exact.
No new unit rerun: runtime/source and binary unchanged. Next return to the
captured cyan shield-HUD artifact or a matched material comparison; sustained
scoreboard/overflow acceptance remains an explicit separate gate.

## Build 73 scoreboard touch bridge (2026-10-02)

Previous turn was progress: new candidate built and existing checkpoint upgrade
verified. This pass keeps build 73 `d1c7243c`, counted guest and ANGLE unchanged.
Private evidence: `ref/xbox-build/passes/2026-10-02/scoreboard73.ctwOYT/`.
Full app/data copied before replacement; no hardware, original-save selection,
network alias changes, upstream patch, accepted-pin promotion or IPA.

Source review confirms new full-screen scoreboard uses only wheel/Page Up/Down
for pagination. Add an optional shared-overlay roster-drag callback; Xbox enables
it on Scoreboard only. Keep the same held BACK action and layout; vertical drag
of 80 points queues one page, positive down. PC defaults remain hold-only, and
ordinary surface/Fire aiming is not redirected. Accessibility hint and Xbox
Controls guide describe the gesture. Use paired SDL Page Up/Down events rather
than wheel events, which upstream could consume as weapon switching outside the
scoreboard. Queue is bounded, preserves fractional displacement, cancels pending
pages on release/focus loss, and retains any required key-up after a delivered
key-down. No change to the guest's own scoreboard implementation.

Validation: 185 Xbox tests pass, including the actual paging helper under
ASan/UBSan. Native overlay suite passes 141 assertions plus its existing layout/
render checks (`docs/artifacts/2026-10-02/G9/overlay-20261002T141738Z`). New checks
cover held score/no aim, final displacement, cancellation, inactive input and
removing the optional adapter. The initial build found the event block inserted
in the wrong function; corrected to `xh_host_sdl_poll_event`. Initial two native
failures were a PC-default fixture that does not subscribe to engine lifecycle
notifications; use the engine-handler initializer for that test. Retain both
failed logs, not as reproduced pre-existing product regressions.

Full library and normal combined app build, strict codesign and in-place install
pass. Installed executable SHA-256:
`39f06f77934f518f29219d84456437f9dc39f78e4265f56099f49c9d7757198e`.
Known eight availability warnings and startup GL 0x502 remain. No new graphics
fix. Real local two-player Blood Gulch fixture uses one upstream stand-in machine
(no game simulation) at this Mac's existing address and a loopback host; online,
clipboard joins and UPnP disabled. No extra local addresses were configured, so
overflow rosters are not established. Both before/after runs need actual shared
Jump/A to start the 30-second lobby countdown despite the host test start log.
Before run reaches the rendered two-player postgame report when its helper exits.

After PID 86042: actual CUA drag from Scoreboard downwards publishes BACK then
four Page Down key-down/key-up pairs, then releases BACK (after-stderr lines
131-140). Camera and plasma weapon remain unchanged; subsequent Fire reduces
charge from 100 to 99 (`after-fire.png`). No additional page events follow.
Retained `scoreboard-drag.mp4` is 53.42s / 246606715 bytes; scene samples, later
4Hz frames and a 2Hz contact sheet show ordinary gameplay/idle animation, without
establishing the scoreboard's visible response. Do NOT claim end-to-end roster
paging or scoreboard display acceptance from input logs. This negative visual
result is a concrete next target: distinguish short-hold/fade timing, BACK
consumption, and the new guest scoreboard's font/layout/render path. Compare
against build 66 if needed; do not toggle held semantics solely for automation.

Final ordinary launch PID 5172 visibly returns to Original-quality picker.
Readback: all 196 Documents files retained; only known app log changed.
Library differences are SplashBoard snapshots and OS KnownSceneSessions state;
preference dictionary and PC registry unchanged. Both helper handles complete
exit 0, video recording stopped. Original inputs/saves and pre-pass app preserved.
This is an implemented/input-verified bridge with visual/overflow acceptance
open, not a resolved-controls or graphics claim. Cyan shield-HUD comparison also
remains pending; no matched test was performed in this pass.

## Build 73 candidate upgrade (2026-10-02)

Freeze official [build 73](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-73)
at `d1c7243cb20eab4488efa1266e259b1f4d5240f6`; do not chase later HEAD during
this pass. Accepted lock remains build 66 `f2ba71d9`. Private evidence and
rollback material: `ref/xbox-build/passes/2026-10-02/upstream73.NGS3eC/`.
Before replacement, preserve full installed `HaloPad-before.app`, full
`data-before/` and `out-before/` build output. No uninstall or hardware access.

66-to-73 review: high-resolution postgame title, desktop-only vsync-off FPS cap,
weapon-reticle-based enemy name range (maximum 70), smaller names, sanitized/
bannable player names and duplicate handling, plus a rewritten scoreboard.
Network protocol is 9 -> 10: do not assume cross-version multiplayer compatibility.
Android guest import/build machinery, `d3d8_gl.c`, `nv2a_psh.c` and saved-game
source have no diff. Existing renderer adaptation applies/restores cleanly;
nested private source is clean at 73.

Build with `XBOX_REV=d1c7243cb20eab4488efa1266e259b1f4d5240f6`,
`HALOPAD_XBOX_GUEST_ADAPTATION=render-visibility-v1`,
`HALOPAD_XBOX_RENDERER=angle-metal` and the existing pinned ANGLE source.
Guest translation reports 699510 instructions, 192 imports and 98 GLES imports.
Optional upstream Android compiler warning does not prevent translation/full
app build. Existing availability/empty-object warnings remain in logs. Package
the normal combined app, without a PC scene; strict codesign verification and
in-place Simulator install pass. Executable SHA-256:
`59764ae4139117e8567f61ae3eee12541af58b9152600ce962f5bf173468ac8c`;
guest SHA-256 `109a9789b518b4f11afe8025c84303d05e9fbf2029770ad47b90fd1c4b1bb4a3`.

`smoke/result.json`: menu 30s passes (lit .643); a10 60s passes the existing
two-nonblack-samples gate (.0775, .4783, .0); scripted local match 65s passes
(tick 1560, 13 shots, lit .98). Final a10 PPM is black; retained screenshot
shows Keyes on the bridge. Do not call this sustained nonblack campaign play.
Match screenshot shows Blood Gulch with the plasma weapon and shared overlay.
Known startup GL 0x502 remains; sampled frames 1/2/120 report zero.

Copy `normal-save66.vLJy5R/after-save-quit` into this pass's `checkpoint/save`.
Use isolated XG_DATA/XG_SAVE and touch-show, disable network options, and omit
init/scripted input/forced edition. PID 73519 follows picker -> Xbox -> Campaign
-> New001 -> Halo in progress -> Normal and loads outside the pod (60/120,
one grenade). Fire gives 59, Look turns, Pause and individually observed Move
steps reach Save and Quit. Wait for main menu and preserve `after-save-quit/`.
Its `z/savegame.bin` SHA-256 is
`90eb0888306203f2fec28f07bce8fa9cd027a5e0e33578f0be71d8d708d7e456`.
Cold PID 75752 follows the same normal menus and restores the checkpoint at
60/120 and one grenade (`cold-resumed.png`). This is bounded upgrade/save
evidence, not arbitrary-version compatibility or new progression.

`checkpoint-controls.png` captures a cyan vertical line extending above/below
the shield HUD. It is absent from the cold-resumed view. Origin is unresolved;
do not attribute it to build 73 without a matched build-66 comparison. Terrain
softness and overall fidelity remain open. No new graphics fix is claimed.

New source-backed touch gap: upstream `platform_scoreboard_scroll` consumes
mouse-wheel/Page Up/Page Down events. Our SDL event adapter emits touch-look
motion/controller-added events, not these pagination inputs; shared Scoreboard
maps to controller BACK. Overflow-page access has no current touch route.
This has not been runtime-reproduced with an overflowing roster. Next isolate
scoreboard display/overflow and design a small adapter-level touch path without
changing ordinary gameplay controls or patching every upstream release.

Do not promote yet: existing unadapted Mac/Simulator acceptance path has not
run for 73, scoreboard interaction needs acceptance, and graphics issues persist.
Keep accepted lock 66 separate from this installed preview. Private nested
source/output and installed app are intentionally candidate 73; normal prepare
without XBOX_REV returns to 66. Prior app/output copies remain available.
Ordinary launch PID 77214 returns to the Original-quality edition picker.
Readback: all 196 Documents files retained, only known app log changed;
Library differences only added/removed SplashBoard snapshots. Preferences and
PC registry identical. No runtime edits, new unit-suite run, hardware, IPA,
push or publication. Corrected stale top-level accepted-pin documentation and
linked the already established unlock-all explanation for profile summaries.

## Counted candidate normal-menu save/reload (2026-10-02)

Private evidence: `ref/xbox-build/passes/2026-10-02/normal-save66.vLJy5R/`.
No rebuild/install. Installed executable SHA-256 remains
`346533d7c066fa1146d94224f78eb59f692f9c3579143c3f7623fc1447df0f02`;
build-66 guest `27700a00...`, `render-visibility-v1`, ANGLE/Metal candidate.
Clone the complete Simulator data container before testing. Copy the older
`checkpoint64.sr7KEQ/after-save-quit` save tree into a new isolated session;
point only `XG_DATA`/`XG_SAVE` there, with maps linked read-only by convention.
No init script, forced edition, direct map command or injected guest input.
Network options disabled. Initial launch omitted touch-show and displayed no
overlay; restart with `XG_TOUCH_SHOW=1` makes shared touch controls available.

PID 62993: actual edition picker -> Xbox -> Campaign -> New001 -> Halo
(game in progress) -> Normal restores the outside-pod checkpoint, rifle 60,
120 reserve and one grenade, with nearby bodies and medkit. Shared Fire lowers
loaded ammo to 59; a background Look drag turns the view. Shared Pause opens
the guest menu. Individually observed Move-stick down gestures reach Save and
Quit; shared A selects it. Saving finishes and the main menu returns before
process termination. Preserve `resume-old-save.png`, `save-complete.png` and
the complete `after-save-quit/` tree. The 16,777,216-byte `z/savegame.bin`
changes from `ee33ca3d2b22261beb49cdca25972a88a5b662ee5178a97a0a5335631f6d5500`
to `050d594383ed1e8f718e0050c4ad75916f3fa68f2e9a2b62fddcaf72125fa6de`.

Cold PID 64436 follows the same picker and normal menu path. Halo still says
game in progress. First person restores the outside-pod checkpoint, original
camera, rifle 60/120 and one grenade (`cold-resumed.png`), not the unsaved
59-round/look state. This is last-checkpoint behavior, not save-anywhere.
The profile card still says The Maw/Legendary, as in the source fixture;
the earlier profile analysis explains upstream's deliberate unlock-all defaults.
This pass does not change that policy or prove fresh-profile creation.

This closes the bounded normal-menu existing-profile save/quit/cold-reload gate
for the current counted candidate. It does not prove a newly reached checkpoint,
all upgrades, rollback compatibility, sustained multi-touch, or graphics parity.
The prior fresh-profile/debug-save failure remains historical evidence, not a
reason to repeat this completed named-profile check. Next prioritize a matched
affected material/reference or another genuinely unmet control/update gate.

Ordinary cold launch PID 65857 returns to the Original picker. All 196 original
Documents files remain with only the app log changed; Library, preferences and
PC registry are identical. No runtime edit, tests rerun, pin promotion, hardware,
IPA or publication. Known initial GL 0x502 persists; sampled frames 1/2/120 zero.

## Water reflection consumer trace (2026-10-02)

Same installed candidate/pins as the mip pass; no rebuild. Private data clone,
trace and shader dumps: `ref/xbox-build/passes/2026-10-02/water-shader.km9ZLU/`.
Run b30 for 90 seconds with render diagnostics, `XG_CAPTURE_MIPS=1`,
`XG_DUMP_SHADERS=1` and `HALO_GPU_TRACE=120`. All requested gates pass.

Guest frame 120 contains a 198-index water reflection draw with vertex shader
24, texture modes `64621`, two combiners and final inputs `2d0f0b00/0c0c0000`.
Stage 0 is a 128x128 four-level render target, linear min/mip filtering,
LOD bias -0.6; stage 3 is a 64x64 five-level cubemap. Blend is destination-alpha
source plus one destination, RGB writes enabled, depth LEQUAL with writes on.
These states match the pinned water-reflection source path. This trace goes
beyond a merely bound texture, but does not directly map this 198-index draw to the
host program-62 snapshot or capture a shader-tag name.

Dumped `ps_2653fcd8.glsl` and `ps_39058af4.glsl` are byte-identical and match
that mode/combiner route. They sample the ripple normal, compute two intermediate
dot products and a third reflection-stage product, derive the eye vector from
the three interpolated w components, then sample the stage-3 cubemap. Generated
`vs024_0.glsl` writes all those coordinates. The reflection-vector expression
agrees with section 3.8.13.1.18 of the
[NVIDIA texture-shader specification](https://registry.khronos.org/OpenGL/extensions/NV/NV_texture_shader.txt),
apart from a small zero-denominator guard. This is not verification of Xbox
quantization/dot-mapping semantics, cubemap orientation, interpolated values,
final compositing or all compiled GPU output. No shader fix is justified yet.

The initial GL 0x502 remains; sampled frames 1, 2 and 120 report zero errors.
Normal cold launch returns to the Original-quality picker. Preservation audit:
196 original Documents files retained except known app log, preferences/PC
registry unchanged. No source change, unit-suite rerun, hardware, IPA or pin
promotion. Next compare this concrete material/cubemap with a matched reference
or capture its actual inputs/output if a visible defect is isolated. Do not
spend another pass re-proving mip existence or merely finding the same equation.

## Water mip-chain readback (2026-10-02)

Private evidence: `ref/xbox-build/passes/2026-10-02/water-mips.ixmd5T/`.
Extend the existing Simulator texture reader to named mip levels and add a
bounded observer for null-data 128x128 multi-level render targets bound at a
draw. Two snapshots of one object, at least 60 presented frames apart; no texture
or sampler writes. Check framebuffer/read-pack/active-unit/binding restoration
and retain prior/readback GL errors. Ordinary runs do not install these hooks.
Use `XG_CAPTURE_MIPS=1` with smoke `--render-diagnostics`; missing/incomplete
captures fail this requested diagnostic, not silently become a normal smoke pass.
The raw host setting is the private output directory. Captures stay ignored.

First approach observed only glGenerateMipmap: `live` ran b30 for 180 seconds,
but failed the mip gate because no snapshots fired. Preserve that result. The
guest can copy all rendered levels without generating a tail; missing observer
output was not proof of missing mip data. The final observer instead inspects
bound null-data mip chains immediately before draws. It records binding/program/
sampler state, not proof that every bound unit is used by the current shader.

Final `draw` b30 run passes 120 seconds. At frames 120/180, texture 87 on unit 0,
program 62, has base 0/max 3 and complete 128/64/32/16-square levels. All eight
readbacks have complete FBOs and zero errors; checked state restores exactly.
Changed RGB byte counts by level are 45758, 11900, 2907 and 535. The two level-0
previews show differing normal-map-like patterns, not blank/frozen contents.
Recorded sampler is trilinear minification, linear magnification and repeat S/T.
The 128-square/four-rendered-level pattern and pinned source route identify this
as the likely water ripple composite; no guest shader-tag identity is captured.
This rules out missing/frozen levels in these samples, not incorrect normal
orientation, shader use, blending, final appearance or a later-frame failure.
No rendering behavior fix is claimed. Next follow the consuming shader/material
or use matched reference evidence; do not repeat mip existence as fidelity proof.

184 Xbox Python tests pass, including malformed/missing capture rejection.
Full engine/combined app builds and strict signature verification pass. Generated
and installed executable SHA256:
`346533d7c066fa1146d94224f78eb59f692f9c3579143c3f7623fc1447df0f02`.
Guest and upstream pins unchanged; upstream checkout clean. App/data cloned and
read back before in-place installation. Observer-off menu smoke passes 30 seconds
even with parent `XG_CAPTURE_MIPS=1` (no render-diagnostics); no mip directory is
created. Preserve initial failed observer evidence and prior app for rollback.
No physical device, Xbox IPA, upstream promotion or publication.

Final ordinary launch returns to the Original-quality edition picker. Audit
checks all 196 original Documents files: only the known app log differs, none
added/removed. Preferences and PC registry are unchanged; reviewed Library
differences are Metal caches and OS snapshots. Diagnostic remains disabled.

## Beach material reproduction route (2026-10-02)

Extend the bounded smoke harness allowlist to `b30` (Silent Cartographer),
including init/load-gate and targeted-case validation tests. Keep installed
candidate, Original quality and pins unchanged. Private evidence and data clone:
`ref/xbox-build/passes/2026-10-02/beach-material.KCw0xS/`.

Actual `--case campaign --campaign-map b30 --seconds 180 --render-diagnostics`
passes with no scripted input: renderer/map gates true, three nonblack late
frames (lit fractions .958/.955/.956), presentation pair retained. Opening
washed-out view clears as the sequence progresses; do not diagnose it from that
single transient frame. First-person landing shows sand, foliage, combatants,
shield effects and moving dropship/exhaust. Background horizontal drags of
300, 180 and 200 screen pixels turn toward the ocean without intentional player
translation. `beach-arrival.png` and `shoreline.png` retain views. Water and
shoreline bands are visible; this does not prove correct ripple detail, blending
or temporal fidelity. The final attempted drag meets the harness deadline/Home,
not a crash. The run is confirmed terminal before ordinary relaunch.

Source review of the pinned engine identifies a distinct next diagnostic:
`source/rasterizer/xbox/rasterizer_xbox_water.c` builds animated ripple mip levels;
`port/linux/src/d3d8_gl.c:mip_composite_get` copies rendered levels into a sampled
composite and generates the remaining levels. This is separate from terrain
filtering. No missing mip or copy failure has been measured. Next capture the
actual water mip chain and sampler state in a stable shoreline view, or compare
that view with a matched reference, before proposing a renderer change. A normal
menu profile/save/reload remains a separate unfinished acceptance gate.

181 Xbox Python tests pass, including expanded b30 subcases. Cold launch returns
to Original-quality picker. Preservation audit checks 196 original Documents
files: only known app log changed, none added/removed; preferences and PC registry
unchanged. Library changes are four Metal cache files. No runtime rebuild/install,
hardware, upstream import, IPA, push or publication. No new graphics fix claimed.

## In-viewport light occlusion and night-scene follow-up (2026-10-02)

Previous turn is progress: counted guest integration is committed as `44556bc`.
Keep that exact installed candidate, Original quality and both upstream pins;
no app replacement or new source patch. New private evidence directory:
`ref/xbox-build/passes/2026-10-02/world-occlusion.KRW3Xt/`. Rediscover the data
container and clone it before isolated tests. No physical device use.

`live` runs a stationary Blood Gulch match for 360 seconds, passing through
tick 10410 with zero shots. Spawn is (41.102,-90.282,0.125), looking toward a
blue light through a base doorway. Background free-look moves that light behind
the foreground weapon and then clear of it, without translating the player.
Unlike the earlier sun-edge experiment, all relevant test rectangles are fully
inside 640x480:

| View | Logical rectangle | Depth | Raw count / area |
| --- | --- | ---: | ---: |
| Doorway light visible | (336,222)–(341,227) | 0.99297148 | 20 / 25 |
| Weapon-covered | (508,425)–(514,431) | 0.991847575 | 0 / 36 |
| Revealed left of weapon | (306,427)–(311,432) | 0.99239248 | 25 / 25 |
| Near silhouette | (369,427)–(375,432) | 0.992350757 | 30 / 30 |

Retain `light-visible.png`, `light-weapon-covered.png`, `light-revealed.png`,
`light-silhouette.png` and logs. A separate farther-depth light test stays zero;
the moving near weapon-associated rectangle remains distinct. Query IDs rotate.
This provides actual foreground-geometry occlusion evidence, not merely viewport
clipping, but does not establish a BSP-wall transition or original-renderer parity.
The small test covers the light core, not the complete glow: near the silhouette
the core can pass fully while the visible surrounding glow is mostly covered.
Do not convert sample ratios into claims about measured displayed brightness.

A short Move-stick drag did not change the logged player position. The supported
CUA drag does not provide a sustained hold; do not change normal stick release
semantics to make this automation appear to walk. Swap did not visibly change
the single starting plasma weapon. Neither observation proves a control defect.
Normal background free-look remained responsive throughout.

The first isolated `a50` campaign run passes 120 seconds, no scripted input;
debug log confirms the map load, retained late frames are nonblack, and manual
inspection sees the night-time transport cinematic followed by first-person
sniper, terrain, foliage and spotlights. This is a50, not the snow map. The
harness deadline stops the app before the intended scope interaction; an AX
target becomes stale and a later coordinate attempt sees Home. This is expected
test cleanup, not a game crash or a successful scope test. A longer independent
`a50-controls` run is used to finish that different check.

`a50-controls` passes 300 seconds and the shared Zoom button reaches ordinary
2x scope (`scope2.png`). Its deadline also precedes the Light interaction, so
continue with a manually owned run instead of another timed race. An initial
normal-menu launch using copied test saves reaches the fresh-profile name dialog:
the debug-start saves do not provide a named player profile. Retain the menu logs;
this is not successful normal-menu save/resume acceptance. Copy the test init
script into the isolated manual data directory and start a50 directly.

That manual campaign verifies shared Zoom cycling 2x -> 10x -> unscoped and
Light enabling green sniper night vision at 2x, retaining it at 10x, then
disabling it at 10x. Screenshots `manual/nightvision2.png`, `nightvision10.png`,
`nightvision-off10.png` and `unscoped.png` preserve the states. No scripted input
or query observer is enabled in this run. These are functional input/effects
checks, not a matched original-renderer comparison or sustained multi-touch
acceptance. Existing startup GL 0x502 remains; sampled later presentation rows
have complete framebuffers and zero reported GL errors, not all-frame proof.

Explicitly stop the manually owned app and cold-launch normally. The edition
picker returns with Original selected (`final-picker.png`). Readback verifies
all 196 original Documents files: only the known app log changes, none added or
removed. Library, full preferences and PC registry are unchanged. Keep the exact
installed candidate and private backups/evidence. This pass changes documentation
only; no new build, unit-suite rerun, pin promotion, hardware, IPA or publication.
Next isolate a different transparent/material or wall-occlusion case and obtain
a matched reference where practical. Do not repeat the completed sun-edge and
sniper button cycle as substitutes for the unresolved texture/shading report.

## Counted guest integration (2026-10-02)

The explicit `render-visibility-v1` guest now pairs with the isolated counted
ANGLE backend. It includes the existing resolution/filtering recipe and disables
the guest atomic-counter branch. After the normal availability check, a private
bridge token requests raw counts; the host intercepts it before GLES. Ordinary
GL_QUERY_RESULT remains boolean. The guest divides by the recorded target-scale
area before returning logical pixels, preserving the upstream desktop behavior.
Exact input/anchor checks and temporary source restoration remain enforced.

Build explicitly with `HALOPAD_XBOX_GUEST_ADAPTATION=render-visibility-v1`,
`HALOPAD_XBOX_RENDERER=angle-metal` and the pinned `XBOX_ANGLE_SOURCE`; then package
with the same first two settings. This candidate is Simulator-only; other SDKs
and Apple GLES are rejected before build. It has a separate `iphonesimulator-angle-counted`
library directory and records the backend's complete generated identity in
`build.json`. Packaging rejects mismatched guest/backend metadata or a counted
backend on an ordinary guest. Normal builds keep the backend option explicitly
OFF. Do not run different guest build workflows concurrently: the generated guest
output is shared. This is not a new upstream pin or default rendering policy.

Private evidence: `ref/xbox-build/passes/2026-10-02/counted-game.X2E6Wq/`.
The combined app builds, passes strict signing verification and installs in
place after an APFS app/data backup with readback. Generated and installed
executable SHA256: `a5a8c6449b6545ee06256aadfa1541f4867e029520a132c486245691061f807e`.
Guest SHA256: `27700a002ff4cc23069e5fa34c03cc171ef72190e797f7be72884e2139773dd7`.
181 Xbox Python tests pass, including compiled/sanitized actual normalization
fragment at 1x/2x, missing/duplicate anchors, bridge dispatch before GLES and
manifest rejection. 36 native Simulator save/launch/quality checks pass in
`generated/xbox-launch-tests/20261002T113800918597Z/`, including failed-backup
refusal and same-pin guest-change/rollback recovery. This is not proof of
cross-version save compatibility.

Actual isolated Blood Gulch `live1` run, Original quality, manual background
free-look without firing, correlates the far-depth sun rectangles below.
The observer now reports **raw sample counts for the private bridge**, not
booleans; interpret traces using the recorded build variant. Rectangle coordinates
remain logical pixels. Query IDs rotate and do not identify Halo flare tags.

| View | Logical rectangle | Area | Raw count at 1x |
| --- | --- | ---: | ---: |
| Inside | (127,310)–(178,361) | 2601 | 2601 |
| Narrow edge slice | (-51,230)–(7,288) | 3364 | 406 |
| Outside | (-75,215)–(-16,274) | 3481 | 0 |
| Return to partial | (-27,244)–(30,301) | 3249 | 1710 |

The edge counts exactly match 7*58 and 30*57 in-viewport pixels. The four saved
screenshots show the sun-associated reflections fade at the edge and return on
reversal. This removes the all-or-nothing coverage loss in this in-game route;
it is not a matched original-renderer comparison, a measured brightness ratio,
world-geometry occlusion acceptance or a fix for the broader texture complaint.
Temporal smoothing and angular factors still affect visible flare intensity.

Original run passes 300 seconds through tick 8490 with zero shots. Select
Sharper through the actual picker, not quality environment overrides, for
`live2`. Presentation reads 1280x960 and world filtering reports 4x. The inside
rectangle (128,306)–(178,357), logical area 2550, returns 10200 raw samples.
The narrow slice (-51,226)–(7,284), logical area 3364, returns 1624 raw samples:
exactly 4*7*58. Outside (-75,211)–(-16,270) returns zero. Retained screenshots
show the reflections fade rather than staying fully visible. Camera vertical
alignment differs slightly between runs; these are not pixel-identical A/B
frames. The trace is before guest normalization; the divided logical result is
established by the exact compiled adaptation-fragment test, not directly logged
from the running guest. Both runs retain the pre-existing startup frame-0
GL 0x502; sampled frames 1, 2 and 120 show zero. Do not call every frame error-free.

Sharper passes 240 seconds through tick 6780 with zero shots. Both harness runs
terminate their owned app/helper at the deadline; return to Home is expected,
not a crash. Original quality is restored through the picker. Post-run readback
checks all 196 original Documents files: only the known app log differs, none
added/removed. PC registry and full preference dictionary are unchanged. Library
differences are reviewed Metal caches, OS snapshots and saved scene state.
Keep the candidate installed for further Simulator checks; the prior app/data
backup remains in the private pass directory. No hardware, upstream pin change,
Xbox IPA, push or publication. Next validate world-geometry occlusion and another
material/effects scene; a viewport-edge fix is not general graphics acceptance.

Final observer-off isolated menu smoke passes 30 seconds, screenshot inspected;
no rectangle/query trace rows. Cold launch returns to the edition picker with
Original displayed. The post-menu preservation audit passes again. Current
tree/index private-path safety and whitespace checks pass; no public-artifact
or hardware acceptance is implied.

## Counted Metal backend candidate (2026-10-02)

The preceding rectangle pass established coverage loss. Implement the necessary
counting mechanism in a separate ANGLE build, not another geometry observer.
`HALOPAD_ANGLE_COUNTED_VISIBILITY=ON` generates three exact-input-checked files
inside the build directory. External ANGLE source stays untouched and clean.
Change Metal query begin/continue to Counting and replace the internal shader's
boolean OR reduction with 64-bit limb addition, saturating on overflow. This
retains counts across render-pass breaks and both old-result handling paths.
QueryMtl's ordinary result conversion stays unchanged: standard GLES callers
still receive GL_TRUE/GL_FALSE. The private render-thread bridge
`halopad_angle_query_samples` uses the normal query resolve/wait path and reads
its resolved buffer; invalid/active/non-occlusion queries fail. Counts wider than
GLuint saturate. This bridge is only compiled into the candidate library.

Build directory: `ref/xbox-build/out/angle-counted-simulator`. Configure with
the existing pinned `ANGLE_SOURCE_DIR`, iOS system, iphonesimulator SDK, arm64,
deployment target 17, Release, and the explicit counted option. Ordinary
`build-ios.sh` explicitly passes OFF, preventing an old CMake cache choice from
silently enabling the candidate. No app-packaging option or guest capability is
implemented yet. Output identity records exact input, generated output, recipe
and bridge hashes. Generated third-party material remains private/ignored.

`scripts/test-xbox-counted-visibility.py --device <dedicated UDID>
--angle-source <pinned source> --angle-build <candidate build>` compiles an
asset-free executable and runs it with `simctl spawn`. It creates a Metal-backed
EGL pbuffer and framebuffer targets, not a HaloPad installation or window, and
does not open game files/saves. Final evidence:
`generated/xbox-counted-tests/20261002T112532041905Z/`.

**56 actual GPU cases pass**, 28 each with `allowBufferReadWrite` forced on and
off; the bridge verifies the actual enabled state before tests. At sizes
64/128/512: empty/reused query, full target, repeated result read, half scissor,
fully depth-hidden, half depth-hidden, two render targets separated by glFlush,
and conservative query. Expected full counts are 4096/16384/262144; two-pass
counts 6144/24576/393216, exercising carry beyond 16 bits. Additional tests:
the measured viewport (-42,203,58,58) returns exactly **928**; the entirely
outside viewport returns zero; two pending queries read in reverse order retain
their independent counts. Every standard GLES result is also checked as boolean.
This verifies backend pixel counts and their 4x growth at doubled dimensions,
**not guest normalization**, MSAA, physical iPad behavior, performance or an
in-game flare fix. The 64-bit saturation branch is not exercised by huge GPU
workloads; do not imply exhaustive integer-range testing.

177 Xbox Python tests pass, including exact-input/duplicate-anchor rejection,
source preservation/output identity and the normal builder's explicit OFF.
Initial configure caught whitespace differences in the embedded shader anchor;
corrected against inspected source. Initial build caught relative includes in
copied DisplayMtl; added the original Metal include directory. First test link
needed CoreGraphics; corrected. Failures remain in private build/test logs.
No failed run is counted as passing evidence.

Next integrate the private capability with a separately identified guest
adaptation and package manifest. Do not send sample counts through ordinary
GL_QUERY_RESULT or promote the backend pin. Guest counts must be normalized by
actual render-target scale, then compared in the same partial-sun and world-depth
views, with original/preview rollback and save backup intact. Existing HaloPad
app and its saves were not replaced or opened by this fixture. No hardware,
Xbox IPA, upstream edits, push or publication.

## Sun visibility rectangle measured (2026-10-02)

Add CPU-only `XG_TRACE_QUERY_RECTS=1`, independent of aggregate query tracing.
It copies the four positions from the pinned guest's immediate-mode upload
(16 float4 attributes per vertex) while an ANY_SAMPLES_PASSED query is active.
Accept only one 1024-byte ARRAY_BUFFER upload and one four-vertex triangle fan,
finite axis-aligned rectangle, common depth and w=1. Other layouts, extra draws,
indexed draws and malformed rectangles are rejected. Correlate the later
existing query result by GL object ID; a 256-slot bounded table evicts collisions.
No GL calls, GPU waits, guest writes or changes to returned results. These are
observed call arguments, not an independent GL-success check. This layout is
specific to the reviewed guest; review before interpreting another upstream.

Emit at most once per ID per second and 4096 rows per process. IDs rotate between
guest visibility slots and are not stable flare identities. Repeated reads and
within-second transitions can be omitted; this is not an event-complete trace.
The Simulator harness forwards either query trace only with render diagnostics
and exact environment value `1`; normal smoke runs explicitly disable both.

Actual Original-quality Blood Gulch run, private `query-rect.ILuHKK/live`, retains
the following stationary views after manual background free-look:

| View | Observed rectangle, logical pixels | Area | Returned boolean |
| --- | --- | ---: | ---: |
| Sun inside | (173,289)–(222,338) | 2401 | 1 |
| Sun partly clipped | (-42,203)–(16,261) | 3364 | 1 |
| Sun outside | (-66,189)–(-7,248) | 3481 | 0 |

The far-depth rectangles (z about 0.99998–0.99999), moving with the visible sun,
are observed on rotating IDs 1/4. A separate near rectangle around (403,304)
remains distinct. This spatial/depth correlation identifies the sun-associated
test with much stronger evidence than aggregate counts, but no Halo flare-index
or tag identity is logged. `sun-inside.png`, `sun-partial.png`, `sun-outside.png`
retain the views. At partial coverage only 16*58=928 of 3364 logical rectangle
pixels can be inside the 640x480 viewport: **at most 27.6%**, before depth tests.
The ES fallback nevertheless maps result 1 to one million and the guest computes
a saturated 255 target. A counted result would permit a target no greater than
about 70 for that rectangle. Displayed flare intensity also has smoothing and
other factors; this is not a measured 3.6x brightness error or a claim that all
texture/shading issues share this cause. No reference-renderer comparison yet.

This is a reproduced coverage-loss limitation, not a fix. Next implement/test an
explicit counted-visibility capability in a separately identified backend/guest
candidate, preserving generic GLES boolean semantics, accumulating across Metal
render passes and normalizing scaled targets. Do not approximate world occlusion
by clipping the rectangle in CPU code; that would only address the screen edge.
No more aggregate-only diagnostics are needed to establish this particular gap.

174 Xbox tests pass, including sanitized actual observer opt-in, association,
collision eviction, malformed/extra/indexed draw rejection and hard log cap;
generator tests check hooks follow real calls. Full combined build and strict
installed signature pass, installed/generated executable hashes match
`69a3588606786889c46e0bfabf74caeb8339b774e850116c8a2473c40a8bbc0a`.
The unchanged guest remains `a2f07097…55d8599` (`render-quality-v1`, Original).
Upstream checkout is clean. Five-minute smoke reaches tick 8580 with zero scripted
shots and presentation captures. The harness ends before the attempted reverse
gesture, so no reverse-sweep query capture is claimed in this run. No hardware,
accepted pin move, Xbox IPA or publication.

Separate 30-second observer-off menu smoke passes; retained screenshot visibly
shows the ordinary Xbox main menu and its log has neither query trace prefix.
Relaunch the ordinary edition picker, verify Original selected. All 196 original
Documents files remain unchanged except app log; original saves and PC registry
are unchanged. Preference dictionary unchanged; only OS SplashBoard snapshots
change in Library. Pre-install app/data backups retained. No files deleted.

Final review tightens collision throttling: replacing a table occupant retains
the slot's last-log time, so ID churn cannot bypass the per-second limit. Add an
actual compiled collision assertion; all 174 tests pass again. Rebuilt, signed
and installed final executable is
`9df901c97225eb26b2972c2983f396ef269cbd965ddda17ee70f4797203501ee`.
The five-minute geometry evidence above belongs to the preceding executable;
this last change affects logging only, not geometry recognition or rendering.
Final-build 30-second observer-off menu smoke also passes, screenshot reviewed;
return to Original picker and repeat preservation audit successfully.

## Blood Gulch sun-flare reproduction route (2026-10-02)

Continue on the dedicated Simulator only, same installed `5a677b38…2d73201e`
combined app and Original graphics. No build, pin, shader or quality changes.
Fresh APFS data backup is verified before launch. Private evidence lives under
`ref/xbox-build/passes/2026-10-02/flare-view.3Y5L6x/`.

The stationary Blood Gulch host smoke passes 180 seconds, reaches tick 5010,
records zero scripted shots and retains its presentation capture. This uses a
local protocol stand-in, not a second playable client or online multiplayer
acceptance. A second isolated run permits manual camera input. With the Device
Hub window at 2200x1600 and game viewport approximately (583,234)–(2114,1382):

1. This manual run spawns outside a base facing its entrance, with a plasma
   pistol. Local position stays (29.604,-76.360,0.305), camera height about 0.9;
   starting aim is (-20,0) degrees in the network diagnostic.
2. Background free-look drag (1540,790) to (1540,640) reveals the sun and
   colored lens reflections. No Fire press is needed; battery remains 100.
3. Drag by (-50,+30), then (+80,0), then (+16,0). The sun approaches and is
   partially clipped by the left viewport edge; colored reflections remain.
4. Drag (+12,0): the central reflections disappear with the sun outside view.
   Reverse (-12,0): the partly clipped sun and reflections return.

These are reproduction coordinates for this exact window/layout and spawn, not
a portable gesture test. The preceding run spawned at a different base; do not
assume the smoke harness fixes position/yaw across launches. Native screenshots
retain visible/edge/outside/edge-return views.
This establishes a repeatable visible effect and reversible free-look input,
not a brightness curve, partial world-geometry occlusion or a matched original
Xbox/PC comparison. The smoke harness does not forward `XG_TRACE_QUERIES`, so
there is no query-to-flare attribution in this run. Do not characterize the
screen-edge transition alone as proof of incorrect rendering.

Source inspection of `_rasterizer_widget_submit_occlusion_test` shows its
denominator is the projected integer rectangle area, not clipped to viewport
bounds (coordinates are only bounded to signed-short range). This makes the
edge route useful for a future counted-versus-boolean comparison, but the
visible corona size is not necessarily the query rectangle. Next capture that
one test's geometry/area and result at matched edge positions, or a matched
reference, before changing backend visibility behavior. Do not repeat aggregate
query logging as a substitute. General texture/shading fidelity remains open.

Read-only fork refresh finds NicholasDominici/halo-ce-ios still at `3f2c1410`
and zimm3rmann/halo-ce-ios-macos still at `e55684aa`, the reviewed revisions.
No additional upstream patch is imported. No hardware, Xbox IPA or publication.

The manual run also passes its 300-second smoke gate, reaches tick 8580, records
zero scripted shots and retains presentation images. The harness terminates
its own helper and app in `finally`; explicitly relaunch the ordinary picker
with tracing off and verify Original graphics. Audit all 196 original Documents
files: only `HaloPad Logs/HaloPad.log` differs, none added/removed. Library changes
only saved scene state. Preferences and PC registry are byte/value unchanged.
Whitespace and tree/index safety checks pass. This documentation-only pass does
not claim a fresh unit-suite run, app rebuild or physical touch acceptance.

## Boolean visibility observation (2026-10-02)

Investigate the known visibility-count mismatch without changing shaders,
textures, depth, the accepted pin or quality choice. Add `XG_TRACE_QUERIES=1`:
generated guest wrappers observe the values already returned by
`glGetQueryObjectuiv` and query targets already passed to `glBeginQuery`.
The observer makes no GL calls, consumes no GL errors, does not add queries,
readbacks or GPU waits, and changes no results. Log at most once per second
when calls arrive, with a hard cap of 180 summaries. These counts represent API
reads, not distinct tests, pixels, frame rate or particular flare objects. The
last partial bucket is not flushed; do not claim totals for the entire run.

Actual Original-mode combined app on the dedicated Simulator, copied a30 save,
normal campaign menus and camera sweeps around the escape pod/outdoor view:
180 summaries span Unix times 1790935646–1790935968. Every reported last target
is `GL_ANY_SAMPLES_PASSED` (0x8c2f). Aggregate reads: 11,866 zero, 103 one, no
other values; six buckets contain both zero and one. 11,970 availability reads
are ready, none pending in this sample. The logger stops at its cap while the
app continues. The pass did **not** isolate a partially covered sun/flare or
obtain a matched reference rendering; it proves active boolean behavior, not
the cause or resolution of the user's broader shading/popping complaint.

Read-only source review establishes the boundary:

- Accepted `d3d8_gl.c` returns one million for any positive ES boolean result;
  the counted Android branch is inactive here (ES3.0, sample counting 0).
- `rasterizer_lights.c` divides visible pixels by test area, clamps its target
  visibility to 255, smooths positive changes and clears immediately on zero.
  Thus positive boolean results lose partial-coverage information. This does
  not mean the displayed flare brightness is instantaneously binary.
- ANGLE `ContextMtl.mm` uses `MTLVisibilityResultModeBoolean` at query begin and
  continuation; `QueryMtl::waitAndGetResult` returns GL_TRUE/GL_FALSE. Merely
  dividing the guest result by resolution cannot reconstruct pixel coverage.
- Other source visibility calls include the debug transparent-pixel counter;
  they are not a general texture-filtering or material-shading implementation.

Do not change generic GLES boolean semantics or guess a coverage fraction.
Next discriminating experiment: isolate one flare's test geometry/area and
partial occluder, then compare coverage and resulting brightness. A counted
backend path would need explicit guest/backend capability, cross-pass count
handling and scale normalization; it is not a one-line filtering fix. The
independent Android atomic-count scaling concern remains untested here.

172 Xbox tests pass. New compiled observer test checks unset/empty/invalid
environment values, aggregation/reset and hard cap with ASan/UBSan; generator
test checks observation occurs after the real call and reads the converted
guest pointer. Full combined app builds, strict signature and installed hash
match `5a677b38bfd865ea8713bfa94f2c4f919360ac57de45e3bb1301c5522d73201e`.
Guest stays the same quality-adapted build 66. Private evidence:
`ref/xbox-build/passes/2026-10-02/query-visibility.VodWdU/` (app/data backups,
logs, summary/analyzer, launch script, scene screenshot, tests and audits).
All original game/save/package files and PC registry remain byte-identical;
only app log, last-engine preference, OS snapshots/scene state differ. Restart
without query/presentation tracing after observation. No hardware, IPA, pin
promotion, push or publication; no graphics fix claimed.

## Player-facing Xbox graphics choice (2026-10-02)

Keep the Windows-left/Xbox-right cards and shared controls unchanged. Add one
44pt-minimum pre-launch control, shown only when the packaged guest advertises
the verified `render-quality-v1` adaptation. Original is the default, including
missing/malformed saved choices. Sharper (Preview) selects 2x render targets and
4x world filtering. The dialog discloses GPU cost, Xbox-only scope and unresolved
rendering issues. Cancel writes nothing; selection persists independently of
PC settings. Apply only immediately before Xbox starts. Nonempty development
quality overrides remain available for independent experiments; empty overrides
behave as unset. Unadapted and scale-only guests receive no quality settings.

Dedicated Simulator, full combined app, copied a30 saves, no `HALOPAD_CHOOSE`,
`HALO_TEST_RENDER_SCALE` or `HALO_TEST_ANISOTROPY` launch overrides:

- Default picker displays Original. Quality/Cancel returns unchanged.
- Xbox card and normal A-button menus reach copied a30. Actual source target
  and viewport are 640x480; runtime reports requested/effective filtering 1x.
- Choose Sharper, terminate and cold-launch. Picker retains Sharper (Preview).
  Xbox card/normal menus reach a30 at 1280x960, requested/effective filtering 4x.
- About/Done still returns to the picker. With Sharper retained, Windows card
  enters the PC license screen using separate copied state/game files. Do not
  accept the agreement or claim PC gameplay from that routing observation.
- Change back to Original through UI, terminate and cold-launch. Picker retains
  Original. Leave the tested preview installed with copied Xbox save overrides.

The 170-test Xbox suite passes. Actual UIKit/Foundation Simulator fixture adds
nine quality checks to the 26 launch/save checks: 35 pass with warnings treated
as errors. Evidence: `generated/xbox-launch-tests/20261002T094322520326Z/`.
Build-66 quality guest remains `a2f0709738df552d4ffa1b83b6c1f510d8f261a49b986dabcef731ae655d8599`;
upstream checkout is clean and accepted pin unchanged. Strict signature and
installed/generated executable hash match:
`92db1be79d12b04e27a734bce4498aacedebc75e1dd6cf3b9ec41e2a188e8a7e`.

Private evidence `ref/xbox-build/passes/2026-10-02/quality-picker.Y3ufr2/` retains
pre-install app/data, candidate app, launch recipe, Original/Sharper screenshots,
raw frame traces, final picker, PC license screenshot and audits. Of 196 original
Documents files, only HaloPad's log changes; game/package/save files and PC
registry are byte-identical. Preference audit permits only last-engine selection
and the new `HaloPadXboxGraphicsQuality=original`; all other values are unchanged.
Library differences otherwise consist of OS snapshots. No hardware, IPA, push
or publication. This is a usable preview choice, not a fix for remaining shading,
material or temporal popping. Next pass should isolate an effects/visibility
case or a different scene, not repeat the completed picker/AF comparisons.

## World filtering on the accepted guest (2026-10-02)

Recheck [Tyberious's upstream PR 35](https://github.com/cybersecurity/halo-ce-universal/pull/35):
still open at this pass, head `f9a4eb5`. Source review confirms that ordinary
game filtering remains 1x unless the game requests anisotropy. Retain that pin
boundary; do not merge the unreviewed config/UI patch or downgrade to its build-65
base. Add an original, small `render-quality-v1` fragment to HaloPad's existing
strictly checked build-66 adaptation. Credit PR35's filtering policy. Both source
anchors and full renderer SHA must match; source restoration and exact guest
identity remain mandatory. No upstream sources are added to tracked files.

`HALO_TEST_ANISOTROPY=4` or `16` affects only non-hires, non-point, mipmapped
samplers whose mip filter is enabled, only when the extension is supported.
Requests clamp to the queried GPU cap. A stronger explicit game request is
preserved. Other/unset values leave the original filtering. Resolution remains
independent (`HALO_TEST_RENDER_SCALE=2`). Compiled tests execute the exact inserted
fragment against inert GL boundaries for 13 cases: caps, unsupported extension,
HUD, point/non-mip exclusions, malformed input and explicit game AF. Manifest
tests reject treating quality as scale-only. Full Xbox suite: 170 tests pass.

Actual candidate guest `a2f0709738df552d4ffa1b83b6c1f510d8f261a49b986dabcef731ae655d8599`
is built on accepted `f2ba71d9`; upstream checkout is clean afterward. Full
combined app installed in place on the dedicated Simulator with pre-install
app/data backups. Three identical copied checkpoints enter a30 through normal
menus, without camera input before the comparison screenshots. All three runs
render at 1280x960; runtime reports GPU cap 16x and effective 1x, 4x and 16x
respectively. Ground texture detail increases at 4x/16x, although some sloped
areas stay soft. This is a visual comparison, not an animation-synchronized
pixel-equivalence test or evidence that Xbox must match PC artwork/materials.

At 16x: fire/drag turns the camera and rifle ammo changes 60 to 59; swap and
centered pistol scope work; grenade count changes 1 to 0 and pause responds.
No full explosion/flare fidelity claim from these sampled observations. Trace
has 15/16/38 sampled source readbacks respectively, all 1280x960 with complete
FBO and zero sampled prior/read errors. All three retain the pre-existing frame-0
`0x502`. No performance conclusion from Simulator timing. Both filtering and
resolution defaults remain unchanged pending broader effects/scene validation
and a simple player-facing quality choice.

Separate source finding, not a demonstrated cause of the reported shading:
ES3.0's Android fallback converts an any-sample query to all-visible, while
`rasterizer_lights.c` expects pixel coverage for gradual flare brightness.
The atomic-counter path also bypasses desktop scale normalization. These remain
independent visibility gates; anisotropy does not repair them.

Private evidence: `ref/xbox-build/passes/2026-10-02/world-filtering.a3Zc93/`
contains candidate app, 1x/4x/16x screenshots/raw frames/logs, launch recipe,
scope/pause screenshots, tests and preservation audits. Default guest rebuild
reproduces `a16a3271…8cc89`. Original 196 Documents files preserved (only log
changed); preferences and PC registry unchanged. Library differences are OS
snapshots and saved scene state. No hardware, IPA, push or publication.

After the full default-app rebuild/install, installed/generated executable
`eb1e2fb044c5b8de7c84838021678f5e70ca58d84b7e1da5d9914a5caaf1f66f`
matches and passes strict signing. Ordinary edition picker verified. Final audit
again preserves saves, registry and preferences; only log and OS snapshots differ
from the pre-pass backup (intermediate scene-state change no longer differs).

## Reproducible guest adaptation and exact save identity (2026-10-02)

Keep upstream build 66 (`f2ba71d9`) pinned. New `guest_adaptation.py` applies only
the six original render-scale lines during the private guest build, with a full
renderer SHA256 and unique-anchor precondition. It restores the source in a
`finally` block, refuses to overwrite concurrent edits, and records a separate
adaptation/recipe identity only after successful compilation. Unknown options,
changed inputs and unrequested packaging fail closed. Dirty checkouts remain
refused; an uncatchable kill requires inspection, not an automatic reset.
No upstream source or generated guest is added to tracked files.

Actual opt-in build from the accepted pin produces guest SHA256
`556cc14c7d2c6b50e9a51f0ae04a3120aeebe25552b08063a4358d609a9e48bc`,
exactly matching the earlier private `97b45239` experiment. Packaging without
the matching opt-in is rejected. The full app is signed and installed in place
on the dedicated Simulator. Using a copied checkpoint and normal campaign menus,
it reaches a30; traces show real 1280x960 source targets/viewport, complete FBOs
and zero errors in sampled presentation readbacks. Shared drag-fire turns the
camera and reduces rifle ammo 60 to 59; swap, centered pistol scope and pause
respond. Existing frame-0 `0x502`, soft sloped ground, and an ignored experimental
anisotropy setting in the isolated data config remain; this is reproducibility
and interaction evidence, not a new material/effects fidelity acceptance.

An unadapted rebuild produces accepted guest SHA256
`a16a327173b74cb7ed79bdff03aba7a022f08b0e35ad2b75adf3d465f2a8cc89` again.
The upstream checkout stays clean after both builds. Restored installed/generated
app executable `c54e09c6c939e73fc1dadfb829c7844cfe09f9934b5e897f40dc1359ccf5e608`
matches, passes strict signing, and opens the Windows-left/Xbox-right picker.
ANGLE still marks it PREVIEW; no accepted renderer/default-resolution change.

Save identity now includes the exact guest hash, not only the upstream commit.
Legacy markers trigger one backup; same guest does not repeat it. 167 Python
Xbox tests pass. The actual launch/save helpers pass 26 Simulator assertions on
synthetic folders and isolated preferences, including failed-copy refusal,
same-pin adapted guest, rollback and missing-identity refusal. This fixture also
needed SDL include paths, GameController linking and inert shared-overlay/input
boundaries after the earlier overlay integration; the failed fixture builds are
retained. Fixture evidence: `generated/xbox-launch-tests/20261002T085954462871Z/`.
Backups preserve recovery data, not snapshot compatibility.

Private pass: `ref/xbox-build/passes/2026-10-02/guest-adaptation.jrHaaE/`.
Original 196 Documents files preserved (only HaloPad log changed); Library
changes are OS snapshots only. PC registry and preferences are byte/value
unchanged. No hardware, IPA, push or publication. Next: visibility effects and
broader scene/material checks using this reproducible candidate.

## Scaled depth observation verified in-game (2026-10-02)

Extend only the opt-in Simulator depth observer and its parser, not normal
rendering. Read bounded full-viewport dimensions (up to 4096 per axis) and query
the sampled depth attachment's actual size through `textureSize` in ES3.0.
Reject mismatched extents before reading depth. Schema 2 records both viewport
and attachment sizes; calibration, GL/restore error, file-length, normalized
sample, target, position/projection and same-frame checks remain mandatory.
Legacy 640x480 records still work; scaled records without verified extents,
unknown schemas, invalid sizes and resized targets fail closed. Existing native
color and synthetic raster-replay tools remain 640x480-only; they are not silently
treated as scaled validators. 151 Xbox tests pass (22 depth/native-pixel tests).

Use private guest experiment `97b45239`, ANGLE and the full shared-control app
with `XG_CAPTURE_DEPTH=1`, exact reference VS17/41 shader bytes, minimum 1,000
indices, and isolated saves. The 2x Blood Gulch match yields four calibrated,
complete 1280x960 depth snapshots at presentation frame 11,614. Attachment size
is independently read as 1280x960. Base/EQUAL pair: 1,071 indices, matching
positions/projection, zero capture/restore errors. Of 1,228,800 samples, the base
draw changes 635,254; 116,388 of those are subsequently overwritten by closer
geometry; EQUAL changes zero depth samples, consistent with disabled writes.
The local stand-in client reaches gameplay; the log reaches tick 2,883 after
the bounded helper leaves. This is not second-human-client acceptance.

At 1x, a separate spawn yields four calibrated 640x480 snapshots at frame 7,257,
with a matching 1,398-index pair: base changes 76,393 of 307,200 samples,
27,698 later overwritten closer, EQUAL changes zero, no capture/restore errors.
Do not compare those counts as identical-scene resolution quality: spawn views
and selected draws differ. Both screenshots are retained; the 2x live match was
visually inspected. Both builds still have the previously observed initial
frame-0 GL error, so these are not whole-run error-free claims.

Setup failures are preserved: the first 2x peer joined after the automatic start
attempt, requiring a fresh live peer and host A to start; the first 1x peer
started before the host was ready and exited. The retry joined before host A;
countdown and capture then succeeded. No failed lobby run is counted as gameplay.

Evidence: `ref/xbox-build/passes/2026-10-02/scaled-depth.TyfGJB/`, including
`capture-1`, `capture-2`, `depth-*-result.json`, logs, screenshots, app/data
backups and the experimental app. Accepted guest/runtime rebuilt afterward;
the diagnostics remain inactive in ordinary launches. The original render-scale
experiment is still unpromoted. Next implement the reproducible local adaptation
with separate identity, then validate visibility effects and broader scene
behavior; the selected depth pairs do not certify all materials or hardware.

Final restoration: strict signature and generated/installed executable match
`f5709a50052929ddb72d4ef71a629435ce7ed7102c3c4ddbc916c68eaf24a126`;
About shows accepted `f2ba71d9` and Done returns to the ordinary picker. All
196 real Documents files retained, only application log changed. PC registry
and preferences unchanged. Library differences are Metal caches, OS snapshots
and saved scene-session state. Original inputs/saves remain intact. No hardware,
IPA, publication or push; tree/index safety and whitespace checks pass.

## Actual 2x render-target experiment (2026-10-02)

Source inspection at accepted build 66 identifies `screen_mode_choose` in
upstream `port/linux/src/d3d8_gl.c`: Android fixes both scale factors at 1.
Desktop already scales screen-sized color/depth targets, viewport/clear
coordinates and presentation independently of the logical 480-line layout.
Changing only the host drawable resolution cannot enable this guest path.

Private experimental revision `97b45239583b33fbc700b9614288209b13169d4a`
adds a strict opt-in `HALO_TEST_RENDER_SCALE=2` in the Android branch. No
anisotropy change is included. It is retained only in the ignored upstream
checkout on local branch `halopad-private-render-scale-20261002`; the accepted
lock is unchanged. Build with `XBOX_REV` using the ordinary candidate workflow,
not by pretending the modified guest is the accepted pin. Full experimental
app executable SHA-256 is
`7eca603ff8a998e7fad7719918f65dc7585f726d7fb61abea2715332cca3a778`.

Use identical copies of the untouched a30 checkpoint, normal menus and no
camera input for each baseline view. At 1x, presentation source and viewport
are 640x480. At 2x, both are 1280x960, while the drawable stays 1376x1032.
The source framebuffer is complete (`0x8cd5`), with zero prior/read errors in
the presentation samples. Both runs report the same initial frame-0 `0x502`;
do not describe either run as wholly GL-error-free. The 2x run retains 27
presentation-source log samples. Screenshots show sharper geometry edges,
weapon/HUD detail and finer foliage; sloped ground textures remain soft.
This proves actual higher-resolution rendering, not a general material fix.

Matching pistol scopes at 1x/2x retain the same centered circle and HUD layout.
At 2x, swap, scope exit, fire-and-drag camera movement and pause still respond;
the pause menu remains aligned. The captures are not synchronized animation
frames, so weapon idle motion and animated textures must not be scored as
resolution differences. No full depth correctness, sustained performance,
campaign progression or match acceptance is claimed from this comparison.

Important remaining source issue: Android atomic visibility results bypass
`query_area` normalization; the desktop correction is compiled out. The tested
ANGLE ES3.0 path has sample counting disabled and substitutes a large count for
any visible sample, so this run does not test the atomic-counter issue. Existing
depth/draw replay diagnostics also assume 640x480 and must not be used to certify
2x depth unchanged. Extend those diagnostics before claiming depth acceptance.

Evidence and rollback are private under
`ref/xbox-build/passes/2026-10-02/render-scale.tXrS4o/`: `scale-1.png`,
`scale-2.png`, both `scale-*-scope.png`, raw presentation PPMs, logs, independent
save copies, app/data/output backups, and `HaloPad-scale-experiment.app`.
Rebuild the accepted guest/runtime and full combined app after the experiment;
restore the accepted in-place app, not user data. No upstream push or IPA.

Restoration verified: clean upstream checkout at `f2ba71d9`, accepted guest
image byte-identical to the pre-experiment backup (`a16a3271…c89`), matching
build manifest, strict app signature and installed/generated executable
`2d563e964011f7f20ae8d982de45ff4fd448a89f0c1822045e2e2c161734cd8d`.
About shows `f2ba71d9`; Done returns to the ordinary picker. Independent audit
finds 196 Documents files, no additions/removals, only application log changed;
PC registry and preference values unchanged, OS snapshots alone differ in Library.

Next implementation gate: a small versioned local guest adaptation, with its
identity recorded separately from the upstream pin, strict source preconditions,
and an unchanged-source/default fallback. Validate 1x/2x target/depth readback,
effects and a bounded match before choosing a default. Keep touch UI wholly in
HaloPad; updating upstream must not overwrite it. This experiment is not yet a
user-facing resolution preference or an accepted renderer upgrade.

## Xbox menu labels on the shared overlay (2026-10-02)

Add small A/B/X/Y badges to the existing Jump/Melee/Use+Reload/Swap buttons,
with accessibility hints. Original gameplay captions, actions and hit targets
stay unchanged. PC buttons have no badges. Badges remain visible with captions
off. An Xbox Controls guide explains MOVE, A/B navigation, drag aim/fire and the
Default profile requirement. Alternate guest profiles are disclosed, not adapted;
this does not pretend to detect menu state or change guest bindings.

134 native assertions pass, including badge containment and original-button hit
testing over 270 phone/tablet/handedness/size/gap combinations. Evidence:
`docs/artifacts/2026-10-02/G9/overlay-20261002T072747Z/`, including the rendered
`xbox-labels.png`. All 145 Xbox Python tests pass. Full combined app builds and
strict signature verification passes. Installed executable SHA-256:
`077ca7c9285c622b92a823939feb1230dd57288b21110ab1c9d2fb2939f57c52`.
Guest remains build 66, renderer remains ANGLE preview.

Actual dedicated Simulator check: ordinary picker -> Xbox; guide opens with all
text visible and Done restores controls. A enters Campaign/Select Profile; B
returns to main menu. Copied New001 -> Halo -> Normal restores the outdoor a30
checkpoint. Fire-and-drag turns and changes rifle 60 to 59; Y swaps to pistol,
Zoom enters/exits 2x; Pause and MOVE navigate to Save and Quit, A accepts, and
the game returns to main menu. A cold ordinary launch restores the edition picker.
Private captures, logs, copied save, prior app/data and new full app are retained
under `ref/xbox-build/passes/2026-10-02/menu-labels.qGJNj7/`.

Independent audit: all 196 Documents files retained, only application log changed;
preferences and PC registry unchanged. Library differences are OS snapshots only.
No physical device, EULA acceptance, IPA, publication or upstream modification.
Texture softness remains visible; this pass changes no rendering behavior.
Sustained human multi-touch, non-default profiles and graphics fidelity remain open.

## Build-66 repeatability and campaign capture (2026-10-02)

Candidate `f2ba71d9af4c6fc65d7419cc22e8f4899b16da88` first passes all six
Mac/Simulator cases (`20261002-121805-f2ba71d9`). The acceptance rerun
`20261002-124403-f2ba71d9` passes Mac menu/campaign/match (2,106 ticks, 16 shots)
and Simulator menu/match (1,560 ticks, 13 shots), but rejects campaign because
the last periodic drawable PPM is entirely black. The pin correctly stays at
build 64. Do not discard or relabel that failed result.

Inspection shows `campaign/frame.ppm` at 12:51:50 and the independent
`campaign/screen.png` at 12:51:59 showing the rendered a10 cinematic. This is
consistent with capturing a cinematic transition, not proof of a persistent
black-screen failure. The original check used only the last ten-second dump.
The updated campaign check retains complete, distinct drawable samples from
the last 30 seconds, including black samples, and requires at least two visible
frames. It rejects stale/incomplete/concurrently changing PPMs. Map-load,
renderer and signal checks remain; menu/match criteria are unchanged. Regression
tests include all-black, one-visible, repeated/stale, malformed and incomplete
samples. This tests bounded rendering, not full progression or material fidelity.

Fresh acceptance run `20261002-125636-f2ba71d9` passes all six cases and moves
the lock to build 66. Mac match: 2,100 ticks/17 shots. Simulator menu 30 seconds,
campaign 60 seconds with three complete late frames (lit fractions 0.067, 0.401,
0.177), match 65 seconds/1,565 ticks/13 shots. Campaign/match screenshots visually
reviewed. 145 Xbox tests and current-tree safety check pass. Save backup is
`ref/xbox-build/save-backups/20261002-125636-from-c55e4e2b.AqAskh/`, independently
compared and checksummed by the gate.

Rebuild the full combined app without the gate's PC scene fixture, install in
place, and verify strict signature plus generated/installed executable SHA-256
`e47cd8033c08fa20ae0a20aeeef36bcd3b5e3d5a0bc25a04d88cc463edd4fb31`.
An additional ordinary rebuild after acceptance produces the same executable.
About/Done shows `f2ba71d9`; ANGLE correctly retains the preview designation.
Normal picker -> Xbox -> Campaign -> copied New001 -> Halo -> Normal restores
the outdoor a30 checkpoint. Fire-and-drag turns left and changes rifle 60 to 59;
Swap equips pistol, Zoom enters/exits 2x, Pause opens the game menu. Move stick
navigates to Save and Quit; Jump accepts and the game returns to its main menu.
Terminate and cold-launch the ordinary edition picker. PC gameplay is not
newly tested, and no EULA is accepted. Full app and checkpoint screenshot retained
as `HaloPad-build66-full.app` / `build66-checkpoint.png` in `shared-controls.cwPagg`.
The brief initial load view has incomplete HUD/material state before settling;
that transient is not proof that ongoing texture/shading issues are resolved.

Final independent readback (`audit-final.log`) again finds 196 Documents files,
only the application log changed, no added/removed files. Preferences and PC
registry are unchanged; Library differences are OS snapshot replacements only.
Original game/package/save inputs remain intact. No hardware, IPA, EULA, push
or publication. The ordinary picker is left open on the dedicated Simulator.

## Filtering comparison (2026-10-02)

An isolated experiment builds upstream [PR 35](https://github.com/cybersecurity/halo-ce-universal/pull/35)
at `f9a4eb5763cbde769f8396c0a58b884f88251382` (based on build 65). It does
not change the accepted engine lock or import a patch into HaloPad's source.
The patch applies configurable anisotropy to linear, mipmapped world samplers;
the original path only enables it when the game explicitly requests anisotropic
filtering. Keep this separate from both the release update and resolution work.

Same full combined app, ANGLE revision, copied a30 checkpoint, initial camera,
and 640x480 internal rendering; separate identical starting save copies. Launch
with `HALO_ANISOTROPIC_FILTERING=1` then `16`. Logs confirm actual 1x/16x with
GPU maximum 16x. Navigate normal campaign menus after a diagnostic Xbox-choice
override; no move/look input before captures. Private evidence in
`shared-controls.cwPagg`: `af-1.png`, `af-16.png`, `af-{1,16}.stderr`, and preserved
`HaloPad-af-pr35.app`. Test configs/saves stay outside the real Documents tree.

The 16x frame has more visible ground detail, especially on the right slope,
but remains substantially soft. Camera/geometry match; live scene animation
and HUD blink timing do not, so this is a visual comparison, not pixel-exact
determinism or performance measurement. No conclusion about shading/popping,
all material types or physical iPad performance. The experimental app is not
the accepted release candidate. Next resolution experiment must scale actual
color/depth targets and viewport, not merely upscale the final image; test HUD,
scope, particles and depth behavior together. Android's `screen_mode_choose`
currently forces both scale factors to 1; the desktop path already scales them.

## Shared PC and Xbox overlay (2026-10-02)

Latest direction supersedes hardware work: dedicated Simulator only. The new
phase-4 goal loop separates wrapper-owned controls, renderer investigations and
upstream candidate promotion. Both old overlays were HaloPad code; upstream's
Android README currently says touchscreen input is not implemented.

The combined Xbox controller now instantiates PC's `HPOverlay` with an input
handler. Default PC routing is unchanged. The small `xg_overlay_input.h` maps
semantic actions into the default Xbox pad layout, retains Use/Reload ownership
when both alias X, and clears on cancellation. Relative look travels through
the upstream SDL mouse-motion path instead of the old short-lived right-stick
velocity pulse. Both editions share size, opacity, handedness, layout and look
settings. Xbox gets System Link plus Controls in the ellipsis menu, not PC-only
chat/join/display commands. The standalone Xbox test app retains its older overlay.

Evidence: private `ref/xbox-build/passes/2026-10-02/shared-controls.cwPagg/`.
Full old app/data preserved by APFS clones; independent `diff -qr` exit 0.
Library and combined app build successfully. Initial test syntax and shell BOOL
block compile errors were corrected; failed logs retained. Native overlay suite:
128 passing assertions (`docs/artifacts/2026-10-02/G9/overlay-20261002T025234Z/`).
Xbox Python suite: 143 tests pass. Actual executable SHA-256:
`e98ef1f9c860b69fc4b64892f596aad4b9f1eec6e98c389499206cf13a613f54`.

Normal chooser -> Xbox -> Campaign -> copied New001 -> Halo -> Normal restores
the outdoor checkpoint. An open-screen drag visibly turns right; dragging Fire
turns left while ammunition changes 60 to 59. Move-stick forward changes position,
Swap equips the pistol, Zoom enters/exits its 2x scope, Pause opens the game menu.
The native Controls submenu and touch settings open and Done closes them. Move
stick selects Save and Quit, Jump accepts, and a cold process launch through the
same normal menus reloads the checkpoint (rifle 60, original checkpoint camera).
This is checkpoint reload, not persistence of the post-checkpoint shot/camera.
Native screenshots and bounded `shared-control-gameplay.mp4` retained; no forced
map, scripted bot or original save was used in this sequence.

After termination, independent hashing reports 196 Documents files: only
`HaloPad Logs/HaloPad.log` changed, no additions/deletions. Library changes are
OS snapshot replacements only; preference dictionary and PC registry unchanged.
The real game inputs/saves remain protected. Only isolated copied saves/config
were opened. No hardware, EULA acceptance, IPA, push or publication.

Limits: menu labels still say Jump/Melee while the game asks for A/B; controls
currently assume the default Xbox profile binding. Full sustained multi-touch,
controller feel and physical performance are untested. The shared handler tests
cover independent inputs but are not human multi-finger acceptance. PC gameplay
is not newly verified. Render log remains 640x480; visibly soft ground and texture
fidelity are not fixed by this input work.

### Source-review leads for the next renderer/update pass

- Upstream [build 66](https://github.com/cybersecurity/halo-ce-universal/releases/tag/build-66),
  `f2ba71d9af4c6fc65d7419cc22e8f4899b16da88`, is latest at recheck. Relative to
  accepted build 64 it adds high-resolution text/titles and multiplayer player
  names. Renderer/cache/font/embedder changes require candidate testing; release
  notes do not establish a world-texture fix.
- [NicholasDominici's iOS port](https://github.com/NicholasDominici/halo-ce-ios/blob/3f2c14101d3ae1c7f0c0a11993a43407fadbeb46/port/ios/README.md)
  separates logical Xbox 480-line UI from native-resolution color/depth targets;
  commit `ff9d86bf65730b780b65fb4b4d3a7d87044623ed` adds render-height controls.
  Our Android guest path hard-codes screen scale 1, although its desktop path
  already has scaled target/viewport support. Test resolution independently,
  with matching depth targets and scope/UI checks; do not stretch only the blit.
- [zimm3rmann's Apple port](https://github.com/zimm3rmann/halo-ce-ios-macos/blob/e55684aac9a06d25bcc60f8c7ea63ff9d59687a1/port/ios/README.md)
  uses ANGLE/Metal. Commit `678ea52913770b12cd6f66b895ac143d7d6cedad` expands
  misaligned vertex attributes into draw-local float4 uploads (Metal alignment
  and conversion-cost issue). `e55684a` separates scratch occlusion query from
  game query zero. These are source leads, not reproduced fixes for this guest.

No external port code copied in this pass. Keep accepted guest, ANGLE and shell
pins independent; first preserve this shared-control checkpoint, then candidate
test build 66 using the established update gate. No blind tracking of main.

The later authorized physical pass is recorded at
[Physical iPad ANGLE preview](#physical-ipad-angle-preview-2026-10-02).
The scope below describes the earlier Simulator-only work.

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

## Upstream profile summary and Xbox touch focus loss (2026-10-02)

**Classification: progress, not goal completion.** Build 64 remains frozen at
`c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9`; this pass's official upstream HEAD
readback still matches. No pin update or change to the physical Windows renderer.
Private evidence: `ref/xbox-build/passes/2026-10-02/profile-lifecycle.BXKtCE/`.

### Resolve the fresh-profile summary without changing saves

The preceding Files-import New001 profile is 512 bytes, SHA-256
`f41861958cf6f6b7032ca756a43b8a936502e9d64738c9b40ac642b8c2340017`.
All ten level flags at bytes 28–37 are `0x0f`, with last-played level zero.
Frozen upstream `saved games/player_profile.c`, `player_profile_new` at line
397, explicitly logs and unlocks every difficulty of every solo level around
427. The actual creation log contains that message. Its highest-completed
routine at 345 chooses level 9/difficulty 3; the interface summary around
`ui_widget_game_data_input_functions.c:3718` caps highest-plus-one at 9.
This explains **The Maw / Legendary**. It is intentional development behavior,
not evidence that the player completed the campaign or a random save mismatch.
Retain `profile-analysis.json` and the full `fresh-profile-save-before` copy;
do not zero flags or rewrite existing profiles. The earlier `checkpoint64.sr7KEQ`
pass already proves a bounded newly reached outside-pod checkpoint; do not
relabel that gate as missing or repeatedly test it as new progress.

### Held-handler failure and narrow reset

Xbox UIKit input previously clears on cancellation/controller hiding, without
explicit app-focus handling. Add an asset-free executable using actual
`XGTouchPad` handlers and the shared `xg_touch_input.h` buffer through a mock host
boundary. Inert NSObject touch tokens exercise move/look/both triggers/A and
real UIButton Start/Back handlers; no fabricated UITouch instances, window,
game data or physical events. First fixture tries `sendActionsForControlEvents`
without UIApplication, so baseline-held and short-Start checks also fail; this
does not establish a game defect. Correct the fixture to call the actual private
handlers on real buttons. Corrected baseline passes those controls but fails
six lifecycle checks: live/unread clearing, tick replay, inactive refusal, late
callbacks, neutral activation and old-touch ownership.

`xg_touch.m` now observes UIApplication will-resign-active/did-become-active.
Both clear ownership, visual/model state and current/unread native input;
inactive/hidden pads refuse new presses/publication. Resume requires fresh
touches. Existing active short-tap latching remains. `xg_ios.m` adds only an
opt-in trace of clearing, not different buffer semantics. New
`scripts/test-xbox-touch.py --device <UDID>` compiles strict-warning UIKit code
and spawns it on an explicitly named booted Simulator, with no install/game
interaction. All **16 checks pass**, including normal cancellation, controller
hide/removal and exactly-one-poll Start. Handler evidence under
`generated/xbox-touch-tests/`: initial flawed `20261001T210942Z`, corrected
six-failure baseline `20261001T211115Z`, fixed `20261001T211242Z`, final rerun
`20261001T212518Z`. UTC directories fall on October 1; the local pass is October 2.
These test simultaneous state at the handler boundary, not real OS multi-touch
routing or physical-controller interruption.

### Actual integrated Home/resume and preservation

Back up outgoing app/library, full Simulator Documents/Library and source
profile. Rebuild frozen Xbox library and normal PC-entry app with explicit
ANGLE/Metal PREVIEW; no development scene. Library SHA-256
`bbd2a75eca70bebc92c6b7813d0b1b2bf9fd55259ed79f63d9b172d1b44696db`;
guest unchanged `102885c274aa95771be8672a65ba16fa88a52d9f4d3889310072e214aabfdaaa`.
Installed executable SHA-256
`58111bcbbd3da23bd47282a11becf3b7c1887e9d6e37c553e884eb1bec29f093`.
In-place install only on `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, iPadOS 26.5.

Use `play-data/maps` linked to trusted private maps and `play-save` cloned from
the previous outside-pod checkpoint, not real installation saves. Clear inherited
bot/network diagnostics and online/UPnP/clipboard join; enable touch SHOW/TRACE.
Actual picker → Xbox → Campaign → New001 → Halo in progress → Normal loads the
outdoor checkpoint. RT decreases rifle 60→58. Device Hub Home logs inactive and
clear at 06:20:55; ordinary foreground launch (without termination) returns the
same **PID 91997**, with active and clear at 06:21:10. Resumed image retains rifle
58; fresh RT lowers it to 57. Y switches pistol, Zoom toggles circular 2x,
finite look changes viewpoint and move produces a short step. Native poll logs
show fresh look/move/trigger axes and their return to zero. Start opens pause;
three separately observed down gestures choose Save and Quit; saving finishes
and the main menu returns. Preserve before-Home and after-Save-Quit images/logs.
This is a normal Home cycle with inputs released beforehand, **not held touch
during OS interruption**, sustained two-thumb play, audio interruption quality,
background-render permission or temporal fidelity acceptance.

The first attempted ordinary launch sets empty `XG_DATA`/`XG_SAVE`, and the card
says Add Disc: getenv still treats those as development overrides. Terminate
that test and **unset** both variables; Play Xbox returns without an import.
Normal PC validates existing files and reopens the original unaccepted EULA;
do not click Accept or fake product identity. Final ordinary app stays at both
edition cards with no Xbox overrides, PID 95450. Real Xbox runtime never opens.

Independent full-file SHA/size audit: Documents adds/removes no files; only
`HaloPad Logs/HaloPad.log` changes. All 114 real Xbox files, PC installation,
package and both disc/fixture files are unchanged. Preference dictionaries match
the backup, PC registry bytes unchanged. Library changes Metal caches, system
SplashBoard snapshots and scene-session metadata; preserve backups rather than
claiming Library unchanged. 128 Xbox tests, five input guards and 13 executed
package checks pass; 16 package checks are skipped. Native handler tests are an
additional 16 passes, not part of Python's 128. Whitespace and repository
tree/index safety pass. No physical operation, Xbox IPA, push, publication or
cleanup. Next pursue affected moving-scene/reference rendering and physical
shading/focus under a coordinated device window; this pass is no visual fix.

## PC live ground-material arithmetic (2026-10-02)

**Classification: progress, not rendering fix or goal completion.** No tracked
runtime/shader/filter changes, upstream pin update or integrated app installation.
Physical iPad shading/focus remains open, affected edition/map unconfirmed.
Private fixture, analysis, controls and derived previews:
`ref/xbox-build/passes/2026-10-02/pc-live-color.L5KDQS/`.

### Probe and failed attempts

The private C wrapper includes the unchanged existing PC host gameplay harness
and uses the nullable native draw hook. Match original 160-byte ps_1_1 shader
SHA-256 `026e77794dbcaa220c78bcec62eccdfa7d93fda8968198246d48d880beccc433`
and all four trusted level-zero texture payloads exactly. Capture frames
3150/3155/3160 only. Borrow the actual geometry, vertex shader, constants,
buffer offsets, indexed draw and samplers; replay into independent RGBA32-float
targets without depth/blending. Nine return-only shader variants capture four
samples, four interpolated coordinates and fog; the tenth runs the unmodified
pixel program. Require identical positive finite coverage and verify original
color-target bytes are unchanged after each probe. This is an isolated component,
not normal PE entry or PC license acceptance. Readbacks/compilation disrupt frame
timing; no performance or audio-deadline claim.

Initial compile fails because a fixture array collides with the harness's
`frames` variable; rename only that array. Remove unused HALOPAD_TEST_MAP, which
the harness never reads: its LAN menus already choose Battle Creek. First linked
run `G3/core-arm64-apple-ios17.0-simulator-20261001T215025Z` passes host checks
and captures 30 motion frames, but spawns indoors and has no matching material.
Whole-run exit 1/captured zero is retained, not an arithmetic verdict. A second
bounded run keeps identities/frame guards and succeeds outdoors; no relaxed
capture or gameplay assertions.

### Successful comparison and limits

`G3/core-arm64-apple-ios17.0-simulator-20261001T215644Z` exits zero, 24 assertions,
30 consecutive presented motion frames; diagnostic executable SHA-256
`3f3da29f696bc841e39d98919ee44498a947d98c5d3bb649b544f66d928c758c`.
Frame 3150/request 19 covers 436,060 pixels; 3155/3160 cover 480,000 each.
The independent interpreter in `scripts/shader-diff.py` executes original
bytecode against captured fragment coordinates/constants, substituting only
the measured four texture samples. All **1,396,060 covered pixel observations**
have exact RGBA agreement (max/p99 absolute residual zero). Three private
analyzer tests pass, including known mixed-alpha/fog calibration, deliberate
0.05 color-error detection and five rejected malformed/missing/identity cases.
Synthetic mixed-alpha/fog calibration is not live coverage: the actual selected
base texture's alpha is one and fog is disabled in all three draws.

Reviewed `second-capture/material-contact.png` shows the grain in the original
target, independent material replay and stage-one detail sample. It remains
unfixed. This weakens arithmetic mistranslation for this particular material.
GPU samples/interpolation are observed inputs, not independent verification of
filtering, UV/LOD correctness or texture identity versus an original driver.
Return variants may optimize differently. Depth/blend were intentionally omitted;
coverage is not visibility or final-frame equivalence. One camera/three samples
are not temporal fidelity, all-shader coverage or the physical complaint.

Only the dedicated Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB` is used.
Ordinary integrated app resumes at the same PID 95450; installed executable
`58111bcbbd3da23bd47282a11becf3b7c1887e9d6e37c553e884eb1bec29f093` is unchanged.
Full Documents audit adds/removes no files and changes only the ordinary log;
all 114 real Xbox files and PC installation/package/disc bytes survive exactly.
Preferences match and PC registry bytes are unchanged. Metal cache/system
snapshot changes are retained, not treated as game-data loss or Library identity.
No physical operation, EULA acceptance, IPA, push, publication or cleanup.
Next matched original-driver/affected scene comparison, including sampling and
geometry/depth discrimination; do not add a speculative visual workaround.

## Empty launch overrides and revision-backup regression (2026-10-02)

**Classification: progress, not graphics fix or goal completion.** Previous
live-material comparison changed the next rendering experiment; this pass
addresses the earlier observed empty-variable launch bug, not repeat gameplay.
Private backups/logs/screens/audit: `ref/xbox-build/passes/2026-10-02/launch-overrides.x2mHge/`.

### Actual helper regression

`xbox_data()` and `xbox_saves()` previously treated any getenv result, even
empty, as a development path. An empty XG_SAVE also skipped real revision
backups/marker handling. All three conditions now require a nonempty value.
No importer, renderer, touch routing, guest or pin change.

New `tests/xbox_launch_paths_test.m` includes the actual app helpers. Only its
Documents lookup, bundle/defaults and file-manager boundary are redirected to
synthetic fixture folders and an isolated preference suite; copying uses real
Foundation operations, with an explicit failed-copy injection. Engine/extractor
stubs abort if called: no game/image/window or real installation access. Run via
`scripts/test-xbox-launch.py --device <booted-UDID>`. First fixture compile fails
on inherited class-property covariance and the app's test-only nil delegate
argument warning. Correct the fixture declaration; suppress known SDK
deprecated/non-null warnings in this test link, not general compile failures.
Retain `generated/xbox-launch-tests/20261001T220825790376Z`.

Baseline `…220850240488Z` fails 12 of 20 checks. Several cascade from the absent
backup/revision marker; these are not 12 separate defects. Fixed `…220906050826Z`
passes all 20: unset/empty paths and map readiness, nonempty isolation, exact
synthetic save copy/current preservation, same-pin no-op, marker isolation,
failed-copy refusal/preservation and successful retry. Simulated copy refusal
does not cover partial copies, disk exhaustion or upstream snapshot compatibility.
No revision-save acceptance claim follows from a synthetic text fixture.

### Integrated source build and preservation

Retain complete outgoing installed app, Documents/Library and builder-output
folder (move aside rather than delete it). Build normal PC-entry app with the
existing source-guarded Xbox/ANGLE archive, no development scene. Ad-hoc signing
verification passes. In-place install only on
`DF51182F-1878-4A54-9AED-CC4AED86BEAB`; actual installed executable SHA-256
`e594a1f9570fd193eb48ee7e8fe89a834d611bf79546528258da90878515feed`.
Frozen build 64 `c55e4e2b`/ANGLE PREVIEW unchanged; no upstream promotion.

Launch PID 7921 with both empty development variables: actual UI shows both
edition cards and Play Xbox instead of Add Disc. About reports build 64 preview;
Done returns to picker. Do not open the real Xbox runtime. Relaunch with both
variables unset, PID 8093: actual Windows card validates installed files, then
normal PE entry reopens original EULA. Do not accept or fake product identity.
Final ordinary cold launch PID 8646 returns both cards. Retain empty-picker and
EULA screenshots and exact logs; these are routing evidence, not PC gameplay.

Full preinstall Documents copy comparison is byte-identical. Final audit adds/
removes no Documents files; only HaloPad's normal log changes. All real game,
package/disc bytes, preference dictionaries and PC registry survive unchanged.
Library system snapshots and scene-session metadata change. Preserve backups.
128 Xbox Python tests, five input guards, 13 executed package checks and 16
UIKit lifecycle checks pass; 16 additional package tests are skipped. Twenty
native launch-helper checks are separate from the Python 128. Current tree/index
and whitespace guards pass. No physical operation, renderer fix, Xbox IPA, push,
publication or cleanup. Goal remains active. Next matched affected rendering
and sustained controls; coordinated physical fidelity/audio/controller acceptance
and normal PC license path still require their own evidence.

## Update routine save-backup failure gates (2026-10-02)

**Classification: progress, not graphics fix or goal completion.** Previous
turn implements and verifies the empty launch-path correction. This pass covers
the goal's repeatable, save-backed pin-update routine; no actual newer pin is
fetched, compiled, installed or accepted. Private evidence:
`ref/xbox-build/passes/2026-10-02/update-backups.Z0EiBQ/`.

`update-pin.sh` previously ignores a failed checksum command, uses a reusable
second-resolution backup name, never compares copied files with their source,
and silently assumes a failed Simulator app-container query has no saves.
Now use unique mktemp directories, byte/directory comparison after Mac/Simulator
copy, required checksum generation and verification, and refusal for failed,
empty or nonexistent app-container lookup. An inspectable installed app with no
saves remains valid. Physical copies retain destination checksums only; no
independent hardware source/readback guarantee is added.

New `tests/test_xbox_update_pin.py` executes the actual shell script in synthetic
folders. Git, build/smoke and device commands are inert; real ditto/diff/checksum
operations copy fixture save bytes. Separate injected checksum failure, corrupted
copy and failed/empty container lookups must refuse candidate actions and leave
the fake pin unchanged. Two attempts at the same mocked second retain distinct
backup directories. Existing candidate-build rejection/rollback, explicit
Simulator requirement and acceptance-after-all-gates checks remain. The latter
prove protocol ordering, not actual compiler/device acceptance.

Frozen original script SHA-256 `68c62868…15552b` matches HEAD's original file.
Final baseline runs nine independent tests and fails five: checksum, copy,
failed/empty container and same-second reuse. Fixed suite passes all nine;
full Xbox Python suite passes **137 tests**. Earlier seven-test run finds four
gaps; an attempted log rerun spans a source edit and is preserved as
`mixed-source-rerun.log` but excluded. An intermediate container subcase reuses
the fake promoted pin, so final failure cases are separate fixtures. Retain
`frozen-baseline-nine.log` and `final-xbox-tests.log` as authoritative comparisons.
Shell syntax, whitespace and tree/index safety pass.

Private `backup-only.sh` executes the production backup section without fetch,
preparation, candidate build/smoke or acceptance. Actual Mac `data/save`,
`save-ios` and the dedicated Simulator save folders copy and compare successfully;
all **121 file checksums** verify. The final empty-container refusal is refined
after this positive-path run; independent controlled tests cover it. Retained
backup: `ref/xbox-build/save-backups/20261002-072643-from-c55e4e2b.jZyQEJ/`
and its sibling checksum receipt. Keep it private and do not delete caches/saves.
Directory comparison is not a point-in-time snapshot of a concurrently writing
game. Stop games before maintenance; this is not snapshot compatibility proof.

Only dedicated Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB` is used.
The backup block terminates its picker app before inspecting saves; no real Xbox
runtime opens. Ordinary launch resumes at PID 25509; actual UI shows both edition
cards. Installed executable remains `e594a1f9…15feed`, Xbox library
`bbd2a75e…4696db`, guest `102885c2…fdaaa`, frozen build 64 `c55e4e2b` unchanged.
Full audit against retained launch-pass Documents/Library backup preserves all
real game/disc/package bytes, preferences and PC registry; only ordinary log and
system snapshots change. Source upstream checkout has no tracked edits.
No app build/install, physical operation, visual fix, Xbox IPA, push, publication
or cleanup. Next pursue affected/reference rendering and sustained controls;
physical graphics/audio/controller and normal PC license gates remain separate.

## Device-SDK ANGLE preview build without installation (2026-10-02)

**Classification: progress, not hardware acceptance or goal completion.**
Previous maintenance pass changes and verifies backup failure gates. This pass
moves the physical milestone forward by preparing a device-SDK renderer/app,
without touching the shared physical iPad. Private evidence:
`ref/xbox-build/passes/2026-10-02/angle-device-build.wGZhkZ/`.

Pinned WebKit `a1fb7ce1` supplies iOS Metal source lists and hardware feature
detection; inspect actual local `PlatformCocoa.cmake` and `DisplayMtl.mm:1243`,
not a newer moving branch. Previously CMake, host and packager deliberately
reject hardware. Now build separate `iphoneos-angle` and `iphonesimulator-angle`
archives using actual SDKs. Mandatory manifest `sdk` and
`angle_feature_overrides` fields gate packaging. Simulator requires the tested
`hasTextureSwizzle` override; iPhoneOS requires an empty override list and uses
upstream detection. The renderer source pin still records the original tested
Simulator override; per-build override metadata records actual platform scope.
Keep Apple default, both source pins and PREVIEW labeling unchanged.

Twenty updated manifest tests initially fail five checks and error on a physical
link-input expectation against the old builder. Final 21 tests pass, including
a device/Simulator-launch refusal before guest preparation; all **143 Xbox
Python tests pass**. Tests use inert archive fixtures for metadata behavior,
not hardware binaries. Actual device build compiles all 342 ANGLE steps, then
fails on an unguarded call to Simulator-only `depth_probe`. Guard depth/replay
calls without exposing those diagnostics to hardware. Retain first error log.
Incremental rebuild succeeds; an intermediate 17.4 standalone target is
aligned with main app/CMake's 17.0 and rebuilt. Retain intermediate main-app
output/logs separately. Existing assembly-zero and empty diagnostic-object
warnings remain visible; they are not hardware performance/fidelity evidence.

Final combined device app SHA-256
`259fb98f256d823e6b1e519162f679214c39d9e42fd25529bb11e84f153f90cd`.
Device library `8c218df298e65a053f84616f436833744f6a245f881495819e75e401fa23a7e4`;
Simulator library `0d8de61e38f7232108fede0efb63f770f898e882e0a18476c04f472be4a7f998`.
Guest unchanged `102885c2…fdaaa`, frozen build 64 `c55e4e2b`.
`prepared-angle-device.app` is normal PC-entry/one-picker, not a development
scene. Verify bundle `dev.halopad.HaloPad`, iPhoneOS supported platform,
LC_BUILD_VERSION IOS/minos 17.0/SDK 27.0, matching bundled guest/manifest,
candidate true, empty physical override list and strict ad-hoc signature.
No provisioning profile is embedded: it is not installable/accepted on a device
just because the signature verifies. New device output contains no IPA.
Retain older outgoing device-app output, including its preexisting PC-only IPA;
do not mislabel that retained file as a newly created Xbox package.

Separately rebuilt Simulator library passes the asset-free pbuffer probe:
two EQUAL cases cover 1,352 pixels each with zero failures; plain/swizzled blits
match and swizzled sampling yields 13/26/204/255 versus plain 204/26/13/255.
No GL errors. Additional 16 actual UIKit handler and 20 launch/save-helper checks
pass on only `DF51182F-1878-4A54-9AED-CC4AED86BEAB`. No current-source game/window
or sustained-play acceptance is inferred from these probes.

Installed Simulator executable remains `e594a1f9…15feed`; no app is installed
or launched on physical hardware. Audit against retained full launch-pass backup
preserves all real Xbox/PC game/disc/package bytes, preferences and PC registry;
only ordinary log/system snapshots differ. Upstream/ANGLE checkouts remain
clean. Source guards now require fresh SDK metadata; legacy default-library
manifests must be rebuilt normally, never retrofitted. Tree/index and whitespace
checks pass. No physical operation, graphics-fix claim, pin move, Xbox IPA, push,
publication or cleanup. Next properly provisioned, backed-up device test in a
coordinated window: matched rendering, audio and controllers remain explicit
unproven gates, alongside the reported shading/focus issue and normal PC license
path. Do not reinterpret this build-only pass as completion.

## Latest-source integrated Simulator runtime (2026-10-02)

**Classification: progress.** The previous device-SDK pass changes authoritative
build support; this pass verifies its latest source in the real combined app,
not merely the asset-free renderer probe. Private evidence:
`ref/xbox-build/passes/2026-10-02/current-source-sim.Hekk3p/`.

Rebuild normal PC-entry app from `84547a5`, no scene replacement, using the
separate `iphonesimulator-angle` archive. Source pins unchanged. Stop only the
dedicated Simulator app, preserve its outgoing installed app and full 6.4 GB
data container with `ditto`, then `diff -qr` verifies exact data copy. Candidate
strict signature passes; candidate/installed executable both SHA-256
`96e8c3176cdbcbccc8de3392552f1c03622f5c13532205b0e16fd36cbeefa652`.
Outgoing installed app `e594a1f9…15feed` remains available. Installation changes
both container UUIDs; rediscover them instead of using old paths.

Dedicated Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, iPadOS 26.5:
actual two-card picker, About panel with frozen build-64 identity/PREVIEW and
Done work. Copy `checkpoint64.sr7KEQ/after-cold-save-quit` into this pass's own
`play-save`; reuse existing diagnostic maps read-only in the earlier play-data
folder (its game logs may change). Clear bot/network test flags, show touch
overlay explicitly; do not force edition selection. Xbox → virtual A Campaign
→ New001 → Halo → Normal restores the copied outdoor checkpoint through normal
menus. Actual finite horizontal/vertical look gestures change the view; RT/Y/
Zoom actions reach a pistol and circular 2x scope. No exact rifle shot-count
claim without reviewing intermediate frames. Pause and separately observed
down-stick gestures safely select Save and Quit; Saving completes and main menu
returns. Two rapidly adjacent drags advance only one item, retained as an input
timing limitation, not sustained controls acceptance.

Retain `manual-pan.mp4`, H.264, 62.313 seconds. Extracted 25s/47s frames verify
outdoor rifle and scoped view in the captured video. Encoded capture rate is
not game FPS. Finite drags/video samples do not prove continuous/multi-touch
handling, full temporal stability, original-driver equivalence or physical
fidelity. Existing frame-zero GL 0x502 log remains visible; sampled subsequent
frame logs are zero. Do not promote the preview to default or call the physical
shading/focus report fixed.

Cold ordinary launch without development overrides presents both cards.
Windows verifies the existing installation, enters normal PC core and presents
the original Custom Edition EULA. Leave I Accept untouched; retain screenshot.
Restore ordinary picker PID 67405, no test overrides. No real Xbox runtime/save
opened. Current-backup audit checks all 196 Documents files: no additions or
removals, only HaloPad log changes. Xbox/PC game/save/package bytes and PC
registry preserved; final preference dictionaries identical. Library Metal
caches, scene state and OS snapshots differ normally.

The older launch-pass audit fails strict preference equality immediately after
Xbox selection; do not hide or treat that as data-loss proof. Private fresh
audit initially misuses `check=True` on a comparator that exits one for expected
log differences; correct exit handling, parse full report, require unchanged
real files/registry/key set and allow only last-edition preference changes.
Final audit passes with no changed preferences. No physical operation, guest
pin movement, rendering change, Xbox IPA, push, publication or cleanup. Goal
active. Next sustained controls and affected/reference graphics; request which
edition/map shows the physical complaint before attributing it to Xbox or PC.
All 143 Xbox Python tests pass again (19.389 seconds); whitespace and current
tree/index safety checks pass. Final picker process is verified live at PID 67405.

## Shared touch-button ownership (2026-10-02)

**Classification: progress.** The previous pass verifies current-source app;
this pass fixes a separate runtime input defect. When two touches own RT or A,
`touchesEnded` formerly clears the action on either release. Extend the real
UIKit-handler/inert-token fixture to consume unread pulses before checking
continued ownership, with concurrent independent move/look and reverse A release
order. Baseline `generated/xbox-touch-tests/20261001T232844Z`: 24 checks, exactly
two failures. Remove ended touch from the existing map, then clear the button
only if no remaining value owns it. Fixed `…/20261001T232950Z`: 24 checks, zero
failures, including the earlier cancellation/focus/controller cases. This is
handler-boundary proof, not fabricated OS events or physical multi-touch proof.
All 143 Xbox Python tests pass in 18.891 seconds. No layout/binding, input-buffer,
controller-merge, guest pin or renderer change.

Private pass `ref/xbox-build/passes/2026-10-02/touch-owners.y1I8qe/` retains
outgoing generated/installed app and Simulator library. Stop only dedicated
Simulator app; preserve full data container and compare bytes before in-place
installation. Rebuild separate Simulator library and combined normal PC-entry
app, no development scene; strict signing passes. Candidate and installed app
SHA-256 `3a933fea134580e424ee7f7ef2028785c811603d55d7f4f5884157289a995e8a`;
library `50a549c3d119f704030e52721058ba7eeec8a18dbd6e6ce89679a1925fe8dae2`.
Rediscover changed container UUIDs. Existing iPhoneOS preview is untouched and
predates this runtime edit; source guards require a normal rebuild, no retagging.

On dedicated Simulator `DF51182F-1878-4A54-9AED-CC4AED86BEAB`, clear bot/network
flags, use newly copied own save and explicit touch overlay. No forced edition.
Shared Device Hub main window changed to connecting physical iPhone before UI
actions; fresh tree prevents stale-index game input. Select only dedicated
Simulator and open a separate Simulator-only window before continuing.
Actual picker → Xbox → Campaign → New001 → Halo → Normal restores copied outdoor
checkpoint; retained `before-fire.png`/`after-fire.png` show single RT 60→59.
This does not execute the duplicate-owner case through actual OS gestures.
Restore ordinary picker PID 74408, no test overrides. Audit (`audit.log`, both
JSON inventories) verifies 196 Documents files, no additions/removals, only log
change. Real Xbox/PC game/save/package files and PC registry unchanged. Preference
key set preserved, only `HaloPadLastEngine` changes; OS snapshots differ.

No physical installation/game input, controller pairing, graphics-fix claim,
pin update, Xbox IPA, push, publication or cleanup. Full sustained controls,
hardware graphics/audio/controllers, normal PC license acceptance and reported
physical shading/focus remain open. Goal active, not complete or blocked.

## Current-source device readiness (2026-10-02)

**Classification: progress, artifact readiness only.** Source inspection confirms
the picker does not initialize both guests: PC initialization is selection-bound;
Xbox starts only after selection/import and revision-backup checks. Xbox reserves
8 GiB of virtual address space before trimming to an aligned 4 GiB guest region;
physical provisioning and allocation feasibility remain unproven. Audio callback
inspection does not establish a race or justify a speculative rewrite. Five
synthetic profile-preflight checks pass, not actual signing-profile verification.

Private pass `ref/xbox-build/passes/2026-10-02/hardware-readiness.X9a0ua/`
preserves the previous device archive and generated output. An initial module
import misses the scripts path; correcting that setup lets the actual archived
library check run and reject stale local sources with the required rebuild
message (`stale-library-guard.log`). No retagging or fabricated manifest.
`build-library.log` and `build-app.log` retain successful current-source device
builds. `verify-build.py`, `build-verification.json` and `device-build.json`
retain actual artifact checks against source `68cb779`:

| Artifact | Verified identity / limit |
| --- | --- |
| Combined iPhoneOS executable | `af1be628abbf5381160953031a91d7d817397a14999b5b25b9a4ada3830dc436` |
| Device Xbox archive | `341f6c0d98f63f9625d7d9c29f9fd6f6d2655dc32313529629fc20497e134702`; SDK `iphoneos`, feature overrides empty |
| Frozen guest | `102885c274aa95771be8672a65ba16fa88a52d9f4d3889310072e214aabfdaaa`; build 64 unchanged |
| Packaging/signing | `dev.halopad.HaloPad`, IOS / minimum 17.0, strict ad-hoc signature; no provisioning profile or new IPA |
| Simulator | Prior source-checked archive still valid; installed app is not changed by this pass |

Historical accepted build-64 bounded Mac menu/campaign/match and dedicated
Simulator results remain reference evidence, not a new hardware pass. Their
retained results are `ref/xbox-build/smoke-results/20261002-000148-c55e4e2b/`
and `ref/xbox-build/simulator-results/20261002-000148-c55e4e2b/`.
The current turn makes no runtime source edit, so the prior 24 UIKit/143 Python
results are not presented as newly rerun tests.

Full inspection of `scripts/install-device.sh` confirms it signs/builds,
installs and copies a PC package but does not back up Documents/Library. Do not
execute it as an unattended hardware-readiness shortcut. Require a coordinated
window, exact device/profile/certificate/entitlement checks, complete backup and
readback, in-place install, then observed memory/launch, graphics, audio and
controls. No BlueWake interruption, pairing, physical installation or input.
Reported physical shading/focus still needs the affected edition/map; it is not
fixed or attributed by this pass. Goal active, no pin promotion, IPA, push,
publication or cleanup.

## Physical iPad ANGLE preview (2026-10-02)

**Classification: progress, bounded hardware execution, not full acceptance.**
Chris explicitly authorizes the hardware iPad. Rediscover the wired/trusted,
Developer-Mode-enabled M2 iPad Pro 12.9-inch (6th generation), iPad14,5,
iPadOS 27.0 / 24A437. The connected iPhone is not a target. Stop only the outgoing
HaloPad; BlueWake's background process remains untouched. No Bluetooth change,
uninstall, reset or unrelated-app input. Private evidence is retained under
`ref/xbox-build/passes/2026-10-02/ipad-hardware.YPnm9V/`.

### Preservation and exact candidate

CoreDevice bulk Documents backup fails after a partial transfer (error 7000,
socket closed/POSIX 60); the separate Library copy succeeds. Preserve both.
Use the already installed AFC CLI serially, never concurrently with CoreDevice
transfers. Complete Documents/Library backup, then independently read them again.
The first readback disconnects; retain that partial too. The successful retry
nests Documents under the preexisting destination. Preserve the initial failed
layout audit; correct the explicit successful root, not the hashes. Actual
`backup-audit.json` verifies **320 files / 3,348,625,943 bytes, no differences**.
This complete independently compared backup precedes installation.

Copy the existing ad-hoc app, preserving source executable
`af1be628abbf5381160953031a91d7d817397a14999b5b25b9a4ada3830dc436`.
Validate a real development profile for the exact bundle/device, certificate
match, expiry and both extended-virtual-addressing/increased-memory entitlements.
Sign only the staged copy. Strict signature/entitlement checks pass; signed
staged executable is
`90437ca63e7c8314c1ee32b887c31299ecd4ddda01355850c86f05892306362e`.
Signing changes embedded executable bytes; do not confuse these two identities.
In-place CoreDevice installation succeeds; rediscover installed bundle/data
metadata. This is not an independent readback/hash of the installed executable.
Bundled frozen build 64 (`c55e4e2b`), guest `102885c2…fdaaa`, device Xbox archive
`341f6c0d…134702`, SDK iphoneos and empty feature overrides are unchanged.
Post-install/pre-launch original Xbox save bytes match exactly; Library changes
are SplashBoard snapshots only, with PC registry/preferences unchanged.

### Observed hardware behavior

Launch the ordinary picker, no forced engine/map or bot. Disable upstream online,
clipboard joins, UPnP and automatic updates using launch-only environment values.
Use a fresh `XG_SAVE` root `Documents/xbox-hardware-save-20261002-YPnm9V`;
original Xbox snapshots are never opened. This isolation deliberately bypasses
the real revision-marker path, so it is **not normal revision-backup acceptance**.
About displays `c55e4e2b` PREVIEW; Done returns to the picker. Xbox → Campaign →
fresh New001 → Normal → a10 ship/bridge cinematic → first-person cryo bay runs
through the actual controls. A/Start cinematic inputs are not a general skip
behavior claim. No PC engine gameplay is tested in this pass.

The actual device log confirms 4 GiB guest memory at `0x7000000000`, the Xbox
image mapping, automatic device feature detection, ANGLE Metal Apple M2 GPU
(ANGLE `eb725ace1839`) and 48 kHz/two-channel audio initialization. Render size
is **640×480**, not native-panel resolution. Bootstrap frame 0 reports GL
`0x502`; sampled frames 1, 2 and 120 report zero and complete framebuffers.
Do not describe all frames as error-free or these samples as sustained FPS.
Upstream config adds new default keys and logs the removed `network.netcode`
setting. The fresh-profile event-handler error is retained, not hidden; startup
continues through profile creation and the campaign.

Native device screenshots retain About, campaign setup, opening and before/
after cryo look views. Sampled floor/walls render without the large black floor
polygons seen in the earlier Simulator Apple comparison. No matched physical
Apple baseline or original-driver scene is available; the user's shading/focus
complaint is **not resolved**, and the affected edition/map remains unspecified.
Short mirrored look drags produce only small camera changes and leave the look
tutorial pending. This is not usable sustained touch, simultaneous fingers,
checkpoint progression or controller acceptance. Request direct-finger feedback;
do not pair a controller or infer its feel from mirrored pointer input.

### Wired video/audio and connection limits

Use the QuickTime wired-capture skill with supported CUA UI controls (its older
Sky entry point is unavailable). Verify the recording timer, then stop via UI.
The first retained unsaved movie is 396.617 seconds / 1,495,917,432 bytes,
H.264 1600×1200 plus mono AAC 48 kHz. Its audio source was not explicitly selected,
so it does **not** establish game-audio output. It covers menu/cinematic/cryo,
not six minutes of sustained play. A second bounded capture explicitly selects
the exact iPad under both Screen and Speaker: **31.368 seconds / 74,470,060 bytes,
H.264 1600×1200 and stereo AAC 48 kHz**. A 12-second sample has nonzero stereo
signal, peak −12.67/−13.45 dB, RMS about −24.95 dB, no NaNs/Infs. This establishes
captured output, not speech/weapon fidelity, underrun-free playback or physical
speaker quality; mirroring changes routing. Both unsaved compositions remain
local in QuickTime autosave, not moved, discarded, uploaded or published.

The CoreDevice console connection later invalidates (error 3 / Mercury 1001).
Read-only process inspection confirms HaloPad **PID 7089 remains live** in the
installed bundle after the captures; this is not an app crash. Do not attribute
the transport loss to QuickTime without a discriminating test. The original
console failure and partial backups remain retained. No runtime rewrite is
justified by this bounded pass. No pin promotion, Xbox IPA, release or cleanup.

### Post-run preservation and next gate

After stopping capture, complete a serial AFC Documents/Library readback to a
fresh destination. HaloPad remains live with isolated saves; this is not an
atomic snapshot of its active new test state. The initial audit incorrectly
classifies the known app log at `Documents/HaloPad Logs/HaloPad.log` as protected;
retain that failed report, then correct that one exact log path. The final audit
passes: 320 original versus 363 current files, **151 protected original Documents
files unchanged**, including original Xbox maps/saves, PC files/packages/profile
data and revision state. PC registry and preference dictionary are unchanged.
Only reviewed config-default additions, game/app logs, fresh isolated test saves
and OS/Metal/dyld caches/scene snapshots differ. No unexplained changes remain.
Config review confirms existing values unchanged, with new defaults appended;
telnet stays disabled. `after-audit.json`/inventories retain the full comparison.

Leave the physical app in the isolated cryo-bay run for Chris's direct-finger
check. Next verify sustained look/move and simultaneous controls, real checkpoint
progression/Save and Quit/cold reload, already-paired controller behavior and
speaker audio. Then reproduce the affected edition/map at matched graphics
settings against an original/reference renderer before changing fidelity code.
No source/runtime fix or newly rerun synthetic suite is claimed in this pass.
The final CUA reconnect reports its native pipe closed; do not restart Codex or
other shared apps to recover it. Purpose-built device screenshot capture still
succeeds and confirms the cryo-bay look tutorial; a final process query confirms
PID 7089 live. This limits further mirrored UI automation, not app execution.
Whitespace and current tree/index safety checks pass; private inputs/evidence
remain ignored. No public-artifact or full gameplay gate is claimed.

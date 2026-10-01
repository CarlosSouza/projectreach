# HaloPad goal loop — phase 3: device readiness

Written 2026-09-29. This is the active loop for the supplied device handoff and the next iPhone/iPad release candidate. It inherits the original loop's private-input, source-integrity, evidence, and safety rules. Current truth is in [STATUS.md](STATUS.md) and [JOURNAL.md](JOURNAL.md); older phase-2 claims describe builds and machines that are not present in this checkout.

## Current priority — 2026-10-01 Simulator / Xbox integration

Chris's latest direction prioritizes the iPad Simulator on this Mac, one-app
Windows/Xbox edition selection, and focused passes against a frozen recent Xbox
upstream build. This pass does not authorize physical installs, an Xbox IPA,
publication, or a recurring automation. The older physical-device work below is
historical context, not the next automatic action.

Latest focused pass: a real Simulator ANGLE/Metal source build is now available
as `HALOPAD_XBOX_RENDERER=angle-metal`, independently pinned and PREVIEW-labeled.
The initial backend removes visible wall bands in sampled views but swaps
texture red/blue. A tiny sampling probe reproduces the missing swizzle; the
candidate's native-swizzle override passes on this Mac/iPadOS 26.5. Reviewed
rebuilt campaign/match images restore colors and lack the pronounced bands.
Normal menu/campaign/scripted-match gates pass (1,530 ticks / 12 shots). This is
an observed improvement, not complete visual acceptance or proof about the
physical Windows edition. Apple remains the default; the comparison held guest 61 frozen.
Copied identical checkpoints now provide the same untouched cryo-bay camera:
Apple has large black floor polygons; the preview draws that floor. Both render
at 640x480. NPC/prompt timing differs; no pixel-synchronized driver claim.
Preview four-direction look, X tube exit, short forward input and Save and Quit
work through actual touch controls. Fresh-process reload restores the last
checkpoint inside the tube, not the unsaved exit position. A later normal-menu
Halo/a30 pass now verifies RT fire (60→58), Y pistol swap, Save and Quit and a
fresh-process pod checkpoint reload. Short drags are not sustained walking.
A separate 90-second upstream-bot rendering diagnostic reaches the outdoor
valley with camera/movement/fire; sampled cliffs, trees and ring lack pronounced
bands. `smoke-simulator.py --campaign-map a30 --scripted-campaign` requires the
explicit campaign/diagnostic case and labels its result accordingly; ordinary
campaign/menu runs clear inherited bot/network-test settings. Next verify
sustained human movement/look, simultaneous controls, a newly reached checkpoint
and moving-image stability. Do not substitute bot screenshots for those gates.
Keep the accepted Apple app available for in-place rollback and all real saves
untouched. Physical iPad shading/focus investigation stays a separate gate.

Accepted experimental update, 2026-10-02: build 64,
`c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9`, four commits after 61. Candidate and
acceptance source-built Mac/ANGLE Simulator menu/a10/scripted-match sets pass.
Normal copied build-61 a30 checkpoint, RT fire, Y pistol swap, circular 2x scope,
pause panel, Save and Quit and fresh-process pod reload verified. `top` menu/pod
snapshots 145M/172M are not physical footprint or a same-scene old/new comparison;
`vmmap` failed. CRC-guarded expanded HUD and meter-alpha ABI reviewed; the old
cache-byte diagnostic remains fail-closed with an explicit new-pin test.
The 2026-10-02 a50 follow-up adds a bounded 90-second rendering diagnostic and
retained video, plus actual normal-menu 2x/10x sniper scope and night vision.
Sampled ladder/border draw without obvious missing sections; scope aim is mostly
ground and finite look drags do not establish sustained pans. Normal Save and
Quit followed by a fresh-process menu path recognizes and restores a50's opening
checkpoint. A partial mixed-map snapshot fixture did not establish continuation;
the successful normal new-level save/reload is recorded separately. These are
not a newly reached later checkpoint or physical rendering acceptance.
Next check continuous moving views against a reference and scope meter fringes,
sustained human controls,
new checkpoint progression and physical memory/audio/fidelity under a separately
coordinated device window. Keep Apple and ANGLE pins independent and PREVIEW
visible; do not chase moving HEAD. Preserve outgoing app and both save backups.

The 2026-10-02 Simulator-only opt-in audio capture now proves finite non-silent
48 kHz stereo reaches the output callback in four-second menu and normal-input
a50 launch windows, with no counted starvation. Captures stay private. This is
not speaker quality, scheduling deadlines, sync or hardware audio acceptance.
The physical shading/focus report remains unresolved. PC source inspection
finds `MIPMAPLODBIAS` explicitly ignored in `halopad_d3d9_draw.c`; investigate
actual nonzero state use and a matched reference before treating it as the cause
or changing shader sampling. The Xbox ANGLE diagnostic does not test that PC path.

Latest PC arithmetic follow-up (`pc-live-color.L5KDQS/`) captures the exact
four-texture Battle Creek ground draw's actual samples/interpolated inputs and
unmodified pixel output in three independent float replays. All 1,396,060 pixel
observations match an independent original-bytecode interpreter exactly. Base
alpha is one and fog off; this is not all material/blend/fog coverage. Grain
remains visible, including in detail samples. Sampling/interpolation are measured
inputs, not independent original-driver correctness; no depth/occlusion or
physical fidelity acceptance. Preserve the first indoor-spawn run's missing
material failure. Successful run passes 24 assertions and 30 motion frames;
analyzer calibration/negative controls pass. No tracked runtime edit, app install
or physical operation; real game bytes/preferences/registry remain unchanged.
Next obtain a matched original-driver or confirmed affected edition/map scene,
then discriminate sampling/geometry/depth. Do not infer the physical issue is
fixed or repeat this arithmetic gate as full visual acceptance.

Follow-up PC/macOS Blood Gulch component run now checks actual sampler state:
all 932 bound observations across 248 requests in frames 300–301 have zero LOD
bias. No nonzero-bias cause established for this view; do not adopt a shader
workaround from that source gap alone. Mac input-posting API link regression
fixed using same-thread delivery; 264 native input checks pass. This remains
different from the reported physical scene and continuous moving-view acceptance.

The next PC/Simulator pass found and fixed a separate texture-lifetime defect:
rewriting a CPU-backed texture before Present incorrectly recoloured an earlier
draw. The exact regression fails on both Mac and Simulator before the fix and
passes afterwards, including preservation of unchanged mip levels. Each platform
passes 260 native D3D9 checks. Basic derivative-based mip filtering passes too.
Blood Gulch still renders after the fix, but its sampled frames have no managed
texture updates, so they do not connect this defect to the physical complaint.
The affected edition/map is still unconfirmed. One-app Simulator compilation
passes; no install or physical validation. Keep the graphics report open and
seek a matched moving scene before calling it fixed.

A subsequent PC moving-component pass now uses the actual host input API on
Simulator and Mac: walk/stop, roughly 25-degree pan and fire checks pass. Explicit
test-only capture retains all 30 consecutive presented pan frames and guest pose
metadata, rather than selected screenshots. In this near-cliff Blood Gulch view,
camera steps are smooth and frame-wide brightness does not abruptly switch;
11,966 bound texture observations again have no nonzero LOD bias/resource LOD
or dirty managed texture. This does not reproduce the physical complaint.
The private one-second movie is a 30-fps frame replay, not measured gameplay
timing. Normal tests do not capture the sequence. No app install or save changes.

The next Xbox build-64 pass closes the narrow new-checkpoint gap: actual picker,
Campaign/New001/Halo/Normal menus load an isolated copy inside the escape pod;
finite forward touch gestures advance outside and collect supplies. Save and
Quit finishes, then a cold process through those same normal menus restores the
outside-pod checkpoint with one grenade (not the subsequent 425-round/two-grenade
inventory). No init script, bot or direct map load. The checkpoint save hash
changes and before/after snapshots remain private. This proves early checkpoint
progression only; the short drags do not establish sustained two-thumb controls.
Real Simulator saves and the physical iPad are untouched. The reported physical
shading/focus issue remains open, with affected edition/map still unconfirmed.
Next prioritize a matched affected moving scene/reference and sustained controls;
do not repeat the now-covered opening-to-outside-pod continuation as full gameplay
or hardware rendering proof. See the pass ledger for exact evidence.

The next PC diagnostic expands moving coverage to Battle Creek through the
existing local LAN menu route, not a direct map load. Its scripted controls now
use the host input API. Ordinary Mac and Simulator runs pass all 19 checks;
opt-in Simulator capture passes 20 including 30 consecutive downward-pan frames.
One traced run fails later grenade-death/respawn/pickup assertions; preserve that
failure, do not relax them. A combined capture/trace repeat passes and shows a
strongly grainy outdoor ground view. Its 9,299 bound observations have zero LOD
bias/resource LOD and no dirty managed textures. A new trace-only native-object
flag distinguishes first uploads from rewrites; 260 native GPU checks pass.
No explanation of the physical complaint is established. Different random spawn
positions render different surfaces; these runs are not pixel-matched references.
Next isolate the grainy ground material and actual mip/detail sampling against
an original reference, rather than repeating the base-floor capture or treating
more green gameplay assertions as visual acceptance. No iPad install/launch,
one-app installation or real save changes. Goal active.

The following PC pass localizes that sampled ground appearance to a world
material, before HUD and final presentation. A bounded test-only draw hook
captures the current render attachment after each encoded draw in frame 3155:
93 images match request numbers 0–92; all 21 component checks pass. Readbacks
interrupt submission, so this is not unmodified ordering or performance proof.
The first colored ground appears at request 21, using four 2D textures with
nine/ten mip levels and trilinear, zero-bias sampling. A separate dump-mapping
run identifies a four-texture ps_1_1 program and its scaled-coordinate vs_1_1
program; its random spawn differs, so it is not a matched image comparison.
Both programs agree with the independent arithmetic interpreter over 4,096
synthetic cases each. This test uses level zero, no projection, and does not
validate actual raster derivatives, mip contents or original-PC fidelity.
The old shader-test runner exceeded Metal's inline constant-data limit; using
a constant buffer fixes that diagnostic failure, not game rendering.
Ordinary final-source Simulator host checks (19), Mac D3D9 checks (260), Xbox
Python tests (109), input guards (5) and safety pass. The ordinary two-edition
picker and real save comparison are intact. No installed app or pin changed.
Next inspect this material's actual moving-view UV/detail/mip inputs and seek
a matched original rendering; do not infer that grain alone is a defect or
change filtering speculatively. Physical shading/focus remains unresolved.

The subsequent live-mip pass catches and fixes a **measurement** defect, not
the reported shading: Metal's short texture-readback selector returns level-zero
crops for nonzero mips on this Simulator. An asset-free probe distinguishes
that readback from correct explicit shader sampling; the full selector works
on both Simulator and Mac. A real-helper regression fails exactly three mip
checks before the correction; all 268 native checks pass on both platforms
afterwards. Corrected capture of the actual four-stage ground material retains
38 levels / 873,812 texels, matching independent BC decoding within one RGB
unit and exact alpha. Initial lower-mip captures are explicitly rejected and
preserved. Guest constants request 100x/60x detail tiling and 12x fourth-stage
scaling, with projection disabled. The corrected frame-3155 guest pose matches
the prior dirt case; its 100,000-pixel ground ROI is identical, confirming this
is not a grain-removal fix. A first component run fails five later gameplay
checks; the corrected capture run passes 21. Do not attribute those outcomes
to this helper without controlled gameplay evidence. Ordinary picker restored,
real saves intact, no app install or pin change. Next inspect actual fragment
LOD selection against a matched original scene and exercise the latest-source
integrated one-app build; keep physical fidelity and controls unaccepted.

The next current-source integration pass (`integrated-current.Uyb7ck/`) builds
and installs source `9888782` in place on the dedicated iPad Simulator after
preserving both outgoing apps and Documents/Library. Installed binary SHA-256
is `db16569da62c6e5c88c6adfad6842645efa884550b6abad25af7c54d25cadd73`.
Picker/About/Done work; Xbox restores the isolated outdoor a30 checkpoint through
normal menus, fires, swaps pistol, toggles 2x scope and completes Save and Quit.
Touch is forced visible because a controller is detected; this is not hardware
input acceptance. Real Xbox data/saves remain byte-identical. Ordinary cold
relaunch returns both cards. PC imports a freshly matched private 89-entry ZIP
through Files; all 78 installed stock files match the signed inventory. The old
handoff ZIP fails closed and remains unchanged. Normal PE entry reaches the
original EULA, left unaccepted pending explicit user confirmation. Thus the
installed normal PC main menu, later product-ID gate and gameplay remain open.
Only the last-edition preference changes; normal PC state and caches are added.
109 Xbox tests and five input guards pass. No runtime edit or upstream promotion,
physical operation, IPA or publication. This is integration progress, not a
shading/focus correction. Next continue the authorized PC path when confirmed
and measure actual fragment LOD/shading in a matched affected moving scene.

The next PC raster pass (`pc-raster-lod.RfwgiV/`) adds only a nullable native draw
descriptor test hook, with two regressions: 270 native assertions pass on Mac
and Simulator. Direct mip queries are unsupported on this Simulator, so a
separately calibrated numeric mip-marker method measures actual sampling in the
exact encoded ground geometry. Six-footprint asset-free calibration passes 384
fragments per platform (fractional marker error ≤0.00390625). First live run
times out and has later combat failures and a clipped draw; preserve it. A
revised capture matches all four texture payloads, skips zero-coverage draws
and retains the full gameplay harness with a longer deadline. It exits 0 with
25 assertions and 30 motion frames. Three independent raster probes cover
1,427,322 pixels / 5,709,288 stage samples, without changing original color.
Stage 1/2 mip maxima fall from 4.0117/4.2695 to 0.8008/1.0586 then zero as the
view points down. Largest clamped derivative-vs-marker residual is 0.0588143
levels; retain 288 samples above 0.05 near the level-zero transition. No depth/
occlusion reference or original-driver/physical equivalence is established.
Visible grain remains. This weakens gross mip-selection failure as an explanation
for this particular ground material; it does not fix or dismiss physical shading.
109 Xbox tests and five input guards pass; no app install, save changes, upstream
promotion or physical operation. Installed PC EULA is still unaccepted. Next
compare a matched affected original-driver view and its material/lighting/UV/
sampler behavior, without visual-workaround edits to detail or mip selection.

The earlier build-61 (`f8937c61`) experimental development pin added upstream's
high-resolution HUD after build 60's menu-glyph fix. Save-backed candidate and
acceptance Mac/Simulator menu/campaign/scripted-match checks pass. The
earlier build-59 real touch pass verified navigation, cryo-bay training,
tube exit and a same-build cold checkpoint reload with isolated saves. A copied
build-59 checkpoint also loads through the normal build-60 menus, and a copy of
that fixture reloads the cryo-bay through normal build-61 menus; real saves
remain unchanged. Mac smoke tests now verify executable/guest hashes.
The normal picker remains on the dedicated **HaloPad Xbox iPad** Simulator.
See [XBOX-SIMULATOR-PASSES.md](XBOX-SIMULATOR-PASSES.md) for the evidence and failed
approaches. The broader goal remains active; this is not full campaign/hardware acceptance.

Earlier Apple-backend investigation: a small reproducible case for remaining geometry/texture artifacts (already
present before final presentation; uniform-location, buffer-upload, anisotropy
and LEQUAL hypotheses did not resolve them). An EQUAL bypass changes artifacts
but introduces incorrect occlusion; keep it diagnostic-only. Stationary runs
still choose different spawns, but a same-process EQUAL → ALWAYS → EQUAL run now
removes/restores stripes at a fixed view. Basic asset-free equal-depth controls
pass. Read-only captures now show exact position/projection identity for a
144-index VS17/VS41 pair; transform-feedback replay gives identical coordinates.
It changes linkage and omits pixel/texture state, so original-program invariance
is not proven. Isolated original-indexed rasterization now preserves all 145,994
pixels under EQUAL for a larger 1,257-index pair on texture-backed targets, with
same-program and ALWAYS controls passing. A fixed-view stencil bypass leaves
stripes. Expanded read-only captures verify the same framebuffer/color/depth
objects for the paired draws. Level-0 2D readbacks match supported CPU upload
references byte-for-byte. Independent decoding of captured original RGB565 bytes
now also matches all 32,768 pixels of the previously banded 256x128 texture.
This verifies that texture's unswizzle/conversion, not all formats, source-byte
production or sampling correctness. A calibrated same-frame observer now proves
intervening depth changes (54,517 pixels; 21,611 base-changed pixels closer),
but not incorrect writes. Next correlate original native-linked EQUAL coverage
with copied live depth/full pixel state to distinguish legitimate occlusion from
missing visible terrain. Stripes remain; normal depth semantics stay unchanged.
The diagnostics above are build-60 evidence, not new build-61 proof. Resume the
depth experiment on the frozen accepted 61 pin. The optional original-byte
reader refuses its changed cache ABI until adapted/reviewed; normal gameplay
and update gates do not use that diagnostic.
The isolated raster probe intentionally uses black textures/default pixel uniforms.
Do not declare a driver bug or relax EQUAL.
Then sustained movement/look/fire and weapon pickup/swap,
a later checkpoint and reload.
Chris's follow-up explicitly reports shading/focus instability on the physical
iPad. It remains an unresolved visual-acceptance gate, not a reason to equate a
menu, checkpoint, or smoke-test pass with a correct image. This pass continues
on the dedicated Simulator; no physical installation or direct observation of
that report is claimed.
Build-61 native pixel replay now copies live color/depth/stencil into owned
texture-backed targets, retaining the original linked program and pixel state.
The calibrated 1,944-index pair repeats and matches the actual live draw exactly;
copied depth also matches. ALWAYS-only color response at base-written pixels
occurs only where later depth became closer (24,262 pixels), never where it was
unchanged. Color differences alone include overdraw and are not missing coverage.
This pair does not prove missing visible terrain. A new frame-120 color timeline
now localizes obvious wall stripes to draw 119: program 84, VS7, EQUAL, 402
indices. Its before-image has unstriped walls and its after-image has horizontal
bands. VS7 source matches `vs007_0.glsl`, not the VS41 previously replayed.
VS17/VS7 captures now verify exact indexed positions/projection and calibrated
live depth. Two sampled batches (228 and 531 indices) have zero native color
response; a third (1,686 indices) fails exact repeat/live color at 43 pixels.
All are rejected as full-pixel evidence. The third pair's transform-feedback
positions match bit-for-bit, with linkage-change limitations intact.
Next capture the visibly stripe-producing material with original before/after
color and texture/alpha inputs; do not infer that an arbitrary VS7 batch is the
defective draw or generalize VS41 controls to this detail pass.
That material trace now captures the actually band-producing draw: all 17
VS7/pixel-shader pairs in its 211-draw frame retain identical uniforms and
level-0 texture bytes, including upload equality. Reviewed draw 117 (402 indices)
changes the walls from unstriped to banded. A second, different-spawn trace has
294 draws / 22 matching material pairs and repeats the symptom at draw 176
(1,377 indices). Its requested 402-index paired/native capture is absent and
fails, so draw-count filters do not establish a repeatable scene. Next attach
the native probe to the material's original trace draw in that same frame, or
evaluate a separately built Simulator ANGLE/Metal backend against the accepted
Apple Software Renderer reference. Do not relax depth/alpha guards or infer a
physical Windows/Metal fix from Xbox Simulator evidence.
The later 984-index VS41 native replay failed exact repeat/live-color checks
and remains rejected. The independent 211-draw timeline passes its own guards.
Maintain upstream by freezing one candidate per pass, backing up saves, building,
testing and visually reviewing it before pin promotion. Keep normal runs free
of input/render diagnostics and preserve the Windows route and both save domains.

## Starting point and claim boundary

2026-10-02 disc-import follow-up: validate untrusted image names/extents/headers
before writing, keep failed copies in unique stages, never replace existing maps
or saves, and reject stale local-source Xbox archives during app packaging.
Corrected current-source ANGLE Simulator app verifies actual malformed-image
rejection → Editions → real USA disc Files import, all 24 map hashes, new-profile
creation, a10 opening cinematic and cold menu/profile recognition in isolated
folders. Follow-up source/byte inspection explains the fresh The Maw/Legendary
summary: upstream deliberately unlocks every level/difficulty in new profiles.
This is not a progression or compatibility pass. Normal Windows entry still presents
the unaccepted EULA. Leave the ordinary app at the edition picker. Existing game
files are unchanged, but normal logs/caches/preferences are not. The physical
shading/focus complaint, matched moving-reference comparison, broader checkpoint
progression and coordinated hardware acceptance remain open; the earlier bounded
outside-pod checkpoint reload is already proved. Do not substitute import/menu
results for them. See the latest [pass ledger](XBOX-SIMULATOR-PASSES.md#native-disc-import-validation-and-stale-library-gate-2026-10-02).

2026-10-02 touch lifecycle follow-up: six real-handler failures before the fix,
16 checks pass after it. Deactivation and activation clear both current/unread
input and ownership; inactive/hidden presses cannot replay. Rebuild the library
and integrated app, not just the manifest. Candidate `58111bcb…29f093` loads the
copied outdoor a30 save through actual menus; Home/resume keeps PID 91997, logs
both resets, and fresh RT/Y/Zoom/finite look/move/Start work. Save and Quit finishes.
This is a real normal Home cycle plus asset-free held-handler tests, not physical
held multi-touch/hardware acceptance. Original Xbox/PC bytes, preferences and
registry are preserved; normal caches/logs change. Leave the ordinary app at the
picker without Xbox environment overrides. An empty `XG_DATA` still selects a
development path; **unset** it and `XG_SAVE` for the ordinary route. No physical
changes or renderer fix. Next prioritize the affected moving material/reference,
not repeated imports or already-proved early checkpoint gates.

- The private kit is `ref/handoff/HaloPad-iPad-test/`: an arm64 iPhoneOS `HaloPad.app` and its matching `.halopad.zip`. The original kit stays untouched. The package verifies its 87 files, including 78 stock records.
- A **copy** of the device app was converted to an ad-hoc Simulator probe under ignored `generated/simulator-probe/`. On this Mac it reaches the main menu on iPad Air 13-inch (M4) and iPhone 17 Pro Simulators. The iPad local LAN match reached first-person play; menu taps and FIRE worked. This probe is not a source-built Simulator binary or physical-device acceptance.
- The iPad local-host **Leave Game** action in the three-dot menu did not leave; Halo's own pause-menu Leave Game did. Touch controls were initially hidden by the connected-controller setting in the Simulator; turning that setting off exposed them. Two-thumb gestures, real controller input, keyboard typing, private-server joining, and iPhone gameplay remain to be accepted on this machine.
- This Mac has Xcode 27 and Apple Development identities on team `VKDH2T9UTF`. A cached development profile for the exact `dev.halopad.HaloPad` App ID and connected iPhone 14 passes both memory entitlement checks. The handoff package supplied the accepted 1.10 executable, four matching modules, stock files, and reference files, all restored to ignored local inputs with hash checks. A fresh source-built iPhoneOS app and IPA were signed and the app installed in place on the iPhone 14 after backing up its existing HaloPad Documents and Library. The matching package imported through the physical iPhone's Files picker and reached Halo's main menu. Phone gameplay and sustained performance remain open. CrossOver and the original installer/patch provenance are still unavailable here, and no iPad is connected.

## Loop rule

Work the lowest useful unblocked item. Keep independent Simulator, signing, source, and documentation work moving while physical-device work waits. A gate passes only with a reproducible artifact and a result in the journal. Separate build, installation, launch, menu, match, network, controls, and sustained-performance claims. After three materially identical failures, change the experiment. Recheck every changed button in its actual menu flow. Do not push private files or generated builds; run `scripts/check-repo-safety.sh` before each commit. Source/documentation fixes may be committed and pushed to the private `main` under Chris's standing direction; no public release is authorized by this loop.

## D1. Make the handoff installable on hardware

1. In Apple Developer, create/check the App ID `dev.halopad.HaloPad` on certificate team `VKDH2T9UTF`, enable Extended Virtual Addressing and Increased Memory Limit, and create a device development profile. Before use, decode the downloaded profile and check App ID, team, certificate, device UDID, expiry, and both entitlements. Never place keys or the profile in Git.
2. While the iPad is absent, retain the verified kit and profile locally. Once it is connected and trusted, inspect the existing bundle/container, preserve `Documents` and `Library`, and install in place using `scripts/install-device.sh --app ... --package ...`. Read back app identity, installed package, and container state. Do not uninstall/reset to make installation work.
3. With the iPhone 14 now connected and explicitly in scope, install and import through its actual Files picker, then reach the main menu. Record the launch log, import result, crash/memory outcome, and screenshot. If the 4 GiB guest reservation fails, capture the exact OS denial before changing code. Repeat the same preservation and acceptance path on the iPad when it is physically present.

**Pass:** signed bundle's entitlements and profile match; installed app launches, imports the matching package, and reaches the menu on the named physical device without losing prior data. A successful `devicectl install` alone is not a pass.

## D2. Make phone and tablet input usable

1. On the iPhone 17 Pro and iPad Air Simulators, walk the full user path: first import; create/select profile; Multiplayer create/join menus; touch move, look, fire, jump, use, reload; pause/leave/rejoin; three-dot menu; touch layout editing; controller hiding/showing; console, server address, chat, and profile-name keyboard fields. Record screenshots for clipped text, small targets, safe areas, keyboard overlap, and loss of input focus. A Simulator permits sequential taps; mark simultaneous two-thumb and physical controller rows pending hardware.
2. Reproduce and fix local-host three-dot **Leave Game**. Verify host and client cases separately, plus no stale touch input after exit. Investigate any keyboard that appears merely from opening the three-dot menu. Keep the menu groups and layout consistent with the existing app.
3. With a real iPhone/iPad and controller, test two-thumb movement/look while firing, menu navigation by touch and controller, controller connect/disconnect during a match, software keyboard typing/dismissal, rotation/app interruption, and recovery without stuck actions. Include a first-time user path with no shell commands after installation.

**Pass:** a new user can import, create a profile, start and leave a game, join a private match, type when needed, and continue after input changes. Each control is accepted by observed game behavior, not appearance alone. Record exact device, OS, map, and input method.

## D3. Prove private online play

1. Restore the original private `haloceded.exe` 1.10 reference server from accepted inputs on a Mac equipped for it, or use an explicitly approved private server. Keep it bound to a controlled address; do not use a public server as this phase's acceptance shortcut.
2. From Halo's own Multiplayer UI, test server discovery or address entry, join, spawn, movement, look, fire, chat, scoreboard, map transition, disconnect/reconnect, and loss recovery on iPad and iPhone. Correlate app frames/logs with server join/leave records.

**Pass:** the named device plays in a private match and survives the tested transitions. Network reachability, a connection message, or a spawned synthetic scene alone is insufficient.

## D4. Decide what an iPhone 14 needs

1. First establish a matched baseline on physical iPhone 14 and iPad: same build, map/scene, resolution, graphics settings, match duration, and thermal starting state. Capture frame-time distribution (median, p95, p99), frame stalls, CPU/GPU time, working/peak memory, memory warnings, launch/import times, thermal state, and battery over a sustained match. Simulator FPS is diagnostic only.
2. Attribute expensive frames before editing. Inspect translated-code dispatch and Windows-service calls, Metal submission/texture decode and streaming, audio, networking, and UIKit overlay separately. The existing per-frame Metal arena is already an optimization; do not count it as future work. Try one small change at a time on the dominant measured cost. Candidate levers include reducing resolution/render workload, avoiding repeated guest/host boundary work, caching stable graphics state, and bounding allocations or map-load spikes. Preserve Halo behavior and network compatibility.
3. Repeat the matched run after each change. Keep a change only if p95/p99 or stability improves materially without broken visuals, input, or multiplayer. If iPhone 14 cannot reserve the required guest address space or is terminated for memory, solve that feasibility gate before frame tuning.

**Pass:** a sustained physical iPhone 14 match meets an explicitly recorded playability target agreed from the baseline; no thermal collapse or jetsam. If it does not, report the measured limiting subsystem and the smallest next experiment. Do not extrapolate from iPhone 17 Pro Simulator or Apple Silicon Mac speed.

## D5. Give the app a real icon

Create an original HaloPad mark that reads at small Home Screen sizes and does not reuse Halo's protected logo/art. Add the required 1024-pixel source and an asset catalog or equivalent icon pipeline to the iOS build. Check iPhone and iPad Home Screen, Settings, Spotlight, light/dark/tinted presentation where supported, and the packaged app's `CFBundleIcons`/asset output. Keep the private handoff kit unchanged; a new source build is required to ship this icon.

**Pass:** the signed source-built app displays the intended icon on both device classes, with no default/blank icon, and the source asset is reproducible from the repository.

## D6. Close the device candidate

Rebuild from accepted original inputs, run targeted native/package/input tests, repeat D1–D4 on that exact build, inspect signing and repo safety, and record a concise device matrix with passes and open limits. Keep private game data outside Git and any public artifact. Do not call the candidate ready for iPhone 14 until its own D1–D4 rows pass.

## Immediate next actions on this Mac

1. The current signed source build is installed in place on the physical iPhone 14. Mirrored primary pointer clicks now navigate Halo's menus. A local LAN Battle Creek match, profile creation using the native Enter / Accept action, and Halo's leave flow through the three-dot menu were observed. Continue direct-finger and controller checks; mirrored pointer drag does not prove two-thumb control feel.
2. The physical iPhone 14 now accepts Halo's 1280 × 720 mode after VA regeneration and the COM/query fixes. Accept the video-test prompt and press **OK** on **Edit Profile Settings** to save it. Relaunch preserved 1280 × 720; a short local Battle Creek match, pause, and leave flow worked with the wider image. Measure sustained frame times and direct-finger mapping before treating this as a final phone display setting.
3. Baseline a sustained physical iPhone local match with frame times and thermal state. A static Battle Creek view showed 30 FPS and a Time Profiler sample found guest dispatch and thread yielding prominent; neither establishes performance during action or loading. Try a small measured optimization only against a matched baseline.
4. Chris's physical iPad Pro 12.9-inch (6th generation) is connected. The stable-controller source candidate is installed in place after a `Documents`/`Library` backup, and existing profile `New001` survived. A local Battle Creek match reached first-person play. The new trace shows a 454 ms gap on match entry and then roughly 30 FPS steady intervals; shader-library and pipeline creation was below reported 0.01 s aggregate time. The physical controller acceptance test is currently gated by iPadOS Bluetooth pairing: the Xbox controller failed to pair in Settings with HaloPad closed and later disappeared from saved devices after user unpairing. Restore OS-level pairing, then test late connect, gameplay actions, video-setting changes, app relaunch, and controller reconnect. Keep `Device Hub` Capture Keyboard off. No live public matches are in scope.
5. Prepare a [PadForge](https://github.com/chrissotraidis/padforge) manifest and game-specific personal builder only after the input profile and source-build pipeline are reproducible from a clean checkout. Keep private inputs and personalized IPA outputs local. Public source and binary publication remain separate rights and audit decisions.

## Suggested `/goal` objective

> Follow `docs/HaloPad-GOAL-LOOP-PHASE3.md` in `/Users/chrissotraidis/GitHub/projectreach`. Work the lowest useful unblocked device-readiness item, keeping Simulator, signing, usability, private-network, performance, and icon evidence distinct. Preserve device and game data; keep `STATUS.md` and `JOURNAL.md` current; run the repo safety gate before commits; push authorized source/docs fixes to private main. Physical iPad and iPhone 14 acceptance require those actual devices.

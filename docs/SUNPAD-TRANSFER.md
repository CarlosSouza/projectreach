# SunPad component transfer (iOS and iPadOS touch controls and menu)

HaloPad's iOS and iPadOS shell adapts SunPad's mobile interaction layer. SunPad is a donor for
behaviour: this file records what came across, so a clean HaloPad checkout never depends on a
local SunPad path. The donor copy used for the work is private and ignored (`ref/sunpad/`).

| field | value |
| --- | --- |
| origin | https://github.com/chrissotraidis/sunpad |
| commit | `e43f0ea6b797e5110787171957c9dc3c6213269c` (main, clean working tree) |
| license | GPL-3.0 for the adapted design; the adapted file names its source |
| adapted into | `port/ios/HaloPadOverlay.m` and `.h` (from `apple/ios/SunPadGameOverlay.mm` and `apple/shared/SunPadSettings.*`) |
| input seam | `halopad_host_post_input`: Windows keys and mouse input, the same stream a keyboard and mouse produce |

## Kept

- The stick view: a circular base and thumb, +y up, reset on release.
- The button behaviour: round, a light border, the pressed scale of 0.92.
- The sizing/editor model: point-sized controls, global size and opacity, and safe-area
  constraints. Current Halo-specific sizes and placement supersede the original donor layout.
- Sparse, per-form-factor persistence of normalized centres and per-control sizes: an absent
  entry keeps the default. The keys are HaloPad's own (`HaloPad.tablet.v1.*`, `HaloPad.phone.v1.*`).
- The layout editor (drag to move, tap to select, a size slider for the selected control, Done)
  and the settings panel (opacity, overall size, hide with a controller, move controls, reset
  this device's layout).
- Settings stay within the safe area on landscape phones. The title and Done button remain
  fixed while the settings rows scroll; rows have a minimum 44-point height. Opening the panel
  releases held touch controls, and its background consumes touches rather than aiming or
  activating gameplay controls underneath it.
- The three-dot menu in the safe-area corner (`ellipsis`, a dark translucent disc), rebuilt after
  each change so checkmarks stay current.
- Controller coexistence: only real controllers hide the touch controls (the Simulator's
  virtual ones do not), and touch state is cleared when one connects.

## Changed

- **The controls are Halo's PC controls**, turned into the keys and mouse input Halo already
  reads through DirectInput, with its default bindings:
  - the move stick is W, A, S and D, digital as Halo's keyboard movement is, with hysteresis
    (on at 0.38, off at 0.25);
  - dragging on open screen looks around (mouse counts, 2.2 per point times Look Speed);
  - FIRE is the left button and GRENADE the right, and both also look while the finger moves;
  - JUMP (Space), CROUCH (left Ctrl), MELEE (F), RELOAD (R), USE (E), SWAP (Tab), ZOOM (Z),
    NADE (G), LIGHT (Q), SCORES (F1) and MENU (Escape).
- **Two-stick layout (2026-09-28 follow-up).** The crowded FIRE ring is replaced by two
  clearly labeled thumb zones and separate action targets:
  - fixed **MOVE** and **LOOK** sticks at the lower left and right; movement uses Halo's
    W/A/S/D bindings with hysteresis, while LOOK produces continuous mouse motion with a radial
    dead zone and a gentle response near centre, timed to the display refresh;
  - **FIRE** centered above LOOK, with RELOAD and ZOOM alongside; USE/MELEE and SWAP/JUMP
    beside aiming. CROUCH and LIGHT sit beside MOVE, with THROW and grenade selection above;
    scores/pause remain at the top;
  - aim by swiping open space or dragging FIRE as well as with the LOOK stick;
  - target size and spacing are constrained together before placement, so enlarging controls
    does not squash them into overlapping circles at the screen edge. Default targets are
    at least 44 points and stay within the safe area. Custom editor positions may still overlap;
  - the left-handed option mirrors the movement and aiming/action groups. Compact/Normal/Spread
    now controls gaps between targets. Labels, opacity, sensitivity, editing and per-device
    sizes remain available through the three-dot menu;
  - positions/sizes use `HaloPad.<device>.v3.*`; the old ring layout is retained under its old
    keys rather than imposed on the new sticks. Reset applies to the current layout version.
- **The controls show only in a game.** The shell reads Halo's current map (`0x643064`) four
  times a second; on its menu map ("ui") the controls hide and touches reach the game view,
  where a tap is a click.
- The centre of the view stays open. Both sticks have symmetric point-based edge offsets,
  rather than moving farther inward as display width grows. Tablet thumb zones sit higher
  along the sides to clear the bottom-left motion tracker; phones retain a lower grip.
  Default target gaps never fall below eight points. Existing v3 custom positions are kept;
  Reset Layout in Touch Control Settings restores these defaults. Stick captions now also
  follow the Labels setting.
- **The menu holds what Halo's own menus do not:**
  - Join Server by Address (and Recent Servers), through Halo's own console command
    `connect ADDRESS "PASSWORD"` (Halo runs with `-console`; typed keys go one every 50 ms so
    Halo's once-a-frame keyboard read sees each);
  - Show Keyboard (the system keyboard types into Halo), Halo Console, Team Chat and All Chat;
  - Display: aspect ratio (Original 4:3 or Stretch to Fill, applied to Halo's layer and to touch
    mapping at once) and an FPS counter (Halo's presented frames);
  - Touch Control Settings, including Look Speed, and Hide Touch Controls;
  - Report a Problem: a privacy-safe text report (app version, device, OS, Halo's map, display
    settings) through the share sheet.

## Left out

- Sunshine's FLUDD analog R trigger, its water animation and the wide R control.
- The GameCube D-pad, its grouped editor, and the "modern C-stick" option: Halo PC has no D-pad
  actions, and looking is a drag.
- SunPad's render-resolution scale, 60 FPS and performance modes: HaloPad renders at Halo's own
  resolution (Halo's video settings choose it).
- SunPad's disc importer and game-data menu: HaloPad's game files come from Halo's installer.
- SunPad's input mixer: HaloPad's controllers go through DirectInput (`port/apple/halopad_gamepad.m`),
  and the touch controls produce keyboard and mouse input.

## Checks

- **Final swipe displacement (2026-09-28):** screen swipes and FIRE/THROW drags retain
  the last consumed point per finger and apply any final movement from `touchesEnded`.
  Cancellation, menus/keyboard and focus loss discard unfinished gestures. This fixes short
  swipes that produce no intermediate move callbacks, without double-counting normal swipes.
  `G9/overlay-20260928T152752Z` passes 44 input/layout assertions, 90 layouts and five renders,
  including interleaved fingers, final-only movement and cancellation of surface/FIRE drags.
- Actual iPad routing evidence: `G9/touch-routing-20260928T152054Z`. Opt-in
  `HALOPAD_TRACE_TOUCH=1` (also forwarded by the app builder) reports phases, geometry and
  timestamps only. CUA's drag delivered direct touch begin/end in 0–1 ms with **no moved
  callbacks**, explaining why it cannot verify a held stick. After the fix, the same screen
  swipe visibly turns Halo; an opposite FIRE drag turns back and fires (battery 100→99).
  The app starts automatically from the migrated stock folder on clean relaunch. Current
  preview PID 59280 has this build. Actual held-stick/multi-touch feel still requires device
  interaction; short input edge retention across runtime pumps remains a separate open row.

- **Aligned action grid and stick travel (2026-09-28):** the six actions beside LOOK now
  share two columns with equal horizontal/vertical pitch. Their lower two rows centre on
  the stick; FIRE remains above it with a full gap. LIGHT and NADE use the same target size
  as the other secondary actions. The stick thumb follows the finger up to its visual rim,
  where input reaches full deflection; dragging farther clamps radially. Saved v3 layouts
  remain intact. `G9/overlay-20260928T150807Z` passes 36 input/layout assertions, all 90
  layout combinations and five native renders. Four new checks exercise actual stick geometry.
- Installed this build on the iPad and reached Battle Creek through the original menus.
  `G9/touch-preview-20260928T150830Z` passes all five handler-driven gameplay checks
  (move, swipe look, fire, jump, held LOOK). Actual touch Pause/Resume works and controls
  hide/restore. CUA drag attempts did not establish clear movement/aim acceptance; actual
  drag routing, simultaneous touches and physical comfort remain open. This is not a claim
  of full touch acceptance. The current Simulator now shows the revised layout.

- Reach/spacing refinement: `G9/overlay-20260928T141747Z` passes 31 input/layout assertions
  and 90 combinations, including eight-point target gaps, equal stick reach, fire alignment,
  movement-side crouch and a conservative tablet radar keepout. Five native UIKit offscreen
  PNGs are included. The full app builds; the live user preview was not replaced. This pass
  does not establish physical ergonomics or new in-game acceptance.

- Two-stick layout checks: `G9/overlay-20260928T114719Z` on iPad and
  `G9/overlay-20260928T114935Z` on iPhone, under `docs/artifacts/2026-09-28/`. Both pass 20 input/
  layout assertions, including 90 combinations of five landscape bounds, three sizes, three
  spacings and both hands, with simulated safe-area insets. All default target rectangles remain
  separate, at least 44 points, and within those insets; both stick centres hit their own views.
  Held LOOK generates repeated motion; clearing it stops motion; its dead zone does not drift.
- Actual Halo gameplay: iPad `G3/ios-app-20260928T114531Z` and iPhone
  `G3/ios-app-20260928T114750Z` pass move, swipe aim, fire, jump and held LOOK checks. LOOK turns
  17.1° and 18.7° respectively over a half-second handler-driven hold. Simulator screenshots were
  inspected in landscape. This does not establish simultaneous physical multi-touch or analog
  movement speed: MOVE still emits Halo's keyboard bindings. The tablet sticks were subsequently
  moved inward to clear the motion tracker, and the final preview is recorded in JOURNAL.md.
- `.venv/bin/python scripts/test-ios-overlay.py --device UDID` runs the real overlay timer and
  control handlers in a separate process on a booted Simulator, capturing their outgoing host
  events. It loads no game core, presents no window, and sends no input to the running preview.
  It checks uppercase/Enter ordering, interruption while Shift and a letter
  are held, cancellation before delivery, ignored inactive input, no replay after resume,
  fresh typing, movement/fire release and discarded fractional look motion. This is an input
  boundary test, not proof of UIKit multi-touch routing or Halo observing every short press.
- On scene deactivation, unfinished typed text is canceled and keys already delivered by the
  text queue are released. Reactivation starts with an empty queue. Clearing touch controls
  also resets fractional look motion. Evidence: `docs/artifacts/2026-09-28/G9/overlay-20260928T113413Z`
  (16 checks pass). The corresponding iPad gameplay smoke is
  `docs/artifacts/2026-09-28/G3/ios-app-20260928T113459Z`: movement/look/fire/jump pass and a
  Home/foreground cycle with the native keyboard open resumes at 30 fps in the same process.
- `scripts/build-ios-app.py --scene tests/halo_app_scene.c --launch` runs Halo from its main menu
  in the app. With `HALOPAD_ARGS='-connect ADDR:PORT'` it joins that server; the screenshot shows
  the game and the controls. `HALOPAD_OVERLAY_DEMO=settings|layout` opens the settings panel or the
  layout editor for an unattended screenshot.

### Keyboard and touch routing follow-up

The app's layout pass now resizes only guest CAMetalLayers. It previously also resized the
invisible keyboard proxy's UIView layer to the entire screen. That proxy now rejects hit tests
explicitly. Console down/up reaches Halo's DirectInput read (`G3/ios-app-20260928T115440Z`),
and its small bottom-left prompt is visible. General short-tap reliability is not closed by
this trace. The next iteration resolved the presentation problem: Simulator's connected hardware
keyboard was suppressing the software keyboard. With it disconnected, the docked keyboard
appears and the render host now fits Halo above it, preserving proportions. Dismissal restores
the full view and saved aspect choice. Pointer conversion uses the same viewport. Actual
keyboard taps typed and erased `help` in the local console (`G3/ios-app-20260928T121340Z`).
Floating/split keyboard occlusion and physical-device behavior remain unverified.

A native **Hide Keyboard** accessory works on phone and tablet. Showing the docked keyboard
releases MOVE, FIRE and LOOK and hides gameplay controls; dismissal restores their prior
visibility settings. The keyboard's reported offscreen frame restores the full view even if
iOS retains an accessory-height layout guide. The boundary suite covers these input releases
and visibility transitions in addition to the 90 default layout combinations.

For Simulator touch testing, use **I/O → Keyboard → Connect Hardware Keyboard** to disconnect
the simulated hardware keyboard if Show Keyboard produces no visible keyboard. This was the
cause of the earlier zero-height keyboard traces; it is a Simulator setting, not a Halo key
check. The project iPad and iPhone previews were tested with it disconnected.


### Pause and child menus

Gameplay controls now follow the active Halo widget as well as the map. Opening Pause releases
held movement, fire and continuous look, hides both sticks/actions and passes the surface through.
The top Pause target becomes a Back chevron, returning one menu level or resuming the game.
It is also hidden while the software keyboard is visible. Halo's state is sampled on its frame
thread and ownership changes are delivered to UIKit; this does not infer pause from mouse capture
or stop an online game's simulation.

Verified in a menu-started local Battle Creek match on iPad (`G3/ios-app-20260928T125447Z`),
including a nested Game Options menu and touch Back/resume. The boundary suite passes 30
assertions and 90 layout combinations. Direct finger taps now position Halo's own cursor before sending a click. The menu adapter
converts the touch through the displayed viewport into Halo's 640×480 UI coordinates, then
supplies ordinary relative mouse input with correction against the observed cursor. It does
not write guest state or replace Halo's menus. Short taps keep separate press/release frames;
menu changes and interruptions discard pending gestures and release a held button.

Verified on the final iPhone build (`G3/ios-app-20260928T131954Z`): sidebars ignore touches,
Settings → Audio Setup opens directly, and small master-volume arrows change 10→9→10.
Final iPad (`G3/ios-app-20260928T132504Z`) completes Multiplayer → Create Game LAN →
Battle Creek → Slayer → Start Game entirely by touch, then Pause → Game Options → original
Back → original Resume Game. Controls hide in menus and reappear in gameplay. These routes
no longer require the Simulator hardware keyboard.

The adapter passes seven gesture-sequence tests and 72 cursor routes against the original x86
menu routine, converging within two correction frames (`G9/menu-touch-20260928T131724Z`).
General very short gameplay-key edges, actual simultaneous multi-touch, the alternate guest
OS-cursor path and physical-device ergonomics remain unverified.


### Short FIRE/THROW taps across host pumps

The overlay tags virtual mouse buttons separately from physical mouse input. A quick down/up
previously disappeared if Halo pumped input twice before its unbuffered mouse read. A failing
DirectInput regression reproduces this with eight pumps (`G9/touch-mouse-edges/red-*`).
The runtime now retains alternating virtual edges per button, advancing once per successful
state read. Invalid reads, Poll and keyboard reads do not consume them. Duplicate presses are
ignored; held state persists normally; physical mouse holds remain independent. Buffered mouse
clients retain their normal event stream without an additional state replay. Pending edge counts
are bounded to 1024 per button; overflow drops a complete pair to preserve final release parity.

The three-dot button uses UIKit’s pre-presentation menu event to clear held input. Native UI
takeover explicitly cancels unread virtual edges and queued touch-button events.
Cancellation is delivered before normal queued events and preserves physical input. Focus loss
and reacquisition clear the runtime backlog. This closes the reproduced virtual mouse case;
it does not establish general short keyboard-edge behavior or simultaneous physical multi-touch.

Verification: 80 DirectInput assertions (`G3/core-arm64-apple-ios17.0-simulator-20260928T154845Z`),
74 USER32 assertions (`G3/core-arm64-apple-ios17.0-simulator-20260928T154751Z`), and 48 overlay
assertions plus 90 layouts/five renders (`G9/overlay-20260928T155429Z`) pass on the iPad Simulator.


### Stick ownership across interruption

Each stick now retains the touch that began its hold. Only that touch may update or end it.
Reset clears ownership before releasing input, so late moves after a native menu/lifecycle
interruption cannot restart movement or the LOOK timer. A previous touch ending also cannot
release a newly started hold. Normal UIKit single-touch-per-control behavior stays in place.

Nine new handler-boundary checks exercise independent MOVE/LOOK holds, releasing either while
the other remains active, reset followed by late moves, unrelated touch events, cancellation,
and new ownership after reset. They use explicit test tokens and the real view callbacks;
they do not synthesize OS input or establish hands-on simultaneous multi-touch acceptance.
Four ownership cases failed before the fix (`G9/overlay-20260928T160258Z`). The final suite passes
57 input/layout assertions, 90 geometry combinations and five renders (`G9/overlay-20260928T160454Z`).


### Buffered keyboard taps reach Halo's consumer

The translated keyboard update at `0x493520` already preserves down/up in one update: it sets
Halo's key state to pressed and records a deferred release, which the next update clears.
Do not copy the virtual mouse workaround into this buffered keyboard path or add timed holds.

The DirectInput harness now connects its keyboard fixture to the original consumer and checks
all eleven keyboard-backed controls (JUMP, RELOAD, USE, MELEE, SWAP, ZOOM, CROUCH, LIGHT, NADE
selection, scoreboard and pause). Each down/up pair is pumped eight times before the consumer
runs. Pressed state and deferred release appear once, clear on the next update and remain clear
on a third. All 135 assertions pass on iPad Simulator (`G9/short-key-consumer`). Fixture globals
are changed only inside the test process; the runtime/translated code is unchanged.

The in-game handler selftest now requires one immediate JUMP press/release, removing the former
300 ms hold and optional second attempt. This checks the first tap honestly. Consumer checks
prove delivery, not the gameplay outcome of every action or simultaneous physical touch routing.
Keyboard source ownership and interruption were subsequently addressed below; multiple taps
within a single consumer update remain separate acceptance work.

The stricter JUMP check passes in the rebuilt device-data iPad app: height -0.22→0.44 after
one immediate handler down/up, with all five gameplay checks green (`G3/ios-app-20260928T161826Z`).
This remains a handler-driven game-state check, not an actual simultaneous-finger claim.


### Cancel pending touch keys when native UI takes over

Gameplay key edges now identify their touch source, independently of hardware and queued text.
The overlay posts cancellation after releasing buttons and both sticks, so the host can remove
all pre-cancel gameplay edges. Events posted afterward remain usable.

DirectInput preserves source ownership and tracks the buffered reader's observed key state.
Cancellation removes unread virtual events and reconciles affected keys with surviving physical
holds. USER32 applies the same policy to queued key messages and async state. Already-read
holds get a release; unread taps disappear. Focus loss cancels virtual keys before deactivation.
Physical buffers retain their existing acquisition behavior; this is not a blanket queue flush.

Four failures reproduced the original bug (`G9/touch-key-cancel/red`). The final iPad DirectInput
suite passes 156 assertions, including Halo's actual keyboard consumer after cancellation,
both physical/touch ownership orders, either-source release, focus/reacquisition, unrelated
physical taps, and fresh input after cancellation. USER32 passes 85 checks, including queued
messages, observed holds, repeated cancellation and unrelated typed characters. The overlay
passes 60 assertions, 90 geometry combinations and five renders (`G9/overlay-20260928T163258Z`).
Actual simultaneous fingers, physical ergonomics, rapid repeated actions and broader lifecycle
acceptance remain open.

The rebuilt device-data preview (`G3/ios-app-20260928T163515Z`) passes all five handler/game-state
checks: movement 3.16 units, swipe 39.7 degrees, battery 1.00→0.89, immediate jump -1.36→-0.70,
and held LOOK 25.7 degrees. Actual UI taps also open/dismiss the native menu and Pause/Resume.


### Check action outcomes in Halo, including player melee

`HALOPAD_ACTION_SELFTEST=1` runs an opt-in handler-driven action sequence after entering a
local match. It presses and releases MELEE immediately, approaches a loose magazine-fed
weapon using MOVE/LOOK, holds USE when offered, taps SWAP, fires, then taps RELOAD. It reads
Halo's inventory, equipped weapon, magazine and player-melee timer; it never creates pickups
or writes gameplay state. It takes precedence over the movement selftest so two drivers do
not compete. The pickup route is bounded and can fail on geometry after a random spawn;
that reports an incomplete fixture, not a passing action sequence.

The old host-suite melee assertion read a whole word at `unit+0x2ac`, including an adjacent
animation index that is already nonzero at idle. That was a false positive. An intermediate
diagnostic also watched the wrong path: `+0x289` and animations 0x1e/0x1f describe unit/AI
melee. The reference repo's `bipeds.c` separates player melee; the locked PC
executable starts its timer byte at `+0x505` in `0x55d226`, applies damage at the attack tick,
and decrements it in `0x55d263`. Both acceptance checks now use the player timer.

Initial action runs verified pickup, SWAP, magazine FIRE (20→15) and RELOAD (15→20); the
corrected player-timer run verified an immediate MELEE tap (0→25→0), but could not complete
its pickup route. Logs, including wrong-predicate failures, remain in `G9/touch-actions`.
These are handler/game-state outcomes, not proof of actual simultaneous-finger routing or
physical comfort. They do not establish melee damage against another player.

The final build completes all five checks in one run (`accepted-stderr.txt`, PID 73862):
MELEE 0→25→0, USE places the offered weapon in the second inventory slot, SWAP changes the
equipped handle, FIRE consumes 60→51 rounds, and RELOAD restores 51→60. The full iPad host
suite also passes with the corrected melee assertion (`G3/core-arm64-apple-ios17.0-simulator-20260928T171348Z`).
Final actual UI checks: swipe changes the view; the native three-dot menu opens and dismisses.
The previous route failures remain relevant to harness reliability.


### Proportional MOVE: original input path verified, overlay integration open

MOVE still emits digital WASD. Before replacing that path, `halo_dinput_test` now follows
controller values through original `0x493520` (DirectInput polling and signed-axis storage)
and `0x48f850` (binding evaluation and movement throttle). Original setter `0x48e360` accepts
four axis-direction bindings plus an independent W binding. Keyboard and pad motion combine,
and releasing either source preserves the other. All fixture configuration is test-only.

The iPad suite passes 175 assertions (`G3/core-arm64-apple-ios17.0-simulator-20260928T173438Z`).
With Halo's requested 10% DirectInput dead zone and a fixture movement threshold of 1,
quarter/half/full forward yield 0.166748/0.444336/1.0. The original x86 consumer matches all
ten native samples bit-for-bit, including neutral, dead zone, reverse, strafe, diagonal and
release (`G9/analog-oracle-20260928T173451Z`). Reproduce the comparison with:

```sh
.venv/bin/python scripts/test-analog-movement.py --native-evidence docs/artifacts/2026-09-28/G3/core-arm64-apple-ios17.0-simulator-20260928T173438Z
```

Next: give touch movement its own cancelable analog input source and stable controller identity,
configure it through Halo's existing input mechanisms without replacing physical-controller or
keyboard mappings, then verify partial/full movement in gameplay and source handoff. The fixture
assigns a logical pad slot and unit thresholds directly; production must not copy those writes
or assume every user's thresholds equal 1. Enumeration/activation, profile changes, cancellation
before the next poll, reconnect and actual two-finger operation still need implementation and
acceptance. No app behavior changed in this investigation.

### Compact action spacing and canceled look (2026-09-28)

Small actions now use their own diameter plus the selected gap. The larger FIRE target
has independent clearance above LOOK, instead of inflating the pitch of the entire grid.
Default gaps are 8/12/18 points before group scaling; FIRE uses a quieter blue treatment.
Both sticks retain their shared baseline and equal edge reach. Existing v3 custom origins
and sizes remain intact. The real UIKit renders pass 90 phone/tablet, size, spacing and
handedness combinations, including 44-point minimum targets and tablet radar clearance.
Evidence: `G9/overlay-20260928T174631Z` (61 assertions plus the layout matrix).

Touch-generated mouse motion now carries source ownership. Opening native UI removes
older touch motion from the host queue, including behind a physical-key barrier. DirectInput
also subtracts already-delivered but unread touch deltas and removes buffered touch motion,
while preserving hardware deltas and new motion posted after cancellation. Consumed motion
cannot be subtracted twice. Four failing regressions were reproduced before the fix in
`G3/core-arm64-apple-ios17.0-simulator-20260928T174609Z`; that run also exposed a test-only
physical-W event leaking into the subsequent analog fixture. The test now consumes that
unrelated tap explicitly. Final DirectInput run passes all 181 assertions:
`G3/core-arm64-apple-ios17.0-simulator-20260928T174735Z`.

The installed iPad development scene reaches Battle Creek through Halo's touch menus and
passes five handler-driven outcomes: movement 2.38 units, swipe 39.7 degrees, FIRE battery
1.00→0.89, immediate JUMP height -0.22→0.42, held LOOK 24.7 degrees. Actual UI swipe and
three-dot open/dismiss also work. Logs and pre-install state backup:
`G9/touch-spacing-look-cancel`. These checks do not establish simultaneous physical fingers
or handheld ergonomics. MOVE remains digital WASD; proportional integration remains open.

### Cancelable analog source and multiplayer movement (2026-09-28)

`HPI_TOUCH_MOVE` supplies absolute axes to a distinct virtual DirectInput device,
HaloPad Touch Move. It has a stable identity separate from physical controllers.
The UIKit queue keeps the newest axes in a separate lane, so a full event queue
cannot swallow release. Cancel clears queued axes and already-polled snapshots;
focus loss discards inactive motion, and physical pad state survives touch cancel.
The regular entry point leaves this device disabled. The explicit development scene
`tests/halo_touch_move_scene.c` enables it, finds an unused/unbound logical slot,
and calls Halo's own `input_activate_joy` evaluator and binding setter. No input
mapping or movement field is directly overwritten. Production ownership, rollback,
profile changes, map reload and reconnect acceptance are still required.

The live scene initially failed a faulty expectation that the analog consumer's
partial value would remain unchanged until Present. Captured input replayed at
`0x48f850` returns 0.222330734 in both original x86 and native ARM64. Temporary
write tracing then identified the second writer: player-command builder `0x473c70`
uses original `0x473c30` when the word at `0x6b47b0` is nonzero (observed mode 2).
That routine maps values above +0.05 to +1, below -0.05 to -1, and the inclusive
middle interval to zero. Thus partial polling is real, but multiplayer walking
is intentionally quantized. No game behavior or translation was changed.
A Present may observe either stage; the corrected live test checks the raw axis
and the original permitted stage, rather than asserting smooth walking speed.

Evidence under `docs/artifacts/2026-09-28/`:

- `G3/core-arm64-apple-ios17.0-simulator-20260928T182711Z`: 210 assertions pass.
- `G3/core-arm64-apple-macosx14.0.0-20260928T182902Z`: 206 assertions pass.
- `G9/analog-oracle-20260928T182753Z`: ten input and twelve quantization cases
  match original x86 bit-for-bit, including both threshold boundaries.
- `G9/touch-analog-device/app9-*`: original-menu Battle Creek; five gameplay
  outcomes plus six axis/stage samples and cancellation pass. Actual three-dot
  open/dismiss and a swipe beside the LOOK corner were also observed.
- `G9/touch-analog-device/partial.bin` and its replay report preserve the original
  misleading end-of-frame observation. `scripts/replay-analog-capture.py` and
  `tests/halo_analog_capture_test.c` replay this data at the consumer stage only;
  neither claims that end-of-frame data is function-entry state.
- `G9/overlay-20260928T182808Z`: 67 assertions, five renders and 90 layouts pass.
  Both sticks highlight an owned touch and relinquish invisible square corners
  to the look surface. Dragging an already-owned stick outside its rim still works.

The latest preview uses the explicit analog scene; the regular scene retains WASD
movement. The profile file hash is unchanged from the retained pre-game backup;
playlist/last-map files changed normally. This is not persistence or profile-switch
acceptance. Next: verify original configuration across profile/map transitions,
retain ownership only of the spare touch slot, and restore/fall back without
modifying physical mappings before enabling the source by default. Actual two-finger
routing and physical handheld ergonomics remain open.

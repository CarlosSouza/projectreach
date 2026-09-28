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
- The sizing model: fixed sizes on iPads at least 1000 points wide, and a scaled 800 x 380
  reference elsewhere (stick 172 or 126, buttons 62, 76 and 104 or 46, 58 and 78). A global size
  and opacity sit on top, and positions are clamped to the safe area.
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
  - **FIRE**, RELOAD, ZOOM and SWAP in a row above aiming; USE/MELEE and CROUCH/JUMP beside it;
    THROW, grenade selection and LIGHT above movement; scores/pause remain at the top;
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
- The centre of the view stays open. Tablet thumb zones sit farther inward to give the
  bottom-left motion tracker space; phone HUD clearance is checked separately in screenshots.
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
  control handlers on a booted Simulator, capturing their outgoing host events. Stop the game
  candidate first. It checks uppercase/Enter ordering, interruption while Shift and a letter
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
assertions and 90 layout combinations. Direct touch positioning of Halo's original menu cursor
and very short input edges remain open; use the connected hardware keyboard for menu navigation
in Simulator for now. Disconnect it again when previewing the software keyboard.

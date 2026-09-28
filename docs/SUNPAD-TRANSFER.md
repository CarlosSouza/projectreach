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
- **The look (2026-09-28 redesign).** SunPad's coloured, text-labelled buttons read poorly over Halo's
  HUD, so the controls became a mobile shooter's layout:
  - a **floating stick**: a finger landing in the lower left (left 40%, below the top 30%) brings
    the stick under it, and lifting returns it to a faint resting place clear of Halo's motion
    tracker;
  - **FIRE** large and low on the right (a muted red glass disc, 128 points on iPad, 94 on phone),
    with **USE, SWAP, ZOOM, THROW, MELEE, RELOAD, CROUCH and JUMP on a ring around it** (every
    40 degrees), all in reach of the right thumb;
  - every control the same dark translucent glass circle with an SF Symbol and a small caption;
    held controls brighten; LIGHT and NADE small on the left edge; scores and pause small at the
    top centre;
  - saved layouts moved to `HaloPad.<device>.v2.*`, so positions saved for the old layout do not
    land on the new one.
  - options a mobile shooter has: **Left-handed** (the stick on the right, FIRE and its ring on
    the left, right of Halo's motion tracker), **Spacing** of the ring (compact, normal, spread)
    and **Button Labels** on or off; icons and captions carry a soft shadow for bright scenes.
- **The controls show only in a game.** The shell reads Halo's current map (`0x643064`) four
  times a second; on its menu map ("ui") the controls hide and touches reach the game view,
  where a tap is a click.
- Default positions keep clear of Halo's HUD: its ammo and shield displays in the top corners
  and its motion tracker in the bottom-left corner.
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

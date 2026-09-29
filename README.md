# HaloPad

Halo: Custom Edition 1.10 on iPhone and iPad, playing online with PC players.

HaloPad is not an emulator. The original `haloce.exe` was translated ahead of time into native
ARM64 code, and HaloPad supplies the Windows, Direct3D 9, DirectInput, DirectSound and Winsock
services it expects on top of Metal, UIKit and GameController. It speaks Halo's own network
protocol, so it joins the same Custom Edition servers PC players use today.

HaloPad contains no game data. You need your own copy of Halo Custom Edition.

## What works

- **Online play.** Halo's own Internet Lobby lists the live public servers; HaloPad has joined
  populated public games and played full matches on private servers.
- **Touch controls.** Two sticks, a FIRE button that also aims, and Halo's actions in thumb reach,
  on both iPhone and iPad. Every control can be moved and resized, and controls follow the
  bindings in Halo's own Controls Setup.
- **Game controllers.** Connect an Xbox, PlayStation or other MFi controller and play: HaloPad
  gives it Halo's own Xbox controller layout the first time, and the touch controls step aside.
  In menus, the D-pad moves, A selects and B goes back.
- **Keyboard, mouse and trackpad** on iPad.
- **The three-dot menu** for what Halo's menus don't cover: join a server by address, recent
  servers, leave the game, controls, chat and the console, display options, custom maps, and
  reporting a problem straight to GitHub Issues.

## Controller layout

| Control | Action | Control | Action |
|---|---|---|---|
| Left stick | Move | Right stick | Look |
| RT | Fire | LT | Throw grenade |
| A | Jump | B | Melee |
| X | Reload | Y | Switch weapon |
| RB | Use, pick up, enter vehicle | LB | Switch grenade |
| Left stick click | Crouch | Right stick click | Zoom |
| D-pad up | Flashlight | View / Menu | Scores / Pause |

Change any of it in Halo's **Settings → Controls Setup**; your layout is saved with your profile.

## Install

With an iPad or iPhone plugged into your Mac and a development signing profile:

```sh
scripts/install-device.sh --identity "Apple Development: Your Name (TEAMID)" \
  --profile HaloPad.mobileprovision --game "/path/to/Halo Custom Edition"
```

Then open HaloPad and choose the package it copied over. Installing on a Mac that does not
hold the build (for example a second Mac): see "Installing from another Mac" in the guide. Details, signing setup and a
first-run checklist: **[Installing on iPhone or iPad](docs/INSTALL-IPHONE.md)**.

## Build from source

```sh
python3 -m venv .venv
CMAKE_POLICY_VERSION_MINIMUM=3.5 .venv/bin/python -m pip install -r scripts/requirements-tools.txt
scripts/bootstrap-sources.sh
scripts/doctor.sh
.venv/bin/python scripts/build-ios-app.py --iphoneos      # device build, writes HaloPad.ipa
.venv/bin/python scripts/build-ios-app.py --launch        # iPad Simulator
```

The CMake setting lets the pinned Unicorn source package build with current CMake versions.

On an iPad Simulator with iPadOS 26, choose **Settings → Multitasking & Gestures → Full Screen
Apps**; otherwise every app, HaloPad included, opens in a resizable window.

## Status and limits

- Built and tested on the iPad and iPhone Simulators; the first physical-device run is pending.
- Halo checks the product ID its original installer writes from your CD key. Development builds
  start through a test scene; see [status](docs/STATUS.md).
- Custom maps: add `.map` files with **three-dot menu → Add Custom Maps…**. Client mods
  (Chimera, OpenSauce, HAC2) are Windows DLLs and cannot load.
- The retail campaign (`halo.exe`) is a separate, later target.

Engineering detail lives in [status](docs/STATUS.md), the [journal](docs/JOURNAL.md), the
[runtime notes](docs/G3-RUNTIME.md) and the [touch controls notes](docs/SUNPAD-TRANSFER.md).
Game inputs and generated code stay under ignored `ref/` and `generated/`; see
[rights status](docs/RIGHTS-STATUS.md). Problems: [open an issue](https://github.com/chrissotraidis/projectreach/issues/new/choose).

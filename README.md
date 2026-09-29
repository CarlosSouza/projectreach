# HaloPad

**Halo: Custom Edition on iPhone and iPad, built from ahead-of-time translated game code.**

HaloPad is the current app name for Project Reach. It is an experimental, privately tested
port, not an official Halo release. The repository contains the Apple app, compatibility
runtime, and build tools. It does not contain Halo's executable, maps, sounds, saves,
product key, or translated game code. Bring your own legitimate Halo Custom Edition 1.10
files. No public IPA is available from this repository.

The Windows game's 32-bit code is translated to native ARM64 ahead of time. HaloPad then
provides the Windows, Direct3D 9, input, audio, and network services the game expects,
with Metal rendering on Apple devices. This is a game-specific compatibility runtime,
not a general Windows emulator.

## Current status

The source-built app is signed and installed on a physical **iPhone 14**. It imported its
matching 87-file package, reached Halo's main menu, and the player joined a local LAN
match. The player reports slow loading and gameplay. Touch look speed and screen coverage
are being refined. Profile-name entry and the local-host leave path now pass a physical
iPhone test. The Internet server list refreshes, but full phone online, controller,
and sustained-performance
acceptance is still in progress.

An iPad Simulator reached a local match, and earlier iPad work joined public Custom
Edition servers. A **physical iPad has not yet been tested in this round**. The iPhone
result does not establish iPad performance or usability. See [current status](docs/STATUS.md)
and the [device-readiness loop](docs/HaloPad-GOAL-LOOP-PHASE3.md) for evidence and open gates.

| Area | Current result |
| --- | --- |
| Game data | Own Custom Edition 1.10 files; package matched to the exact build |
| iPhone | Physical iPhone 14 import, menu, and local LAN play observed; performance and controls need work |
| iPad | Simulator gameplay observed; current physical-iPad test pending |
| Multiplayer | Native lobby/network path exists; current phone acceptance open |
| Input | Touch and controller mappings exist; real phone touch is being tuned |
| Distribution | Private engineering build; no public app or game-data download |

## Playing

After a development build is installed, choose its prepared `.halopad.zip` package from
HaloPad's first-run screen. Create a Halo profile, then use the game's own Multiplayer
menus to host or join. The three-dot button exposes touch settings, keyboard and chat,
server address entry, display choices, custom-map import, and help. **Controls → Look
Speed & Touch Settings** adjusts touch look; **Controller Guide** shows the mapping.
The iOS keyboard has an **Enter / Accept** action for Halo dialogs.

The touch overlay can be moved and resized. A supported game controller uses Halo's
controller layout, and the overlay can hide while a controller is connected. Final feel,
all menu paths, and controller connection changes still need physical-device checks.
**Open Leave Game Menu…** opens Halo's original pause menu; select **Leave Game** there.

Custom `.map` files can be added through **three-dot menu → Add Custom Maps…**. Windows
client DLL mods such as Chimera, OpenSauce, and HAC2 do not load in this app. The retail
campaign executable `halo.exe` is a separate future target.

## Build and install

You need macOS, Xcode, this repository's pinned tools, your own supported game files,
and an Apple development profile for the app's bundle ID. The profile must allow
**Extended Virtual Addressing** and **Increased Memory Limit**, because the runtime
reserves Halo's 32-bit guest address space. Building from a fresh clone also requires
private input preparation; the repository alone cannot produce a playable IPA.

Start with `scripts/doctor.sh` and follow [Installing on iPhone or iPad](docs/INSTALL-IPHONE.md)
for signing, build, device install, package preparation, and first launch. An in-place
update should preserve the same bundle ID and existing app data. Do not uninstall an
existing app to update it.

The app icon is built from [HaloPadIcon.svg](assets/HaloPadIcon.svg) and the checked-in
asset catalog. The signed iPhone build returns the intended icon from iOS; physical iPad
Home Screen appearance remains to be checked.

## Questions and limits

**Can I download an IPA?** No public release is available. This is an active private
development build. An IPA by itself would not contain the game files.

**Does it work on an iPhone 14?** It launches, imports, reaches the menu, and has played
a local LAN match. Loading and gameplay can be slow. A reliable frame rate, thermal
behavior, and a complete comfortable control flow have not been established.

**Can it play with PC players?** The app uses Halo Custom Edition's network protocol,
and prior development builds joined public PC servers. Current iPhone 14 online play
and reconnect behavior still need a full test on this exact build.

**Will updates keep my profile?** In-place development installs have preserved the app
container in testing. Back up `Documents` and `Library` before changing signing or build
identity; [the install guide](docs/INSTALL-IPHONE.md) explains the device path.

**Why does resolution say 800 × 600?** That is Halo's internal rendering resolution.
The original 4:3 presentation keeps geometry correct on iPhone. The app's Fill option
stretches it. Halo lists 1280 × 720, but applying that mode exits the current iPhone 14
build to the Home Screen; it recovers on relaunch. True, undistorted widescreen is open.

## Project map

- [Install guide](docs/INSTALL-IPHONE.md): signing, packaging, and device setup.
- [Status](docs/STATUS.md) and [journal](docs/JOURNAL.md): tested builds and observations.
- [Phase 3 goal loop](docs/HaloPad-GOAL-LOOP-PHASE3.md): iPhone/iPad acceptance plan.
- [Execution model](docs/EXECUTION-MODEL.md): translation coverage and guest dispatch.
- [Rights status](docs/RIGHTS-STATUS.md): input, generated-code, and publication boundaries.

## Credits and legal

Halo: Custom Edition and Halo belong to their respective owners. This project is
independent and is not affiliated with or endorsed by Microsoft, Xbox, or Halo Studios.
HaloPad does not grant rights to distribute or download their game content.

The translation pipeline builds on [SR](https://github.com/M-HT/SR). Runtime research
also uses [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) and other pinned
dependencies. Ogg/Vorbis are by Xiph.Org contributors. Their separate licenses and
notices apply to their code; this repository does not claim a blanket license over
upstream work or Halo content. See [rights status](docs/RIGHTS-STATUS.md) before any
distribution decision.

For a reproducible problem report, include the device, iOS/iPadOS version, build, map,
what you tapped, and whether a controller was connected. Use the app's **Report a
Problem** action or [open an issue](https://github.com/chrissotraidis/projectreach/issues/new/choose).

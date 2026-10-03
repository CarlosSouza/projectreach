# HaloPad

<p align="center"><img src="assets/Assets.xcassets/AppIcon.appiconset/AppIcon.png" alt="HaloPad icon: a teal orbital arc around an amber star" width="128"></p>

<p align="center">
  <strong>Halo on iPhone and iPad: Custom Edition multiplayer and the original Xbox campaign.</strong><br>
  The PC game's code translated to ARM64 ahead of time, plus a preview of the Xbox edition, with touch controls, controllers and real networking.
</p>

<p align="center">
  <img alt="iOS and iPadOS 17 or later" src="https://img.shields.io/badge/iOS%20%2F%20iPadOS-17%2B-0A84FF?logo=apple">
  <img alt="Metal renderer" src="https://img.shields.io/badge/renderer-Metal-5E5CE6">
  <img alt="Ahead-of-time x86 to ARM64 translation" src="https://img.shields.io/badge/x86-ahead--of--time%20to%20ARM64-FF9F0A">
  <img alt="Direct3D 9 on Metal" src="https://img.shields.io/badge/Direct3D%209-on%20Metal-30D158">
  <img alt="Game data not included" src="https://img.shields.io/badge/game%20data-not%20included-FF453A">
  <img alt="Status: developer preview" src="https://img.shields.io/badge/status-developer%20preview-FFD60A">
  <a href="https://discord.gg/xwHfUD2bxW"><img alt="Join the community on Discord" src="https://img.shields.io/badge/Discord-Join%20the%20community-5865F2?logo=discord&amp;logoColor=white"></a>
</p>

![HaloPad at Halo's main menu in the iPad Simulator, with the HaloPad three-dot menu button in the corner](docs/images/halopad-menu.jpg)

*HaloPad at Halo's own main menu (iPad Simulator development build). Physical iPhone 14 and iPad Pro builds have also played local matches; see [Current status](#current-status).*

**[What is it](#what-is-halopad) · [Status](#current-status) · [Playing](#playing) ·
[Build it](#build-and-install) · [FAQ](#frequently-asked-questions) · [Discord](https://discord.gg/xwHfUD2bxW)**

> [!IMPORTANT]
> **Bring your own game.** HaloPad needs your own legitimate copy of Halo: Custom Edition 1.10 for PC,
> and for the Xbox edition your own Halo: Combat Evolved Xbox disc image.
> This repository contains no Halo executable, maps, sounds, saves, product key, engine source or
> translated game code.
>
> **Developer preview.** No prebuilt IPA is available. HaloPad is built on a Mac from your own game
> files and signed with your own Apple development profile. It is playable on real hardware today,
> but frame pacing, touch feel and online play on phones are still being tuned.
>
> **AI disclosure:** HaloPad is developed with substantial AI assistance for code, testing,
> documentation and debugging. The [status log](docs/STATUS.md) records what has actually been
> checked, and on which device.

**Questions, testing or bugs?** Join the [Discord](https://discord.gg/xwHfUD2bxW) or
[open an issue](https://github.com/chrissotraidis/projectreach/issues/new/choose).

## What is HaloPad?

HaloPad (the codebase is Project Reach) takes the 32-bit Windows code of Halo: Custom Edition and
translates it to native ARM64 ahead of time, on your Mac. Nothing is compiled on the device while you
play, so it needs no JIT.

Around that code, HaloPad supplies the Windows services the game expects: Direct3D 9 rendered through
Metal, DirectInput mapped to touch and game controllers, audio, files, the registry and Winsock
networking. Every one of Halo's 804 shader programs is translated to Metal and checked against a
reference interpreter. It is a compatibility runtime built for one game, not a general Windows emulator,
and it is not a streaming client.

Because it speaks Custom Edition's own network protocol, HaloPad development builds have joined real
community servers found through Halo's in-game lobby, alongside PC players.

## Two editions

When a personal build includes the Xbox engine, HaloPad asks which edition to open at every launch:

| | Halo Custom Edition (Windows, left) | Halo: Combat Evolved (Xbox, right) |
| --- | --- | --- |
| Engine | Your own `haloce.exe` 1.10, translated ahead of time | [halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal), a port of the Xbox decompilation, fetched and built on your Mac |
| Game files | Your Custom Edition package | Your own Xbox disc image, imported in the app |
| Online | Custom Edition servers and LAN | System link with other copies of that port |
| Campaign | No | The original Xbox campaign (preview) |
| Status | Developer preview | Experimental preview |

Each edition keeps its own saves and multiplayer, and both share HaloPad's touch controls. To switch,
close HaloPad and open it again. The Xbox card has **Original** and **Sharper** graphics.

The Xbox engine is never part of this repository. Your Mac downloads the pinned upstream release
(currently build 74, the latest) and builds it from source. Upstream notes that parts of the
decompilation were reconstructed with help from leaked Bungie material, which is why the Xbox edition
is only ever a personal build; see [Xbox engine](docs/XBOX-ENGINE.md).

## Current status

The physical gameplay builds use a development scene to enter Halo's menu. Full
original startup still needs the product ID written by an original Halo PC
installer; that private provisioning path is being tested.

| Area | Where it stands |
| --- | --- |
| **iPad** | Physical iPad Pro 12.9" (6th gen) imports its game package, plays local Slayer matches at about 30 FPS and has joined online games. A crash in busy scenes (a shader needing more than eight texture setups) is fixed. First-time loading of new effects still stutters |
| **iPhone** | Physical iPhone 14 imports, creates a profile and plays a local LAN match, including 1280 × 720 widescreen. About 30 FPS in a static scene; loading and busy play are still slow |
| **Multiplayer** | Halo's own LAN and Internet menus work. Development builds joined public Custom Edition servers from the Mac and iPad Simulator; online play on physical phones is still to be tested |
| **Controls** | Movable, resizable touch overlay, look-speed settings, iOS keyboard for names and chat, Xbox-style controllers (including connecting after launch) and iPad trackpad/mouse in menus |
| **Custom maps** | Import `.map` files from the app menu. Client DLL mods (Chimera, OpenSauce, HAC2) do not load |
| **Campaign** | Through the Xbox edition: menus, campaign, controls, Save and Quit and reload pass in the iPad Simulator; a physical iPad has run a preview build. Custom Edition has no campaign |
| **Xbox edition** | Experimental. Simulator checks pass for campaign, a 12 minute system-link match and saves. Real-iPad feel and performance are not yet accepted |
| **Distribution** | Source only for now. Build your own with your own game files |

Details, measurements and open gates live in [docs/STATUS.md](docs/STATUS.md) and the
[device-readiness loop](docs/HaloPad-GOAL-LOOP-PHASE3.md).

## Playing

On first launch, choose your prepared `.halopad.zip` game package. Create a Halo profile, then use the
game's own **Multiplayer** menus to host or join.

- **⋯ menu:** touch settings, keyboard and chat, join a server by address, display options,
  custom-map import, controller guide, **Report a Problem** and **Share Diagnostic Log**
- **Touch:** move and resize the overlay; tune look speed under **Controls › Look Speed & Touch Settings**
- **Controllers:** connect before opening HaloPad for the most reliable result; connecting later is
  supported (tested in the Simulator so far). In Halo's menus the D-pad or left stick
  moves, **A** selects and **B** goes back; **Menu** pauses. While the on-screen keyboard is open
  (chat, console, a profile name), **A** sends Enter and **B** cancels. Halo's "Button 6" pickup
  prompt is **RB** on an Xbox controller
- **Leaving a match:** **⋯ › Open Leave Game Menu…** opens Halo's pause menu; choose **Leave Game** there
- **Resolution:** Halo renders at 800 × 600 by default in its original 4:3 shape. Halo's 1280 × 720 mode
  gives a true widescreen view, or use **Fill** to stretch 4:3
- **Smoother recording:** the first time a map, weapon or effect appears, its graphics are prepared and
  play can hitch for up to about a second. Later appearances are much faster, so play a warm-up
  match on the same map before recording

## Build and install

You need:

- a Mac with Apple silicon and Xcode
- your own Halo: Custom Edition 1.10 files
- an iPhone or iPad on iOS/iPadOS 17 or later, with Developer Mode on
- an Apple development profile for HaloPad's bundle ID that allows **Extended Virtual Addressing** and
  **Increased Memory Limit** (the runtime reserves Halo's full 32-bit address space)

Start with `scripts/doctor.sh`, then follow [Installing on iPhone or iPad](docs/INSTALL-IPHONE.md)
for signing, building, packaging your game files and first launch. Today this is a developer workflow
with private input preparation, not a one-command build.

A one-command personal build through [PadMint](https://github.com/chrissotraidis/padmint) is planned.
PadMint runs HaloPad's own builder against your verified game files on your Mac; game files,
translated code and signing material never leave it. HaloPad's [draft manifest](padmint.json) marks
iOS as planned.

The [HaloPad icon](assets/Assets.xcassets/AppIcon.appiconset/AppIcon.png) is an original orbital-arc design;
the alternatives considered are in [assets/icon-concepts](assets/icon-concepts).

**An app you build contains code translated from your game: it is yours alone. Never share or upload it.**

### Adding the Xbox edition

The Xbox edition is optional. Build the engine before the app:

```sh
scripts/xbox/build-ios.sh --device      # fetches and builds the pinned engine on your Mac
.venv/bin/python scripts/build-ios-app.py --iphoneos --identity "..." --profile ...
```

A build that includes the Xbox engine stops at a signed `HaloPad.app` and never creates an IPA.
Then pick your disc image in the app. [Xbox engine](docs/XBOX-ENGINE.md) covers requirements, updates
to newer upstream releases and the tested disc (NTSC-US, maps build `01.10.12.2276`).

Install updates over the existing app. Deleting HaloPad deletes your profiles and imported files, so back
up its `Documents` and `Library` first if you ever need to change signing.

## Known issues

- **Loading and frame pacing.** First-time graphics preparation causes hitches; play runs at about 30 FPS
  on iPad and slower on iPhone.
- **Controller dropouts.** Some players have seen a controller stop responding after reconnecting it or
  switching from touch; in one report it came back after starting a new game. If it happens, please
  share the diagnostic log (below).
- **Video mode changes on iPad.** Changing Halo's resolution in its Video settings is tested on iPhone,
  not yet on iPad. The default 800 × 600 is the safest choice.
- **Touch ergonomics.** Two-thumb feel is still being tuned on real phones.
- **Online on physical phones.** Online games have been joined from a physical iPad; iPhone online play
  has not had a full test.
- **Xbox edition preview.** One rare crash during a long Battle Creek camera test was seen once in the
  Simulator and has not recurred in more than two hours of retesting. Crash reports now identify the
  exact fault if it happens again. Touch feel and performance on a real iPad are still being checked.

## Getting help

- **Discord:** [discord.gg/xwHfUD2bxW](https://discord.gg/xwHfUD2bxW) for questions, testing and updates
- **Bug reports:** use **⋯ › Report a Problem** or [open an issue](https://github.com/chrissotraidis/projectreach/issues/new/choose).
  Include your device, iOS version, build, map, what you tapped and whether a controller was connected.
  Please never attach game files, maps or app packages.
- **Diagnostic log:** HaloPad keeps a small log of controller, display, stall and crash events in
  **Files › On My iPad/iPhone › HaloPad › HaloPad Logs**, or share it from **⋯ › Help › Share Diagnostic
  Log…**. It contains no typed text, names, chat or server addresses. Attach it to bug reports

## Frequently asked questions

### Can I download an IPA?

Not yet. HaloPad runs code translated from the game, so each player builds their own from their own
files. An IPA on its own would not contain the game either.

### Is this an emulator?

Not in the usual sense. Halo's x86 code is translated to ARM64 ahead of time and runs natively, with no
JIT. HaloPad then provides the Windows, Direct3D 9, input, audio and network pieces the game calls into.

### Can I play with people on PC?

That is the goal. HaloPad uses Custom Edition's own network protocol, and development builds have joined
public PC servers and spawned into games. A full online session on a physical iPhone or iPad is the next
thing to test.

### Which version of Halo works?

For the Windows edition, Halo: Custom Edition 1.10 only; the build checks your files against that exact
version. For the Xbox edition, an original Xbox Halo: Combat Evolved disc image; the NTSC-US release is
the tested one. The PC retail `halo.exe` and the Master Chief Collection are not supported.

### Does it run on iPhone?

Yes. An iPhone 14 has imported the game and played a local match, including in widescreen. Loading
and busy scenes are slow there for now.

### Do controllers work?

Yes, iOS-supported controllers work. Connecting before HaloPad opens is the most tested path.
**⋯ › Controller Guide** shows the mapping.

### Can I use mods and custom maps?

Custom `.map` files, yes: **⋯ › Add Custom Maps…**. Windows DLL mods such as Chimera, OpenSauce and
HAC2 cannot load into a translated game.

### Will updates keep my profile?

In-place updates have preserved app data in testing. Install over the existing app and never delete it
to update.

## Documentation

- [Install guide](docs/INSTALL-IPHONE.md): signing, packaging and device setup
- [Xbox engine](docs/XBOX-ENGINE.md): the optional Xbox edition, its build, updates and boundaries
- [Status](docs/STATUS.md) and [journal](docs/JOURNAL.md): tested builds and observations
- [Device-readiness loop](docs/HaloPad-GOAL-LOOP-PHASE3.md): the iPhone and iPad acceptance plan
- [Execution model](docs/EXECUTION-MODEL.md): translation coverage and guest dispatch
- [Graphics contract](docs/GRAPHICS-CONTRACT.md) and [D3D9 inventory](docs/D3D9-INVENTORY.md): Direct3D 9 on Metal
- [Runtime and networking](docs/G3-RUNTIME.md): Windows services and public-server results
- [Rights status](docs/RIGHTS-STATUS.md): inputs, generated code and publication boundaries

## Credits

HaloPad stands on a lot of other people's work. Thank you to:

- [SR](https://github.com/M-HT/SR) by M-HT, the static x86 recompiler at the heart of the translation pipeline
- [halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal) by cybersecurity and its
  contributors, the Xbox engine port, built on the decompilations [bnunu/halo-1](https://github.com/bnunu/halo-1)
  and [punpckhdq/halo](https://github.com/punpckhdq/halo)
- [ANGLE](https://chromium.googlesource.com/angle/angle), whose Metal backend draws the Xbox edition
- [xboxrecomp](https://github.com/sp00nznet/xboxrecomp) by sp00nznet, used for runtime research
- [SunPad](https://github.com/chrissotraidis/sunpad), whose touch overlay and three-dot menu HaloPad adapts
- Xiph.Org contributors for Ogg and Vorbis, and udis86 for disassembly
- The Halo Custom Edition community, who have kept servers, maps and the master server running for over twenty years

Each project keeps its own license and notices. This repository does not claim a blanket license over
upstream work or Halo content.

## Legal

HaloPad is an independent fan project. It is not affiliated with or endorsed by Microsoft, Xbox or
Halo Studios. Halo and Halo: Custom Edition are trademarks of their respective owners. HaloPad grants
no rights to Halo content: you need your own legitimate copy and are responsible for the laws that apply
to it. No project-wide license has been chosen yet; see [rights status](docs/RIGHTS-STATUS.md).

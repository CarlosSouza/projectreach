# Xbox engine (second HaloPad engine)

Status, 2026-09-30: **HaloPad offers Halo PC or Halo Xbox at launch.** The Xbox engine reaches its
menu, loads a campaign level and plays a system link match on the Mac; runs in HaloPad's own app on
the iPad Simulator (picker, disc import, touch gamepad); and runs on the physical iPad Pro to its menu
and a campaign level. Open: an iPad-to-iPad or iPad-to-Mac match (needs the iPad's Local Network
permission, below), human play with touch and a controller on the device, and a join-by-address screen.

HaloPad now opens with a choice:

| | Halo PC | Halo Xbox |
|---|---|---|
| Engine | Custom Edition 1.10, translated from the player's own `haloce.exe` | [cybersecurity/halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal), a port of the Xbox decompilation |
| Game files | Custom Edition package | The player's own Xbox disc image (maps copied into Documents/Halo Xbox) |
| Online | Custom Edition servers | Other copies of that port (system link) |
| Campaign | Not yet | Yes, plus split-screen |

The two cannot play online together. One engine runs per launch (both use the same guest memory), so
switching means closing HaloPad and opening it again; the picker appears at every launch.

## Personal-build boundary (Chris's decision, 2026-09-30)

- The upstream engine is **fetched and built on the player's own Mac** at the revision in
  [config/xbox-engine.lock.json](../config/xbox-engine.lock.json). Its sources, its guest image, the
  translation and the engine library live only under the ignored `ref/xbox-build/`.
- HaloPad's repository holds only HaloPad's own code: the translator, the host runtime, the picker and
  scripts. A HaloPad build made without the local engine has no picker and behaves as before.
- A device build of HaloPad that includes the engine (and the `HaloPad.ipa` that
  `scripts/build-ios-app.py --iphoneos` writes next to it under `generated/`) contains translated
  upstream code: **it is the builder's alone and is never shared or published.**
- Updates stay **pinned**: moving the pin is a deliberate, tested step (below); saves are backed up first.
- Upstream's documentation says parts of the decompilation were reconstructed with help from leaked
  Bungie material ([REVIEW-HALO1-DECOMP.md](REVIEW-HALO1-DECOMP.md)). That is why the engine stays a
  personal build, and why this document is not a rights clearance.

## How it works

Upstream's Android build compiles the game as **arm64_32** (AArch64 instructions, 32-bit pointers,
because the game's data files hold 32-bit pointers) and links one static image at guest address
`0x88000000`, with the Xbox memory window at `0x80000000`. Apple platforms reserve the low 4 GiB of every
process, so that image cannot run where it was linked. HaloPad therefore:

1. **Builds upstream's own Android guest** with two extra compiler flags
   ([scripts/xbox/guest-cc.sh](../scripts/xbox/guest-cc.sh)): x27 and x28 are reserved, and there are no
   jump tables.
2. **Translates the linked image** ([scripts/xbox/translate.py](../scripts/xbox/translate.py)) into
   ordinary ARM64. Guest memory is one **4 GiB-aligned** host reservation whose base is in x28, so guest
   address *g* is at x28 + *g* and the low 32 bits of any host address inside it are the guest address.
   Loads and stores add the base (`add x27, x28, wN, uxtw`); adrp/adr become constants; direct branches
   go to translated labels; blr/br go through one dispatch table (an entry per guest instruction); calls
   to upstream's import stubs become direct calls into the host. The stack pointer is a real host address
   inside guest memory, and any copy of it into a register is cut back to 32 bits. No JIT.
3. **Runs it on a Darwin host** ([port/xbox](../port/xbox)): guest memory and the game's mmap; Linux
   system calls converted to Darwin (flags, structures, errno, futexes on `os_sync_wait_on_address`);
   threads with stacks in guest memory; OpenGL ES wrappers generated from upstream's own list
   ([scripts/xbox/gen-host-gl.py](../scripts/xbox/gen-host-gl.py)), with the real functions declared with
   their original argument types (upstream widens stack arguments to 8-byte slots on the guest side only);
   and upstream's `port/linux/src/posix_*.c` compiled for the host from the pinned checkout.
   - **Mac** ([xg_sdl.c](../port/xbox/xg_sdl.c), [xg_main_macos.c](../port/xbox/xg_main_macos.c)): SDL3,
     OpenGL ES through ANGLE's Metal back end.
   - **iOS** ([xg_ios.m](../port/xbox/xg_ios.m)): no SDL. Apple's OpenGL ES 3.0 on a layer-backed
     framebuffer that stands in for framebuffer 0, the GameController framework, Remote I/O audio, the
     game on its own thread. [xg_touch.m](../port/xbox/xg_touch.m) is an Xbox-layout touch gamepad merged
     into player 1 (floating move stick, drag to look, RT/LT, A/B/X/Y, RB/LB, crouch, zoom, Start, Back);
     it hides while a controller is connected. [xg_xiso.c](../port/xbox/xg_xiso.c) copies maps/ out of
     the player's disc image.
   - **HaloPad** ([port/ios/HaloPadXbox.m](../port/ios/HaloPadXbox.m)): the launch picker and the Xbox
     screen (disc import, then the game). [HaloPadApp.m](../port/ios/HaloPadApp.m) uses the picker
     through a weak reference.

Apple devices use 16 KiB pages and the game 4 KiB ones: inside the Xbox window and the image the game's
own mapping calls are emulated, and Direct3D write tracking protects whole 16 KiB pages. The game's
start-up invite link is kept off the player's clipboard.

## Build, run and install

```sh
scripts/xbox/extract-maps.py "ref/Halo - Combat Evolved (USA).xiso.iso" ref/xbox-build/data
scripts/xbox/build-mac.sh                 # the Mac program (also prepares everything below)
scripts/xbox/smoke-mac.py                 # Mac checks: menu, campaign, match
scripts/xbox/build-ios.sh                 # engine library for the Simulator (+ a stand-alone test app)
scripts/xbox/build-ios.sh --device        # engine library for devices
.venv/bin/python scripts/build-ios-app.py [--iphoneos --identity ... --profile ... --scene tests/halo_app_scene.c]
```

Needs Homebrew `llvm`, `lld`, `ninja` and `sdl3` (Mac only). The checkout lives on a case-sensitive
disk image (upstream has a header that includes itself on a case-insensitive disk). On the Mac, OpenGL ES
comes from ANGLE: any Chromium/Electron app's `libEGL.dylib` and `libGLESv2.dylib` work for a local
test (`--angle`). Development switches: `XG_FRAME_DUMP=<file.ppm>` saves the game's own frames
(`XG_FRAME_DUMP_DOCUMENTS=1` on a device), `XG_GL_CHECK=1` names failing OpenGL calls, `HALOPAD_ENGINE` or
`HALOPAD_CHOOSE` (`pc`/`xbox`) skip or press a picker card, `HALOPAD_XBOX_IMPORT=<image>` imports a disc image,
`XG_TOUCH_SHOW=1` keeps the touch gamepad up, and upstream's `HALO_*` settings pass through (for example
its `HALO_NETWORK_TEST` scripted matches). `init.txt` in the data folder holds console commands.

On a device, back up HaloPad's Documents and Library first, install over the existing app, then either
pick the disc image in the app (Files) or copy an extracted `maps` folder to Documents/Halo Xbox/maps.

## Updating the engine

`scripts/xbox/update-pin.sh [--to REV] [--device UDID] [--accept]` lists upstream's changes (flagging the
parts the host depends on), backs up the Xbox saves (this Mac's and, with `--device`, the device's
Documents/Halo Xbox/save), builds the candidate, runs `smoke-mac.py`, and moves the pin only with `--accept`
when every check passes. Without `--accept` it returns the checkout and the build to the pinned engine.
After accepting, rebuild the libraries and HaloPad and install over the existing app.

## Evidence (2026-09-30)

| Check | Result |
|---|---|
| Disc | `Halo - Combat Evolved (USA).xiso.iso`: all 24 maps build `01.10.12.2276` (NTSC, upstream's reference speed); `default.xbe` SHA-256 `ed3a8e96…e3a3ac`; 1.7 GB of maps. The in-app extractor's copy is byte-identical to the Mac extraction |
| Pin | `b47f237d` |
| Guest image | 7.0 MB ELF; .text 2.6 MB, 657,553 instructions, no use of x27/x28, 191 import stubs, all resolved by the host |
| Mac (M3 Max, ANGLE Metal) | Menu; `map_name levels\a10\a10` loads The Pillar of Autumn; Blood Gulch Slayer with a stand-in machine: 2,100+ ticks, kills, respawns, shield damage, no corrections; in-match first-person frame with HUD. `smoke-mac.py` passes all three |
| iPad Simulator (iPadOS 26.5) | HaloPad's own app: picker, the Halo Xbox card, disc import into Documents/Halo Xbox (24 maps), menu, touch gamepad drawn. The Halo PC card still starts Custom Edition (its license dialog on a fresh install) |
| iPad Simulator match | HaloPad (the iOS host) hosts Blood Gulch; a stand-in machine joins from the Mac: 1,469 ticks, 8 hits, in-match first-person frame with the Link button. A white triangle in that frame is not yet explained (the Simulator draws with Apple's software renderer) |
| Physical iPad Pro 12.9" (6th gen, M2) | Signed with the existing profile (both memory entitlements). Guest memory reserved at `0x7000000000`; OpenGL ES 3.0 on the M2 GPU; 48 kHz audio; menu; The Pillar of Autumn's opening. Backups before each in-place install: `generated/device-backups/ipad-20260930-192030-before-xbox` (Documents + Library, SHA-256 list) and `…-193829-before-xbox-2` |
| Update routine | Upstream `c68db561` (10 commits newer, including changes to its Android build) builds through the translator and passes the smoke test; the pin was left at `b47f237d` so the installed iPad build matches it |

## Open items

- **iPad match (needs Chris):** iOS allows broadcast only with Apple's restricted multicast
  entitlement, so the Xbox screen's **Link** button lists the other devices' addresses (and shows this
  device's own), which the game searches instead of broadcasting. Even so, an iPad joining a Mac-hosted
  game found nothing: a plain listener on the Mac's game port received **no packets** from the iPad,
  while the Mac copy's own search reached it through the same host code. iOS is blocking HaloPad's local
  network traffic; HaloPad never had the **Local Network** permission (the PC game uses internet
  servers). The builds now declare it; Chris needs to allow it on the iPad (the prompt, or Settings ›
  Privacy & Security › Local Network › HaloPad).
- **Human play on the device:** touch gamepad feel and a Bluetooth controller on the iPad are untested
  by a player.
- **Mac presentation:** the Mac proof draws in part of its fullscreen window (the iOS host fills the
  screen); the Mac program is a test tool, not a product.
- **Internet play:** the game starts internet hosting and asks public STUN servers for its address at
  start-up; it should be opt-in in the app. UPnP and Discord are stubbed.
- **Missing on OpenGL ES 3.0:** `glCopyImageSubData` and `glDrawElementsBaseVertex` (upstream falls back);
  Bink movies are skipped, as upstream does.

# Xbox engine (second HaloPad engine)

Status, 2026-10-02: **HaloPad offers Windows Custom Edition or Xbox Combat Evolved at launch.**
The accepted **experimental development pin** is upstream **build 64, `c55e4e2b`**,
which expands the high-resolution HUD/scopes and fixes meter alpha and flat menu fills.
Save-backed candidate and acceptance Mac/ANGLE iPad Simulator menu/a10/scripted-match
gates pass. A copied build-61 a30 checkpoint loads through normal menus; actual
fire, pistol swap, 2x Zoom, Save and Quit and fresh-process pod reload work.
An isolated build-64 normal-menu pass now advances outside the pod using actual
touch gestures and restores that newly reached checkpoint after Save and Quit
and a cold launch (one grenade retained). This is bounded early progression,
not sustained multi-touch, campaign completion or physical graphics acceptance.
ANGLE/Metal remains an independently pinned opt-in Simulator PREVIEW, not the
default or physical-device renderer. The earlier build-59 real touch pass verified
navigation, cryo-bay training, tube exit, Save and Quit, and a same-build cold checkpoint
reload with isolated saves. A copied build-59 checkpoint also loads through the
normal build-60 menus; a copy of that fixture also reloads the cryo-bay in build 61.
This is not general snapshot compatibility or full gameplay acceptance:
geometry/texture artifacts, full campaign progression, split-screen, human system link,
audio quality and physical performance remain open. No physical iPad changes in this pass.
The Simulator-only presentation fix remains narrow: temporarily neutralize texture unit/
sampler 0 during final presentation, then restore it. Physical rendering is unchanged.
See [XBOX-SIMULATOR-PASSES.md](XBOX-SIMULATOR-PASSES.md).

HaloPad now opens with a choice:

| | Halo PC | Halo Xbox |
|---|---|---|
| Engine | Custom Edition 1.10, translated from the player's own `haloce.exe` | [cybersecurity/halo-ce-universal](https://github.com/cybersecurity/halo-ce-universal), a port of the Xbox decompilation |
| Game files | Custom Edition package | The player's own Xbox disc image (maps copied into Documents/Halo Xbox) |
| Online | Custom Edition servers | Other copies of that port (system link) |
| Campaign | Not yet | Original Xbox campaign; experimental. Split-screen is upstream functionality, unverified in HaloPad |

The two cannot play online together. One engine runs per launch (both use the same guest memory), so
switching means closing HaloPad and opening it again; the picker appears at every launch.

## Personal-build boundary (Chris's decision, 2026-09-30)

- The upstream engine is **fetched and built on the player's own Mac** at the revision in
  [config/xbox-engine.lock.json](../config/xbox-engine.lock.json). Its sources, its guest image, the
  translation and the engine library live only under the ignored `ref/xbox-build/`.
- HaloPad's repository holds only HaloPad's own code: the translator, the host runtime, the picker and
  scripts. A HaloPad build made without the local engine has no picker and behaves as before.
- A device build containing the engine stops at the signed `HaloPad.app`; the app builder
  **does not create an IPA** for Xbox personal builds. The app contains translated upstream
  code: **it is the builder's alone and is never shared or published.** PC-only packaging is unchanged.
- Updates stay **pinned**: moving the pin is a deliberate, tested step (below); saves are backed up first.
- Upstream's documentation says parts of the decompilation were reconstructed with help from leaked
  Bungie material ([REVIEW-HALO1-DECOMP.md](REVIEW-HALO1-DECOMP.md)). That is why the engine stays a
  personal build, and why this document is not a rights clearance.

## Opt-in Simulator renderer comparison

Apple OpenGL ES remains the default; physical-device rendering is unchanged.
`HALOPAD_XBOX_RENDERER=angle-metal` builds a **Simulator-only preview** from the
separately pinned [WebKit ANGLE source](https://github.com/WebKit/WebKit/tree/a1fb7ce122d0cd99f7d6cc82775f02565e266ece/Source/ThirdParty/ANGLE).
[config/xbox-angle.lock.json](../config/xbox-angle.lock.json) records both source
revisions and the enabled feature. This is independent of the Xbox guest pin.
Do not retag a macOS ANGLE library as a Simulator library.

Fetch the source into scratch, not another project checkout in `GitHub`:

```sh
angle_work=$(mktemp -d /tmp/halopad-angle.XXXXXX)
git clone --filter=blob:none --depth=1 --no-checkout https://github.com/WebKit/WebKit.git "$angle_work/WebKit"
git -C "$angle_work/WebKit" fetch --depth=1 origin a1fb7ce122d0cd99f7d6cc82775f02565e266ece
git -C "$angle_work/WebKit" sparse-checkout init --cone
git -C "$angle_work/WebKit" sparse-checkout set Source/ThirdParty/ANGLE
git -C "$angle_work/WebKit" checkout --detach a1fb7ce122d0cd99f7d6cc82775f02565e266ece
export XBOX_ANGLE_SOURCE="$angle_work/WebKit/Source/ThirdParty/ANGLE"
HALOPAD_XBOX_RENDERER=angle-metal scripts/xbox/build-ios.sh
```

The small CMake wrapper reuses upstream source lists and builds with the actual
Simulator SDK. Source revision/dirty-tree guards run before guest preparation.
The archive/manifest live under ignored `ref/xbox-build/out/iphonesimulator-angle/`,
leaving the default library untouched. Package with the same renderer setting
using the normal `scripts/build-ios-app.py` workflow; it refuses a physical
target, missing candidate library, mismatched renderer/source or stale hashes.
The picker identifies this renderer build as PREVIEW even with the accepted guest
pin. Preserve the previous app and actual saves before an in-place installation.

Run the asset-free probe before accepting this backend on another Simulator:

```sh
xcrun --sdk iphonesimulator clang -target arm64-apple-ios17.0-simulator \
  -fobjc-arc -I"$XBOX_ANGLE_SOURCE/include" tests/xbox_angle_probe.m \
  ref/xbox-build/out/angle-simulator/libhalopad-angle.a -lc++ -lz \
  -framework Foundation -framework CoreGraphics -framework IOSurface \
  -framework QuartzCore -framework Metal -o "$angle_work/angle-probe"
SIMCTL_CHILD_HALOPAD_ANGLE_NATIVE_SWIZZLE=1 xcrun simctl spawn SIMULATOR_UDID "$angle_work/angle-probe"
```

It tests equal-depth coverage, swizzle-independent blitting and swizzled texture
sampling. The unmodified ANGLE Simulator default fails the last test here:
it deliberately disables `hasTextureSwizzle` in
[DisplayMtl](https://github.com/WebKit/WebKit/blob/a1fb7ce122d0cd99f7d6cc82775f02565e266ece/Source/ThirdParty/ANGLE/src/libANGLE/renderer/metal/DisplayMtl.mm).
The native-feature override passes on this Mac/iPadOS 26.5 and is confined to this
opt-in candidate; it is not a general driver fix or a physical-device override.
The smoke runner verifies the logged renderer against its manifest, allows a
30-second cold ANGLE menu capture, and retains image/progression gates. Neither
probe nor smoke results establish correct lighting, full gameplay or hardware
acceptance. See the pass ledger for actual scene review and remaining defects.

The bounded campaign smoke defaults to Pillar of Autumn/a10 without scripted
input. For later assets, `--case campaign --campaign-map a30` selects Halo;
`--campaign-map a50` selects Truth and Reconciliation for night/sniper coverage.
For a moving-camera diagnostic only:

```sh
.venv/bin/python scripts/xbox/smoke-simulator.py --device SIMULATOR_UDID \
  --case campaign --campaign-map a30 --scripted-campaign \
  --render-diagnostics --seconds 90
```

This uses upstream's `bot:7` for movement/look/fire and fresh isolated saves.
The result records the requested map and `scripted-render-diagnostic` input
mode; the load gate checks that actual map, not merely any campaign request.
It is not normal-menu checkpoint, human-control or campaign-completion proof.
Scripted campaign input requires both explicit diagnostic flags. Ordinary
menu/campaign runs override inherited bot/network-test settings with empty
values. Review the images separately; a lit frame is not visual acceptance.

For output-signal diagnostics, add `--audio-diagnostics` to a Simulator smoke
run (allow at least 14 seconds after the audio unit starts). This explicitly
enables Simulator-only `XG_AUDIO_CAPTURE=1`: skip ten seconds of callback frames,
then copy four seconds of interleaved float32 output, including any zero-filled
starvation. Allocation occurs at setup; the callback does no file I/O. After a
release/acquire completion handoff, the game thread writes `audio-output.f32le`
and rate/channel/frame/underrun metadata in the isolated evidence folder. The
runner rejects missing, truncated, invalid, nonfinite or silent captures and
reports RMS, peak and samples outside ±1. Range excursions/underruns are reported,
not hidden by the signal gate. Ordinary runs explicitly clear this diagnostic.
This is delivery to the output callback, not audible quality, deadline timing,
audio/video sync or physical-device acceptance. Keep captured game audio private.

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

Check upstream releases on a regular maintenance pass (weekly is the proposed cadence), then
freeze an exact commit for validation. Do not chase changing HEAD during a pass. This is a
local build/update workflow, not an in-app executable updater or a scheduled job already installed.

```sh
scripts/xbox/update-pin.sh --to c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9 --simulator <dedicated-simulator-UDID>
# Only after all checks and visual review pass:
scripts/xbox/update-pin.sh --to c55e4e2b9d90550b0e761eb78dfe9d7c74880cb9 --simulator <dedicated-simulator-UDID> --accept
```

The script lists upstream changes, backs up Mac and selected Simulator Xbox saves, builds the
candidate, and runs isolated Mac and Simulator menu/campaign/match tests. `--accept` requires an
explicit Simulator and all checks passing. `--device UDID` additionally backs up physical-device
Xbox saves; it does not establish physical gameplay acceptance. On rejection/interruption the
checkout and Mac build return to the accepted pin. A candidate installed in the Simulator remains
an explicitly labeled preview; revision/hash checks reject stale libraries during ordinary builds.
Rebuild and install in place after accepting. Screenshots require human/agent visual review, not
just a nonblack-pixel check. An accepted pin is the repeatable development baseline,
not full progression/fidelity or physical-device acceptance. The Xbox card continues to
say **EXPERIMENTAL** even after a candidate's regression gates pass.

The app also copies nonempty Xbox saves to `Documents/Halo Xbox/Save Backups/<previous-revision>-<time>`
before a changed engine opens them. A failed backup blocks startup. This preserves recovery data,
**not save-format compatibility**; upstream saves are snapshots. PC saves are not migrated into Xbox
saves. The **About these builds** panel shows the bundled revision and preview status.

Optional original-Xbox texture-byte diagnostics are revision-specific: builds 61
and 64 change the cache layout, so the runner rejects that diagnostic until its ABI is
adapted/reviewed. Normal builds, updates and gameplay do not use that reader.

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

- **iPad match (separate physical gate):** iOS allows broadcast only with Apple's restricted multicast
  entitlement, so the Xbox screen's **Link** button lists the other devices' addresses (and shows this
  device's own), which the game searches instead of broadcasting. Even so, an iPad joining a Mac-hosted
  game found nothing: a plain listener on the Mac's game port received **no packets** from the iPad,
  while the Mac copy's own search reached it through the same host code. An iOS permission block
  was a hypothesis, not a demonstrated root cause. Local Network authorization,
  route/address selection and socket diagnostics still need checking on the physical iPad. The
  builds declare Local Network usage. Do not infer hardware results from the Simulator host match.
- **Human play on the device:** touch gamepad feel and a Bluetooth controller on the iPad are untested
  by a player.
- **Visual fidelity:** corrected Mac drawable capture shows a properly letterboxed cinematic.
  The earlier lower-left picture was an invalid test capture, not a presentation defect.
  Simulator geometry/texture artifacts remain, including pale triangles in the match frame.
  The Mac program is a test tool, not a product.
- **Internet play:** the game starts internet hosting and asks public STUN servers for its address at
  start-up; it should be opt-in in the app. UPnP and Discord are stubbed.
- **Missing on OpenGL ES 3.0:** `glCopyImageSubData` and `glDrawElementsBaseVertex` (upstream falls back);
  Bink movies are skipped, as upstream does.

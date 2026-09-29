# Installing HaloPad on an iPhone or iPad

HaloPad runs Halo Custom Edition 1.10, translated ahead of time to native ARM64. It ships no game
data: you prepare your own Custom Edition 1.10 files on the Mac and move them to the device.

## What you need

- A Mac with Xcode 26 and this repository built (see the README).
- An iPhone or iPad on iOS/iPadOS 17 or later, with **Developer Mode** on
  (Settings → Privacy & Security → Developer Mode).
- An Apple ID in Xcode (Settings → Accounts). A paid developer team is needed for the
  memory entitlements below; a free Personal Team works only if its profile grants them.
- Your Halo Custom Edition 1.10 folder.

## 1. Signing

Halo is a 32-bit Windows program. HaloPad keeps its whole address space in one 4 GiB
reservation, which iOS allows only with Apple's
`com.apple.developer.kernel.extended-virtual-addressing` entitlement. The build also asks for
`com.apple.developer.kernel.increased-memory-limit`.

1. In the Apple Developer portal, register the App ID `dev.halopad.HaloPad` (or change
   `BUNDLE_ID` in `scripts/build-ios-app.py` to one your team owns) and enable
   *Extended Virtual Addressing* and *Increased Memory Limit*.
2. Create a development provisioning profile for that App ID and your device, and download it.
3. Find your signing identity: `security find-identity -v -p codesigning`.

## 2. Build, install and add your game (one command)

Connect the iPad or iPhone with a cable, unlock it and tap **Trust**. Then:

```sh
scripts/install-device.sh \
  --identity "Apple Development: Your Name (TEAMID)" \
  --profile ~/Downloads/HaloPad_Development.mobileprovision \
  --game "/path/to/Halo Custom Edition"
```

This builds HaloPad for the device (the first build compiles the translated game, a few
minutes), signs it, prepares a `.halopad.zip` of your game files for exactly this build,
installs the app and copies the package into HaloPad's Documents folder. It warns if the
profile lacks either memory entitlement.

## 3. First launch

Open HaloPad. The first screen asks for your game files: tap **Choose Prepared Package…**
and pick the `device-….halopad.zip` file. HaloPad verifies all 87 files, installs them and
starts Halo at its main menu. (If Settings asks, trust the developer under General → VPN &
Device Management first.) This was checked end to end on a freshly installed Simulator app.

## 4. Doing it by hand

```sh
.venv/bin/python scripts/build-ios-app.py --iphoneos --identity "…" --profile … \
  --scene tests/halo_touch_move_scene.c
.venv/bin/python scripts/prepare-game-data.py \
  --app-data generated/srw/<profile>/<run>/ios-app-arm64-apple-ios17.0/HaloPad.app/data \
  --game "/path/to/Halo Custom Edition"
xcrun devicectl device install app --device <ID> generated/srw/<profile>/<run>/ios-app-arm64-apple-ios17.0/HaloPad.app
```

Then AirDrop the package to the device or drop it into HaloPad in Finder's device window,
and choose it on the first screen. A package matches only the build that prepared it;
prepare a new one after rebuilding.

## What to check on first hardware run

1. HaloPad opens and the import finishes (memory reservation works on this device).
2. The Halo main menu appears and responds to taps.
3. Multiplayer → Create Game → LAN → any map → Start: touch controls move, look and fire.
4. Connect a controller: touch controls hide, the stick walks, RT fires.
5. Three-dot menu → Help → Report a Problem… files anything that goes wrong.

If HaloPad closes immediately at launch, the provisioning profile most likely lacks
*Extended Virtual Addressing*; Xcode → Devices → Open Console shows
`HALOPAD: reserving guest address space` in that case.

## Play

- **Multiplayer → Join Game → Internet → Get List** lists public servers; **Direct IP** or the
  three-dot menu's **Join Server by Address…** joins a private one.
- Touch controls: left stick moves, right stick or a drag on open screen looks, FIRE also aims
  while held. Rearrange them in the three-dot menu → **Touch Control Settings…** → Edit Layout.
- A game controller works as in Halo; touch controls hide when one connects (a setting).
- The three-dot menu also has **Leave Game**, **Add Custom Maps…** (Custom Edition `.map`
  files for servers running them), display options, the keyboard, Halo's console, team and
  all chat, **About HaloPad** and **Report a Problem…**.

## Known limits

- Not yet run on physical hardware: the 4 GiB reservation, memory use, heat and battery are
  unmeasured on devices.
- Halo's own startup check needs the product ID its official installer writes from a Halo PC
  key. HaloPad does not create one; see STATUS.md.

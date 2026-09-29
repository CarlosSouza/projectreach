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

## 2. Build

```sh
.venv/bin/python scripts/build-ios-app.py --iphoneos \
  --identity "Apple Development: Your Name (TEAMID)" \
  --profile ~/Downloads/HaloPad_Development.mobileprovision
```

The first device build compiles the translated game for the device (a few minutes). It writes
`generated/srw/<profile>/<run>/ios-app-arm64-apple-ios17.0/HaloPad.ipa`. The script warns if the
profile lacks either entitlement. Without `--identity` it still builds, ad-hoc signed; that
checks the build but cannot be installed.

## 3. Install

Connect and trust the device, then:

```sh
xcrun devicectl list devices
xcrun devicectl device install app --device <DEVICE-ID> \
  generated/srw/custom-en-1.0.10.0621/<run>/ios-app-arm64-apple-ios17.0/HaloPad.app
```

Xcode's *Devices and Simulators* window (drag in `HaloPad.ipa`) works too. The first launch
may ask you to trust the developer in Settings → General → VPN & Device Management.

## 4. Add your game files

```sh
.venv/bin/python scripts/prepare-game-data.py \
  --app-data generated/srw/custom-en-1.0.10.0621/<run>/ios-app-arm64-apple-ios17.0/HaloPad.app/data \
  --game "/path/to/Halo Custom Edition"
```

AirDrop the resulting `.halopad.zip` to the device (or copy it with Finder into HaloPad's
files). Open HaloPad and choose **Choose Prepared Package…**. HaloPad checks every file against
this exact build before installing. A package is tied to the build that verified it; rebuild the
package after rebuilding the app.

## 5. Play

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

# Xbox / edition-picker passes, 2026-10-01

Scope: HaloPad's existing checkout, local personal builds, and the dedicated
**HaloPad Xbox iPad** Simulator (iPadOS 26.5). No physical iPad installation,
IPA creation, publication, or upstream modifications.

## Build decision

Upstream release **build-58**, commit
`943abae15a0ebb7b7d6f3e6fe1b8895a740cb8e2`, was fetched and built.
It adds a Windows update-unpacking fix over build 57; this does not fix Apple's
Simulator renderer. The repository's accepted pin stays at `b47f237d`.
The Simulator uses build 58 as an explicitly labeled **Xbox Preview**.

Use `XBOX_REV=943abae15a0ebb7b7d6f3e6fe1b8895a740cb8e2` on both Xbox and
HaloPad build commands to reproduce the preview. Without that override,
rebuild the Xbox library at the accepted pin before packaging. The app builder
checks the revision and guest/library hashes; mixed or rejected outputs fail closed.

## Completed changes

- Large Windows Custom Edition / Xbox Combat Evolved cards, with distinct
  multiplayer descriptions and an installed-build panel. Each cold launch
  offers the choice; switching a running engine still requires terminating
  and relaunching the app.
- Dynamic Type, a vertical layout and scrolling at accessibility sizes.
  The maximum accessibility size was exercised with a real swipe to the lower
  edition/action, and the Simulator was restored to its original `large` size.
- Xbox import can return to Editions before an engine starts. Opening and
  canceling Files, then returning, works. Gamepad creation is deferred until
  game startup: import no longer shows ghost controls or keeps a hidden pad polling.
- Quick touch-button presses are retained until the engine polls them.
  Single A clicks advanced Campaign → Profile → Level → Difficulty.
- A changed revision backs up nonempty Xbox saves before opening them.
  A backup failure prevents startup; isolated test saves cannot change the
  real installation's revision marker.
- Version manifests, old/new UPnP argument compatibility, the newer Android
  user-secret stub, and updated synthetic join-packet support.
- Explicit-UDID Simulator regression runner, fresh per-run save/config folders,
  and Simulator/save-backup gates in the pin updater. Xbox personal device
  packaging stops at the signed app; PC-only packaging is unchanged.

## Evidence and limits

Private evidence is under `ref/xbox-build/passes/2026-10-01/` (ignored).

| Test | Result |
|---|---|
| Build 57, fresh Mac | Menu and a10 opening picture; scripted match 2,130 ticks / 17 shots |
| Build 58, fresh Mac | Menu and a10 opening picture; scripted match 2,108 ticks / 16 shots |
| Build 57, Simulator | Menu; scripted Blood Gulch match 1,336 ticks / 11 shots; terrain striping and pale weapon-side triangle remain |
| Build 58, Simulator | Menu and scripted match 1,331 ticks / 10 shots; visible terrain striping remains. Campaign still black at 60 seconds; full regression gate fails |
| Old pin, Simulator campaign A/B | Black at 60 seconds, same failure as newer candidate |
| Build 57 campaign diagnostics | Black at 120 seconds; menu-driven entry also black. Game thread samples are in software GL drawing/shader compilation, not proof of a CPU deadlock |
| Picker / import routes | Both edition cards route correctly; Windows reaches its existing missing-data setup; Xbox Files cancel and Editions return work |
| Save preservation | All 39 files in the pre-update Xbox save backup match the independent copy byte-for-byte |
| Python / repository checks | 66 tests, 16 skipped; shell syntax, diff whitespace and repository safety guards pass |

The map log says **starting precaching**, not proof of completed load or
campaign gameplay. A nonblack screenshot is only a smoke signal and needs
visual review. The build-58 Mac campaign capture shows the opening ship in
a lower-left viewport with black around it, not a correct full-screen presentation.
Synthetic network machines are not playable peers or human
multiplayer acceptance. This pass did not establish audio quality, split-screen,
controller feel, campaign progression, physical performance, or hardware networking.

The supplied disc remains `ref/Halo - Combat Evolved (USA).xiso.iso`:
24 extracted retail NTSC maps, build `01.10.12.2276`. The engine's menu
build string `01.01.14.2342` identifies its decompilation lineage, not a
requirement to replace those retail maps with a debug disc.

## Next focused pass

1. Inspect the Simulator campaign's render targets before the final blit and
   the presented drawable after it. Compare the same scene on ANGLE/Metal (Mac)
   versus Apple's software OpenGL ES renderer (Simulator), with isolated saves.
   The initial blit error also occurs on successful menu runs, so it alone
   does not explain the campaign failure.
2. Use a small reproducible GL/shader case to narrow the black output and
   terrain artifacts. Keep diagnostics out of normal player builds. If this
   proves a software-driver limitation, evaluate a properly built
   Simulator ANGLE/Metal host; do not patch platform tags on Mac binaries.
3. Repeat menu/campaign/match and visible navigation on the exact candidate.
   Only then accept its pin and rebuild the personal app. Full campaign/save
   progression and physical acceptance remain distinct later gates.

Maintenance policy: check releases regularly (proposed weekly), freeze one
candidate per pass, back up saves, build, test, review frames, then accept.
No automatic upstream executable downloads and no recurring job were created.

# G4 review: Custom Edition locally playable on macOS (M09–M10)

Reviewed 2026-09-28 against [HaloPad-GOAL-LOOP.md](HaloPad-GOAL-LOOP.md) (G4) and
[HaloPad-PRD.md](HaloPad-PRD.md) (M09, M10). Translation run `20260928T060918Z-85892` (lifter
`9d6c88b47852-7ddadfc0`). Every check below reads Halo's own game state or its own output; none uses
a replacement scene. Evidence directories are under `docs/artifacts/<date>/G3/` (ignored).

## Result

**Open, with a lifecycle regression under investigation.** In the post-arena run
`20260928T082315Z`, launch 1 passed but launch 2 exited after 1,438 frames, without leaving `ui`.
The profile file existed; the test did not play the requested map. Earlier passes do not close
this failure. The isolated saved-profile recheck `lifecycle-recheck-20260928T105102Z` passed
(4,702 frames, `ui beavercreek ui`, New001, scripted Quit confirmation). A full consecutive
relaunch with the corrected harness is still due. G4 also remains subject to the phase-2 requirement for an original-client
comparison; G1b's client row is parked on a legitimate product key.

## Requirement by requirement

| G4 item | Evidence (test, what it checks) | Status |
| --- | --- | --- |
| Actual menus and loading (M09) | `halo_menu_test`: Halo's main menu drawn and navigated; `halo_host_test`: Multiplayer > Create Game > LAN > Battle Creek > Slayer > Start Game with a new profile, keys only; `halo_maps_test`: `ui.map` and `bloodgulch.map` loaded into tag memory | met |
| Resources and audio initialize (M09) | `halo_startup_test`, `halo_raster_test`: every start-up check and the whole graphics start-up; `halo_dsound_test`, `halo_vorbis_test` | met |
| Blood Gulch | `halo_bloodgulch_test`, `halo_play_test`, `halo_vehicle_test` (Halo's `-exec` start-up script, then `main`) | met |
| Another stock map | `halo_host_test`: Battle Creek (`beavercreek`), hosted by Halo from its menus; the network scenarios also play Battle Creek after a map change | met |
| Player control | `halo_play_test`: W walks, the mouse turns the look vector; iOS self-test: the touch stick walks 3–5 units, a drag turns 39.7° | met |
| Collision | `halo_vehicle_test`: the Warthog stops against the canyon wall and tips the driver out, Halo's own behaviour; `halo_host_test`: the pickup route is blocked by geometry and strafes around it | met |
| Weapons | `halo_play_test`: the assault rifle's magazine empties by 5+ while firing; `halo_host_test`: projectiles appear; plasma pistol overcharge (battery 1.00 → 0.89) as a network client | met |
| Grenades | `halo_host_test`: right button throws (frag count down by one), damage to shields and health | met |
| Melee | `halo_host_test`: F swings (unit `+0x2ac` during the swing) | met |
| Pickups | `halo_host_test`: walks to a loose weapon, holds E when Halo offers it, the weapon joins the unit's weapons | met |
| Vehicle | `halo_vehicle_test`: Halo offers the driver seat, E enters, W drives about 10 units, E exits | met |
| Death and respawn | `halo_host_test`: death by own grenades, Slayer respawns a new unit | met |
| HUD | screenshots in every play test: ammo, grenades, shields, motion tracker, crosshair, messages | met |
| Meaningful audio | `halo_host_test` records the session's mix (`session.wav`): menu music about −11 dBFS, the shot 15+ dB and the fatal explosion 20+ dB over the game's ambience (macOS and iPad Simulator) | met |
| Menu return, map reload | `halo_lifecycle_test`: Escape > Leave Game back to the main menu, the same game again (the map loads a second time) | met |
| Clean relaunch | Earlier `halo_lifecycle_test --relaunch` passes; `20260928T082315Z` launch 2 failed to reach gameplay. Per-step screenshot trace added for diagnosis. | **reopened** |
| Original-client comparison | G1b's `haloce.exe` row, parked on a legitimate key | **held open** |

## Known weaknesses (not G4 blockers)

- The host test depends on grenade physics after a random spawn: with the final version, 5 of 5
  on the Mac and 1 of 2 on the iPad Simulator; the suite retries once.
- The Metal arena improved steady-state Simulator frame times in the measured scene. The
  handler-driven touch self-test does not establish that every physical touch edge survives
  multiple input pumps per frame; real multi-touch and lifecycle routing remain open.

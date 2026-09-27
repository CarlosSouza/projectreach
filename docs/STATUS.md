# HaloPad status

Updated 2026-09-26. **ACTIVE — phase 2 loop. Translation work is unblocked; no native Halo code runs yet.**

Operating loop: [HaloPad-GOAL-LOOP-PHASE2.md](HaloPad-GOAL-LOOP-PHASE2.md) (replaces G0–G2 of the original loop; inherits the rest).

- **Lowest unmet goal:** G0′ — make a local protective commit of the untracked workspace. Then G1a.
- **Engineering input:** Custom Edition `haloce.exe` **1.0.10.621** derived from the supplied CE 1.00 installer plus Bungie's official CE 1.10 patch, applied in the `halopad-patch` CrossOver bottle without an installation or key. SHA-256 `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`. Also patched: `haloceded.exe` 1.0.10.621 (`7789c4a0…015f`), `strings.dll` (`efa7beb8…a2f4`). Working copy in ignored `generated/patchwork/ce-1.00-to-1.10/`. Profile hash **not yet accepted** (G1a does that after a scripted, repeated reproduction).
- **Oracle:** Unicorn 2.0.1 runs original Halo functions on the Mac. Halo's CRC32 at `0x59f2a2` returned `0xCBF43926` for "123456789", popped exactly 12 bytes of arguments, and matched zlib on randomized inputs. Not yet scripted (G2b).
- **Executable facts:** relocations stripped; 267 static imports across 6 DLLs; 9 delay-import DLLs; d3d9/dinput8/etc. loaded dynamically; TLS present; entry `0x5ccac7`.
- **Tooling fix:** `scripts/inspect-inputs.py` compared versions as strings (`621` ≠ `0621`); it now compares numerically. Nine tests pass.
- **Reference environment:** CrossOver chosen for the system-level baseline (G1b). The Parallels "Windows 11" VM is an invalid registration with no files and is not used.

## Parked, waiting on Chris

| Item | Parks | Status |
|---|---|---|
| Legitimate Halo PC product key (used boxed Halo PC) | G1b if the client refuses to start without one; G5–G6 online identity | Not supplied |
| Physical iPhone/iPad + signing | Final G2e capsule row | Not requested yet |
| Second legitimately provisioned player | G5 | Not needed yet |
| Retail `halo.exe` 1.10 + campaign data | G7 | A boxed copy would cover it |

Rights: private-engineering-authorized; publication-not-authorized. No commit or push has occurred yet. No candidate process or Simulator is running.

Known-good commands:

```sh
scripts/doctor.sh
scripts/verify-sources.sh
scripts/check-repo-safety.sh
.venv/bin/python -m unittest discover -s tests -v
```


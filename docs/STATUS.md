# HaloPad status

Updated 2026-09-26. **ACTIVE — phase 2 loop. Translation work is unblocked; no native Halo code runs yet.**

Operating loop: [HaloPad-GOAL-LOOP-PHASE2.md](HaloPad-GOAL-LOOP-PHASE2.md) (replaces G0–G2 of the original loop; inherits the rest).

- **G0′ PASS:** workspace committed locally on `codex/halopad-phase2` at `5c85449`; safety check green; no push.
- **G1a PASS (engineering tier):** `scripts/prepare-patched-client.sh` reproduced the 1.10 files twice in fresh bottles with identical hashes; `scripts/assemble-custom-original.py` built ignored `ref/inputs/custom-original/` (104 files, manifest). Profile `accepted_sha256` = `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`, `input_state` ENGINEERING_DERIVED; `inspect-inputs.py` passes 1.10, fails 1.00. Cross-check: ProcessChecker's same-size 1.10 copy has a different MD5; see `INPUTS.md`.
- **Lowest unmet goals now:** G1b (CrossOver baseline, parkable) and G2a (executable audit). G2b (oracle harness) is independent and next in order.
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

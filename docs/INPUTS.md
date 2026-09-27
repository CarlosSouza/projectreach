# Input inventory

Last inspected: 2026-09-06. **No accepted executable profile exists.**

Chris supplied `ref/HaloCESetup.exe` and explicitly instructed its use. The file is 186,757,648 bytes and identifies as an NSIS self-extracting PE32 archive. Its SHA-256 is preserved privately in `docs/artifacts/2026-09-06/G0/bootstrap/initial-state.txt`. It was not executed. Installer filename, archive structure and version resources do not establish authenticity or an original-PC-key installation.

The archive contains a member named `haloce.exe`. Only that exact member was extracted, through stdout into an ignored inspection directory. Its fixed version resource reports **1.0.0.609** (`01.00.00.0609` in string resources). It is PE32/x86 with image base `0x00400000` and entry RVA `0x001cced5`. It is **not** the selected `custom-en-1.0.10.0621` build. Never use its addresses or hash for that profile.

Private PE/import/delay-import/section/resource/TLS/relocation inventory:
`docs/artifacts/2026-09-06/G1/inspect-f13eeb6d8a2c/pe-inventory.json`.
This is static inspection of an unaccepted input, not function coverage, accepted language/provenance, baseline execution or evidence of native Halo.

| Profile | State | Missing |
|---|---|---|
| Custom English 1.0.10.0621 | BLOCKED_EXTERNAL | Accepted installation/provenance and edition-specific patch; exact resulting client; original-client baseline |
| Retail English 1.0.10.0621 | BLOCKED_EXTERNAL | Owned retail installation/resources and corresponding patched client; campaign baseline |
| Second reference player | NOT_IDENTIFIED | Legitimately provisioned separate identity/player for simultaneous tests |

**2026-09-26 update.** The supplied installer's `01.00.00.0609` client is the normal Custom Edition 1.00 release, so the expected route is to install it and then apply the Custom Edition 1.10 update. That update was fetched (not executed) from Bungie's original URL via the Wayback Machine into ignored `ref/inputs/patches/haloce-patch-1.0.10.exe`: 3,266,496 bytes, SHA-256 `33818f3f56b7dddc8c61d654af6567c9c5b9220ca75d6ac23a52611038257508`, Authenticode-signed PE32, version resource "Halo Patch" / `HULaunch.exe` (Microsoft Corporation). The earlier Parallels "service unavailable" state has changed: the service now runs, but its only registered VM ("Windows 11") is `invalid` because `~/Parallels` no longer contains the VM bundle. CrossOver and Whisky are also installed. Still missing: the chosen reference environment and a Halo PC product key entered by Chris in the installer.

The config profiles intentionally have `accepted_sha256: null`. Inspection never changes that field. Accepting a hash requires the actual approved acquisition, patch identity and baseline record; changing the expected version to fit this installer is not a repair.

Reproduce the current failed identity test:

```sh
.venv/bin/python scripts/inspect-inputs.py \
  --profile custom-en-1.0.10.0621 \
  --executable generated/inspection/supplied-installer/haloce.exe
```

Expected: exit 2; version mismatch and unaccepted profile hash, with a new private JSON inventory. The original installer remains unchanged. No game saves/profile exist to back up, and no prepared package or AOT core has been created.

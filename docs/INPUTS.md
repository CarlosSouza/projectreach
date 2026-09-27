# Input inventory

Last updated: 2026-09-26. **Custom Edition engineering profile accepted; retail profile missing.**

## Custom English 1.0.10.0621 — `ENGINEERING_DERIVED`

| Item | Value |
|---|---|
| Source installer | `ref/HaloCESetup.exe`, 186,757,648 bytes, SHA-256 `150e430dc54ffb265cbe96605ef8909c9ba0065fa11bdbf170bfd88391cf98ba`. NSIS 2.25 repackage of the stock Custom Edition 1.00 file set (client 1.0.0.609, all stock maps, `haloceded.exe`). Never executed. |
| Official update | `ref/inputs/patches/haloce-patch-1.0.10.exe`, 3,266,496 bytes, SHA-256 `33818f3f56b7dddc8c61d654af6567c9c5b9220ca75d6ac23a52611038257508`, Microsoft Authenticode, fetched from Bungie's original URL via the Wayback Machine. Cabinet: `haloupdate.exe` + `patch.rtp` (targets `haloce.exe`, `haloceded.exe`, `strings.dll`, `binkw32.dll`, `config.txt`; adds five `content/Gallery` images). |
| Method | `scripts/prepare-patched-client.sh` (fresh `halopad-patch-*` CrossOver 26.3 bottle, `haloupdate.exe processrtp=patch.rtp updateversion=01.00.10.0621`, bottle deleted afterwards) then `scripts/assemble-custom-original.py`. No installation, no product key. |
| Reproducibility | Two independent runs, identical hashes for all 12 output files: `docs/artifacts/2026-09-26/G1a/prepare-20260927T013752Z-91939`, `…-013822Z-92133`. |
| `haloce.exe` | 1.0.10.621, 2,404,352 bytes, SHA-256 `feea46fce285ec071016cf5534abe47ecf36f6cfac8f1973ee6919851ea5a037`, MD5 `6388cf3d1f162ce1766a562481f61432`. PE32/x86, image base `0x400000`, entry RVA `0x1ccac7`, relocations stripped. |
| `haloceded.exe` | 1.0.10.621, SHA-256 `7789c4a0bd2c735e7fdc154d63f4bbb1134952e6904e5809eb00d4d1da94015f` |
| Input root | Ignored `ref/inputs/custom-original/`: 104 files (8 from the patch), read-only, `MANIFEST.json` with path/size/SHA-256/origin; 28 installer-only entries (NSIS plugins, DirectX, redistributables) excluded and listed. Copy: `docs/artifacts/2026-09-26/G1a/assemble/manifest.json`. |
| Identity check | `inspect-inputs.py --profile custom-en-1.0.10.0621`: PASS for the patched client (exit 0); FAIL for the 1.00 client (exit 2). |

**Independent cross-check (one bounded attempt).** ProcessChecker lists a 1.0.10.621 `haloce.exe` of the same size, 2,404,352 bytes, with MD5 `7c3fd3b07a6c41a4dbdbdba1b4442580`. Same version and size, different bytes. The source of that copy is unknown; a post-install header change (for example a Large Address Aware flag) or an updater difference when patching an installed copy could each explain it. No public record of our SHA-256 was found. **Consequence:** this profile is accepted for engineering only. When a key-installed original client exists (G1b), hash it; if it differs, diff the two byte-for-byte and re-derive any addresses before relying on them.

Decision: Chris directed on 2026-09-26 that the supplied repo copy be used. Not established: key-installed original-client baseline, independent confirmation, distribution rights (G12).

## Other inputs

| Profile | State | Missing |
|---|---|---|
| Retail English 1.0.10.0621 | BLOCKED_EXTERNAL (needed by G7) | Owned retail installation/resources (a boxed Halo PC would supply it) and the retail 1.10 patch |
| Second reference player | NOT_IDENTIFIED (needed by G5) | Legitimately provisioned separate identity |

Earlier inspection of the unpatched 1.00 client remains at `docs/artifacts/2026-09-06/G1/inspect-f13eeb6d8a2c/pe-inventory.json`. Never use its addresses for the 1.10 profile.

Reproduce:

```sh
scripts/prepare-patched-client.sh
.venv/bin/python scripts/assemble-custom-original.py --patched-run generated/patchwork/run-<id>   # refuses to overwrite an existing root
.venv/bin/python scripts/inspect-inputs.py --profile custom-en-1.0.10.0621 --executable ref/inputs/custom-original/haloce.exe
```


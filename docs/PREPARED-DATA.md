# Prepared game-data packages

The Mac tool creates and verifies `.halopad.zip` files for an exact built core. The new app
build includes native package import and a prepared-package picker. The live user-controlled
Simulator preview has not been replaced and still has the previous folder-only UI. This is
M29 progress, not acceptance of the full prepared-data workflow.

## Prepare locally

Build the app with `scripts/build-ios-app.py` first (omit `--launch` to leave a live preview
alone). The build now writes `HaloPad.app/data/core-identity.json` before signing. Then run:

```sh
.venv/bin/python scripts/prepare-game-data.py \
  --app-data path/to/HaloPad.app/data \
  --game path/to/your/HaloCustomEdition
```

The default output is `generated/prepared/<profile>/<core-id-prefix>.halopad.zip`. Use
`--output <new-file.halopad.zip>` to choose another local filename. Existing output is never
overwritten. The source trees are read-only; output inside either source tree is rejected.
The producer stages on the output volume, verifies the written archive, then publishes with
a no-clobber hard link. Failed preparation removes only its temporary output.

Verify an existing package against a trusted locally built candidate:

```sh
.venv/bin/python scripts/prepare-game-data.py \
  --app-data path/to/HaloPad.app/data \
  --verify path/to/data.halopad.zip
```

Neither command downloads or runs game code. Packages contain private proprietary data and
remain ignored; the repository guard also rejects forced staging of `.halopad.zip` files.
No publication authorization is implied.

## Format, schema 1

The ZIP contains a UTF-8 `manifest.json`, `game/<approved-stock-path>` members and
`core-data/<bundled-data-path>` members. Files use stored or DEFLATE compression; the producer
uses DEFLATE and permits ZIP64. Paths use `/`, are relative, and cannot contain traversal,
backslashes, drive/stream colons, control characters, empty components or trailing spaces/dots.
Case/Unicode-normalization aliases (including parent components), duplicates, file/directory
collisions, symlinks, special files and encrypted entries are rejected. The Mac verifier does
not extract. The native importer extracts verified stock files into a private staging folder
before publication. Limits are 4,096 archive entries, 2 GiB per file, 8 GiB expanded payload
and 2 MiB manifest. These are parsing bounds, not tested physical-device memory/storage claims.

The manifest has exactly `schema`, `format` (`halopad-data`), `profile`, `core_id`, and `files`.
Each `files` entry gives byte size and SHA-256. `core_id` is SHA-256 over the canonical JSON of
its core identity, excluding the identity's own `id`: ASCII-escaped UTF-8, sorted object keys,
compact separators. The identity contains:

- Schema, profile ID, target and locked original executable SHA-256.
- Hashes/sizes of the five compiled Halo/module objects, dispatch IR and shared generated
  runtime IR used by the candidate. Object bytes are **not** packaged.
- Hashes/sizes of the candidate's bundled inert images, profile, reference resources and
  registry seed. These inert resources are packaged; native modules and signing material are not.
- Approved stock paths/hashes/sizes from the engineering input manifest. The executable and
  translated source-module hashes are cross-checked with the locked profile at build time.

Stock selection includes maps, shaders, content and controls plus the named client resources
in `STOCK_TOP`. It excludes installer/update/server programs, Watson, temporary installer files,
unknown extra DLLs, player state and arbitrary extra files. Additional custom-map packaging
is not implemented yet; it must be added with the required map-validation policy.

The verifier takes its expected identity from the **trusted candidate**, never from the ZIP.
It requires the exact manifest/inventory, verifies every expanded byte and rejects a different
core even if the profile name is the same. A self-consistent archive hash is not authentication.
The CLI assumes its `--app-data` directory is trusted; it does not certify the bundle's signature
or provenance. The native importer must compare with its own signed bundled identity.

## Evidence and remaining work

`G9/package-20260928T140152Z` records two real-data preparations: 78 stock files and nine inert
core-data files, 456,118,096 expanded bytes, 181,798,693 archive bytes. Both outputs have identical
SHA-256, and final CLI verification passes. Build output, expected identity, source hashes,
archive identities and test output remain in ignored evidence. No Simulator was restarted.

Thirteen package tests cover reproducibility, expected-core mismatch, modified inputs/resources,
case mapping, unsafe trees/paths/ZIP members, malicious manifest hashes, duplicate JSON keys,
size limits, no-clobber publication, injected write failure and source mutation after preflight.
Five repository guard tests pass, including ignored packages rejected when forcibly staged.
The first run exposed use of `Path.is_relative_to`, unavailable in the pinned Python 3.8 tools
venv; containment now uses resolved parents.

## Native import

`port/ios/HaloPadPackage.m` uses the signed bundle's `data/core-identity.json` as the expected
inventory. The archive cannot provide a replacement identity. The setup panel explains Mac
preparation and offers Choose Prepared Package, with ZIP selection through Files. Security-scoped
access and NSFileCoordinator remain active for the whole background operation.

The reader implements stored/DEFLATE, ZIP64 metadata and signed/unsigned data descriptors using
public SDK zlib. It validates central/local agreement, physical record boundaries, duplicate
and aliased names, file types, flags, sizes and checksums. Streaming uses 64 KiB buffers, with
output bounded by the independently trusted size. SHA-256 and CRC32 cover every expanded file.
Manifest parsing rejects duplicate decoded JSON keys, wrong value types and excessive nesting;
equivalent whitespace and property ordering are accepted. Metadata is never used to load code.

Native parsing requires contiguous local records, then the central directory, optional ZIP64
end records and the classic end record/comment. Self-extracting prefixes, unexplained padding,
extra records, split/encrypted archives and methods other than stored/DEFLATE are rejected.
The existing producer emits this structure. Additional native bounds: nine GiB physical archive,
compressed member at most two GiB plus eight MiB, path at most 512 UTF-8 bytes, JSON depth 32.
Expanded limits remain those above. These are input bounds, not physical-device capacity claims.
ZIP field layout follows [PKWARE APPNOTE 6.3.10](https://pkware.cachefly.net/webdocs/casestudies/APPNOTE.TXT).
The pinned UTP ZIP implementation was inspected but not copied: its whole-file decompression
and no ZIP64 support do not meet this package format's requirements.

Only `game/` entries are written. The nine `core-data/` entries are hash-checked and discarded;
runtime images, modules and registry seed continue coming from the signed bundle. Publication
shares the folder importer's atomic rename/swap and retains the previous complete installation.
Failed writes, validation and commit remove only the new stage. Backup-naming failure preserves
the old tree and reports its actual path. File data is flushed before publication; power-loss
recovery/directory durability and abandoned-stage cleanup remain unverified.

Run `.venv/bin/python scripts/test-native-package.py`; add `--sanitize` for macOS AddressSanitizer
and UndefinedBehaviorSanitizer, or `--device <booted-project-UDID>` for Simulator. The harness
runs import on a background dispatch worker with inert fixtures, presents no window and loads
no Halo core. `--real-package <archive> --app-data <trusted-app/data>` additionally imports into
ignored evidence storage and compares every output byte with the trusted stock inventory.

Sixteen native tests cover normal producer archives, stored/DEFLATE, forced local ZIP64,
ZIP64 end records and descriptors (including CRC equal to the optional signature), malformed
bounds, truncation, links/FIFOs, aliased/traversal names, manifest attacks, wrong content/core,
expansion bombs, invalid bundled metadata, first/replacement imports, failed/partial writes,
publication failure and failed backup naming. The first tests exposed an autoreleased NSError
escaping an inner pool; the pool was removed and these failure paths now pass.

Folder import and device startup now share full stock-content validation with the ZIP path;
see [IMPORT.md](IMPORT.md) for stock-only selection, legacy-folder backup and background startup
checks. Remaining: custom-map policy, restore/remove/recovery, iPhone harness and both-device
actual picker/provider/migration/launch acceptance.
Campaign/licensed-startup/device gates remain unchanged. M29 is open.

Native evidence (2026-09-28): Mac background-worker sanitizer suite and real-data import
`G9/native-package-20260928T143600Z`; iPad final background-worker suite and final app build/
signature verification `G9/native-package-20260928T143716Z`; iPad real-data import before the
harness switched to a background worker `G9/native-package-20260928T143436Z`. Every real-import
output has exactly 78 approved stock files with matching hashes; nine inert-data entries also
verify. The source archive is unchanged and the previous test install is retained. These are
filesystem service tests, not a new gameplay launch or picker/provider acceptance.

The shared identity/path policy now lives in `HaloPadDataIdentity.m`. The native harness also
accepts `--real-folder <installation> --app-data <trusted-app/data>`: it copies to disposable
evidence, compares all output files with the signed stock inventory, verifies source files
(including excluded extras) are unchanged, checks the retained backup, and runs the startup
validator against the imported result. It never updates the live app's game folder.

Actual iPhone Files-picker acceptance (2026-09-28): a fresh project iPhone 17 Pro
Simulator selected a newly prepared `.halopad.zip` through Choose Prepared Package →
Browse → HaloPad. The app imported all 78 stock records with matching signed-inventory
hashes and reached a private original-server match. The archive remains in Documents;
no existing phone saves or game install were present. Relaunch revalidates the same
device data. Evidence: `G9/iphone-touch-network/import-identity.json` and app/server logs.
This covers the local Files provider; remote providers and replacement through the
package picker remain open. iPad folder-picker migration is recorded in IMPORT.md.
Restore/remove and crash recovery remain required; full G9 acceptance is still open.

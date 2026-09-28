# Prepared game-data packages

The Mac tool creates and verifies `.halopad.zip` files for an exact built core. Native ZIP
import is still pending; the installed app currently accepts folders only. This is M29
progress, not acceptance of the full prepared-data workflow.

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
collisions, symlinks, special files and encrypted entries are rejected. No extraction is done
by the current verifier. Limits are 4,096 archive entries, 2 GiB per file, 8 GiB expanded payload
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

Next: implement bounded native archive parsing/staging against the signed identity, connect the
picker and preparation instructions, reuse the transactional folder publication, and verify
wrong-core, malformed/truncated/ZIP64/oversized/traversal inputs preserve the installed data on
both Simulators. Then complete recovery, data-management, custom-map and device/provider rows.
The existing folder path also needs the same full content checks; its executable-only identity
check does not close that requirement. Campaign/licensed-startup/device gates remain unchanged.

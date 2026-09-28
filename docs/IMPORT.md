# Game-data import

The new app build accepts prepared `.halopad.zip` packages and installed Custom Edition
folders. See [PREPARED-DATA.md](PREPARED-DATA.md) for Mac preparation, native ZIP validation
and its separate evidence. The live user-controlled preview still has the old folder-only UI;
the new package picker is compiled but has not been exercised there. M29 remains open.
Imported EXE/DLL bytes are never loaded as native code; the signed build's fixed module table
and compiled profile remain authoritative.

## Folder transaction

`port/ios/HaloPadImport.m` now validates every required stock file's size and SHA-256
against the signed bundle's `core-identity.json`. Folder and ZIP imports share path, Unicode/
case alias and inventory rules in `HaloPadDataIdentity.m`. A folder cannot supply its own
expected hashes. The former executable-only/minimum-resource check has been removed.

Source enumeration includes hidden entries and rejects links, special files, unsafe names and
ambiguous sibling names. It streams directory entries, stopping at 4,096 entries plus the
signed inventory's implied directories. Nesting is bounded at 32 or the signed inventory's
required depth, whichever is larger; relative paths stay within 512 UTF-8 bytes. Thus signed
inventories accepted by package import are not rejected merely for having more directory
components than the usual stock layout.

Only approved stock files are copied, with canonical destination names. Other installer files,
unknown DLLs, saves and extra maps stay in the source. File opens traverse relative directory
descriptors with `O_NOFOLLOW`; source growth during copying cannot exceed the trusted byte
limit. The staged tree is fully hashed again before publication. The UI explains stock-only
selection. Custom-map policy/import remains required by M16; this does not declare it supported.

The picker holds security-scoped access and coordinates reading the source and replacing the
destination. Controls are disabled during the background operation. Copies go to a unique
sibling stage, then an atomic same-volume rename publishes the verified tree. Replacement uses
`RENAME_SWAP`; the previous tree is retained as `Halo Custom Edition Backup <UUID>` in Documents.
If that backup's cosmetic rename fails, it remains at the returned `.halopad-import-<UUID>` path.
The UI reports the backup and waits for Check Again. No previous install is deleted.

Selecting a fully verified, stock-only installed folder is a no-op. Selecting the installed
folder when it has extra files prepares a stock-only sibling and retains the entire old tree
as a backup. Ancestor/descendant overlap remains rejected. Failed validation, bounded copy or
publication removes only incomplete staging. Player state in Application Support is untouched.
Publication is atomic; power-loss directory durability and abandoned-stage recovery remain open.
The caller must serialize transactions; the UI enforces one at a time.

## Startup and older installations

Device-path startup and Check Again use the same complete stock validator. The import panel
shows a verification message and hashes on a background worker before starting Halo. A success
latch prevents repeated taps from starting duplicate cores. Explicit development builds that
supply `HALOPAD_IMAGE` retain their separate Mac-data path; these are not device import evidence.

Folders copied by the earlier 105-file importer include unsupported installer/update resources.
They no longer start automatically: Choose Folder can select that same installed folder to
prepare a verified stock copy while retaining the original backup. A missing/changed required
file still fails and cannot be repaired by simply filtering extras. Direct Files edits are
rechecked on the next launch; no weak executable-only fallback remains. This is startup-time
validation, not a continuous file-integrity monitor while Halo is running.

The new startup/picker flow is compiled but has not replaced the live user-controlled preview.
Its actual presentation, migration taps, provider behavior and game launch remain to be exercised.

## Evidence

Run `.venv/bin/python scripts/test-ios-import.py` for macOS, or add `--device <booted-project-UDID>`
for iOS Simulator. Fixtures are disposable inert bytes with their own expected inventory; production
reads the signed bundle inventory. Add `--sanitize` for macOS ASan/UBSan. No product ID or key is generated or imported by this code.
Tests cover first import, same-folder selection, wrong identity, missing/empty/directory-valued
resources, symlinks, special files, tree overlap, replacement/backup, partial-copy failure,
staged corruption, publication failure, backup-rename failure and case-insensitive lookup.
Write corruption/failure, source mutation and rename failures are injected only in the test
translation unit. The current suite also checks all stock-file kinds, filtering and same-folder
backup, bounded enumeration, approved deep inventory paths, growing sources and parent-link races.

2026-09-28 results:

- macOS `G9/import-20260928T134206Z`: all 12 reported scenario groups pass.
- iPad `G9/import-20260928T134207Z`: same groups pass.
- iPhone `G9/import-20260928T133717Z`: same transaction tests pass, before the missing-folder
  error wording was improved; transaction behavior is unchanged.
- Actual iPad picker `G3/ios-app-20260928T133918Z`: selected the retained installation through
  On My iPad → HaloPad → Import Test Source, copied it and reached Halo's original menu using
  device paths and the development scene. All 105 copied files match the retained source bytes
  (`G9/import-ui-20260928T133826Z/copy-comparison.json`). Application Support state was backed up
  before the UI test; the source folder remains in Documents. Final relaunch
  `G3/ios-app-20260928T134505Z` recognizes the imported folder and reaches the original menus.

Still required: actual startup/folder-migration/package-picker/provider and
iPhone validation, custom-map policy/import, user-facing verify/reimport/remove/restore controls,
crash recovery, actual cloud/external file-provider failures, collision fixtures on a
case-sensitive volume, physical-device acceptance, and phone picker/layout inspection.
Normal licensed PE startup remains a separate parked gate.

Full-content validation evidence (2026-09-28):

- Folder suite: 20 scenario groups, Mac ASan/UBSan `G9/import-20260928T145721Z` and iPad
  `G9/import-20260928T145651Z`, with warnings treated as errors.
- ZIP regression after sharing identity policy: all 16 tests pass, Mac ASan/UBSan
  `G9/native-package-20260928T145718Z` and iPad `...145720Z`.
- Those latter two runs also import the real 105-file engineering installation into isolated
  evidence folders. Exactly 78 approved files are installed, every output hash matches, all
  105 source-file hashes remain unchanged, the previous fixture is retained, and the full
  startup validator accepts the result. This is service-level acceptance, not a new game launch.

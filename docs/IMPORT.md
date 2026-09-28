# Game-data import

The current app accepts an installed Custom Edition folder. This is an incremental folder
importer; native `.halopad.zip` handling required by the PRD is still pending. The Mac preparation
and verification tool now exists ([PREPARED-DATA.md](PREPARED-DATA.md)). M29 remains open.
Imported EXE/DLL bytes are never loaded as native code; the signed build's fixed module table
and compiled profile remain authoritative.

## Folder transaction

`port/ios/HaloPadImport.m` is shared by startup validation, the folder picker and the Foundation
tests. The app supplies the locked executable SHA-256 from its bundled profile. The service
requires a matching `haloce.exe` and nonempty regular resource files for the existing minimum
stock-file set. It checks the whole tree for symlinks, special files and filenames ambiguous
under normalized case-insensitive lookup. This is not complete map/content hash validation.

The picker holds security-scoped access and coordinates reading the source and replacing the
destination. Import controls are disabled while the background operation is running. The
service validates the source, copies to a unique sibling stage, validates the copy, then uses
an atomic same-volume rename to publish it. Replacement uses `RENAME_SWAP`; the prior tree is
retained as `Halo Custom Edition Backup <UUID>` in Documents. If that backup's cosmetic rename
fails, it remains at the returned `.halopad-import-<UUID>` path. The UI reports the retained name
and waits for Check Again before starting. It never deletes a previous install after swapping.

Selecting the installed folder validates it without copying. Ancestor/descendant overlap is
rejected to prevent copying into the source tree. Copy, validation and commit failures leave
the destination intact; only the incomplete stage is removed. Publication is atomic, but
power-loss durability and recovery/cleanup of abandoned pre-commit stages are not yet verified.
The caller must serialize transactions; the UI enforces one at a time.

## Evidence

Run `.venv/bin/python scripts/test-ios-import.py` for macOS, or add `--device <booted-project-UDID>`
for iOS Simulator. Fixtures are disposable inert bytes with their own expected hash; production
still reads the bundled locked hash. No product ID or key is generated or imported by this code.
Tests cover first import, same-folder selection, wrong identity, missing/empty/directory-valued
resources, symlinks, special files, tree overlap, replacement/backup, partial-copy failure,
staged corruption, publication failure, backup-rename failure and case-insensitive lookup.
Copy and rename errors are injected only in the test translation unit.

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

Still required: native prepared archive importer and UI, full folder/profile/content identity,
malformed/archive traversal/size limits, user-facing verify/reimport/remove/restore controls,
crash recovery, actual cloud/external file-provider failures, collision fixtures on a
case-sensitive volume, physical-device acceptance, and phone picker/layout inspection.
Normal licensed PE startup remains a separate parked gate.

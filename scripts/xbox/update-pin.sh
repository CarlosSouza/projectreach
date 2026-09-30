#!/bin/sh
# Moves the Xbox engine's pin to a newer upstream revision, safely
# (docs/XBOX-ENGINE.md, "Updating the engine").
#
#   scripts/xbox/update-pin.sh [--to REV] [--device UDID] [--accept]
#
# 1. fetches upstream and lists what changed since the pin, flagging changes to
#    the parts HaloPad's host depends on (the Android guest and its imports);
# 2. backs up the Xbox saves: this Mac's test saves and, with --device, the
#    iPad/iPhone's Documents/Halo Xbox/save (upstream's saves are memory
#    snapshots that a changed engine may not load);
# 3. builds the candidate (scripts/xbox/build-mac.sh) and runs the Mac smoke
#    test (scripts/xbox/smoke-mac.py: menu, campaign, match);
# 4. writes the new revision into config/xbox-engine.lock.json only with
#    --accept and only if every check passed; otherwise the checkout goes back
#    to the pinned revision.
# After accepting, rebuild the apps (scripts/xbox/build-ios.sh, then
# scripts/build-ios-app.py) and install them over the existing app.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK="$ROOT/ref/xbox-build"
ENGINE="$WORK/vol/engine"
LOCK="$ROOT/config/xbox-engine.lock.json"
TO=origin/main
DEVICE=""
ACCEPT=0
while [ $# -gt 0 ]; do
	case "$1" in
	--to) TO=$2; shift ;;
	--device) DEVICE=$2; shift ;;
	--accept) ACCEPT=1 ;;
	*) echo "unknown option $1" >&2; exit 2 ;;
	esac
	shift
done
PINNED=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['revision'])" "$LOCK")
"$ROOT/scripts/xbox/prepare.sh" >/dev/null   # mounts the volume and checks out the pin
git -C "$ENGINE" fetch -q origin
TARGET=$(git -C "$ENGINE" rev-parse "$TO")
if [ "$TARGET" = "$PINNED" ]; then
	echo "already pinned at $PINNED"
	exit 0
fi
echo "==> upstream changes $PINNED..$TARGET"
git -C "$ENGINE" log --oneline "$PINNED..$TARGET"
echo "==> changes to what HaloPad's host depends on"
git -C "$ENGINE" diff --stat "$PINNED" "$TARGET" -- port/android port/include tools/android_build.py \
	tools/android_imports.py tools/android_gl_stubs.py tools/android_posix_stubs.py port/linux/src/posix.h | cat

STAMP=$(date +%Y%m%d-%H%M%S)
BACKUP="$WORK/save-backups/$STAMP-from-$(echo "$PINNED" | cut -c1-8)"
echo "==> backing up saves to $BACKUP"
mkdir -p "$BACKUP"
for folder in data/save save-ios; do
	[ -d "$WORK/$folder" ] && ditto "$WORK/$folder" "$BACKUP/mac-$(echo $folder | tr / -)"
done
if [ -n "$DEVICE" ]; then
	xcrun devicectl device copy from --device "$DEVICE" --domain-type appDataContainer \
		--domain-identifier dev.halopad.HaloPad --source "Documents/Halo Xbox/save" --destination "$BACKUP/device-save" >/dev/null
fi
find "$BACKUP" -type f -exec shasum -a 256 {} + > "$BACKUP.sha256" || true

echo "==> building and testing $TARGET"
git -C "$ENGINE" checkout -q "$TARGET"
RESULT=1
if XBOX_REV="$TARGET" "$ROOT/scripts/xbox/build-mac.sh" > "$WORK/update-build.log" 2>&1; then
	if python3 "$ROOT/scripts/xbox/smoke-mac.py" --out "$WORK/smoke-results/$STAMP-$(echo "$TARGET" | cut -c1-8)"; then
		RESULT=0
	fi
else
	echo "build failed; see $WORK/update-build.log"
fi
if [ $RESULT -eq 0 ] && [ $ACCEPT -eq 1 ]; then
	python3 - "$LOCK" "$TARGET" <<'EOF'
import datetime, json, sys
path, revision = sys.argv[1], sys.argv[2]
lock = json.load(open(path))
lock["revision"] = revision
lock["pinned"] = datetime.date.today().isoformat()
open(path, "w").write(json.dumps(lock, indent=2) + "\n")
EOF
	echo "==> pinned $TARGET; rebuild the apps and install over the existing app"
	exit 0
fi
git -C "$ENGINE" checkout -q "$PINNED"
# put the build outputs back to the pinned engine's
"$ROOT/scripts/xbox/build-mac.sh" > "$WORK/update-restore.log" 2>&1 || echo "rebuilding the pinned engine failed; see $WORK/update-restore.log"
if [ $RESULT -eq 0 ]; then
	echo "==> $TARGET passed; run again with --accept to pin it (the checkout is back at $PINNED)"
	exit 0
fi
echo "==> $TARGET did not pass; the pin stays at $PINNED"
exit 1

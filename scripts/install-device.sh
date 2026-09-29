#!/usr/bin/env bash
# One command for a connected iPhone/iPad: build, sign, install, and put your game package
# in HaloPad's Documents so the first launch can import it (docs/INSTALL-IPHONE.md).
#
#   scripts/install-device.sh --identity "Apple Development: Name (TEAMID)" \
#       --profile ~/Downloads/HaloPad.mobileprovision --game "/path/to/Halo Custom Edition"
#
# Options: --device ID (default: the only connected device), --work RUN_DIR.
# Development builds use the menu scene (tests/halo_touch_move_scene.c) until Halo's normal
# start-up can find the product ID its original installer writes.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PY="$ROOT/.venv/bin/python"
IDENTITY="" PROFILE="" GAME="" DEVICE="" WORK=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --identity) IDENTITY=$2; shift 2 ;;
    --profile) PROFILE=$2; shift 2 ;;
    --game) GAME=$2; shift 2 ;;
    --device) DEVICE=$2; shift 2 ;;
    --work) WORK=$2; shift 2 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done
die() { echo "error: $*" >&2; exit 1; }
[[ -n "$IDENTITY" ]] || die "--identity is required (security find-identity -v -p codesigning)"
[[ -f "$PROFILE" ]] || die "--profile must name a .mobileprovision for dev.halopad.HaloPad"
[[ -d "$GAME" ]] || die "--game must name your Halo Custom Edition 1.10 folder"

if [[ -z "$DEVICE" ]]; then
  JSON=$(mktemp)
  xcrun devicectl list devices --json-output "$JSON" >/dev/null
  DEVICE=$("$PY" "$ROOT/scripts/device-id.py" "$JSON")
  [[ -n "$DEVICE" ]] || die "connect and trust exactly one iPhone/iPad, or pass --device (xcrun devicectl list devices)"
fi

echo "==> Building for the device"
WORKARG=()
[[ -n "$WORK" ]] && WORKARG=(--work "$WORK")
"$PY" "$ROOT/scripts/build-ios-app.py" --iphoneos --identity "$IDENTITY" --profile "$PROFILE" \
  --scene "$ROOT/tests/halo_touch_move_scene.c" "${WORKARG[@]}"
APP=$(ls -td "$ROOT"/generated/srw/*/run-*/ios-app-arm64-apple-ios17.0/HaloPad.app | head -1)

echo "==> Preparing your game package for this build"
PKG="$ROOT/generated/prepared/device-$(date +%Y%m%d-%H%M%S).halopad.zip"
"$PY" "$ROOT/scripts/prepare-game-data.py" --app-data "$APP/data" --game "$GAME" --output "$PKG"

echo "==> Installing on $DEVICE"
xcrun devicectl device install app --device "$DEVICE" "$APP"
xcrun devicectl device copy to --device "$DEVICE" --domain-type appDataContainer \
  --domain-identifier dev.halopad.HaloPad --source "$PKG" --destination "Documents/$(basename "$PKG")"

echo "Done. Open HaloPad, tap Choose Prepared Package, and pick $(basename "$PKG")."

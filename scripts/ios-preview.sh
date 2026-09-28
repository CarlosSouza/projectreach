#!/usr/bin/env bash
# Preview HaloPad on this Mac's iOS Simulator (development build).
#
# Builds the app with the play scene (tests/halo_app_scene.c: Halo from its main menu, prepared
# as the component tests prepare it), boots the Simulator, shows its window and launches HaloPad.
# The app keeps running for you to play: click or drag in the Simulator window as touches, type
# on the Mac keyboard, or use the three-dot menu (Join Server by Address, touch-control settings).
#
# Usage: scripts/ios-preview.sh [--iphone] [--connect ADDR:PORT]
#   --iphone               the iPhone 17 Pro Simulator instead of the iPad Pro 13
#   --connect ADDR:PORT    join that server at start (e.g. a public server from Halo's Internet
#                          lobby, or scripts/public-servers.py)
# Stop with the device-specific command printed below.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
DEVICE=E129A00F-D338-4FDC-8AE8-BB243E9BA61B        # "HaloPad iPad Pro 13"
ARGS=""
while [[ $# -gt 0 ]]; do
  case "$1" in
    --iphone) DEVICE=7E8E357A-30DD-4EB3-B8C7-83BB555E67B7; shift ;;   # iPhone 17 Pro
    --connect) ARGS="-connect $2"; shift 2 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done
WORK=$(cat "$ROOT/generated/current-run" 2>/dev/null || ls -td generated/srw/custom-en-1.0.10.0621/run-* | sed -n 1p)
xcrun simctl boot "$DEVICE" 2>/dev/null || true
open -a Simulator --args -CurrentDeviceUDID "$DEVICE"
HALOPAD_ARGS="$ARGS" SDKROOT=$(xcrun --sdk iphonesimulator --show-sdk-path) \
  .venv/bin/python scripts/build-ios-app.py --work "$WORK" --device "$DEVICE" --scene tests/halo_app_scene.c --launch --wait 3 | grep -E '^(built|evidence)'
echo "HaloPad is running in the Simulator. Stop it with: xcrun simctl shutdown $DEVICE"

#!/usr/bin/env bash
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
SRC="$ROOT/assets/HaloPadIcon.svg"
OUT="$ROOT/assets/Assets.xcassets/AppIcon.appiconset/AppIcon.png"
command -v magick >/dev/null || { echo "ImageMagick magick is required to render the icon" >&2; exit 1; }
magick -background none -density 96 "$SRC" -resize 1024x1024 -alpha off -type TrueColor PNG24:"$OUT"
identify -format '%wx%h %[opaque]\n' "$OUT"

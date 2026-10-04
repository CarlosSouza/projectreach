#!/usr/bin/env bash
# HaloPad's one-command personal build (PadMint's entry point; padmint.json).
#
#   scripts/builder/build.sh <folder or HaloCESetup.exe> --ipa HaloPad.ipa [--out DIR] [--product-key-file FILE]
#
# The folder (or the installer's own folder) holds your Halo Custom Edition installer
# (HaloCESetup.exe) and the official 1.10 update (haloce-patch-1.0.10.exe); both are checked
# against the recorded hashes. It
# runs HaloPad's existing steps in order: tool build, 1.10 patch (CrossOver), translation of
# Halo and its four DLLs, the app, its game package and an unsigned IPA. Install the IPA with
# your own signing, then import the game package in HaloPad (Files).
#
# Halo refuses to start without the product ID its installer writes. With
# product-key.txt beside the installer (or --product-key-file, or a hidden prompt when run
# in a terminal) your key goes through scripts/product-id.sh into your app only.
# Everything this makes contains translated game code and your game files: it is yours
# alone. Never share or upload it.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
cd "$ROOT"
PY=.venv/bin/python
export HALOPAD_BUILDER=1                              # steps leave tracked repository files unchanged
INPUT=""; OUT="$ROOT/generated/builder"; IPA=""; KEY_FILE=""
while [ $# -gt 0 ]; do
	case "$1" in
	--out) OUT=$2; shift ;;
	--ipa) IPA=$2; shift ;;
	--product-key-file) KEY_FILE=$2; shift ;;
	--jobs) shift ;;                                  # accepted for PadMint; the steps size themselves
	-*) echo "unknown option $1" >&2; exit 2 ;;
	*) INPUT=$1 ;;
	esac
	shift
done
[ -f "$INPUT" ] && INPUT=$(dirname "$INPUT")          # PadMint passes the installer itself
[ -d "$INPUT" ] || { echo "usage: scripts/builder/build.sh <HaloCESetup.exe, beside haloce-patch-1.0.10.exe> --ipa FILE" >&2; exit 2; }
[ -n "$IPA" ] || IPA="$OUT/HaloPad.ipa"
mkdir -p "$OUT"
mkdir -p "$(dirname "$IPA")"
IPA="$(cd "$(dirname "$IPA")" && pwd)/$(basename "$IPA")"
OUT="$(cd "$OUT" && pwd)"
step() { printf '\n==> %s\n' "$*"; }
sha() { shasum -a 256 "$1" | cut -d' ' -f1; }

step "checking tools"
for tool in xcodebuild 7zz; do command -v "$tool" >/dev/null || { echo "missing $tool (Xcode; brew install sevenzip)" >&2; exit 2; }; done
if [ ! -x "$PY" ]; then                               # a fresh checkout (PadMint's worktree)
	step "setting up HaloPad's Python tools (.venv)"
	python3 -m venv .venv
	$PY -m pip install -q -r scripts/requirements-tools.txt
fi

step "finding your installer and the 1.10 update by hash"
INSTALLER_SHA=150e430dc54ffb265cbe96605ef8909c9ba0065fa11bdbf170bfd88391cf98ba
PATCH_SHA=33818f3f56b7dddc8c61d654af6567c9c5b9220ca75d6ac23a52611038257508
INSTALLER=""; PATCH=""
while IFS= read -r -d '' f; do
	case "$(sha "$f")" in
	"$INSTALLER_SHA") INSTALLER=$f ;;
	"$PATCH_SHA") PATCH=$f ;;
	esac
done < <(find "$INPUT" -maxdepth 2 -iname '*.exe' -print0)
[ -n "$INSTALLER" ] || { echo "no Halo Custom Edition 1.00 installer (HaloCESetup.exe) with the expected hash in $INPUT" >&2; exit 3; }
# 1.10 files assembled by an earlier build are reused; the update is needed only the first time
ACCEPTED=$($PY -c "import json;print(json.load(open('config/profiles/custom-en-1.0.10.0621.json'))['accepted_sha256'])")
ASSEMBLED=0
[ -f ref/inputs/custom-original/haloce.exe ] && [ "$(sha ref/inputs/custom-original/haloce.exe)" = "$ACCEPTED" ] && ASSEMBLED=1
[ -n "$PATCH" ] || [ $ASSEMBLED = 1 ] || { echo "no official 1.10 update (haloce-patch-1.0.10.exe) with the expected hash in $INPUT" >&2; exit 3; }
# the steps read them from these ignored paths
mkdir -p ref/inputs/patches
[ -f ref/HaloCESetup.exe ] && [ "$(sha ref/HaloCESetup.exe)" = "$INSTALLER_SHA" ] || cp "$INSTALLER" ref/HaloCESetup.exe
[ -z "$PATCH" ] || [ -f ref/inputs/patches/haloce-patch-1.0.10.exe ] || cp "$PATCH" ref/inputs/patches/haloce-patch-1.0.10.exe

step "fetching pinned sources and building the translator"
scripts/bootstrap-sources.sh
BUILD=$(scripts/build-lifter.sh | sed -n 's/^BUILT: //p')
[ -n "$BUILD" ] || { echo "translator build failed" >&2; exit 4; }

if [ $ASSEMBLED = 1 ]; then
	step "reusing your assembled 1.10 game files"
else
	step "applying the 1.10 update and assembling the game files"
	[ -x /Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin/wine ] || { echo "CrossOver is needed for Halo's 1.10 update" >&2; exit 2; }
	PATCHED=$(scripts/prepare-patched-client.sh | sed -n 's/^RUN_DIR=//p')
	$PY scripts/assemble-custom-original.py --patched-run "$PATCHED"
fi
$PY scripts/extract-reference-components.py

step "translating Halo and its DLLs"
runs() { find generated/srw/custom-en-1.0.10.0621 -mindepth 1 -maxdepth 3 -type d -name 'run-*' -prune 2>/dev/null | sort; }
RUNS_BEFORE=$(runs)                                   # this build's translation runs are the new ones
for module in haloce keystone ksimeui controls msxml4; do
	scripts/srw-pipeline.sh "$BUILD" --module "$module"
done
WORK=$(ls -dt generated/srw/custom-en-1.0.10.0621/run-*/ | head -n 1)
$PY scripts/va-model.py --work "$WORK" --llasm "$BUILD/llasm/llasm"

PRODUCT_ID=()
[ -n "$KEY_FILE" ] || [ ! -f "$INPUT/product-key.txt" ] || KEY_FILE="$INPUT/product-key.txt"
if [ -n "$KEY_FILE" ] || [ -t 0 ]; then
	step "your product ID (Halo's installer step)"
	if [ -n "$KEY_FILE" ]; then scripts/product-id.sh --installer "$INSTALLER" < "$KEY_FILE"
	else scripts/product-id.sh --installer "$INSTALLER"; fi
	PRODUCT_ID=(--product-id generated/product-id/product-id.txt)
else
	echo "warning: no product key given; Halo will stop with 'Your product key is invalid' until you add one" >&2
fi

step "building HaloPad for iPhone and iPad"
APP_LOG="$OUT/build-app.log"
# the Windows edition; the optional Xbox edition is a separate local engine build (README)
HALOPAD_XBOX=off $PY scripts/build-ios-app.py --iphoneos --work "$WORK" ${PRODUCT_ID[@]+"${PRODUCT_ID[@]}"} | tee "$APP_LOG"
APP=$(sed -n 's/^built //p' "$APP_LOG" | tail -n 1)
[ -d "$APP" ] || { echo "app build failed" >&2; exit 5; }

step "packaging your game files and the IPA"
rm -f "$OUT/Halo-CE.halopad.zip" "$IPA"                # this builder's own earlier outputs
$PY scripts/prepare-game-data.py --app-data "$APP/data" --game ref/inputs/custom-original --output "$OUT/Halo-CE.halopad.zip"
STAGE=$(mktemp -d "$OUT/ipa.XXXXXX")
trap 'rm -rf "$STAGE"' EXIT
mkdir "$STAGE/Payload"
cp -R "$APP" "$STAGE/Payload/"
(cd "$STAGE" && zip -qry "$IPA" Payload)
# keep this build's finished translation (adding the Xbox edition reuses it) and drop its
# intermediate runs and the previous builder's translation (gigabytes each); other runs stay
NEW_RUNS=$(comm -13 <(printf '%s\n' "$RUNS_BEFORE") <(runs))
for run in $RUNS_BEFORE; do [ ! -f "$run/.builder" ] || rm -rf "$run"; done
for run in $NEW_RUNS; do
	if ls "$run"/*.ll >/dev/null 2>&1; then touch "$run/.builder"; else rm -rf "$run"; fi
done
printf '\nDone.\n  App (unsigned): %s\n  Game package:   %s\nInstall the IPA with your own signing, open HaloPad and choose the game package.\nBoth are yours alone: never share them.\n' "$IPA" "$OUT/Halo-CE.halopad.zip"

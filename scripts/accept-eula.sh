#!/usr/bin/env bash
# Read and accept the Halo Custom Edition license shown on first run (the game's Eula.rtf).
# HaloPad never accepts the license on the player's behalf: Halo's first-run check
# (EBUEula) passes only after the player runs this at an interactive terminal.
# Records generated/runtime-state/eula-acceptance.txt with the license file's SHA-256.
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
EULA="$ROOT/ref/inputs/custom-original/Eula.rtf"
OUT="$ROOT/generated/runtime-state/eula-acceptance.txt"
[[ -t 0 && -t 1 ]] || { echo "accept-eula.sh must be run by the player at an interactive terminal" >&2; exit 2; }
[[ -f "$EULA" ]] || { echo "missing $EULA" >&2; exit 2; }
textutil -convert txt -stdout "$EULA" | "${PAGER:-less}"
echo
read -r -p "Type I ACCEPT to accept this license, anything else to decline: " answer
if [[ "$answer" != "I ACCEPT" ]]; then echo "Declined; nothing recorded."; exit 1; fi
mkdir -p "$(dirname "$OUT")"
{
  echo "sha256 $(shasum -a 256 "$EULA" | cut -d' ' -f1)"
  echo "file Eula.rtf"
  echo "accepted $(date -u +%Y-%m-%dT%H:%M:%SZ) by $(id -un) at an interactive terminal"
} > "$OUT"
echo "Recorded: $OUT"

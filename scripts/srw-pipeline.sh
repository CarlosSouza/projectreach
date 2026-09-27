#!/usr/bin/env bash
# Whole-image SRW pipeline for one guest module of the accepted Custom Edition 1.10 profile.
# Usage: scripts/srw-pipeline.sh generated/tool-builds/<key> [--strict] [--module haloce|keystone|ksimeui]
#   default: diagnostic SRW run (census; every gap is a named trap)
#   --strict: SRW must convert everything without diagnostic fallbacks
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
BUILD=${1:?tool build directory}
shift
MODE=--diagnostic
MODULE=haloce
while [[ $# -gt 0 ]]; do
  case "$1" in
    --strict) MODE="" ;;
    --module) MODULE=$2; shift ;;
    *) echo "unknown argument $1" >&2; exit 2 ;;
  esac
  shift
done
PY=.venv/bin/python
M="--module $MODULE"
if [[ "$MODULE" == haloce ]]; then
  PROFILE_DIR=generated/srw/custom-en-1.0.10.0621; STEM=haloce
else
  PROFILE_DIR=generated/srw/custom-en-1.0.10.0621/modules/$MODULE; STEM=$MODULE
fi
latest() { ls -dt "$PROFILE_DIR"/run-*/ | head -n 1; }

$PY scripts/audit-executable.py $M
$PY scripts/srw-capability-scan.py --build "$BUILD" $M
$PY scripts/simd-reachability.py $M > /dev/null
$PY scripts/srw-traps.py $M
$PY scripts/srw-flags.py $M
# Pass 1 discovers SRW's labels; pass 2 closes procedures correctly after traps.
$PY scripts/run-srw.py --build "$BUILD" --diagnostic $M
$PY scripts/srw-traps.py --srw-output "$(latest)" $M
$PY scripts/srw-flags.py $M
$PY scripts/run-srw.py --build "$BUILD" $MODE $M
WORK=$(latest)
( cd "$WORK" && "$ROOT/$BUILD/llasm/llasm" -m64 -ptrofs -I "$ROOT/port/llasm-support" -o $STEM.ll $STEM.llasm > llasm.out 2>&1 ) \
  && echo "LLASM OK: $WORK/$STEM.ll ($(wc -c < "$WORK/$STEM.ll") bytes)" \
  || { echo "LLASM FAILED:"; tail -n 4 "$WORK/llasm.out"; exit 1; }

#!/usr/bin/env bash
# Whole-image SRW pipeline for the accepted Custom Edition 1.10 profile.
# Usage: scripts/srw-pipeline.sh generated/tool-builds/<key> [--strict]
#   default: diagnostic SRW run (census; every gap is a named trap)
#   --strict: SRW must convert everything without diagnostic fallbacks
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
BUILD=${1:?tool build directory}
MODE=--diagnostic
[[ "${2:-}" == "--strict" ]] && MODE=""
PY=.venv/bin/python
PROFILE_DIR=generated/srw/custom-en-1.0.10.0621
latest() { ls -dt "$PROFILE_DIR"/run-*/ | head -n 1; }

$PY scripts/audit-executable.py
$PY scripts/srw-capability-scan.py --build "$BUILD"
$PY scripts/simd-reachability.py > /dev/null
$PY scripts/srw-traps.py
$PY scripts/srw-flags.py
# Pass 1 discovers SRW's labels; pass 2 closes procedures correctly after traps.
$PY scripts/run-srw.py --build "$BUILD" --diagnostic
$PY scripts/srw-traps.py --srw-output "$(latest)"
$PY scripts/srw-flags.py
$PY scripts/run-srw.py --build "$BUILD" $MODE
WORK=$(latest)
( cd "$WORK" && "$ROOT/$BUILD/llasm/llasm" -m64 -ptrofs -I "$ROOT/port/llasm-support" -o haloce.ll haloce.llasm > llasm.out 2>&1 ) \
  && echo "LLASM OK: $WORK/haloce.ll ($(wc -c < "$WORK/haloce.ll") bytes)" \
  || { echo "LLASM FAILED:"; tail -n 4 "$WORK/llasm.out"; exit 1; }


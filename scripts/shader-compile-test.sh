#!/usr/bin/env bash
# Compile coverage for the shader translator over Halo's own shaders (G4).
# Needs generated/analysis/shaders/ from: scripts/run-core.py --main tools/halo_shader_extract.c
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
OUT="$ROOT/generated/tools/shader-compile-test"
mkdir -p "$(dirname "$OUT")"
clang -O1 -fobjc-arc -Wall -std=c2x -x c "$ROOT/port/runtime/halopad_shader.c" -x objective-c "$ROOT/tools/shader_compile_test.m" \
  -framework Foundation -framework Metal -o "$OUT"
"$OUT" "$ROOT/generated/analysis/shaders"

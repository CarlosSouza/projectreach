#!/bin/sh
# Guest compiler for the Xbox engine (see docs/XBOX-ENGINE.md).
#
# Upstream's Android build compiles the game as arm64_32 (AArch64 code with
# 32-bit pointers). HaloPad re-targets that code to base-relative ARM64
# (scripts/xbox/translate.py). That needs two registers the guest never uses
# (x28 holds the guest memory base, x27 is scratch) and no jump tables, so
# every indirect branch goes to a function entry.
LLVM_BIN="${XBOX_LLVM_BIN:-/opt/homebrew/opt/llvm/bin}"
exec "$LLVM_BIN/clang" -ffixed-x27 -ffixed-x28 -fno-jump-tables \
    -Wno-unused-command-line-argument "$@"

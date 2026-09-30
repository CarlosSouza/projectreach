#!/bin/sh
# Builds the Xbox engine as a macOS program (the Mac proof):
#   scripts/xbox/build-mac.sh     -> ref/xbox-build/out/halopad-xbox
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
"$ROOT/scripts/xbox/prepare.sh"
WORK="$ROOT/ref/xbox-build"
ENGINE="$WORK/vol/engine"
OUT="$WORK/out"
INC="$WORK/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"

# ---------- the macOS program
CFLAGS="-O2 -g -Wall -Wno-unused-function -I$ROOT/port/xbox -I$OUT -I$INC -I/opt/homebrew/include -mmacosx-version-min=14.4"
OBJ="$OUT/obj-macos"
mkdir -p "$OBJ"
for f in xg_memory xg_thread xg_syscall xg_gl xg_sdl xg_posix xg_main_macos; do
	clang $CFLAGS -c "$ROOT/port/xbox/$f.c" -o "$OBJ/$f.o"
done
clang $CFLAGS -c "$OUT/xg_gl_gen.c" -o "$OBJ/xg_gl_gen.o"
for f in posix_files posix_net; do
	clang -O2 -w -include "$ROOT/port/xbox/xg_darwin_compat.h" -I"$ENGINE/port/linux/src" \
		-c "$ENGINE/port/linux/src/$f.c" -o "$OBJ/upstream_$f.o"
done
clang -c "$ROOT/port/xbox/xg_runtime.s" -o "$OBJ/xg_runtime.o"
clang -c "$OUT/guest.s" -o "$OBJ/guest.o"
clang -o "$OUT/halopad-xbox" "$OBJ"/*.o -L/opt/homebrew/lib -lSDL3 -mmacosx-version-min=14.4
echo "built $OUT/halopad-xbox"

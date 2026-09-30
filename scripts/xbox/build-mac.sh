#!/bin/sh
# Builds the Xbox engine as a macOS program (the Mac proof).
#
#   scripts/xbox/build-mac.sh            build ref/xbox-build/out/halopad-xbox
#
# Everything upstream-derived stays under the ignored ref/xbox-build/:
#   engine.sparsebundle   a case-sensitive volume (upstream's headers need one)
#   vol/engine            the pinned checkout (config/xbox-engine.lock.json)
#   ndk/                  the few Android NDK pieces the guest build asks for
#   out/                  the guest image, its translation and the program
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
WORK="$ROOT/ref/xbox-build"
LLVM=${XBOX_LLVM_BIN:-/opt/homebrew/opt/llvm/bin}
LOCK="$ROOT/config/xbox-engine.lock.json"
REV=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['revision'])" "$LOCK")
URL=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['url'])" "$LOCK")
mkdir -p "$WORK/out" "$WORK/vol"

# ---------- the case-sensitive volume and the pinned checkout
if [ ! -d "$WORK/engine.sparsebundle" ]; then
	hdiutil create -quiet -type SPARSEBUNDLE -fs 'Case-sensitive APFS' -volname HaloXboxEngine -size 40g "$WORK/engine.sparsebundle"
fi
if ! mount | grep -q " on $WORK/vol "; then
	hdiutil attach -quiet -nobrowse -mountpoint "$WORK/vol" "$WORK/engine.sparsebundle"
fi
ENGINE="$WORK/vol/engine"
if [ ! -d "$ENGINE/.git" ]; then
	git clone -q "$URL" "$ENGINE"
fi
if [ "$(git -C "$ENGINE" rev-parse HEAD)" != "$REV" ]; then
	git -C "$ENGINE" fetch -q origin
	git -C "$ENGINE" checkout -q "$REV"
fi

# ---------- the NDK pieces: upstream's guest build only needs lld, llvm-ar
# and the Khronos OpenGL ES headers
NDK="$WORK/ndk"
BIN="$NDK/toolchains/llvm/prebuilt/linux-x86_64/bin"
INC="$NDK/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"
mkdir -p "$BIN" "$INC/GLES3" "$INC/GLES2" "$INC/KHR"
ln -sf "$(command -v ld.lld)" "$BIN/ld.lld"
ln -sf "$LLVM/llvm-ar" "$BIN/llvm-ar"
ln -sf "$LLVM/clang" "$BIN/clang"
KHRONOS=https://raw.githubusercontent.com/KhronosGroup
for f in GLES3/gl32.h GLES3/gl31.h GLES3/gl3.h GLES3/gl3platform.h GLES2/gl2ext.h GLES2/gl2.h GLES2/gl2platform.h; do
	[ -f "$INC/$f" ] || curl -fsSL "$KHRONOS/OpenGL-Registry/main/api/$f" -o "$INC/$f"
done
[ -f "$INC/KHR/khrplatform.h" ] || curl -fsSL "$KHRONOS/EGL-Registry/main/api/KHR/khrplatform.h" -o "$INC/KHR/khrplatform.h"

# ---------- the guest image (upstream's Android guest, our compiler flags)
(cd "$ENGINE" && python3 configure.py --release --android-ndk "$NDK" \
	--android-guest-cc "$ROOT/scripts/xbox/guest-cc.sh" >/dev/null &&
	ninja build/android/halo_guest.elf)
GUEST="$ENGINE/build/android"
OUT="$WORK/out"
cp "$GUEST/halo_guest.elf" "$OUT/halo_guest.elf"

# ---------- translation and generated host sources
python3 "$ROOT/scripts/xbox/translate.py" "$OUT/halo_guest.elf" "$OUT/guest.s" \
	--imports "$ENGINE/port/android/host_imports.list" "$GUEST/guest/gen/gl_imports.list" "$GUEST/guest/gen/posix_imports.list"
sed -n 's/^#define __NR_\([a-z0-9_]*\)[[:space:]]*\([0-9]*\)$/#define LX_NR_\1 \2/p' \
	"$GUEST/guest/libc_include/bits/syscall.h" > "$OUT/xg_linux_nr.h"
python3 "$ROOT/scripts/xbox/gen-host-gl.py" "$GUEST/guest/gen/guest_gl.c" "$OUT/xg_gl_gen.c"

# ---------- the macOS program
CFLAGS="-O2 -g -Wall -Wno-unused-function -I$ROOT/port/xbox -I$OUT -I$INC -I/opt/homebrew/include -mmacosx-version-min=14.4"
OBJ="$OUT/obj-macos"
mkdir -p "$OBJ"
for f in xg_memory xg_thread xg_syscall xg_sdl xg_posix xg_main_macos; do
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

#!/bin/sh
# Builds the Xbox engine's iOS test app (port/xbox/xg_app_ios.m) and, for the
# Simulator, installs and launches it with the Mac's extracted game data.
#
#   scripts/xbox/build-ios.sh [--device] [--launch UDID] [--identity NAME --profile FILE]
#
# A personal build: the app bundles the translated engine and must never be
# shared (docs/XBOX-ENGINE.md).
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
TARGET=arm64-apple-ios17.0-simulator
SDK=iphonesimulator
LAUNCH=""
IDENTITY="-"
PROFILE=""
while [ $# -gt 0 ]; do
	case "$1" in
	--device) TARGET=arm64-apple-ios17.0; SDK=iphoneos ;;
	--launch) LAUNCH=$2; shift ;;
	--identity) IDENTITY=$2; shift ;;
	--profile) PROFILE=$2; shift ;;
	*) echo "unknown option $1" >&2; exit 2 ;;
	esac
	shift
done
if [ -n "$LAUNCH" ] && [ "$SDK" != iphonesimulator ]; then
    echo "--launch is Simulator-only; device builds are not installed by this script" >&2
    exit 2
fi
RENDERER=${HALOPAD_XBOX_RENDERER:-apple-gles}
COUNTED=OFF
case "${HALOPAD_XBOX_GUEST_ADAPTATION:-none}" in
render-visibility-v1|render-water-v1|render-border-v1|shared-input-v1|render-present-v1)
    [ "$RENDERER" = angle-metal ] || {
        echo "Counted visibility requires the ANGLE renderer" >&2; exit 2;
    }
    COUNTED=ON
;;
esac
case "$RENDERER" in
apple-gles) ;;
angle-metal)
    [ -n "${XBOX_ANGLE_SOURCE:-}" ] || { echo "XBOX_ANGLE_SOURCE is required for ANGLE" >&2; exit 2; }
    ANGLE_REV=$(python3 -c "import json,sys;print(json.load(open(sys.argv[1]))['revision'])" "$ROOT/config/xbox-angle.lock.json")
    [ "$(git -C "$XBOX_ANGLE_SOURCE" rev-parse HEAD)" = "$ANGLE_REV" ] || { echo "ANGLE source differs from the renderer pin" >&2; exit 2; }
    [ -z "$(git -C "$XBOX_ANGLE_SOURCE" status --porcelain --untracked-files=no)" ] || { echo "Preserve ANGLE source edits before building" >&2; exit 2; }
    ;;
*) echo "Unknown HALOPAD_XBOX_RENDERER" >&2; exit 2 ;;
esac
"$ROOT/scripts/xbox/prepare.sh"
WORK="$ROOT/ref/xbox-build"
ENGINE="$WORK/vol/engine"
OUT="$WORK/out"
INC="$WORK/ndk/toolchains/llvm/prebuilt/linux-x86_64/sysroot/usr/include"
OBJ="$OUT/obj-$SDK"
APP="$OUT/$SDK/HaloPadXbox.app"
BUILD_SDK=$SDK
ANGLE_LIB=""
ANGLE_FLAGS=""
if [ "$RENDERER" = angle-metal ]; then
    BUILD_SDK=$SDK-angle
    OBJ="$OUT/obj-$BUILD_SDK"
    APP="$OUT/$BUILD_SDK/HaloPadXbox.app"
    ANGLE_BUILD="$OUT/angle-simulator"
    [ "$SDK" != iphoneos ] || ANGLE_BUILD="$OUT/angle-iphoneos"
    # The counted backend has its own ANGLE build per SDK (same reviewed recipe).
    COUNTED_BUILD="$OUT/angle-counted-simulator"
    [ "$SDK" != iphoneos ] || COUNTED_BUILD="$OUT/angle-counted-iphoneos"
    if [ "$COUNTED" = ON ]; then
        BUILD_SDK=$SDK-angle-counted
        OBJ="$OUT/obj-$BUILD_SDK"
        APP="$OUT/$BUILD_SDK/HaloPadXbox.app"
        ANGLE_BUILD="$COUNTED_BUILD"
    fi
    cmake -S "$ROOT/scripts/xbox/angle" -B "$ANGLE_BUILD" -G Ninja \
        -DHALOPAD_ANGLE_COUNTED_VISIBILITY=$COUNTED \
        -DANGLE_SOURCE_DIR="$XBOX_ANGLE_SOURCE" -DCMAKE_SYSTEM_NAME=iOS \
        -DCMAKE_OSX_SYSROOT=$SDK -DCMAKE_OSX_ARCHITECTURES=arm64 \
        -DCMAKE_OSX_DEPLOYMENT_TARGET=17.0 -DCMAKE_BUILD_TYPE=Release
    cmake --build "$ANGLE_BUILD" --parallel 12
    ANGLE_LIB="$ANGLE_BUILD/libhalopad-angle.a"
    ANGLE_FLAGS="-DXG_USE_ANGLE=1 -I$XBOX_ANGLE_SOURCE/include"
    [ "$COUNTED" != ON ] || ANGLE_FLAGS="$ANGLE_FLAGS -DXG_COUNTED_VISIBILITY=1"
fi
SYSROOT=$(xcrun --sdk $SDK --show-sdk-path)
CC="xcrun --sdk $SDK clang -target $TARGET -isysroot $SYSROOT"
CFLAGS="-O2 -g -Wall -Wno-unused-function -DGLES_SILENCE_DEPRECATION -fobjc-arc -I$ROOT/port/xbox -I$OUT -I/opt/homebrew/include $ANGLE_FLAGS"
mkdir -p "$OBJ" "$APP"
for f in xg_memory xg_thread xg_syscall xg_gl xg_posix xg_xiso; do
	$CC $CFLAGS -I"$INC" -c "$ROOT/port/xbox/$f.c" -o "$OBJ/$f.o"
done
$CC $CFLAGS -I"$INC" -c "$OUT/xg_gl_gen.c" -o "$OBJ/xg_gl_gen.o"
for f in xg_ios xg_touch xg_draw_capture xg_draw_replay xg_depth_capture xg_app_ios; do
	$CC $CFLAGS -c "$ROOT/port/xbox/$f.m" -o "$OBJ/$f.o"
done
for f in posix_files posix_net; do
	$CC -O2 -w -include "$ROOT/port/xbox/xg_darwin_compat.h" -I"$ROOT/port/xbox/compat" -I"$ENGINE/port/linux/src" \
		-c "$ENGINE/port/linux/src/$f.c" -o "$OBJ/upstream_$f.o"
done
$CC -c "$ROOT/port/xbox/xg_runtime.s" -o "$OBJ/xg_runtime.o"
[ "$OBJ/guest.o" -nt "$OUT/guest.s" ] || $CC -c "$OUT/guest.s" -o "$OBJ/guest.o"
# the engine as a library for HaloPad's own app (scripts/build-ios-app.py links
# it with port/ios/HaloPadXbox.m when it exists)
LIB="$OUT/$BUILD_SDK/libhalopad-xbox.a"
xcrun libtool -static -o "$LIB" $(ls "$OBJ"/*.o | grep -v xg_app_ios.o) $ANGLE_LIB
# Keep the library tied to its exact guest image; app packaging checks this.
python3 - "$ENGINE" "$OUT" "$BUILD_SDK" "$RENDERER" "$ROOT/config/xbox-angle.lock.json" "$ROOT" <<'PY'
import datetime, hashlib, json, pathlib, subprocess, sys
sys.path.insert(0, str(pathlib.Path(sys.argv[6]) / 'scripts/xbox'))
from runtime_manifest import sources, guest_adaptation
engine, out, sdk = sys.argv[1], pathlib.Path(sys.argv[2]), sys.argv[3]
manifest = {
    'revision': subprocess.check_output(['git', '-C', engine, 'rev-parse', 'HEAD'], text=True).strip(),
    'built': datetime.datetime.now(datetime.timezone.utc).strftime('%Y-%m-%d'),
    'guest_sha256': hashlib.sha256((out / 'halo_guest.elf').read_bytes()).hexdigest(),
    'guest_adaptation': json.loads((out / 'guest-adaptation.json').read_text()),
    'library_sha256': hashlib.sha256((out / sdk / 'libhalopad-xbox.a').read_bytes()).hexdigest(),
    'renderer': sys.argv[4],
    'sdk': sdk.split('-')[0],
    'runtime_sources': sources(),
}
if sys.argv[4] == 'angle-metal':
    manifest['angle_source'] = json.loads(pathlib.Path(sys.argv[5]).read_text())
    manifest['angle_feature_overrides'] = ['hasTextureSwizzle'] if manifest['sdk'] == 'iphonesimulator' else []
if manifest['guest_adaptation']['name'] in guest_adaptation.COUNTED_ADAPTATIONS:
    counted = 'angle-counted-iphoneos' if manifest['sdk'] == 'iphoneos' else 'angle-counted-simulator'
    manifest['visibility_backend'] = json.loads((out / counted / 'counted-visibility-v1/identity.json').read_text())
(out / sdk / 'build.json').write_text(json.dumps(manifest, indent=2) + '\n')
PY
GL_LINK="-framework OpenGLES"
[ "$RENDERER" != angle-metal ] || GL_LINK="-lc++ -lz -framework Metal -framework IOSurface"
$CC -o "$APP/HaloPadXbox" "$OBJ"/*.o $ANGLE_LIB -framework UIKit -framework QuartzCore $GL_LINK \
	-framework GameController -framework AudioToolbox -framework AVFoundation -framework Foundation -framework CoreFoundation -framework CoreGraphics
cp "$OUT/halo_guest.elf" "$APP/halo_guest.elf"
PLATFORM=iPhoneSimulator
[ "$SDK" = iphoneos ] && PLATFORM=iPhoneOS
cat > "$APP/Info.plist" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>CFBundleIdentifier</key><string>dev.halopad.HaloPad.xbox-test</string>
<key>CFBundleName</key><string>HaloPad Xbox</string>
<key>CFBundleExecutable</key><string>HaloPadXbox</string>
<key>CFBundlePackageType</key><string>APPL</string>
<key>CFBundleShortVersionString</key><string>0.1</string>
<key>CFBundleVersion</key><string>1</string>
<key>CFBundleSupportedPlatforms</key><array><string>$PLATFORM</string></array>
<key>MinimumOSVersion</key><string>17.0</string>
<key>UIDeviceFamily</key><array><integer>1</integer><integer>2</integer></array>
<key>UIRequiresFullScreen</key><true/>
<key>UILaunchScreen</key><dict/>
<key>UIStatusBarHidden</key><true/>
<key>UISupportedInterfaceOrientations</key><array><string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string></array>
<key>UISupportedInterfaceOrientations~ipad</key><array><string>UIInterfaceOrientationLandscapeLeft</string><string>UIInterfaceOrientationLandscapeRight</string></array>
<key>GCSupportsControllerUserInteraction</key><true/>
<key>UIApplicationSceneManifest</key><dict><key>UIApplicationSupportsMultipleScenes</key><false/></dict>
<key>CFBundleInfoDictionaryVersion</key><string>6.0</string>
<key>CFBundleDisplayName</key><string>HaloPad Xbox</string>
</dict></plist>
EOF
cat > "$OUT/xbox.entitlements" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0"><dict>
<key>com.apple.developer.kernel.extended-virtual-addressing</key><true/>
<key>com.apple.developer.kernel.increased-memory-limit</key><true/>
</dict></plist>
EOF
[ -z "$PROFILE" ] || cp "$PROFILE" "$APP/embedded.mobileprovision"
if [ "$SDK" = iphoneos ]; then
	codesign --force --sign "$IDENTITY" --entitlements "$OUT/xbox.entitlements" --timestamp=none "$APP"
else
	# the Simulator refuses ad-hoc apps that claim restricted entitlements (and
	# runs on the Mac's kernel, where the memory reservation needs none)
	codesign --force --sign - --timestamp=none "$APP"
fi
echo "built $APP"
if [ -n "$LAUNCH" ]; then
	xcrun simctl boot "$LAUNCH" 2>/dev/null || true
	xcrun simctl bootstatus "$LAUNCH" -b >/dev/null
	xcrun simctl install "$LAUNCH" "$APP"
	SIMCTL_CHILD_XG_DATA="$WORK/data" SIMCTL_CHILD_XG_SAVE="$WORK/save-ios" \
	SIMCTL_CHILD_XG_FRAME_DUMP="$WORK/ios-frame.ppm" SIMCTL_CHILD_XG_FRAME_DUMP_SECONDS=4 SIMCTL_CHILD_XG_GL_CHECK="${XG_GL_CHECK:-}" \
		xcrun simctl launch --terminate-running-process --stdout="$WORK/ios-stdout.txt" --stderr="$WORK/ios-stderr.txt" \
		"$LAUNCH" dev.halopad.HaloPad.xbox-test
fi

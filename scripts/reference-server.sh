#!/usr/bin/env bash
# G1b, step 1: the original Halo Custom Edition 1.10 dedicated server (haloceded.exe) in the
# project-owned CrossOver bottle "halopad-reference", on a controlled local port, private
# (sv_public 0: never listed with the master server), bound to 127.0.0.1, running Blood Gulch
# Slayer. The game files are copied into the bottle; the originals under ref/ are never used
# in place. The server is queried on its port with the GameSpy status query it answers, its
# console is captured, and everything this script started is stopped at the end.
# The client (haloce.exe), which shows the first-run license, is not run here.
#
# Usage: scripts/reference-server.sh [--seconds N] [--port P]
# Evidence: docs/artifacts/<date>/G1b/server-<stamp>/
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PY="$ROOT/.venv/bin/python"
CX=/Applications/CrossOver.app/Contents/SharedSupport/CrossOver/bin
BOTTLE=halopad-reference
BOTTLE_DIR="$HOME/Library/Application Support/CrossOver/Bottles/$BOTTLE"
SRC="$ROOT/ref/inputs/custom-original"
SECONDS_RUN=45
PORT=2302
while [[ $# -gt 0 ]]; do
  case "$1" in
    --seconds) SECONDS_RUN=$2; shift 2 ;;
    --port) PORT=$2; shift 2 ;;
    *) echo "unknown option $1" >&2; exit 2 ;;
  esac
done

die() { echo "FAIL: $*" >&2; exit 1; }
[[ -x "$CX/wine" ]] || die "CrossOver command-line tools not found at $CX"
[[ -f "$SRC/haloceded.exe" ]] || die "missing $SRC/haloceded.exe (run scripts/assemble-custom-original.py)"
WANT=$("$PY" -c 'import json,sys; m=json.load(open(sys.argv[1])); print(next(f["sha256"] for f in m["files"] if f["path"].lower()=="haloceded.exe"))' "$SRC/MANIFEST.json")
GOT=$(shasum -a 256 "$SRC/haloceded.exe" | cut -d' ' -f1)
[[ "$WANT" == "$GOT" ]] || die "haloceded.exe differs from the manifest"

STAMP=$(date -u +%Y%m%dT%H%M%SZ)
EVID="$ROOT/docs/artifacts/$(date +%Y-%m-%d)/G1b/server-$STAMP"
mkdir -p "$EVID"
if [[ ! -d "$BOTTLE_DIR" ]]; then
  "$CX/cxbottle" --bottle "$BOTTLE" --create --template winxp --description "HaloPad reference (G1b)" > "$EVID/cxbottle-create.log" 2>&1 \
    || die "could not create bottle $BOTTLE (see $EVID/cxbottle-create.log)"
fi
GAME="$BOTTLE_DIR/drive_c/Program Files/Microsoft Games/Halo Custom Edition"
mkdir -p "$GAME"
rsync -a --delete --exclude '$TEMP' --exclude 'halopad-ded' "$SRC/" "$GAME/"
cat > "$GAME/halopad-init.txt" <<'EOF'
sv_name "HaloPad reference"
sv_public 0
sv_maxplayers 16
sv_log_enabled 1
sv_map bloodgulch slayer
EOF
printf 'crossover %s\nbottle %s\nhaloceded.exe sha256 %s\nport %s\n' \
  "$(defaults read /Applications/CrossOver.app/Contents/Info.plist CFBundleShortVersionString)" "$BOTTLE" "$GOT" "$PORT" > "$EVID/environment.txt"

SERVER_PID=""
stop_all() {
  WINEPREFIX="$BOTTLE_DIR" "$CX/wineserver" -k >/dev/null 2>&1 || true
  if [[ -n "$SERVER_PID" ]]; then kill "$SERVER_PID" 2>/dev/null || true; wait "$SERVER_PID" 2>/dev/null || true; fi
  if pgrep -f 'haloceded.exe' >/dev/null; then pkill -9 -f 'haloceded.exe' || true; fi
}
trap stop_all EXIT
trap 'stop_all; trap - EXIT; exit 143' INT TERM

(cd "$GAME" && WINEDEBUG=-all "$CX/wine" --bottle "$BOTTLE" --no-gui haloceded.exe -port "$PORT" -ip 127.0.0.1 \
   -exec halopad-init.txt -path 'C:\Program Files\Microsoft Games\Halo Custom Edition\halopad-ded' > "$EVID/console.log" 2>&1) &
SERVER_PID=$!

sleep "$SECONDS_RUN" & wait $!                            # (wait: a stop signal takes effect at once)
"$PY" - "$PORT" "$EVID" <<'EOF'
import json, socket, sys
port, evid = int(sys.argv[1]), sys.argv[2]
out = {}
for query in (b'\\status\\', b'\\basic\\\\info\\'):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(3)
    s.sendto(query, ('127.0.0.1', port))
    try:
        data, _ = s.recvfrom(65535)
        out[query.decode()] = data.decode('latin-1')
    except socket.timeout:
        out[query.decode()] = None
    s.close()
json.dump(out, open(evid + '/query.json', 'w'), indent=1)
print(json.dumps(out, indent=1)[:1500])
EOF
alive=$(kill -0 "$SERVER_PID" 2>/dev/null && echo yes || echo no)
find "$GAME/halopad-ded" -iname "*.log" -exec cp {} "$EVID/" \; 2>/dev/null || true
echo "server alive after $SECONDS_RUN s: $alive" | tee "$EVID/alive.txt"
stop_all
trap - EXIT
echo "evidence ${EVID#$ROOT/}"

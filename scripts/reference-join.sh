#!/usr/bin/env bash
# G5, step 1: HaloPad's Halo joins the original Custom Edition 1.10 dedicated server.
#
# Starts the reference server (scripts/reference-server.sh: haloceded.exe in the project's
# CrossOver bottle, private, sv_public 0, bound to 127.0.0.1) on port 2310, waits until it
# answers a status query, then runs tests/halo_connect_test.c on HaloPad: Halo's own -connect
# path, client port 2305 (the server itself holds 127.0.0.1:2302-2303 on the default ports).
# Passes when the client test passes and the server's own log records the join from HaloPad's
# address. Everything started here is stopped at the end. Public servers are never contacted
# (run-core.py sets HALOPAD_NET=lan).
#
# Usage: scripts/reference-join.sh [--test tests/X.c] [--clients N] [run-core.py options, e.g. --work DIR,
#        --target T --run-prefix ...]. --clients 2 with tests/halo_match_test.c: two clients in one game.
set -euo pipefail

ROOT=$(cd "$(dirname "$0")/.." && pwd)
PY="$ROOT/.venv/bin/python"
PORT=2310
TEST="$ROOT/tests/halo_connect_test.c"
CLIENTS=1
while [[ $# -gt 0 ]]; do
  case "$1" in
    --test) TEST=$(cd "$(dirname "$2")" && pwd)/$(basename "$2"); shift 2 ;;
    --clients) CLIENTS=$2; shift 2 ;;
    *) break ;;
  esac
done
BOTTLE_DIR="$HOME/Library/Application Support/CrossOver/Bottles/halopad-reference"
LOG="$BOTTLE_DIR/drive_c/Program Files/Microsoft Games/Halo Custom Edition/halopad-ded/haloserver.log"
before=$(stat -f %z "$LOG" 2>/dev/null || echo 0)

SERVER_OUT=$(mktemp -t halopad-server)
bash "$ROOT/scripts/reference-server.sh" --seconds 600 --port "$PORT" > "$SERVER_OUT" 2>&1 &
SERVER=$!
cleanup() { kill -TERM "$SERVER" 2>/dev/null || true; wait "$SERVER" 2>/dev/null || true; }
trap cleanup EXIT

"$PY" - "$PORT" <<'EOF'
import socket, sys, time
port = int(sys.argv[1])
for _ in range(60):
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.settimeout(2)
    try:
        s.sendto(b'\\status\\', ('127.0.0.1', port))
        if s.recvfrom(4096)[0]:
            print(f'reference server answering on 127.0.0.1:{port}')
            sys.exit(0)
    except OSError:
        time.sleep(2)
    finally:
        s.close()
sys.exit('FAIL: the reference server did not answer')
EOF

set +e
HALOPAD_TEST_SERVER="127.0.0.1:$PORT" "$PY" "$ROOT/scripts/run-core.py" --main "$TEST" --clients "$CLIENTS" --timeout 240 "$@"
set -e
EVID=$(ls -td "$ROOT"/docs/artifacts/*/G3/core-* | sed -n 1p)
# every client's test must end with "PASS: 0 failure(s)" (run-core.py itself does not judge)
client=0
for out in "$EVID"/stdout.txt "$EVID"/client-*/stdout.txt; do
  [[ -f "$out" ]] || continue
  tail -1 "$out" | grep -q '^PASS: 0 failure' || { echo "FAIL: ${out#$ROOT/}: $(tail -1 "$out")"; client=1; }
done
set +e
"$PY" - "$LOG" "$before" "$EVID" "$CLIENTS" <<'EOF'
import pathlib, sys
log, before, evid = pathlib.Path(sys.argv[1]), int(sys.argv[2]), pathlib.Path(sys.argv[3])
raw = log.read_bytes()[before:] if log.exists() else b''
if before % 2:
    raw = raw[1:]
text = raw.decode('utf-16-le', errors='replace').lstrip('\ufeff')
(evid / 'server-log.txt').write_text(text)
clients = int(sys.argv[4])
ports = [f'127.0.0.1:{2305 + i}' for i in range(clients)]
joins = [l for l in text.splitlines() if '\tJOIN\t' in l and 'SUCCESS' in l]
for l in joins:
    print('server log:', l.strip())
missing = [p for p in ports if not any(p in l for l in joins)]
print(f'the reference server logged {clients} HaloPad client(s) joining' if not missing else f'FAIL: no join from {missing} in the server log')
sys.exit(0 if not missing else 1)
EOF
server=$?
echo "client test exit $client; server join check exit $server"
[[ $client -eq 0 && $server -eq 0 ]]

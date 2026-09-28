#!/usr/bin/env bash
# G6 (M14): a longer session in a public Custom Edition game with the players already there.
#
# HaloPad's Halo joins ADDR:PORT through Halo's own -connect (HALOPAD_NET=internet), stays for
# MINUTES (default 3), keeps moving so the server does not treat it as idle, and saves a
# screenshot every 1500 frames. Meanwhile the server is asked for its status every 15 seconds
# (ref/tools/HaloQuery): its own player list, scores and pings are recorded in
# server-view.jsonl beside the client's evidence, so the session is seen from both sides.
# The client sends Halo's own key value; nothing is made up.
#
# Usage: scripts/public-session.sh ADDR:PORT [MINUTES] [-- run-core.py options]
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
SERVER=$1; shift
MIN=3
if [[ $# -gt 0 && "$1" != "--" ]]; then MIN=$1; shift; fi
[[ "${1:-}" == "--" ]] && shift
HQ="$ROOT/ref/tools/HaloQuery/lib/cli.js"
VIEW=$(mktemp -t halopad-server-view)
MAP=$(node "$HQ" "$SERVER" -r -t 4000 | .venv/bin/python -c 'import sys; f=sys.stdin.read().strip().split("\\")[1:]; print(dict(zip(f[0::2],f[1::2])).get("mapname",""))')
( while true; do
    node "$HQ" "$SERVER" -r -t 4000 | .venv/bin/python -c '
import json, sys, time
f = sys.stdin.read().strip().split("\\")[1:]
d = dict(zip(f[0::2], f[1::2]))
players = [{"name": d[k], "score": d.get("score_" + k[7:]), "ping": d.get("ping_" + k[7:])} for k in d if k.startswith("player_")]
print(json.dumps({"time": time.strftime("%H:%M:%S"), "map": d.get("mapname"), "numplayers": d.get("numplayers"), "players": players}))' >> "$VIEW"
    sleep 15
  done ) &
WATCH=$!
FRAMES=$((MIN * 60 * 45))
HALOPAD_NET=internet HALOPAD_TEST_SESSION=1 HALOPAD_TEST_FRAMES=$FRAMES HALOPAD_TEST_SERVER="$SERVER" HALOPAD_TEST_MAP="$MAP" \
  .venv/bin/python scripts/run-core.py --main tests/halo_connect_test.c --timeout $((MIN * 60 * 3 + 120)) "$@" > /tmp/halopad-public-session.log 2>&1
kill $WATCH 2>/dev/null; wait $WATCH 2>/dev/null
D=$(ls -td docs/artifacts/*/G3/core-* | sed -n 1p)
cp "$VIEW" "$D/server-view.jsonl"
echo "$SERVER" > "$D/server.txt"
.venv/bin/python - "$D" <<'EOF'
import json, sys, pathlib
d = pathlib.Path(sys.argv[1])
rows = [json.loads(l) for l in open(d / 'server-view.jsonl') if l.strip()]
out = (d / 'stdout.txt').read_text(errors='replace').splitlines()
print('client:', out[-1] if out else '(no output)')
for l in out:
    if 'maps in order' in l or 'spawned on' in l or 'firing' in l:
        print('client:', l.strip())
ours = [r for r in rows if any(p['name'].startswith('New0') for p in r['players'])]
print(f'server view: {len(rows)} status replies, HaloPad listed in {len(ours)}')
for r in rows[::max(1, len(rows) // 6)]:
    me = next((p for p in r['players'] if p['name'].startswith('New0')), None)
    print(f"  {r['time']} {r['map']} {r['numplayers']} players; HaloPad: {me}")
print('evidence', d)
EOF

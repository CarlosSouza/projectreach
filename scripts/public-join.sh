#!/usr/bin/env bash
# G5: HaloPad's Halo joins public Custom Edition servers, the ones people play on.
#
# For each address given (or, with --list N, the N most populated 1.10 servers without a
# password that the master server s1.master.hosthpc.com lists), the server's current map is read
# with a status query (ref/tools/HaloQuery, Sigmmma's halo-query), then tests/halo_connect_test.c
# joins it through Halo's own -connect with the host's network (HALOPAD_NET=internet), stays for
# about 25 seconds (1,200 frames), and quits. Evidence per server under docs/artifacts (join.ppm,
# the Winsock trace). The client sends Halo's own key value; nothing is made up: a server that
# checks keys may refuse it, and that is recorded as the result.
#
# Usage: scripts/public-join.sh [--list N] [ADDR:PORT ...] [-- run-core.py options]
set -euo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
PY="$ROOT/.venv/bin/python"
HQ="$ROOT/ref/tools/HaloQuery/lib/cli.js"
[[ -f "$HQ" ]] || { echo "halo-query missing: clone Sigmmma/HaloQuery into ref/tools/HaloQuery and npm run build"; exit 2; }
LIST=0; SERVERS=()
while [[ $# -gt 0 ]]; do
  case "$1" in
    --list) LIST=$2; shift 2 ;;
    --) shift; break ;;
    *) SERVERS+=("$1"); shift ;;
  esac
done
if [[ $LIST -gt 0 ]]; then
  while read -r s; do SERVERS+=("$s"); done < <(node "$HQ" ce -a -t 8000 | "$PY" "$ROOT/scripts/public-servers.py" "$HQ" "$LIST")
fi
[[ ${#SERVERS[@]} -gt 0 ]] || { echo "no servers"; exit 2; }
field() { "$PY" -c 'import sys; f=sys.stdin.read().strip().split("\\")[1:]; d=dict(zip(f[0::2],f[1::2])); print(" ".join(d.get(k,"").strip() for k in sys.argv[1:]))' "$@"; }
SUMMARY=""
for s in "${SERVERS[@]}"; do
  status=$(node "$HQ" "$s" -r -t 4000 || true)
  map=$(printf '%s' "$status" | field mapname)
  name=$(printf '%s' "$status" | field hostname numplayers maxplayers gametype sapp)
  echo "=== $s: $name, map $map"
  [[ -n "$map" ]] || { SUMMARY+="$s: no answer to the status query"$'\n'; continue; }
  set +e
  HALOPAD_NET=internet HALOPAD_TRACE_NET=1 HALOPAD_TEST_SERVER="$s" HALOPAD_TEST_MAP="$map" \
    "$PY" "$ROOT/scripts/run-core.py" --main "$ROOT/tests/halo_connect_test.c" --fresh-state --timeout 240 "$@" > /tmp/halopad-public-join.log 2>&1
  set -e
  EVID=$(ls -td "$ROOT"/docs/artifacts/*/G3/core-* | sed -n 1p)
  printf '%s\n%s\n' "$s" "$name" > "$EVID/server.txt"
  pk=$(grep -c "NET recvfrom.*$s" "$EVID/stderr.txt" || true)
  SUMMARY+="$s ($name; map $map): $(tail -1 "$EVID/stdout.txt"); $pk packets received; evidence ${EVID#$ROOT/}"$'\n'
done
printf '\n%s' "$SUMMARY"

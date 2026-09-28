#!/usr/bin/env bash
# G5 (M13, M15): HaloPad's Halo against the private reference server (scripts/reference-join.sh)
# in the situations a player meets:
#   mapchange   the server's map cycle (1-minute time limit) moves the game from Blood Gulch to
#               Battle Creek; the client follows and spawns on both
#   reconnect   "disconnect" returns to the menus; "connect" joins again and spawns
#   password    a server with a password: without it Halo shows "Your password was rejected by
#               the server." and stays in its menus; with it (connect ADDR "PASSWORD") it joins
#   timeout     nothing listens at the address: Halo shows "Unable to join game."
# Refusals save Halo's message as refused.ppm. Prints one line per case and exits non-zero if any
# case fails. Usage: scripts/network-scenarios.sh [run-core.py options, e.g. --work DIR]
set -uo pipefail
ROOT=$(cd "$(dirname "$0")/.." && pwd)
cd "$ROOT"
FAILS=0
result() {
  local name=$1 D
  D=$(ls -td docs/artifacts/*/G3/core-* | sed -n 1p)
  local line; line=$(tail -1 "$D/stdout.txt")
  echo "$name: $line | $(grep -E 'maps in order' "$D/stdout.txt" | sed 's/^ *//') | ${D#$ROOT/}"
  [[ "$line" == PASS* ]] || FAILS=$((FAILS + 1))
}
HALOPAD_SERVER_INIT=tests/server/mapcycle.txt HALOPAD_TEST_VIA=mapchange bash scripts/reference-join.sh "$@" --timeout 420 > /tmp/halopad-scn-mapchange.log 2>&1; result mapchange
HALOPAD_TEST_VIA=reconnect bash scripts/reference-join.sh "$@" --timeout 420 > /tmp/halopad-scn-reconnect.log 2>&1; result reconnect
HALOPAD_SERVER_INIT=tests/server/password.txt HALOPAD_TEST_EXPECT=refused bash scripts/reference-join.sh "$@" > /tmp/halopad-scn-password-wrong.log 2>&1; result password-wrong
HALOPAD_SERVER_INIT=tests/server/password.txt HALOPAD_TEST_PASSWORD=halopad bash scripts/reference-join.sh "$@" > /tmp/halopad-scn-password-right.log 2>&1; result password-right
HALOPAD_TEST_SERVER=127.0.0.1:2399 HALOPAD_TEST_EXPECT=refused .venv/bin/python scripts/run-core.py "$@" --timeout 240 --main tests/halo_connect_test.c > /tmp/halopad-scn-timeout.log 2>&1; result timeout
exit $FAILS

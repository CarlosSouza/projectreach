#!/bin/sh
set -eu
cd "$(dirname "$0")/.."
exec python3 scripts/sources.py verify "$@"

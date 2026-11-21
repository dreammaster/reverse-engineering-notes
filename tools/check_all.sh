#!/bin/sh
# Build the C port, run its tests, the headless smoke run over every level and the rename-table checks.   sh tools/check_all.sh [build dir]
set -e
here=$(cd "$(dirname "$0")/.." && pwd)
b=${1:-$here/build}
cmake -S "$here/src" -B "$b" >/dev/null
cmake --build "$b" -j4 2>&1 | grep -E "warning|error" || true
"$b/test_formats" "$here" | tail -2
python3 "$here/tools/smoke_walk.py" "$b/cadaver" | tail -1
python3 "$here/tools/renames.py" check cadaver1/cad | tail -1
python3 "$here/tools/renames.py" check cadaver2/game | tail -1
python3 "$here/tools/test_renames.py" 2>&1 | tail -1

#!/bin/bash
# Runs every room of every level of both games headless for 200 ticks and prints the native TAD routine failures (step limit, unsupported opcode,
# wild jumps) that CAD_TADERR reports.   sh tools/scan_tad_errors.sh [build/cadaver]    (about 900 runs, 4 in parallel)
exe=${1:-build/cadaver}
here=$(cd "$(dirname "$0")/.." && pwd)
one() { # game level tata room
  extra=""; [ "$3" = 1 ] && extra="--tata"
  CAD_TADERR=1 SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy timeout 60 "$EXE" -g "$HERE/$1" --scene walk --level "$2" --room "$4" $extra --sim wait:200 --screenshot /dev/null.bmp 2>&1 | grep TADERR | sort | uniq -c | sed "s/^/$1 L$2 t$3 R$4 /"
}
export -f one; export EXE="$here/$exe" HERE="$here"
(for g in "cadaver1 1 0" "cadaver1 2 0" "cadaver1 3 0" "cadaver1 4 0" "cadaver1 5 0" "cadaver2 1 1" "cadaver2 2 1" "cadaver2 3 1" "cadaver2 4 1"; do set -- $g; for r in $(seq 0 99); do echo "$1 $2 $3 $r"; done; done) | xargs -P4 -L1 bash -c 'one $0 $1 $2 $3'
echo "scan done"

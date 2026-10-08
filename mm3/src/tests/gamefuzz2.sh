#!/bin/bash
# keys + mouse clicks fuzz: tests/gamefuzz2.sh N [START_ARGS]   (run from mm3/src after make mm3game); reports non-zero exits
keys=(4800 4800 4B00 4D00 5000 1051 2E43 1352 1F53 2F56 3920 1E41 3249 1769 1C0D 011B 3B00 3C00 3D00 3E00 3F00 4000 0231 0332 0433 0534 0635 2064 1E41 2146)
for seed in $(seq 1 $1); do
  RANDOM=$seed; s=""
  for i in $(seq 1 40); do
    if [ $((RANDOM % 3)) -eq 0 ]; then s="$s,m$((RANDOM % 320)):$((RANDOM % 200))"; else s="$s,${keys[$((RANDOM % ${#keys[@]}))]}"; fi
  done
  s=${s:1}
  SDL_VIDEODRIVER=dummy timeout -s KILL 60 ./mm3game ../data --headless $2 --keys $s --shot build/g2_$seed.bmp >build/g2_$seed.log 2>&1
  rc=$?
  [ $rc -ne 0 ] && echo "seed $seed rc=$rc: $(tail -1 build/g2_$seed.log) keys=$s"
done

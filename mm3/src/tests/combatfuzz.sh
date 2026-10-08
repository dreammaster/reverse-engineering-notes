#!/bin/bash
# combat key fuzz from the skeleton encounter on map 2: tests/combatfuzz.sh N   (run from mm3/src after make mm3game)
keys=(1E41 2146 1F53 2E43 1C0D 011B 3920 1769 2F56 3249 3032 1372 0231 0332 0433 4800 5000 4B00 4D00 1051 1177 2064)
for seed in $(seq 1 $1); do
  RANDOM=$seed; s="4800,3920,3920"
  for i in $(seq 1 40); do s="$s,${keys[$((RANDOM % ${#keys[@]}))]}"; done
  SDL_VIDEODRIVER=dummy timeout -s KILL 60 ./mm3game ../data --headless --at 2,1,4,0 --keys $s --shot build/cf_$seed.bmp >build/cf_$seed.log 2>&1
  rc=$?
  [ $rc -ne 0 ] && echo "seed $seed rc=$rc: $(tail -1 build/cf_$seed.log) keys=$s"
done

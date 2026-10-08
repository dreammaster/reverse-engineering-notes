#!/bin/bash
# run from mm3/src after make mm3game: tests/gamefuzz.sh N
# random command-key fuzz: reports exit codes other than 0
keys=(4800 4B00 4D00 5000 1051 2E43 1352 1F53 2F56 3920 1E41 3249 1769 1C0D 011B 3B00 3C00 3D00 3E00 3F00 4000 0231 0332 0433 0534 0635 3224 2064)
for seed in $(seq 1 $1); do
  RANDOM=$seed; s=""
  for i in $(seq 1 25); do s="$s,${keys[$((RANDOM % ${#keys[@]}))]}"; done
  s=${s:1}
  SDL_VIDEODRIVER=dummy timeout -s KILL 40 ./mm3game ../data --headless --keys $s --shot build/f_$seed.bmp >build/f_$seed.log 2>&1
  rc=$?
  [ $rc -ne 0 ] && echo "seed $seed rc=$rc: $(tail -1 build/f_$seed.log) keys=$s"
done

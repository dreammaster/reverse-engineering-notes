#!/bin/bash
# play random key/mouse scripts with every readable replacement also run through the translated original (MM3_SHADOW=1): usage tests/shadowcheck.sh N
keys=(1E41 2146 1F53 2E43 1C0D 011B 3920 1769 2F56 3249 0231 0332 4800 5000 4B00 4D00 1051 2064)
for seed in $(seq 1 ${1:-4}); do
  RANDOM=$seed; s="4800,3920,3920"
  for i in $(seq 1 50); do if [ $((RANDOM % 4)) -eq 0 ]; then s="$s,m$((RANDOM % 320)):$((RANDOM % 200))"; else s="$s,${keys[$((RANDOM % ${#keys[@]}))]}"; fi; done
  MM3_SHADOW=1 SDL_VIDEODRIVER=dummy timeout -s KILL 110 ./mm3game ../data --headless --at 2,1,4,0 --keys $s 2>&1 | grep -a "SHADOW"
done
echo "shadow check finished"

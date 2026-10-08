#!/bin/bash
# scripted fight on map 2: walk into the skeleton and attack until it dies; the log must show the treasure window
# usage (from mm3/src after make mm3game): tests/combatwin.sh
ks="4800,3920,3920"; for i in $(seq 1 60); do ks="$ks,1E41"; done
MM3_TEXTLOG=1 SDL_VIDEODRIVER=dummy timeout -s KILL 90 ./mm3game ../data --headless --at 2,1,4,0 --keys $ks --shot build/combatwin.bmp 2>&1 | grep -q "Hit a key" && echo "combat won: OK" || { echo "FAIL: no treasure window"; exit 1; }

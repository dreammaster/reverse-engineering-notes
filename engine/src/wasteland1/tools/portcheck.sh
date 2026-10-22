#!/bin/sh
# portcheck.sh [extra wlrctest args]: builds wlrctest and checks every ported function against the oracle on the captured
# states in tmp_out (see docs/wl-engine.md); prints failures only. With UPDATE=1 the junk lists of ports.txt are refreshed.
cd "$(dirname "$0")/../../.." || exit 1
ninja -C build wlrctest 2>&1 | grep -E "error|FAILED" -A6 | head -40
STATES="../tmp_out/s_map.bin ../tmp_out/s_world.bin ../tmp_out/s_combat.bin ../tmp_out/s_town.bin ../tmp_out/s_title.bin ../tmp_out/s_start.bin ../tmp_out/s_ranger.bin ../tmp_out/s_m5.bin"
./build/src/wasteland1/wlrctest.exe ../ida_scripts/wl_image.bin ../tmp_out/gc2 src/wasteland1/tools/recomp/entries.txt $STATES --ported "$@" > ../tmp_out/rct.txt 2>/dev/null
if [ "$UPDATE" = "1" ]; then
  python src/wasteland1/tools/recomp/junk.py ../tmp_out/rct.txt
  ninja -C build wlrctest 2>&1 | grep -E "error|FAILED" -A6 | head -20
  ./build/src/wasteland1/wlrctest.exe ../ida_scripts/wl_image.bin ../tmp_out/gc2 src/wasteland1/tools/recomp/entries.txt $STATES --ported "$@" > ../tmp_out/rct.txt 2>/dev/null
fi
grep -v "^[0-9A-F]* [a-z,]*$" ../tmp_out/rct.txt | grep -v "skipped" | cut -c1-220

#!/bin/sh
# fightcheck.sh: replays whole fights (runCombat, WL.EXE 11D39) on the oracle and on the native code and compares memory, screen and
# registers at the end. The entry state is made by wlemu from the usual start: it plays the key script until the breakpoint at 11D39.
# Both sides get a deterministic clock: every keyboard poll advances the timer (WLTICKPOLL).
cd "$(dirname "$0")/../../.." || exit 1
IMG=../ida_scripts/wl_image.bin
GAME=../tmp_out/gc2
STATE=../tmp_out/s_fight.bin
if [ ! -f $STATE ]; then
  ./build/src/wasteland1/wlemu.exe $IMG $GAME bp 1000:1D39 idle on run 60 key 3920 run 40 key 1C0D run 40 key 316E run 40 save $STATE > /dev/null
fi
ninja -C build wlrctest 2>&1 | grep -E "error|FAILED" -A6 | head -20
run() {
  name=$1; keys=$2
  r=$(WLTICKPOLL=1 WLSTEPS=900000000 timeout 2400 ./build/src/wasteland1/wlrctest.exe $IMG $GAME src/wasteland1/tools/recomp/entries.txt $STATE --ported --only 1D39 --seeds 1 --keys "$keys" 2>&1 | grep -E "skipped:|DIFF|ok " | head -1 | cut -c1-300)
  echo "$name: $r"
}
ESC="011b*100000"
run attack  "1c0d,1e61,1e61,1e61,1e61,1559,$ESC"
run evade   "1c0d,1265,1265,1265,1265,1559,$ESC"
run mixed   "1c0d,1e61,1265,1e61,1265,1559,$ESC"
run two     "1c0d,1e61,1e61,1e61,1e61,1559,1e61,1e61,1e61,1e61,1559,$ESC"

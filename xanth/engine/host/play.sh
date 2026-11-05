#!/bin/bash
# Plays Companions of Xanth through the host with a scripted pointer, e.g. from xanth/engine/host:
#   ./play.sh "<HOST_MOUSE script>" <labels to run> [screen.ppm]
# The fast boot (timer tick every 16 labels) reaches the first room after about 20 million labels (two seconds); keys for the restore dialog are queued at 1M, 2M, 4M and 8M.
# Pointer coordinates are the game's 320x200 screen: compass NW (11,137), W (11,151); WAIT (15,175); verbs at x=20, y = 8 (Take), 18 (Put), 28 (Look at), 38 (Open), 48 (Close), 58 (Talk to), 68 (Look).
# Clicking a verb and then an object runs it; DOUBLE-clicking an object runs its default verb (shown while hovering). Hovering prints the object name (Font_WriteString events).
cd "$(dirname "$0")"
GAME=${GAME:-../../game}
[ -n "$3" ] && export HOST_SHOT="$3"
export HOST_TICKMASK=15 HOST_DRAW=1 HOST_INPUT="1000000:316E,2000000:316E,4000000:316E,8000000:316E" HOST_MOUSE="$1" HOST_STEPS=$2 HOST_MAIN=1
timeout 600 ./host_test.exe "$GAME" ../../game_out/image.bin ../../reversing/analysis/xanth_objects.tsv init 2>&1

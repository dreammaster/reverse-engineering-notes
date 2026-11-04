#!/bin/bash
# Run an original Cadaver game under DOSBox on a virtual X display, optionally
# send keys, and grab native-resolution screenshots (DOSBox's Ctrl+F5 capture).
# Reference oracle for checking the C reimplementation against the real thing.
#
#   tools/dosbox_capture.sh <game_dir> <out_dir> <exe> "<t:key> <t:key> ..." 
#
# Each step is  <seconds_to_wait>:<xdotool key name | SHOT>.  Example:
#   tools/dosbox_capture.sh cadaver1 /tmp/out cadaver "5:SHOT 1:Return 3:SHOT"
set -e
GAME=$(readlink -f "$1"); OUT=$(readlink -f "$2"); EXE=${3:-cadaver}; STEPS=$4
mkdir -p "$OUT"
RUN=$(mktemp -d); cp -r "$GAME"/. "$RUN"/
cat > "$RUN/../dbx_$$.conf" <<EOF
[sdl]
fullscreen=false
output=surface
[dosbox]
machine=vgaonly
memsize=16
captures=$OUT
[render]
aspect=false
[cpu]
core=normal
cycles=max
[mixer]
nosound=true
[sblaster]
sbtype=none
[autoexec]
mount c $RUN
c:
$EXE
exit
EOF
pkill -f "Xvfb :99" 2>/dev/null || true
Xvfb :99 -screen 0 800x600x24 >/dev/null 2>&1 &
XPID=$!
sleep 1
export DISPLAY=:99 SDL_AUDIODRIVER=dummy
dosbox -conf "$RUN/../dbx_$$.conf" >/dev/null 2>&1 &
DPID=$!
sleep 2
for s in $STEPS; do
  t=${s%%:*}; k=${s#*:}
  sleep "$t"
  if [ "$k" = SHOT ]; then xdotool key ctrl+F5; else xdotool key "$k"; fi
done
sleep 1
kill $DPID 2>/dev/null || true; kill $XPID 2>/dev/null || true
rm -rf "$RUN" "$RUN/../dbx_$$.conf"
ls "$OUT"

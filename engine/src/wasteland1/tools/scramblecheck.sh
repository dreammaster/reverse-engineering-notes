#!/bin/sh
# scramblecheck.sh: plays scripted sessions twice on the recompiled game, once with the junk registers of every port
# overwritten by garbage (WL_SCRAMBLE), and compares the data segment, graphics buffer and screen. A difference means some
# caller reads a register or flag a port declares junk.
cd "$(dirname "$0")/../../.." || exit 1
W=./build/src/wasteland1/wlrc.exe
IMG=../ida_scripts/wl_image.bin
GAME=../tmp_out/gc2
run() {
  name=$1; shift
  unset WL_SCRAMBLE
  WL_MAXEDGES=2000000000 timeout 600 $W $IMG $GAME "$@" save ../tmp_out/sc_ref.bin > /dev/null 2>&1
  WL_SCRAMBLE=1 WL_MAXEDGES=2000000000 timeout 600 $W $IMG $GAME "$@" diff ../tmp_out/sc_ref.bin 2>&1 | tail -3 | sed "s/^/$name: /"
}
K="key"; W3="wait 300"
run combat wait 300 $K 3920 $W3 $K 1C0D $W3 $K 316E $W3 $K 1C0D $W3 $K 1E61 $W3 $K 1E61 $W3 $K 1E61 $W3 $K 1E61 wait 400
run world wait 300 $K 3920 $W3 $K 1C0D $W3 $K 1559 $W3 $K 1950 $W3 $K 4D00 $W3 $K 4D00 $W3 $K 5000 $W3 $K 4B00 $W3 $K 4800 $W3 $K 4D00 $W3 $K 4D00 wait 400
run menus wait 300 $K 3920 $W3 $K 1C0D $W3 $K 1559 $W3 $K 1950 $W3 $K 1665 $W3 $K 011B $W3 $K 1265 $W3 $K 011B $W3 $K 1F73 $W3 $K 011B $W3 $K 3231 $W3 $K 011B $W3 $K 3332 $W3 $K 011B wait 400

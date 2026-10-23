#!/bin/sh
# Regenerate src1/gen/ProgNGen.{h,cpp} from the IDA exports in yendor1/ (see tools/asm2cpp.py).
cd "$(dirname "$0")/.." || exit 1
for n in 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18; do
  python tools/asm2cpp.py prog$n Prog${n}Gen gen/Prog${n}Gen "sendPaletteCommand=m.paletteCommand()" || exit 1
done

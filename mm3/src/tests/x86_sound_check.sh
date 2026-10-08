#!/bin/bash
# Check the x86 interpreter against Unicorn on the AdLib driver: same API calls, the OPL/PIT register writes must be identical.
# usage (from mm3/src): tests/x86_sound_check.sh   (needs python3 + unicorn, ../data/MM3.CC)
set -e
mkdir -p build/drv
python3 -I - <<'PY'
import sys
sys.path.insert(0, '../tools')
import mm3_cc
d = open('../data/MM3.CC', 'rb').read()
want = {mm3_cc.name_id(n): n for n in ['ADLIB.DRV', 'MM3THEME.M', 'COMBAT.M']}
for ident, off, size in mm3_cc.read_toc(d):
    if ident in want:
        open('build/drv/' + want[ident], 'wb').write(mm3_cc.member(d, off, size)[0])
PY
cc -O1 -o build/test_x86_sound tests/test_x86_sound.c x86.c
FX=$(seq 0 150 | tr '\n' ' ')
for song in COMBAT MM3THEME; do
  ./build/test_x86_sound build/drv/ADLIB.DRV build/drv/$song.M 1500 $FX > build/x86_c.txt
  python3 tests/x86_sound_ref.py build/drv/ADLIB.DRV build/drv/$song.M 1500 $FX > build/x86_ref.txt
  cmp build/x86_c.txt build/x86_ref.txt && echo "$song: $(wc -l < build/x86_c.txt) register writes identical"
done

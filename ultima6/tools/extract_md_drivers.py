#!/usr/bin/env python3
"""Unpacks the drivers of Martian Dreams (the video drivers TM.DRV and SUPERTM.DRV, the sound driver STRAX.DRV) into their raw 16-bit code blocks.

    extract_md_drivers.py md/game/MARTIAN md/game/drv

Each *.DRV is an archive with a size header (u32 file size, then u32 entries: the low 24 bits the offset of the item, the high byte its flag, 1 = LZW) holding one block per kind of display (item 0 is the VGA
one, MARTIAN.EXE's word_29658 = 0; 1 the EGA, 3 the Tandy or CGA, 2 shares the block of the next item). A block begins with a table of near entry offsets (word 0 is a flag the code sets while it
runs, word n the entry n), the code and the data follow; the game also puts a far pointer to its resource locator (sub_222E8) into the table (TM: offset 0x2E, SUPERTM: 0xC).
"""
import os
import struct
import sys

src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), 'lzw_extract.py')).read().split('if __name__')[0]
ns = {}
exec(src, ns)
lzw = ns['lzw']

indir, outdir = sys.argv[1], sys.argv[2]
os.makedirs(outdir, exist_ok=True)
for name in ('TM', 'SUPERTM', 'STRAX'):
    d = open(os.path.join(indir, name + '.DRV'), 'rb').read()
    first = struct.unpack_from('<I', d, 4)[0] & 0xFFFFFF
    n = (first - 4) // 4
    for k in range(n):
        e = struct.unpack_from('<I', d, 4 + 4 * k)[0]
        if e >> 24 != 1:
            continue
        out = bytes(lzw(d, e & 0xFFFFFF))
        path = os.path.join(outdir, '%s_%d.bin' % (name, k))
        open(path, 'wb').write(out)
        print(path, len(out))

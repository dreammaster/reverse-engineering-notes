#!/usr/bin/env python3
"""Decode `OUTDAT.DAT` -- Legacy of the Ancients' overworld name / stat
table (distinct from `OUTDATA.BSV`, the overworld graphics).

    python decoders/outdat_dat.py [C:\\games\\lota\\OUTDAT.DAT]

The reader
----------
`OUT.EXE` `outInit` opens `OUTDAT.DAT` as file #1 (the string constant is
at DGROUP `0x24A2`, descriptor `0x24AC`), `SEEK`s to byte 7 (past the
6-byte header) and reads **7 BASIC arrays** straight through with
`rtm_FE68` (string array) / `rtm_FE37` (int array).  Their `DIM` bounds
(from the `rt_AF` calls at the top of `outInit`) exactly tile the file:

    rtm_FE68 ds:2106  DIM$(23)  -> 24  PLACE names   (pool 1 [0..23])
    rtm_FE68 ds:2118  DIM$(23)  -> 24  GEM names     (pool 1 [24..47])
    rtm_FE37 ds:209A  DIM(31)   -> 32 words  A1  (0x185)
    rtm_FE37 ds:20AC  DIM(31)   -> 32 words  A2  (0x1C5)
    rtm_FE37 ds:20BE  DIM(31)   -> 32 words  A3  (0x205)
    rtm_FE37 ds:20D0  DIM(23)   -> 24 words  A4  (0x245)
    rtm_FE68 ds:20E2  DIM$(31)  -> 32  CREATURE names (pool 2)

    (32+32+32+24) words = 240 bytes = exactly the 0x185..0x274 block.

Layout (1012-byte file)
----------------------
    0x000-0x005  header 15 42 90 03 07 00
    0x006-0x184  pool 1: 48 length-prefixed strings
                   [0..23]  24 place names   ZARYL, TARMALON, MERRILL, ...
                   [24..47] 24 gem/stone names DIAMOND, EMERALD, RUBY, ...
    0x185        A1  32 words, one per creature.  Each word packs two
                 bytes: low = HP-ish (15..200; SPRAYFISH 160, MAMMOTH
                 SCREECHER 200), high = attack / XP (20..55).
    0x1C5        A2  32 words, one per creature.  low byte = a second
                 stat (tracks A1 loosely -- reward?), high byte = a
                 group/biome tag (usually 0x63 = 99, else 0x01-0x04).
    0x205        A3  32 words -- first 8 are small ints (3,4,6,3,10,15,
                 18,12), then byte-pairs with a 0xFF in the run.  Not
                 split.
    0x245        A4  24 words, one per place (or gem):
                 86,77,51,39,4,14,7,31,48,55,84,82,43,82,57,78,80,42,
                 24,6,31,16,4,3  -- an overworld map/region index?
    0x275-0x3F3  pool 2: 32 creature names, index-aligned to the 32
                 `OUTDATA.BSV` image/AND-mask sprite pairs (0x1400+):
                 PIXIE, STRIDER, FARMER, EATON WARRIOR, BANDIT, ...
                 SCORPOD, ... MAMMOTH SCREECHER, MIME GHOUL, MASTON LEAPER
"""
import struct
import sys

HEADER = 6


def pool(d, i, maxlen=40):
    out = []
    while i < len(d):
        n = d[i]
        if n == 0 or n > maxlen or i + 1 + n > len(d):
            break
        s = d[i + 1:i + 1 + n]
        if not all(0x20 <= b < 0x7F for b in s):
            break
        out.append(s.decode("latin1"))
        i += 1 + n
    return out, i


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\OUTDAT.DAT"
    d = open(path, "rb").read()
    print(f"{path}  {len(d)} B   header {struct.unpack_from('<3H', d, 0)}")

    p1, e1 = pool(d, HEADER)
    places, gems = p1[:24], p1[24:48]
    print(f"\n24 PLACES : {', '.join(places)}")
    print(f"24 GEMS   : {', '.join(gems)}")

    a1 = struct.unpack_from("<32H", d, e1)
    a2 = struct.unpack_from("<32H", d, e1 + 64)
    a3 = struct.unpack_from("<32H", d, e1 + 128)
    a4 = struct.unpack_from("<24H", d, e1 + 192)

    p2, _ = pool(d, e1 + 240)
    print(f"\n32 CREATURES + stats  (A1 = HP|atk, A2 = stat|biome):")
    for i, name in enumerate(p2):
        hp, atk = a1[i] & 0xFF, a1[i] >> 8
        s2, tag = a2[i] & 0xFF, a2[i] >> 8
        print(f"  [{i:2}] {name:18} HP={hp:3} atk={atk:2}   A2={s2:3} tag={tag:#04x}")

    print(f"\nA3 (32 w): {list(a3)}")
    print(f"A4 (24 w, per place?): {list(a4)}")


if __name__ == "__main__":
    main()

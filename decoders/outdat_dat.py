#!/usr/bin/env python3
"""Decode `OUTDAT.DAT` -- Legacy of the Ancients' overworld name / data
table (24 place names + 24 gem names + 32 creature names + numeric
sub-tables).  Distinct from `OUTDATA.BSV` (the overworld graphics).

    python decoders/outdat_dat.py [C:\\games\\lota\\OUTDAT.DAT]

Layout (1012-byte file)
----------------------
    0x000-0x005  header: 15 42 90 03 07 00  (words 0x4215, 0x0390, 0x0007)
    0x006-0x184  POOL 1 -- 48 length-prefixed strings (`db len ; db chars`):
                   [0..23]  24 PLACE names   (ZARYL, TARMALON, MERRILL, ...)
                   [24..47] 24 GEM/stone names (DIAMOND, EMERALD, RUBY, ...)
    0x185-0x274  a 240-byte NUMERIC block, 3 sub-tables:
                   ~0x185  small byte pairs (map coords / ids?)
                   ~0x1C9  `(value, 0x63)` pairs with 0x01-0x04 run markers
                   0x209   `03`, then 7 words (4,6,3,10,15,18,12), then
                           ~25 byte-pairs (0xFF-terminated), then
                   0x243   24 words: 86,77,51,39,4,14,7,31,48,55,84,82,43,
                           82,57,78,80,42,24,6,31,16,4,3
                 (exact field meaning TBD -- likely per-place/per-gem
                 coordinates + per-creature or per-gem stats.)
    0x275-0x3F3  POOL 2 -- 32 CREATURE names, length-prefixed.  These map
                 1:1 to the 32 image/AND-mask sprite pairs in
                 `OUTDATA.BSV` at 0x1400 (see decoders/outdata.py):
                   PIXIE, STRIDER, FARMER, EATON WARRIOR, BANDIT,
                   SHADOW WISP, HUGGYN, SPRAYFISH, WAVE SKIMMER,
                   SEA SWALLOW, GIANT MANTARAY, WIND STALKER, SCORPOD,
                   BONE DWELLER, PRACTON PIERCER, CARRION MANGLER,
                   VENTRO FLAILER, STINGING RAKISH, BLISTOPOD,
                   PIT STRIKER, SLASH NETTLE, VENOM FLOATER, PULP CRAWLER,
                   THRUST CREEPER, SLIME WIERD, SCRABBLER, NEURAL CLOUD,
                   CHURLER, ROCK BEETLE, MAMMOTH SCREECHER, MIME GHOUL,
                   MASTON LEAPER.

`OUTDAT.DAT` is not opened by literal name in any disassembled module --
it is loaded through a computed filename (the LEGLIB loader resolves the
disk from `DRCONFIG.DAT`; the file is on disk 3).
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


def find_next_pool(d, i):
    """Skip binary bytes to the next length-prefixed ASCII run."""
    while i < len(d):
        n = d[i]
        if 3 <= n <= 20 and i + 1 + n <= len(d) and \
                all(0x41 <= b <= 0x5A or b == 0x20 for b in d[i + 1:i + 1 + n]):
            return i
        i += 1
    return len(d)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\OUTDAT.DAT"
    d = open(path, "rb").read()
    print(f"{path}  {len(d)} B   header {struct.unpack_from('<3H', d, 0)}")

    p1, e1 = pool(d, HEADER)
    print(f"\nPOOL 1  0x{HEADER:03X}..0x{e1:03X}  ({len(p1)} strings)")
    print("  places [0..23]: " + ", ".join(p1[:24]))
    print("  gems  [24..47]: " + ", ".join(p1[24:]))

    j = find_next_pool(d, e1)
    print(f"\nnumeric block  0x{e1:03X}..0x{j:03X}  ({j - e1} B)")
    print("  " + " ".join(f"{b:02x}" for b in d[e1:j]))
    tail24 = struct.unpack_from("<24H", d, j - 48)
    print(f"  last 24 words (0x{j-48:03X}): {list(tail24)}")

    p2, e2 = pool(d, j)
    print(f"\nPOOL 2  0x{j:03X}..0x{e2:03X}  ({len(p2)} creature names)")
    for k, s in enumerate(p2):
        print(f"  [{k:2}] {s}")
    if e2 < len(d):
        print(f"\ntrailing 0x{e2:03X}..0x{len(d):03X}: {d[e2:].hex()}")


if __name__ == "__main__":
    main()

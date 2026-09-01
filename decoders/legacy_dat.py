#!/usr/bin/env python3
"""Decode `LEGACY.DAT` -- Legacy of the Ancients' master string / data
table, loaded once at start-up by `MENU.EXE`'s `menuStartup`.

    python decoders/legacy_dat.py [C:\\games\\lota\\LEGACY.DAT]

Layout (2945-byte file):

    0x000-0x005  header:  05 06  (the shared BASIC-data marker, cf.
                 CHAR.DAT), then  dw 0x0A04 , dw 0x017E (= 382, the
                 CHAR.DAT record length).
    0x006-0x605  the 8x8 CGA software font -- 96 glyphs (ASCII 0x20..0x7F),
                 16 B each, 2 bpp field-interleaved.  See decoders/legacy_font.py.
    0x606-0x647  two small resident tables: the game-speed timing table
                 (ds:1E8E) and the first-person movement table (ds:1C4E).
    0x648-0xA02  the STRING POOL -- 123 length-prefixed strings
                 `db len ; db chars`, in fixed order:
                   [0..18]   the A-Z command names (Armor, Climb,
                             Disembark, End, Fight, Gamespeed, Hold,
                             Inventory, Leave, Magic, Open, Pass, Rob,
                             Speak, Take, Use, " " (Q, unused), Weapon,
                             Xamine)
                   [19..23]  weapon condition (Shoddy..Superb)
                   [24..32]  weapons (bare hands .. Compound bow)
                   [33..37]  armor (Studded hide .. Mythan plate)
                   [38..54]  per-item "use" verbs
                   [55..78]  items (nothing, Gold armband .. the 7 gem
                             coins)
                   [79..84]  spells (Magic flame .. Seek spell)
                   [85..88]  directions (North/East/South/West)
                   [89..100] menu responses (Yes/No/Buy/Sell/... /Nothing)
                   [101..110] the digits "0".."9"
                   [111..122] the 12 town names (Isle City .. Eagle
                             Hollow)
    0xA03-end    a trailing 382-byte block = the **new-character
                 template** copied into empty `CHAR.DAT` slots: a
                 14-byte name ("empty" + spaces) + the 368-byte scalar
                 block (default stats / starting kit).  Its tail is a
                 shop **price table** (weapon / armor / item costs:
                 400, 350, 350, ... 1200, 300, 420, ... 2000).
"""
import struct
import sys

POOL_START = 0x648
GROUPS = [
    (0, "commands (A-Z)"), (19, "weapon condition"), (24, "weapons"),
    (33, "armor"), (38, "item use-verbs"), (55, "items"),
    (79, "spells"), (85, "directions"), (89, "menu responses"),
    (101, "digits"), (111, "town names"),
]


def load(path):
    return open(path, "rb").read()


def header(d):
    return struct.unpack("<HHH", d[:6])


def strings(d):
    out = []
    i = POOL_START
    while i < len(d):
        n = d[i]
        if n == 0 or i + 1 + n > len(d):
            break
        s = d[i + 1:i + 1 + n]
        if not all(0x20 <= b < 0x7F for b in s):
            break
        out.append((i, s.decode("latin1")))
        i += 1 + n
    return out, i


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\LEGACY.DAT"
    d = load(path)
    m, w1, w2 = header(d)
    print("LEGACY.DAT  %d B   header: %04X %04X %04X" % (len(d), m, w1, w2))
    print("font: 0x006 - 0x605  (96 glyphs; see legacy_font.py)")
    print("tables: 0x606 - 0x%03X  (game-speed timing + first-person move)" % (POOL_START - 1))

    strs, pool_end = strings(d)
    gi = {i: name for i, name in GROUPS}
    print("\nstring pool: 0x%03X - 0x%03X   (%d strings)" % (POOL_START, pool_end - 1, len(strs)))
    for n, (off, s) in enumerate(strs):
        if n in gi:
            print("  -- %s --" % gi[n])
        print("  [%3d] @%04X  %r" % (n, off, s))

    tail = d[pool_end:]
    name = tail[:14].rstrip(b" \x00").decode("latin1")
    print("\ntrailing block: 0x%03X - end  (%d B = the CHAR.DAT template)" % (pool_end, len(tail)))
    print("  name field: %r" % name)
    # the price table is the run of round numbers at the end
    words = struct.unpack("<%dH" % (len(tail) // 2), tail[:len(tail) // 2 * 2])
    prices = [w for w in words[-45:] if 0 < w <= 5000]
    print("  price-table tail (round values): %s" % prices)


if __name__ == "__main__":
    main()

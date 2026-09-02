#!/usr/bin/env python3
"""
Reader for the DOS Wizardry SCENARIO.DATA scenario database.

Layout (all little-endian, 512-byte blocks, offsets from the start of the file):

  block 0    TSCNTOC header (table of contents)
  block N..  one contiguous run of records per data type

TSCNTOC (the parts we use):

    off  0   GAMENAME   STRING[40]           (len byte + 40, +1 pad = 42 bytes)
    off 0x2A RECPER2B[8] u16   records packed per 1024-byte (2-block) unit
    off 0x3A RECPERDK[8] u16   total record count for the type
    off 0x4A RECSIZE[8]  u16   record size in bytes   (Apple's "UNUSEDXX")
    off 0x5A BLOFF[8]    u16   first block of the type's data
    off 0x6A RACE[6]     STRING[9]  ("NO RACE","HUMAN","ELF","DWARF","GNOME","HOBBIT")
    0x0A6    CLASS[8], 0xF6 STATUS[8], 0x146 ALIGN[4]  (all STRING, 10 B/entry)
             SPELLHSH[51] u16, SPELLGRP (packed 3-bit ×51), SPELL012 (packed 2-bit ×51)

The 8 data types (enum TZSCN):

    0 ZZERO     the TOC itself
    1 ZMAZE     maze levels          (10)
    2 ZENEMY    monsters             (101)
    3 ZREWARD   reward / treasure    (24)
    4 ZOBJECT   items                (101)
    5 ZCHAR     character roster     (20 slots)
    6 ZSPCCHRS  "special characters" (0)
    7 ZEXP      experience tables    (1)

Record R of type T lives at:
    block  = BLOFF[T] + 2*(R // RECPER2B[T])
    offset = RECSIZE[T] * (R % RECPER2B[T])       within that 1024-byte unit

Usage:
    python tools/scenario.py toc     <SCENARIO.DATA>
    python tools/scenario.py list    <SCENARIO.DATA> <type>
    python tools/scenario.py rec     <SCENARIO.DATA> <type> <index>   [--raw]
    python tools/scenario.py dump    <SCENARIO.DATA> <type> <outdir>

<type> is a name (maze, monster, reward, object, char, exp) or number 0-7.
"""

import os
import sys
import struct

try:
    sys.stdout.reconfigure(encoding="utf-8", errors="replace")
except Exception:
    pass


def _clean(bs):
    return bytes(c for c in bs if 32 <= c < 127).decode("ascii", "replace")


BLOCK = 512
TYPE_NAMES = ["zero", "maze", "monster", "reward", "object", "char", "spcchr", "exp"]
TYPE_ALIASES = {
    "toc": 0, "zero": 0, "maze": 1, "mazes": 1, "monster": 2, "monsters": 2,
    "enemy": 2, "reward": 3, "rewards": 3, "object": 4, "objects": 4, "item": 4,
    "items": 4, "char": 5, "chars": 5, "character": 5, "spcchr": 6, "exp": 7,
}
RACE = ["NO RACE", "HUMAN", "ELF", "DWARF", "GNOME", "HOBBIT"]
CLASS = ["FIGHTER", "MAGE", "PRIEST", "THIEF", "BISHOP", "SAMURAI", "LORD", "NINJA"]
STATUS = ["OK", "AFRAID", "ASLEEP", "PLYZE", "STONED", "DEAD", "ASHES", "LOST"]
ALIGN = ["UNALIGN", "GOOD", "NEUTRAL", "EVIL"]


class Scenario:
    def __init__(self, path):
        with open(path, "rb") as fh:
            self.data = fh.read()
        self.path = path
        self.nblocks = len(self.data) // BLOCK
        h = self.data
        nlen = h[0]
        self.game_name = h[1:1 + nlen].decode("ascii", "replace")
        self.recper2b = list(struct.unpack_from("<8H", h, 0x2A))
        self.count = list(struct.unpack_from("<8H", h, 0x3A))
        self.recsize = list(struct.unpack_from("<8H", h, 0x4A))
        self.bloff = list(struct.unpack_from("<8H", h, 0x5A))

    # -- string arrays in the TOC -----------------------------------------
    def _strings(self, off, n, width):
        out = []
        for i in range(n):
            p = off + i * width
            ln = self.data[p]
            out.append(_clean(self.data[p + 1:p + 1 + ln]))
        return out, off + n * width

    def toc_strings(self):
        off = 0x6A
        race, off = self._strings(off, 6, 10)
        klass, off = self._strings(off, 8, 10)
        status, off = self._strings(off, 8, 10)
        align, off = self._strings(off, 4, 10)
        return {"race": race, "class": klass, "status": status,
                "align": align, "_end": off}

    # -- record access ---------------------------------------------------
    def record(self, t, r):
        size = self.recsize[t]
        per = self.recper2b[t]
        blk = self.bloff[t] + 2 * (r // per)
        off = blk * BLOCK + size * (r % per)
        return self.data[off:off + size]

    def records(self, t):
        return [self.record(t, r) for r in range(self.count[t])]

    def type_blocks(self, t):
        per = self.recper2b[t]
        cnt = self.count[t]
        units = (cnt + per - 1) // per if per else 0
        return self.bloff[t], self.bloff[t] + 2 * units


def resolve_type(s):
    s = s.lower()
    if s in TYPE_ALIASES:
        return TYPE_ALIASES[s]
    return int(s, 0)


def hexdump(b, base=0):
    for i in range(0, len(b), 16):
        row = b[i:i + 16]
        h = " ".join(f"{x:02x}" for x in row)
        t = "".join(chr(x) if 32 <= x < 127 else "." for x in row)
        print(f"  {base + i:04x}  {h:<47}  {t}")


def pstr(b, off):
    """UCSD STRING at b[off]: length byte then chars."""
    ln = b[off]
    return _clean(b[off + 1:off + 1 + ln]), off + 1 + ln


def cmd_toc(args):
    sc = Scenario(args[0])
    print(f"{sc.path}  ({len(sc.data)} bytes, {sc.nblocks} blocks)")
    print(f"game name : {sc.game_name!r}\n")
    print(f"  {'type':<9} {'count':>6} {'recsize':>8} {'per2b':>6} {'bloff':>6} {'blocks':>12}")
    for t in range(8):
        a, b = sc.type_blocks(t)
        print(f"  {TYPE_NAMES[t]:<9} {sc.count[t]:>6} {sc.recsize[t]:>8} "
              f"{sc.recper2b[t]:>6} {sc.bloff[t]:>6}   {a:>3}..{b - 1:<3}")
    s = sc.toc_strings()
    print(f"\n  race  : {s['race']}")
    print(f"  class : {s['class']}")
    print(f"  status: {s['status']}")
    print(f"  align : {s['align']}")
    print(f"  toc string block ends at byte {s['_end']:#x}")


# name-pool keys are positional, not stored in the record:
#   monster r -> 13000 + 4*r + {0 unidSing, 1 unidPlur, 2 realSing, 3 realPlur}
#   object  r -> 14000 + 2*r + {0 unidName, 1 realName}
NAME_KEY = {2: lambda r: 13000 + 4 * r + 2, 4: lambda r: 14000 + 2 * r + 1}


def cmd_list(args):
    sc = Scenario(args[0])
    t = resolve_type(args[1])
    pool = None
    if len(args) > 2:
        import strpool
        pool = strpool.StringPool(args[2])
    for r in range(sc.count[t]):
        if pool and t in NAME_KEY:
            b = pool.get(NAME_KEY[t](r))
            nm = b.decode("latin1") if b else "?"
        else:
            nm, _ = pstr(sc.record(t, r), 0)
        print(f"  [{r:3d}] {nm}")


def cmd_rec(args):
    sc = Scenario(args[0])
    t = resolve_type(args[1])
    r = int(args[2], 0)
    rec = sc.record(t, r)
    a, _ = sc.type_blocks(t)
    blk = sc.bloff[t] + 2 * (r // sc.recper2b[t])
    print(f"type {TYPE_NAMES[t]} record {r}/{sc.count[t]}  "
          f"size={len(rec)}  block={blk} off={sc.recsize[t] * (r % sc.recper2b[t])}")
    hexdump(rec)


def cmd_dump(args):
    sc = Scenario(args[0])
    t = resolve_type(args[1])
    outdir = args[2]
    os.makedirs(outdir, exist_ok=True)
    for r in range(sc.count[t]):
        with open(os.path.join(outdir, f"{TYPE_NAMES[t]}_{r:03d}.bin"), "wb") as fh:
            fh.write(sc.record(t, r))
    print(f"wrote {sc.count[t]} {TYPE_NAMES[t]} records to {outdir}/")


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        sys.exit(2)
    {"toc": cmd_toc, "list": cmd_list, "rec": cmd_rec, "dump": cmd_dump}[argv[0]](argv[1:])


if __name__ == "__main__":
    main(sys.argv[1:])

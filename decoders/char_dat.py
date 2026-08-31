#!/usr/bin/env python3
"""Decode `CHAR.DAT` -- the Legacy of the Ancients character roster (which
doubles as the in-progress save; there is no separate save file).

    python decoders/char_dat.py [C:\\games\\lota\\CHAR.DAT]

Container
---------
6-byte header, then 9 fixed-length records of 382 bytes each
(6 + 9*382 == 3444 == the shipped file size).

    header[0:4]  05 06 07 00   -- purpose unconfirmed (format/version
                                  marker; `LEGACY.DAT` starts the same way
                                  with `05 06 04 0A`)
    header[4:6]  7E 01         -- 0x017E = 382 = the record length

The game `OPEN`s this as a BASIC random-access file, reclen 382, and
`GET`/`PUT`s record `rosterIndex + 1`.  A record is FIELDed as:

    +000  14 bytes   name, space-padded ("empty" = an unused slot)
    +00E  74 bytes   37 scalar words -- a straight copy of the resident
                     LEGLIB DGROUP block ds:1AC0..ds:1B08.  Known fields
                     (offset within the record):
                        +01C  dword  experienceish  (ds:1AC2)
                        +020  dword  partyGold      (ds:1AD2)  -- 20 for a new character
                        +028  word   hitPoints      (ds:1ADA)
                        +03E  word   intelligence   (ds:1AF0, cap 28)
                     playerX/playerY (ds:1B02/1B06) are in here too.
    +058  294 bytes  7 BASIC integer arrays (DGROUP descriptors at
                     ds:1B0C, 1B3A, 1B68, 1B96, 1BC4, 1BF2, 1C20 --
                     stride 0x2E) written/read element-by-element by
                     rtm_FE39 / rtm_FE37.  Contents: inventory, spells,
                     quest / map flags, museum coins, per-category stats
                     (exact split still TBD -- needs a populated save and
                     the array DIMs traced in LEGLIB).

Write side: SAVER.EXE `saveRosterToDisk`.  Read side: MENU.EXE
`enumerateRoster` / `readCharDat`.  The roster block lives in LEGLIB's
*resident* DGROUP, so it survives the OUT<->DUN<->TWNDR... EXE chaining.
"""
import struct
import sys

HEADER = 6
RECLEN = 382
NAME = 14
SCALARS = 37            # words, ds:1AC0..ds:1B08
SCALAR_BYTES = SCALARS * 2

# record offset -> ('w'|'d', label) for the scalar fields identified so far
FIELDS = [
    (0x1C, "d", "experience?"),
    (0x20, "d", "partyGold"),
    (0x28, "w", "hitPoints"),
    (0x3E, "w", "intelligence"),
]


def load(path):
    b = open(path, "rb").read()
    hdr = b[:HEADER]
    reclen = struct.unpack_from("<H", hdr, 4)[0]
    n, rem = divmod(len(b) - HEADER, reclen)
    return b, hdr, reclen, n, rem


def dump(path):
    b, hdr, reclen, n, rem = load(path)
    print(f"{path}")
    print(f"  header      {hdr.hex()}   (reclen = {reclen})")
    print(f"  records     {n} x {reclen} bytes"
          + (f"   (+{rem} trailing bytes!)" if rem else ""))
    for i in range(n):
        rec = b[HEADER + i * reclen: HEADER + (i + 1) * reclen]
        name = rec[:NAME].split(b"\x00")[0].rstrip().decode("latin1")
        used = name.lower() != "empty" and name != ""
        line = f"  slot {i}: {name!r:20} {'USED' if used else 'empty'}"
        if used:
            for off, kind, label in FIELDS:
                if kind == "w":
                    v = struct.unpack_from("<h", rec, off)[0]
                else:
                    v = struct.unpack_from("<i", rec, off)[0]
                line += f"  {label}={v}"
        print(line)


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else r"C:\games\lota\CHAR.DAT"
    dump(path)


if __name__ == "__main__":
    main()

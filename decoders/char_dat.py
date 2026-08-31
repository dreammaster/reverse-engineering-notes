#!/usr/bin/env python3
"""Decode `CHAR.DAT` -- the Legacy of the Ancients character roster (which
doubles as the in-progress save; there is no separate save file).

    python decoders/char_dat.py [C:\\games\\lota\\CHAR.DAT]

Container
---------
6-byte header, then 9 fixed-length records of 382 bytes each
(6 + 9*382 == 3444 == the shipped file size).

    header[0:2]  05 06         -- shared BASIC-data marker (LEGACY.DAT too)
    header[2:4]  07 00         -- 7 == the number of FIELDed integer arrays
    header[4:6]  7E 01         -- 0x017E = 382 = the record length

The game `OPEN`s this as a BASIC random-access file, reclen 382, and
`GET`/`PUT`s record `rosterIndex + 1`.

Record layout -- fully framed 2026-09-01
---------------------------------------
Write path: SAVER.EXE `saveRosterToDisk`
  -> `rt_AB`(name) then `rtm_FE35` peek of ds:1AC0..ds:1B08 word-by-word
     then `rtm_FE39` x7 over the array descriptors.
Read path : MENU.EXE `showEmptyCharacterSlots` / `readCharDat`
  -> `rt_5F`(name), `rtm_FE36` poke loop, `rtm_FE37` x7.
The 7 arrays are `DIM ...(bound) AS INTEGER` at MENU.EXE seg000_entry
(the `rt_AF` calls, params `(desc, 0x101, 2, bound, 0, 0)`); the bounds
sum to exactly 294 bytes == 382 - 14 (name) - 74 (scalars).

    +0x000  14 B   NAME, space-padded.  "empty" == a free slot.
    +0x00E  74 B   SCALAR BLOCK -- 37 words, a verbatim image of the
                   resident LEGLIB DGROUP range ds:1AC0..ds:1B08.  This
                   block survives the OUT<->DUN<->TWNDR<->MUS<->CASDR EXE
                   chaining, which is why partyGold / hitPoints / playerX
                   sit at the same DGROUP offset in every play module.
    +0x058  16 B   S0  ds:1B0C  DIM(7)   8 words
    +0x068  16 B   S1  ds:1B68  DIM(7)   8 words
    +0x078  60 B   S2  ds:1BC4  DIM(29)  30 words
    +0x0B4  34 B   S3  ds:1B3A  DIM(16)  17 words
    +0x0D6  76 B   S4  ds:1B96  DIM(37)  38 words
    +0x122  84 B   S5  ds:1BF2  DIM(41)  42 words
    +0x176   8 B   S6  ds:1C20  DIM(3)    4 words   (ends at 0x17E = 382)

Scalar fields (record offset <- DGROUP word; default = the LEGACY.DAT
new-character template).  [C] = cross-checked in the play-module code,
[?] = inferred from the template value + usage pattern.

    +0x0E <- 1AC0   [?] party/context flag (tmpl 15)
    +0x10 <- 1AC2   [C] EXPERIENCE, dword (tmpl 0; TWNDR add/adc accumulates)
    +0x14 <- 1AC6   [?] museum walk state (MUS only)
    +0x16 <- 1AC8   [?] GAME SPEED (tmpl 4; read by MENU + every module)
    +0x18 <- 1ACA   [C] chain-return / location context (SAVER returnTarget)
    +0x1A <- 1ACC   [?] attribute / resource (tmpl 15; potion-wizard +5, cap 0x24)
    +0x1C <- 1ACE       unreferenced
    +0x1E <- 1AD0       unreferenced (tmpl 0x4270 -- stale dev-memory junk)
    +0x20 <- 1AD2   [C] PARTY GOLD, dword (tmpl 20)
    +0x24 <- 1AD6   [C] dungeon return position, dword (tmpl -99 = "none")
    +0x28 <- 1ADA   [C] HIT POINTS (tmpl 200)
    +0x2A <- 1ADC   [?] portrait / message index (<= 11)
    +0x2C <- 1ADE   [?] museum-adjustable stat (tmpl 15; MUS showGold +10)
    +0x2E <- 1AE0   [C] COMPENDIUM VOLUMES / museum access rank 1..7 (tmpl 1)
    +0x30 <- 1AE2   [C] dungeon position, level<<8 | cell (DUN; cmp 0x700)
    +0x32 <- 1AE4   [C] dungeon / museum facing 0..3
    +0x34 <- 1AE6   [C] dungeon light / step counter (DUN inc/dec)
    +0x36 <- 1AE8   [C] dungeon spell-effect timer (DUN dec to 0)
    +0x38 <- 1AEA   [C] INVENTORY COUNT (tmpl 5; index into S1, guard < 8)
    +0x3A <- 1AEC   [C] paired count / damage multiplier (tmpl 9; DUN imul)
    +0x3C <- 1AEE   [?] menu / dialog scratch (all modules incl. MENU/SAVER)
    +0x3E <- 1AF0   [C] STRENGTH (tmpl 15; cap 0x1C=28; potion-wizard gate)
    +0x40..0x48 <- 1AF2..1AFA   unreferenced (5 words)
    +0x4A <- 1AFC   [C] selected-item cursor (tmpl 99 = "none"; index, guard < 8)
    +0x4C <- 1AFE   [C] second-list count (paired with 1AFC)
    +0x4E <- 1B00   [?] castle / town interior scratch (CASDR + TWNDR only)
    +0x50 <- 1B02   [C] OVERWORLD X (new game sets 40)
    +0x52 <- 1B04   [?] castle / town interior scratch
    +0x54 <- 1B06   [C] OVERWORLD Y (new game sets 30)
    +0x56 <- 1B08   [?] attribute-like (tmpl 15; DUN subtracts from it)

Array roles (element-level split still needs a populated save):

    S0  encounter / combat scratch -- zeroed at the top of every outInit,
        so not persistent character data (FIELDed only for convenience).
    S1  inventory slot data, 8 slots -- cursor ds:1AEA; `rtm_FE50` clamps
        elements to 0..4 (= the LEGACY "weapon condition" scale
        Shoddy..Superb).  Template all-zero (a new character's 5 items
        have not been given condition/ID values in the template dump).
    S2  world / quest state -- 30 flags.  Cross-referenced against the
        overworld object array (ds:1C7C) and landmark logic; index 29 is
        used as its own bound.  Template: [15]=1, [17]=2, rest 0.
    S3  museum progress -- MUS increments [15] on every museum entry,
        sets [14]=1, tests [1] == 0xFFFE.  Template all-zero except
        [0] = -1.
    S4  the main stat / map-state block -- 38 words, the workhorse array
        (~150 read/write sites).  outInit copies S4[19] (tmpl 200) ->
        hitPoints, confirming S4 carries the persistent copy that the
        ds:1AC0.. scalars are re-derived from.  Template: [0..2] =
        1500/3099/31058 (RNG / world-hash, stale in the dev dump),
        [22]=32000, [25..29]=32767 sentinels, [33]=3.
    S5  SHOP PRICE TABLE -- S5[0]=7, then 41 prices (weapon / armor /
        item / food costs): 400,350,350,13,500,220,950,450, 150,170,200,
        170,250, ... 2000,1500,1700,5,1300,2000,5,21.  Identical for
        every character (stored per-record only because it is FIELDed).
    S6  reserved -- 4 words, always 0, no read/write site in any module.
"""
import struct
import sys

HEADER = 6
RECLEN = 382
NAME = 14
SCALARS = 37                      # words, ds:1AC0..ds:1B08
SCALAR_OFF = NAME                 # 0x0E
ARRAY_OFF = NAME + SCALARS * 2    # 0x58

# name, dgroup, (bound) -> element count = bound + 1
ARRAYS = [
    ("S0", 0x1B0C, 7),
    ("S1", 0x1B68, 7),
    ("S2", 0x1BC4, 29),
    ("S3", 0x1B3A, 16),
    ("S4", 0x1B96, 37),
    ("S5", 0x1BF2, 41),
    ("S6", 0x1C20, 3),
]

# record offset -> (kind, label); kind 'w' signed word, 'd' signed dword
FIELDS = [
    (0x10, "d", "experience"),
    (0x16, "w", "gameSpeed"),
    (0x20, "d", "partyGold"),
    (0x28, "w", "hitPoints"),
    (0x2E, "w", "compendiumRank"),
    (0x38, "w", "invCount"),
    (0x3E, "w", "strength"),
    (0x50, "w", "overworldX"),
    (0x54, "w", "overworldY"),
]


def load(path):
    b = open(path, "rb").read()
    hdr = b[:HEADER]
    reclen = struct.unpack_from("<H", hdr, 4)[0]
    n, rem = divmod(len(b) - HEADER, reclen)
    return b, hdr, reclen, n, rem


def array_layout():
    """[(name, rec_off, count)] in FIELD order."""
    out = []
    off = ARRAY_OFF
    for name, _dg, bound in ARRAYS:
        cnt = bound + 1
        out.append((name, off, cnt))
        off += cnt * 2
    assert off == RECLEN, off
    return out


def dump(path):
    b, hdr, reclen, n, rem = load(path)
    print(f"{path}")
    print(f"  header      {hdr.hex()}   reclen={reclen}  arrays={hdr[2]}")
    print(f"  records     {n} x {reclen}"
          + (f"   (+{rem} trailing!)" if rem else ""))
    print(f"  array map   " + "  ".join(
        f"{nm}@0x{o:03X}x{c}" for nm, o, c in array_layout()))
    for i in range(n):
        rec = b[HEADER + i * reclen: HEADER + (i + 1) * reclen]
        name = rec[:NAME].split(b"\x00")[0].rstrip().decode("latin1")
        used = name.lower() not in ("empty", "")
        line = f"  slot {i}: {name!r:18} {'USED' if used else 'empty'}"
        if used:
            for off, kind, label in FIELDS:
                fmt = "<i" if kind == "d" else "<h"
                line += f"  {label}={struct.unpack_from(fmt, rec, off)[0]}"
        print(line)


def dump_template(legacy_path):
    """Show the new-character template (LEGACY.DAT tail) split by field."""
    d = open(legacy_path, "rb").read()
    rec = d[-RECLEN:]
    print(f"\nLEGACY.DAT new-character template ({RECLEN} B):")
    print(f"  name   {rec[:NAME].rstrip()!r}")
    sc = struct.unpack_from(f"<{SCALARS}h", rec, SCALAR_OFF)
    print("  scalars (rec+0x0E..0x56):")
    for k, v in enumerate(sc):
        print(f"    +0x{SCALAR_OFF + 2*k:02X}  ds:{0x1AC0 + 2*k:04X}  {v}")
    for nm, off, cnt in array_layout():
        vals = struct.unpack_from(f"<{cnt}h", rec, off)
        print(f"  {nm} @0x{off:03X} ({cnt}w): {list(vals)}")


def main():
    args = sys.argv[1:]
    path = args[0] if args else r"C:\games\lota\CHAR.DAT"
    dump(path)
    legacy = args[1] if len(args) > 1 else r"C:\games\lota\LEGACY.DAT"
    try:
        dump_template(legacy)
    except OSError:
        pass


if __name__ == "__main__":
    main()

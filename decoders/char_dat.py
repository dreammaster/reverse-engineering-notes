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

Scalar fields (record offset <- DGROUP word).  [C] = cross-checked in
code, [P] = pinned by the PAULA save diff (2026-09-01: new game -> museum
-> bought+equipped a weapon -> town 1), [?] = inferred.  KEY: `outInit`
loads S4[19] -> hitPoints (ds:1ADA) and re-derives other working scalars
from the arrays -- so where a scalar and an array element disagree, the
ARRAY is authoritative and the scalar is a stale cache (PAULA never ran
outInit, so her ds:1ADA=200 is stale template data while her real max HP
S4[19]=20).

    +0x0E <- 1AC0   [?] context flag (tmpl 15, PAULA 15)
    +0x10 <- 1AC2   [C] EXPERIENCE, dword (PAULA 0 -- no kills yet)
    +0x14 <- 1AC6   [P] museum walk state (MUS only; PAULA -10 = 0xFFF6)
    +0x16 <- 1AC8   [C] GAME SPEED (tmpl 4; menuStartup sets 3)
    +0x18 <- 1ACA   [C] chain-return / location context (SAVER returnTarget)
    +0x1A <- 1ACC   [?] attribute / resource (tmpl 15; potion-wizard +5, cap 0x24)
    +0x1C <- 1ACE       dead -- no ref, PAULA 0
    +0x1E <- 1AD0   [P] a COUNTER/checksum (tmpl 0x4270, PAULA 0x426E = -2);
                        thought dead but it does change -- purpose unknown
    +0x20 <- 1AD2   [P] PARTY GOLD, dword (PAULA 120; hi word +0x22 = 0)
    +0x24 <- 1AD6   [P] dungeon-return marker, dword (tmpl -99/-1 was uninit;
                        PAULA 0/0 = "not in a dungeon")
    +0x28 <- 1ADA   [C] hit points -- WORKING cache of S4[19] (stale unless
                        outInit ran; PAULA 200 is stale, real max = S4[19])
    +0x2A <- 1ADC   [?] portrait / message index (<= 11; PAULA 1)
    +0x2C <- 1ADE   [?] museum-adjustable stat (tmpl 15; MUS showGold +10)
    +0x2E <- 1AE0   [C] COMPENDIUM VOLUMES / museum access rank 1..7 (PAULA 1)
    +0x30 <- 1AE2   [P] first-person-view (museum/dungeon) position, kept
                        across the overworld (OUT never touches it; PAULA 180)
    +0x32 <- 1AE4   [P] first-person-view facing 0..3 (PAULA 3)
    +0x34 <- 1AE6   [C] dungeon light / step counter (DUN inc/dec)
    +0x36 <- 1AE8   [C] dungeon spell-effect timer (DUN dec to 0)
    +0x38 <- 1AEA   [?] inventory slot count / max (tmpl 5, PAULA 5 -- did
                        NOT change when she gained an item, so maybe max=5)
    +0x3A <- 1AEC   [?] paired count / damage multiplier (tmpl 9; DUN imul)
    +0x3C <- 1AEE   [C] SAVER write-marker -- saver.asm sets it to 0xEA
                        (PAULA 234 = 0xEA); transient
    +0x3E <- 1AF0   [C] STRENGTH (tmpl 15, PAULA 15; cap 0x1C=28; potion gate)
    +0x40..0x48 <- 1AF2..1AFA   DEAD -- SAVER's blind peek loop captures
                        whatever DGROUP garbage is there (PAULA 0 / 16128 /
                        36 / 16820 -- not fields)
    +0x4A <- 1AFC   [C] selected-item cursor (tmpl 99="none", PAULA 0)
    +0x4C <- 1AFE   [P] count paired with 1AFC (PAULA 1 -- one equipped item?)
    +0x4E <- 1B00   [P] town/castle interior scratch (TWNDR/CASDR; PAULA 74)
    +0x50 <- 1B02   [P] OVERWORLD X (new game 40; PAULA 14)
    +0x52 <- 1B04   [P] town/castle interior scratch (PAULA 17)
    +0x54 <- 1B06   [P] OVERWORLD Y (new game 30; PAULA 42)
    +0x56 <- 1B08   [?] attribute-like (tmpl 15, PAULA 15; DUN subtracts)

Array roles (PAULA diff 2026-09-01; per-element split still partial):

    S0  8-slot INVENTORY item-id list.  PAULA S0[0] 0->1 after she got a
        weapon.  CASDR writes S0[0]=7, S0[7]=0xC -> values are small ids.
        (Earlier "combat scratch" guess was wrong -- it persists.)
    S1  8-slot INVENTORY item CONDITION (0..4, the LEGACY Shoddy..Superb
        scale -- rtm_FE50 clamps it).  PAULA S1[0] 0->1, parallel to S0[0].
    S2  30 world / quest flags.  PAULA: only [17] changed (2->0), and the
        template's [17]=2 looks like dev junk -- so still mostly opaque.
        Indexed against the overworld object array (ds:1C7C).
    S3  17 museum-progress words.  PAULA: [15] 0->2 (= museum ENTRY COUNT,
        mus.asm:498 increments it each visit); [14] 0->1 (used an exhibit
        PORTAL, mus.asm:1637); [1]=-2, [2]=1 (other exhibit progress).
    S4  38-word MAIN CHARACTER + map block -- authoritative.  Confirmed:
        S4[1] = saved museum/exhibit position (chainToTown/chainToDungeon
        write ds:1AE2 here; PAULA 32); S4[19] = MAX HIT POINTS (buyFood
        compares it to current ds:1ADA and heals up to it; PAULA 20 = a
        new peasant's HP, tmpl 200 was dev junk).  PAULA also changed
        [10]=1, [11]=3 (=facing?), [34] 3->0.  [22]=32000, [25..29]=32767
        stay sentinel.  [0..2] (1500/32/31058) still look like RNG/hash.
    S5  SHOP PRICE TABLE -- S5[0]=7 then 41 prices.  UNCHANGED in PAULA's
        save -> confirmed static / identical for every character.
    S6  4 words -- NOT dead: PAULA [0]=17, [3]=18 (tmpl all 0).  No direct
        `si,1C20h` site, so written via a computed index or LEGLIB.
        Purpose unknown (last-town? re-entry coords? -- needs more saves).

Still needs saves at known points to finish: which S4 elements are the
RPG stats (dex/stamina/etc), the S2 quest-flag meanings, S6, and 1AD0.
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
    (0x28, "w", "hitPoints(cache)"),
    (0x2E, "w", "compendiumRank"),
    (0x3E, "w", "strength"),
    (0x30, "w", "viewPos"),
    (0x32, "w", "viewFacing"),
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


def split_record(rec):
    sc = list(struct.unpack_from(f"<{SCALARS}h", rec, SCALAR_OFF))
    arrs = {nm: list(struct.unpack_from(f"<{c}h", rec, o))
            for nm, o, c in array_layout()}
    return sc, arrs


def diff(path_a, path_b, slot_a=0, slot_b=0):
    """Field-by-field diff of two CHAR.DAT records (or a record vs the
    LEGACY.DAT template).  Pass a .DAT path for either side; slot index
    selects the record (ignored for LEGACY.DAT)."""
    def rec_of(path, slot):
        d = open(path, "rb").read()
        if path.lower().endswith("legacy.dat"):
            return d[-RECLEN:], "template"
        r = d[HEADER + slot * RECLEN: HEADER + (slot + 1) * RECLEN]
        return r, r[:NAME].split(b"\x00")[0].rstrip().decode("latin1")

    ra, na = rec_of(path_a, slot_a)
    rb, nb = rec_of(path_b, slot_b)
    sa, aa = split_record(ra)
    sb, ab = split_record(rb)
    print(f"diff  A={na!r}  ->  B={nb!r}\n")
    print("  scalars (rec off / ds:):")
    for k in range(SCALARS):
        if sa[k] != sb[k]:
            print(f"    +0x{SCALAR_OFF + 2*k:02X}  ds:{0x1AC0 + 2*k:04X}   "
                  f"{sa[k]:6d} -> {sb[k]:6d}")
    for nm in aa:
        ch = [(i, aa[nm][i], ab[nm][i])
              for i in range(len(aa[nm])) if aa[nm][i] != ab[nm][i]]
        if ch:
            print(f"  {nm}: " + ", ".join(f"[{i}] {x}->{y}" for i, x, y in ch))


def main():
    args = sys.argv[1:]
    if args and args[0] == "--diff":
        a = args[1] if len(args) > 1 else r"C:\games\lota\LEGACY.DAT"
        b = args[2] if len(args) > 2 else r"C:\games\lota\CHAR.DAT"
        sb = int(args[3]) if len(args) > 3 else 0
        diff(a, b, 0, sb)
        return
    path = args[0] if args else r"C:\games\lota\CHAR.DAT"
    dump(path)
    legacy = args[1] if len(args) > 1 else r"C:\games\lota\LEGACY.DAT"
    try:
        dump_template(legacy)
    except OSError:
        pass


if __name__ == "__main__":
    main()

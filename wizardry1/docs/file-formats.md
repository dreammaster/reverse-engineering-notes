# Wizardry I (DOS) — file formats

Files carved from `WIZ1.DSK` with `tools/ucsd_disk.py`. Guide structures come
from the Apple Pascal source in `sources/` (`Wiz1WizardryPascal.txt`); the DOS
records are an evolved, more tightly packed variant of the same layout, so
field *order* transfers but *sizes/packing* must be checked against the data.

All multi-byte integers are little-endian. "STRING[n]" = 1 length byte then
`n` chars (UCSD `STRING`); inside records it is often followed by pad bytes to
the declared width + word alignment.

---

## SCENARIO.DATA — scenario database  ✅ container decoded & validated

`tools/scenario.py`. 74 blocks. One TOC block, then one contiguous run of
fixed-size records per data type.

### TSCNTOC header (block 0)

| off | field | type | value in WIZ1 |
|---|---|---|---|
| `0x00` | `GAMENAME` | STRING[40] (+1 pad → 42 B) | `"PROVING GROUNDS OF THE MAD OVERLORD!"` |
| `0x2A` | `RECPER2B[8]` | u16 ×8 | records packed per 1024-byte (2-block) unit |
| `0x3A` | `RECPERDK[8]` | u16 ×8 | total record count |
| `0x4A` | `RECSIZE[8]` | u16 ×8 | record size in bytes *(Apple field `UNUSEDXX`, repurposed)* |
| `0x5A` | `BLOFF[8]` | u16 ×8 | first block of the type's data (from file start) |
| `0x6A` | `RACE[6]` | STRING[9] (10 B each) | `NO RACE HUMAN ELF DWARF GNOME HOBBIT` |
| `0x9E` | `CLASS[8]` | STRING[9] (10 B) | `FIGHTER MAGE PRIEST THIEF BISHOP SAMURAI LORD NINJA` |
| `0xEE` | `STATUS[8]` | STRING[8] (9 B) | `OK AFRAID ASLEEP PLYZE STONED DEAD ASHES LOST` |
| `0x136` | `ALIGN[4]` | STRING[9] (10 B) | `UNALIGN GOOD NEUTRAL EVIL` |
| `0x15E` | `SPELLHSH[51]` | u16 ×51 | spell name hashes |
| … | `SPELLGRP` | packed 3-bit ×51 | spell level 1..7 |
| … | `SPELL012` | packed 2-bit ×51 | `GENERIC / PERSON / GROUP` targeting |

The 8 types (`TZSCN = ZZERO ZMAZE ZENEMY ZREWARD ZOBJECT ZCHAR ZSPCCHRS ZEXP`):

| # | type | count | recsize | per/2blk | blocks | contents |
|--:|---|--:|--:|--:|---|---|
| 0 | ZZERO | 1 | – | – | 0 | the TOC |
| 1 | ZMAZE | 10 | 894 | 1 | 12–31 | maze levels 1–10 |
| 2 | ZENEMY | 101 | 94 | 10 | 32–53 | monsters |
| 3 | ZREWARD | 24 | 168 | 6 | 54–61 | treasure / reward tables |
| 4 | ZOBJECT | 101 | 46 | 22 | 62–71 | items |
| 5 | ZCHAR | 20 | 208 | 4 | 2–11 | starter character roster |
| 6 | ZSPCCHRS | 0 | – | – | – | *(unused in WIZ1)* |
| 7 | ZEXP | 1 | 624 | 1 | 72–73 | experience-per-level table |

Record `R` of type `T`:

```
block  = BLOFF[T] + 2 * (R // RECPER2B[T])
offset = RECSIZE[T] * (R % RECPER2B[T])          within that 1024-byte unit
```

Block accounting closes exactly (`ceil(count/perUnit)*2` blocks per type,
each type starting where the previous ends), which validates the whole scheme.

### Where the text is — `ASCII.KRN` string pool  ✅ decoded

Monster / item / reward records contain **no name strings** — names, spell
names and every message live in **`ASCII.KRN`**, keyed by integer. Decoder:
`tools/strpool.py` (recovered from `SYSTEM.PASCAL` — `GetStr` = WIZARDRY
proc 38, loader = WIZARDRY proc 82, tree loader = KANJIREA procs 8/10).

`ASCII.KRN` (54 blocks, LE):

| region | contents |
|---|---|
| block 0 | header: `u16 offBlk, offLen, treeBlk, treeLen` |
| blocks 1–44 | enciphered string data (Pascal strings, word-aligned) |
| block 45 (`offBlk`) | `strOffsets[]` — `u16` per key slot → word offset of the string |
| block 51 (`treeBlk`) | `strTree[]` — 5×`u16` nodes `{startIdx, endIdx, indexOffset, left, right}`; node 0's word[4] is the root |

Lookup for key `KN`: binary-search `strTree` for the node whose
`[startIdx,endIdx]` contains `KN`; `SVAL = strOffsets[node.indexOffset + KN -
node.startIdx]`; the string is at byte `2*SVAL` (`len` byte, then `len`
bytes); decipher each byte `k = 1..len`:

```
plain[k] = (raw[k] - 67*(KN mod 51) - 23*k) mod 256
```

Key ranges: `600–1500` UI / combat templates, `5000+` spell names,
`10000+` class names, `13000 + 4*monsterIdx + {0 unidSing,1 unidPlur,2
sing,3 plur}`, `14000 + 2*objIdx + {0 unid,1 real}`, `20000 + 50*msgNo +
line` scenario messages. Full dump: [`docs/strings.txt`](strings.txt).

*Open:* a few quest-item names carry an embedded token byte (post-decipher
`0x77`) for word-join / " OF " — not yet mapped; everything else is clean.

### Record layouts (partial — field order per Apple, sizes per DOS)

**ZMAZE (894 B)** — `TMAZE`: four `PACKED ARRAY[0..19][0..19] OF TWALL`
(W,S,E,N; `TWALL = OPEN WALL DOOR HIDEDOOR`, 2 bits) ≈ perimeter shows as
runs of `0x55`; then `FIGHTS` (1 bit/sq), `SQREXTRA` (4 bits/sq, indexes into
the next tables), `SQRETYPE[16]` (`TSQUARE`: NORMAL STAIRS PIT CHUTE SPINNER
DARK TRANSFER OUCHY BUTTONZ ROCKWATE FIZZLE SCNMSG ENCOUNTE — DOS renames
SPINNER→TURNRANDOM, DARK→SDARKNESS, TRANSFER→STELEPORT, OUCHY→DAMAGE), then
`AUX0/AUX1/AUX2[16]` (u16, stairs targets / message #s / encounter data) and
`ENMYCALC[1..3]` (5×u16: MINENEMY MULTWORS WORSE01 RANGE0N PERCWORS). Exact
DOS bit packing TBD (Phase 3 validation vs known maps).

**ZENEMY (94 B)** — `TENEMY` minus the 4 leading name strings: `PIC` (image
#), `CALC1` (TWIZLONG hp-dice), `HPREC`, `CLASS`, `AC`, `RECSN`+`RECS[1..7]`,
`EXPAMT` (TWIZLONG), `DRAINAMT`, `HEALPTS`, `REWARD1/2` (→ ZREWARD index),
`ENMYTEAM`, `TEAMPERC`, `MAGSPELS`, `PRISPELS`, `UNIQUE`, `BREATHE`,
`UNAFFCT`, `WEPVSTY3` + `SPPC` (packed bit flags). + name key.

**ZOBJECT (46 B)** — `TOBJREC` minus name strings: `OBJTYPE` (WEAPON ARMOR
SHIELD HELMET GAUNTLET SPECIAL MISC), `ALIGN`, `CURSED`, `SPECIAL`,
`CHANGETO`, `CHGCHANC`, `PRICE` (TWIZLONG), `BOLTACXX`, `SPELLPWR`,
`CLASSUSE` (8-bit), `HEALPTS`, weapon-vs-type bitfields, `WEPHITMD`,
`WEPHPDAM` (THPREC), `XTRASWNG`, `CRITHITM`. + name key.
Object 0 = the null/"nothing" item (`06 00 …` then `FF FF`).

**ZEXP (624 B)** — `TEXP = ARRAY[FIGHTER..NINJA] OF ARRAY[0..12] OF TWIZLONG`
(8 × 13 × 6 B). XP thresholds for levels 1..13 per class.

**ZCHAR (208 B)** — `TCHAR`: `NAME` STRING[15], `PASSWORD` STRING[15],
`INMAZE`, `RACE`, `CLASS`, `AGE`, `STATUS`, `ALIGN`, `ATTRIB` (6 × packed
0..18), … party/inventory state. The shipped roster is test data
(`THESUS`, `NEB`, …) — this is really player data, low priority.

`TWIZLONG` = 3 × i16 `{LOW, MID, HIGH}`, a base-10000 bignum (see Apple
`ADDLONGS`): value = `LOW + MID*10000 + HIGH*10^8`.

---

## Art files (Phase 1, not yet decoded)

| file | size | notes |
|---|---|---|
| `200.CHARSET` / `400.CHARSET` | 8192 | 256 glyphs; two resolutions (200/400-line) |
| `200.TITLE` / `400.TITLE` | 5120 | title screen bitmap |
| `200.MONSTERS` | 16384 | monster portraits — draw-primitive display lists; header `00 00 04 00 05 00 0d 00 …` then nibble draw data. `.MONSTERS` file selected by `Concat2(tstr,'.MONSTERS'); FirstBlock(4,tstr)` |
| `ASCII.KRN` | 27648 | compressed keyed string pool (see above) |
| `HAS.CACHE` / `KANA.KEYMAP` | 512 / 1024 | share a 512-byte prefix (offset/prefix table?); `KANA.KEYMAP` feeds the `KANJIREA` segment |
| `HAS.STROPS` | 512 | **native x86** string-op helpers loaded as a machine-code segment |

---

## PLAYER.DATA / SAVEn.DSK (Phase 1)

`SAVE1.DSK`–`SAVE5.DSK` are UCSD p-System volumes too; the party save is
`PLAYER.DATA`. Not yet examined.

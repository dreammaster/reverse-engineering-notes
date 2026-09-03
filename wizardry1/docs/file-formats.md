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
| `0x6A` | `RACE[6]` | 10 B/entry | `NO RACE HUMAN ELF DWARF GNOME HOBBIT` |
| `0xA6` | `CLASS[8]` | 10 B | `FIGHTER MAGE PRIEST THIEF BISHOP SAMURAI LORD NINJA` |
| `0xF6` | `STATUS[8]` | 10 B | `OK AFRAID ASLEEP P-LYZE STONED DEAD ASHES LOST` |
| `0x146` | `ALIGN[4]` | 10 B | `UNALIGN GOOD NEUTRAL EVIL` |
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

**ZMAZE (894 B / 447 words)** — `TMAZE`.  Bit packing **confirmed** from DOS
`RUNNER` procs 13/19 + UCSD packed-array rules, and validated by rendering
level 1 (start `@(0,0)` NORTH on the castle stairs, the long door-lined
corridor, the big `DARK` room).  All grids are `[x][y]`, x = east, y = north,
both mod 20.  `engine/wiz/maze.h` reads it.

| word | field | pack |
|--:|---|---|
| 0 | `W[x][y]` | 2 bits, 3 words/row, `IXP 8,2` |
| 60 | `S[x][y]` | " |
| 120 | `E[x][y]` | " |
| 180 | `N[x][y]` | " |
| 240 | `FIGHTS[x][y]` | 1 bit, 2 words/row, `IXP 16,1` |
| 280 | `SQREXTRA[x][y]` | 4 bits, 5 words/row, `IXP 4,4` → 0..15 |
| 380 | `SQRETYPE[0..15]` | 4 bits, `IXP 4,4` → `TSQUARE` |
| 384 | `AUX0[0..15]` | i16 |
| 400 | `AUX1[0..15]` | i16 |
| 416 | `AUX2[0..15]` | i16 |
| 432 | `ENMYCALC[1..3]` | 5×i16: MINENEMY MULTWORS WORSE01 RANGE0N PERCWORS |

`TWALL = OPEN(0) WALL(1) DOOR(2) HIDEDOOR(3)`.  `TSQUARE = NORMAL STAIRS PIT
CHUTE SPINNER DARK TRANSFER OUCHY BUTTONZ ROCKWATE FIZZLE SCNMSG ENCOUNTE`
(0..12; DOS renames SPINNER→TURNRANDOM, DARK→SDARKNESS, TRANSFER→STELEPORT,
OUCHY→DAMAGE).  `SQRETYPE`/`AUX*` are indexed by the cell's `SQREXTRA` value —
per-level there are ≤ 16 distinct special-square descriptors.  Stairs/chutes/
teleports: `AUX0` = target level, `AUX1` = target y, `AUX2` = target x.

**ZENEMY (94 B)** — `TENEMY` minus the 4 leading name strings: `PIC` (image
#), `CALC1` (TWIZLONG hp-dice), `HPREC`, `CLASS`, `AC`, `RECSN`+`RECS[1..7]`,
`EXPAMT` (TWIZLONG — **unused**; XP is `CALCKILL` from the other stats),
`DRAINAMT`, `HEALPTS`, `REWARD1/2` (→ ZREWARD index),
`ENMYTEAM`, `TEAMPERC`, `MAGSPELS`, `PRISPELS`, `UNIQUE`, `BREATHE`,
`UNAFFCT`, `WEPVSTY3` + `SPPC` (packed bit flags). + name key.

**ZREWARD (168 B / 84 words)** — `TREWARD` (REWARDS `CHSTGOLD`, decoded in
`engine/wiz/rewards.h`): `BCHEST` (w0), `BTRAPTYP` (w1, packed `bool[0..7]` —
trap types possible for this chest), `REWRDCNT` (w2), then `REWARDXX[1..9]` at
w3+, **9 words each**: `REWDPERC` (0), `BITEM` (1, 0 = gold), then a 7-word
variant record — gold `{TRIES,AVEAMT,MINADD,MULTX,TRIES2,AVEAMT2,MINADD2}` or
item `{MININDX,MFACTOR,MAXTIMES,RANGE,PERCBIGR,·,·}`.  See `docs/combat.md`.

**ZOBJECT (46 B)** — `TOBJREC` minus name strings: `OBJTYPE` (WEAPON ARMOR
SHIELD HELMET GAUNTLET SPECIAL MISC), `ALIGN`, `CURSED`, `SPECIAL`,
`CHANGETO`, `CHGCHANC`, `PRICE` (TWIZLONG), `BOLTACXX`, `SPELLPWR`,
`CLASSUSE` (8-bit), `HEALPTS`, weapon-vs-type bitfields, `WEPHITMD`,
`WEPHPDAM` (THPREC), `XTRASWNG`, `CRITHITM`. + name key.
Object 0 = the null/"nothing" item (`06 00 …` then `FF FF`).

**ZEXP (624 B)** — `TEXP = ARRAY[FIGHTER..NINJA] OF ARRAY[0..12] OF TWIZLONG`
(8 × 13 × 6 B). XP thresholds for levels 1..13 per class.

**ZCHAR / `TCHAR` (208 B = 104 words)** — party member. `CHARACTR` = global
word 363 (`ARRAY[0..5] OF TCHAR`, stride 104 words). Field **word** offsets
*within the record*, recovered from the DOS ROLLER p-code and verified by a
byte-exact round-trip of all 20 shipped roster records
(`engine/wiz/character.{h,cpp}`):

| word | field | word | field |
|--:|---|--:|---|
| 0 | `NAME` STRING[15] (8 w) | 29 | `POSS.POSSCNT` |
| 8 | `PASSWORD` STRING[15] (8 w) | 30 | `POSS.POSSESS[1..8]` — 4 w each: EQUIPED, CURSED, IDENTIF, EQINDEX (all unpacked, unlike Apple) |
| 16 | `INMAZE` | 62 | `EXP` (TWIZLONG: LOW/MID/HIGH) |
| 17 | `RACE` | 65 | `MAXLEVAC` |
| 18 | `CLASS` (8 = "none", set by ROLLER) | 66 | `CHARLEV` |
| 19 | `AGE` (weeks) | 67 | `HPLEFT` |
| 20 | `STATUS` | 68 | `HPMAX` |
| 21 | `ALIGN` | 69 | `SPELLKN[0..49]` packed 1-bit (`IXP 16,1`), 4 w |
| 22 | `ATTRIB[STR..LUCK]` packed 5-bit (`IXP 3,5`), 2 w | 73 | `MAGESP[1..7]` |
| 24 | `LUCKSKIL[0..4]` packed 5-bit, 2 w | 80 | `PRIESTSP[1..7]` |
| 26 | `GOLD` (TWIZLONG) | 88 | `ARMORCL` |

Words 87 and 89–103 (`HPCALCMD`, `HEALPTS`, `CRITHITM`, `SWINGCNT`,
`HPDAMRC`, `WEPVSTY2/3/P`, `LOSTXYL`) are not modelled yet — carried through
verbatim on write. `IXP 3,5` packs 3 fields/word from bit 0 (5 bits each,
bit 15 spare); `IXP 16,1` uses all 16 bits. UCSD string writes touch only
the length byte + chars (stale tail bytes are preserved).

The shipped roster is test data (`THESUS`, `NEB`, …). Creation rules in
`engine/wiz/roller.h`.

`TWIZLONG` = 3 × i16 `{LOW, MID, HIGH}`, a base-10000 bignum (see Apple
`ADDLONGS`): value = `LOW + MID*10000 + HIGH*10^8`.

---

## Art files (Phase 1, not yet decoded)

| file | size | notes |
|---|---|---|
| `200.CHARSET` / `400.CHARSET` | 8192 | **font: 16 w × 8 h, 16 bytes/glyph (2 B/row, MSB left), 512 glyphs.** 0–255 = text (ASCII + box-draw); 256–511 = a 2nd bank (kana + graphics). `200`/`400` are the two screen modes; the bitmaps are near-identical. Rendered by `engine/wiz/font.h`. |
| `200.TITLE` / `400.TITLE` | 5120 | the "Wizardry" script logo — **320×64, 2bpp CGA** (each byte = 4 px, high pair first). `200` uses colour 2 only (solid); `400` also uses colour 3 (white body + outline). `engine/wiz/bitmap.h` `loadTitle`. The DOS title also shows key 2018 `"WELCOME TO THE WORLD OF WIZARDRY!"` + an `S)TART GAME` / `M)AKE SCENARIO DISK` / language menu (Apple's "PREPARE YOURSELF…" is Apple-only). |
| `200.MONSTERS` | 16384 | 29 monster-portrait records × 512 B (blocks 29–31 are disk slack — MPW/Mac-Pascal source scraps). **Payload compressed**, scheme not yet cracked — see below. |
| `ASCII.KRN` | 27648 | compressed keyed string pool (see above) |
| `HAS.CACHE` / `KANA.KEYMAP` | 512 / 1024 | share a 512-byte prefix (offset/prefix table?); `KANA.KEYMAP` feeds the `KANJIREA` segment |
| `HAS.STROPS` | 512 | **native x86** string-op helpers loaded as a machine-code segment |

### `200.MONSTERS` — monster portraits (partly decoded)

**Load path.** `KANJIREA` proc 12 (`LoadMonsters`):
`FINDFILE(".MONSTERS")` → fallback `FINDFILE("200.MONSTERS")` →
`UNITWRITE(CONUNIT, @fileNum, 0, subfn=18, fileNum, 0)`. Subfn 18 hands the
whole file to the `SYSTEM.INTERP` console-unit driver, which caches it
(analogue of Apple loading the pics into `IOCACHE`). Portraits are then blitted
on demand from combat (`CINIT`/`CUTIL`) via further `CONUNIT` `UNITWRITE`
subfns.

**Console-unit (`CONUNIT` = global word 60) `UNITWRITE` sub-functions** seen in
the p-code (`CSP UNITWRITE`, the byte before the CSP is the subfn):

| subfn | caller | meaning |
|--:|---|---|
| 3 | many | present / flip `WINDOW1` to the visible page |
| 13 | `KANJIREA` 13, `CUTIL` 38 | blit buffer → screen (masked?) |
| 14 | `KANJIREA` 11 | load `*.CHARSET` into the driver |
| 17 | `KANJIREA` 13, `CUTIL` 38/41 | blit an offscreen buffer into `WINDOW1` |
| 18 | `KANJIREA` 12 | load `*.MONSTERS` into the driver cache |
| 19 | `KANJIREA` 13, `CUTIL` 41 | per-row / scanline poke |
| 23 | `KANJIREA` 13 | blit a loaded bitmap file (`*.TITLE`) |

**Record grid.** 29 records of 512 bytes (`29 × 512 = 14848`; the file is
padded to 16384 / 32 blocks, and blocks 29–31 hold unrelated Mac-Pascal
source-file slack). `TENEMY.PIC` (DOS word 0 of the 94-byte monster record;
Apple has it at word 32, after four `STRING[15]` names that DOS drops)
indexes this grid — the shipped scenario uses `PIC` values `0‥28`.

**Apple reference format** (`ENEMYPIC`, Apple `CINIT` proc 2 = `P010502`):
each pic is **70 × 50 px, 1bpp**, stored as 50 rows × 10 bytes = 500 bytes
(record padded to 512). Apple blits it with
`FOR PICLINE := 23 TO 72 DO MOVELEFT(IOCACHE[ENEMYID], scrnaddr, 10);
ENEMYID += 10` — i.e. the file data is linear rows; only the *screen* address
is hi-res-interleaved.

**Status: the DOS payload is compressed / transformed — not raw pixels.**
Rendered as a raw 1bpp bitmap at every plausible geometry (widths
32/40/48/56/64/70/80 × 50 rows, both bit orders; 64×64; a full 320×200 2bpp
CGA page) it is pure noise. Each 512-byte record starts with a small control
block — record 0 is `00 00 | 04 00 05 00 0d 00 | 01 40 03 40 01 d0 00 00 …`
(words `0, 4, 5, 13, 0x4001, 0x4003, 0xD001, …`; the `0x4000`/`0xD000` high
bits look like RLE run/literal tags). Byte histogram after the header is
dominated by `0x00` then the dither/bitmask bytes `0xAA 0x55 0xFF 0x11 0x22
0x44 0x88 0x20 0x40 0x80` — consistent with a compressed 1bpp image.
Zero-run RLE (`00 <count>`) decodes to ~4.3 KB/record — not a clean pic size,
so the scheme is more than plain zero-RLE.

**To finish:** the decompressor is native x86 inside `SYSTEM.INTERP` (only
16384 bytes — fully covered by `wiz1_interp.asm`), in the `CONUNIT` driver's
subfn-18 / portrait-blit path. Needs a focused static RE of that driver or a
live DOSBox trace. **Not on the critical path** for `CASTLE`/`SHOPS`/`RUNNER`.

---

## PLAYER.DATA / SAVEn.DSK (Phase 1)

`SAVE1.DSK`–`SAVE5.DSK` are UCSD p-System volumes too; the party save is
`PLAYER.DATA`. Not yet examined.

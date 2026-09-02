# Wizardry I — reverse engineering overview

Goal: play *Wizardry: Proving Grounds of the Mad Overlord* (DOS release) from a
clean C++ reimplementation, first as a standalone subproject, later as a ScummVM
engine. The DOS version is the canonical target for file formats and behavior;
the Apple II Pascal source is the algorithmic reference.

## The two codebases

| | Apple II (1981) | DOS / "Ultimate Wizardry Archives" (1987) |
|---|---|---|
| Language | Apple Pascal 1.1 (UCSD p-System II.1) | UCSD p-System, x86 p-machine, © 1987 RWI Inc. |
| Source we have | Thomas Ewers' 2014 byte-exact reconstruction in `sources/` | none — must be recovered from p-code |
| Delivery | 6502 booter, several `.DSK` volumes | x86 booter (`WIZ1.COM` + `WIZ1.DSK`) |
| Graphics | hi-res vector line art | bitmap, two resolutions (`200.*` / `400.*`) |
| Extras | — | Japanese kana/kanji input (`KANJIREA` segment, `KANA.KEYMAP`) |

The DOS p-code is an **evolved** version of the same program: identical module
decomposition and algorithms, different I/O layer (real `SCENARIO.DATA` /
`PLAYER.DATA` files instead of hardcoded `UNITREAD` block numbers), mixed-case
identifiers, C-style string escapes. The Apple Pascal source maps segment-for-
segment onto the DOS build (see below), so it is a strong guide but not a
literal match.

## DOS delivery mechanism

`WIZ1.COM` (1808 bytes) is a real-mode shim: it hooks `INT 13h` to serve
`WIZ1.DSK` as a fake 320 KB floppy (40 cyl × 2 heads × 8 sec × 512) and boots
it. `WIZ1.DSK` is a **linear 512-byte-block image of a UCSD p-System volume**
(not FAT). Boot block → `SYSTEM.INTERP` (x86 p-machine) → `SYSTEM.PASCAL`
(the game, as linked p-code).

## WIZ1.DSK contents (`tools/ucsd_disk.py list`)

Volume `WIZBOOT`, 640 blocks in the image (header nominally claims 1272; the
`ZOT` entry and the extra blocks are cruft — real data ends at block 428).

| file | kind | bytes | notes |
|---|---|---|---|
| `SYSTEM.PASCAL` | code | 89088 | the game: 16 p-code segments (below) |
| `ASCII.KRN` | data | 27648 | high-entropy; font/graphics "kernel" or packed data — TBD |
| `SCENARIO.DATA` | data | 37888 | scenario DB: `"$PROVING GROUNDS OF THE MAD OVERLORD!"` + record tables |
| `200.MONSTERS` | data | 16384 | monster portraits as draw-primitive display lists (low res) |
| `HAS.CACHE` | data | 512 | small table, possibly stale scratch |
| `HAS.STROPS` | data | 512 | **native x86** string-op helpers callable from p-code |
| `KANA.KEYMAP` | data | 1024 | kana input map |
| `400.TITLE` / `200.TITLE` | data | 5120 | title screen bitmaps, two resolutions |
| `400.CHARSET` / `200.CHARSET` | data | 8192 | 256-glyph fonts, two resolutions |
| `SYSTEM.INTERP` | data | 16384 | the x86 UCSD p-machine (© dated 9/10/87) |
| `ZOT` | — | — | bogus directory entry, ignore |

## p-code segments in SYSTEM.PASCAL

Segment dictionary parses cleanly (16 entries). Names and the Apple source
files they correspond to:

| # | segment | ~bytes | Apple source |
|--:|---|--:|---|
| 0 | `WIZARDRY` | 6630 | `wiz1a/wiz.txt` (main) |
| 1 | `COMBAT` | 784 | `wiz1b/combat*.txt` |
| 2 | `CASTASPE` | 4642 | cast-a-spell (`wiz1b/combat*`, `castle`) |
| 3 | `SWINGASW` | 3032 | swing-a-sword (melee resolution) |
| 4 | `CINIT` | 2676 | combat init |
| 5 | `CUTIL` | 7256 | combat util |
| 6 | `KANJIREA` | 6720 | *(DOS only)* kanji reader |
| 7 | `UTILITIE` | 5584 | `wiz1b/utilitie*.txt`, `wiz1c/utilitie.txt` |
| 8 | `SHOPS` | 5880 | `wiz1a/shops2.txt`, `wiz1b/shops.txt` (Boltac's) |
| 9 | `SPECIALS` | 4502 | `wiz1b/specials*.txt` (maze special squares) |
| 10 | `CASTLE` | 5092 | `wiz1c/castle*.txt` (town/castle menus) |
| 11 | `ROLLER` | 5290 | `wiz1c/roller.txt` (character creation) |
| 12 | `CAMP` | 6112 | `wiz1c/camp*.txt` (camp menu, inspect) |
| 13 | `REWARDS` | 5636 | `wiz1c/rewards*.txt` (treasure, chests) |
| 14 | `RUNNER` | 8262 | `wiz1c/runner*.txt` (maze movement / 3D view) |
| 15 | `GAMEUTIL` | 6492 | shared game utilities |

Native assembly primitives from the Apple `wiz1d/` disk (line draw, cursor,
RNG, `KEYAVAIL`) correspond to `HAS.STROPS` and native routines inside
`SYSTEM.INTERP` on DOS.

## SYSTEM.INTERP (the p-machine)

16 KB x86 real-mode. `jmp` at 0, relocates itself down (`mov ax,cs; sub
ax,0x1000; mov ss,ax`). Contains the canonical UCSD runtime error strings
(`Stack overflow!!`, `EXIT to uncalled procedure`, `call unlinked segment`,
`Unimplemented Instruction`, `OOPS! end of code encountered!!`, …) and what
looks like a p-code mnemonic table near `0x2fb0` (`AND`, `ARRAY`, …).
Recovering its opcode dispatch tells us the exact p-code dialect (II vs IV) —
prerequisite for trusting a p-code disassembly of `SYSTEM.PASCAL`.

# On-disk file formats

Every file in `C:\games\lota\` that the executables read or write, and
what's confirmed about each one's layout. Confirmed facts get cited by
the function/global that proves them; anything untraced is flagged as a
guess. See [overview.md](overview.md) for the code side and
[roadmap.md](roadmap.md) for what's still open.

Status: **just started (2026-08-30).** Only the container format below is
confirmed so far; no field-level decoding of any file yet.

## `.BSV` / `.GLB` / `.GMP` / `.BS1` / `.BS2` — Microsoft BASIC `BSAVE` images

Every one of these begins with the 7-byte Microsoft BASIC **BSAVE**
header:

| Off | Size | Field |
|---|---|---|
| 0 | 1 | `0xFD` magic (BSAVE marker) |
| 1 | 2 | segment (word, little-endian) — where the data was `BSAVE`d from / gets `BLOAD`ed to |
| 3 | 2 | offset (word) — usually `0x0000` |
| 5 | 2 | length (word) — bytes of payload following the header |
| 7 | … | raw payload, `length` bytes |

Verified: `file size == 7 + length` for `TITLE.GLB` (len `0x200A`),
`TITLE.GMP` (`0x0838`), `TOWN0.BSV` (`0x138E`), `CASTLE.BS1` (`0x32A0`),
`OUTDATA.BSV` (`0x3794`, offset `0x2B22`). So the game `BLOAD`s these
straight into fixed memory locations rather than parsing them — the
segment/offset in the header says where. Decoding each one is therefore a
matter of finding the `BLOAD` call site in the disassembly and seeing
what code then reads that memory region.

Naming convention (guessed): `.GLB` = "globals" (palettes / tile
graphics / object tables), `.GMP` = "game map", `.BSV` = generic BSAVE
(maps, monster/object tables, animations), `.BS1`/`.BS2` = paired
castle/fort layout banks.

## Inventory

| File(s) | Size | Used by (guess) | Kind (guess) |
|---|---|---|---|
| `TITLE.GLB` / `TITLE.GMP` | 8209 / 2111 | `MENU` | title-screen graphics + map |
| `TOWN0.BSV`…`TOWNB.BSV` | ~5013–5123 | `TWNDR` | 12 town layouts |
| `CASTLE.BS1` / `.BS2`, `FORT.BS1` / `.BS2` | 12967 | `CASDR` | castle / fort layout banks |
| `TCASOBJ.BSV` | 4911 | `CASDR` | town/castle object table |
| `FORTANIM.BSV` | 263 | `CASDR` | fort animation |
| `OUTDATA.BSV`, `OUTOBJ.BSV`, `OUTM0.BSV`…`OUTM2.BSV`, `OUTDAT.DAT` | — | `OUT` | overworld map / objects / monsters |
| `DUNDATA.BSV`, `DUNOBJ.BSV`, `DUNM1.BSV`…`DUNM3.BSV`, `DUNMONA.BSV` / `DUNMONB.BSV` | — | `DUN` | dungeon map / objects / monsters |
| `DIS0.BSV`…`DIS15.BSV` (+ `DIS0A`, `DIS1A`) | 727–2055 | ? | "display" screens? ~18 of them |
| `CEL0.BSV`…`CEL3.BSV` | 1573 / 2597 | `CELDRV` | cel animations |
| `MUSDATA.BSV`, `MUSOBJ.BSV`, `MUSMSG.TXT` | 8055 / 12961 / 11229 | `MUS` | music data / objects / message text |
| `SDMAP.GLB` / `.GMP`, `SDOBJ.GLB` | 4113 / 2081 / 6161 | `SDEFENDR` | Space Defender minigame map / objects |
| `BJCHR.GLB` | 6161 | `GMB1`/`GMB2` | blackjack ("BJ") character graphics? |
| `BIGNUM.DAT` | 420 | ? | large-digit font |
| `D.BSV`, `R.BSV`, `PEGASUS.BSV` | 3527 / 3527 / 1159 | ? | ? |
| `CHAR.DAT` | 3444 | `MENU` | character roster (referenced from the menu's "erase / restart a character" screens) |
| `LEGACY.DAT` | 2945 | `MENU`/`OUT` | **no `0xFD` magic** — not BSAVE; format unknown (config / progress?) |
| `OUTDAT.DAT` | 1012 | `OUT` | not checked yet |
| `DRCONFIG.DAT` | 1015 | `CONFIGUR` | driver / hardware config written by `CONFIGUR.EXE` |
| `STDRVSCR.DAT` | 6192 | `STDRV` | script text |
| `TWNMSG.TXT` / `MUSMSG.TXT` | 1911 / 11229 | `TWNDR` / `MUS` | plain-text message tables |

`LEGACY.BAT` (4 bytes: `menu`), `manual.txt`, and
`Passed.through.ANTiQUE.Shop` are not game data.

## Menu text (in `MENU.EXE` itself, not a file)

`MENU.EXE`'s `seg003` holds ~4 KB of screen text starting at offset
`0x21D0` (file `0x6ED2`): the main menu items ("1. play a game", "2.
simple instructions", "3. game credits", "4. sound is currently …"), the
GAME CREDITS screen (designers John & Charles Dougherty, IBM version Al
DeYoung, art Rick Tumanis / Dan Stechow / Roseann Miller, additional
programming Gregg Seelhoff / Johnny Klonaris / Bob Luzenski), the SIMPLE
INSTRUCTIONS / COMMANDS / CHARACTER MOVEMENT help screens, the
new-game/erase/restart character-management prompts, the "poor peasant on
the world of Tarmalon" intro narrative, and MML-style music strings
(`t120l4cl8ef…`). Not yet marked up as strings in `menu.idb`.

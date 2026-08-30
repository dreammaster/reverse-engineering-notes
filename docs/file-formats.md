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

## Screen-string pool (in the `.EXE`, not a file) — decoded 2026-08-30

Every module keeps its on-screen text as a **pool of string-constant
records**, each:

```
dw  length            ; 1..~200
dw  dgroup_ptr         ; near offset of the descriptor in the DGROUP segment
db  length bytes       ; the raw text
(db 00 or F4)          ; 0-1 byte, to word-align the next record
```

Arrangement differs by how the module is linked:

- **`MENU.EXE`** (not EXEPACK'd) — the pool *is* the DGROUP segment
  (`seg003`), starting ~`seg003:0x2150`. `dgroup_ptr` is self-relative
  (points at the text 4 bytes after the descriptor). Code does
  `mov ax,<descriptor_addr> / push ax / call basStrAssign`.
- **`OUT.EXE`** and the other EXEPACK'd modules (`DUN`, `TWNDR`,
  `CASDR`, `CONFIGUR`) — the pool sits in the **code segment's tail**
  (`out`: `seg000:~0x748C`–`0x81B0`) as a DGROUP *initialiser* list. The
  BASIC startup copies each record's text to `dgroup_ptr` in the (BSS)
  DGROUP segment (`seg001`) and builds a 4-byte descriptor at
  `dgroup_ptr − 4`. Code immediates are those `dgroup_ptr − 4` values.

`ida_scripts/dump_strings.py` walks the pool for either arrangement,
maps each record to the code sites that reference it, and (with
`ANNOTATE = True`) writes the text as an inline comment at each
`mov reg,<dgrp>`.

### In-string control codes (interpreted by `drawString` / `rtm_FE26`)

The text is not plain — a handful of punctuation characters are
directives:

| code | hex | meaning |
|---|---|---|
| `%` | `0x25` | newline. A leading run (`%%%`) positions the text N lines down; mid-string `%` breaks a line. |
| `@` | `0x40` | column / cursor-position marker (leading). |
| `!` `#` `$` `&` | `0x21` `0x23` `0x24` `0x26` | trailing paragraph / page-break / wait-for-key directives (exact split TBD). |

What looks like a 2-byte prefix in a hex dump (`N,` `j-` `B.` `F'`) is
just the low+high byte of the *previous* record's `dgroup_ptr` shown as
ASCII — not a code.

### What's in each pool

`MENU.EXE` `seg003:0x2150`+ (~4 KB): main-menu items, GAME CREDITS
(Doughertys / Al DeYoung / Tumanis / Stechow / Miller / Seelhoff /
Klonaris / Luzenski), SIMPLE INSTRUCTIONS / COMMANDS / CHARACTER
MOVEMENT, character-management prompts, the "poor peasant on the world of
Tarmalon" intro, MML music strings (`t120l4cl8ef…`).

`OUT.EXE` `seg000:0x748C`+ (~140 records): overworld messages — terrain
("GRASSLANDS", "A FOREST", "THE MOUNTAINS", "THE WATER"), travel/raft
("THE RAFT MUST STAY IN THE WATER.", "YOUR RAFT SINKS."), combat
("ATTACKED BY ", "YOUR ATTACK MISSES.", "ENEMY HIT BY BLOW OF "),
shop ("DO YOU WANT TO BUY", "MUSEUM COIN FOR "), death ("YOU DIED!!!",
"THE POWERS OF THE MUSEUM / RESURRECT YOU FROM THE GRAVE!!"), the museum
access code ("World- / Stone- / Ring- "), and the chained-EXE names.

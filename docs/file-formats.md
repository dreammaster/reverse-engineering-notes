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
| `TITLE.GLB` / `TITLE.GMP` | 8209 / 2111 | `MENU` | **`.GLB` = the 8x8 tile bitmaps, `.GMP` = the per-cell tile-index map** (proven by `menu.idb` `seg001`'s `loadTitleImage`: it reads `.GLB` to `titleGlbBuf` + 0x11 = the bitmap data, `.GMP` to `titleGmpBuf`, then `blitCharCell` copies `map[cell]`-th 8x8 bitmap to `0xB800`). `scrollTitleImage` slides it horizontally (`titleScrollX`, step 40, wrap 160). |
| `TOWN0.BSV`…`TOWNB.BSV` | ~5013–5123 | `TWNDR` | 12 town layouts |
| `CASTLE.BS1` / `.BS2`, `FORT.BS1` / `.BS2` | 12967 | `CASDR` | castle / fort layout banks |
| `TCASOBJ.BSV` | 4911 | `CASDR` | town/castle object table |
| `FORTANIM.BSV` | 263 | `CASDR` | fort animation |
| `OUTDATA.BSV`, `OUTOBJ.BSV`, `OUTM0.BSV`…`OUTM2.BSV`, `OUTDAT.DAT` | — | `OUT` | overworld map / objects / monsters |
| `DUNDATA.BSV`, `DUNOBJ.BSV`, `DUNM1.BSV`…`DUNM3.BSV`, `DUNMONA.BSV` / `DUNMONB.BSV` | — | `DUN` | dungeon map / objects / monsters |
| `DIS0.BSV`…`DIS15.BSV` (+ `DIS0A`, `DIS1A`) | 727–2055 | ? (`DIS9` → `CELDRV`) | "display" screens? ~18 of them. `celdrv_entry` BLOADs `DIS9.BSV` as one of the five ending image banks. |
| `CEL0.BSV`…`CEL3.BSV` | 1573 / 2597 | `CELDRV` | endgame-cinematic image banks — `celdrv_entry` BLOADs `CEL0`/`CEL1`/`CEL2` (loop), then `DIS9.BSV`, then `CEL3.BSV`, via `rt_FE07`, and relocates each one's internal offset table by its load segment. |
| `MUSDATA.BSV`, `MUSOBJ.BSV`, `MUSMSG.TXT` | 8055 / 12961 / 11229 | `MUS` | MUSEUM data / exhibit objects / message text |
| `SDMAP.GLB` / `.GMP`, `SDOBJ.GLB` | 4113 / 2081 / 6161 | `SDEFENDR` | combat-training arena playfield / sprites (loaded into `seg004`, driven by the hand-written `arenaGameLoop`) |
| `BJCHR.GLB` | 6161 | `GMB1`/`GMB2` | card sprites ("BJ" = BlackJack) — `GMB1` (BlackJack) BLOADs it into `seg004`; `GMB2` shares it |
| `BIGNUM.DAT` | 420 | `GMB2` (at least) | large-digit font — `GMB2` (Flip-Flop Parlour) BLOADs it to render the GOLD / BET / winnings numbers |
| `D.BSV`, `R.BSV`, `PEGASUS.BSV` | 3527 / 3527 / 1159 | ? | ? |
| `CHAR.DAT` | 3444 | `MENU` / `SAVER` / all play modules | character roster **and** the in-progress save (there is no separate save file). `SAVER.EXE`'s `saveRosterToDisk` is the write side; the menu roster screens + `readLegacyDat` read it. The "is not on this / character disk" / "empty" strings in `SAVER` imply a per-slot / removable "character disk" scheme. |
| `LEGACY.DAT` | 2945 | `MENU`/`OUT` | **no `0xFD` magic** — not BSAVE; format unknown (config / progress?) |
| `OUTDAT.DAT` | 1012 | `OUT` | not checked yet |
| `DRCONFIG.DAT` | 1015 | `CONFIGUR` + all disk-loading code | **disk-drive layout**, not hardware config — which drive letter(s) hold the game floppies (or HD floppy / hard disk), so the loaders know where to look and can prompt for swaps. Written by `CONFIGUR.EXE` (`_main`); at offset near the start it holds a config-type byte (`'0'`/`'1'`/`'2'` — HD/hard-disk vs. 360K vs. 720K) and one or two drive-letter bytes. |
| `STDRVSCR.DAT` | 6192 | `STDRV` | "Stones of Wisdom" rules / instruction text — the walk-through the dealer narrates ("YOU AND THE DEALER BOTH RECEIVE FIVE DICE…", "THE LOSER OF A GAME GIVES UP ONE DIE…"). Read by `stonesOfWisdomMain`; **not** a story/cut-scene script. |
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

Once a module is unpacked (see [overview.md](overview.md#packing)) the
pool **is** the DGROUP segment — for both menu (`seg003:~0x2150`+) and
out (`seg003:~0x2150`+). `dgroup_ptr` is self-relative (the text is the
4 bytes right after the descriptor); code does
`mov ax,<descriptor_addr> / push ax / call basStrAssign`.

(In the *still-packed* image, DGROUP is BSS and the pool sits as an
initialiser list in the code segment's tail with `dgroup_ptr` = the BSS
destination — which is why the un-relocated far pointers and un-copied
DGROUP made the packed `out.idb` unreliable. Unpack first.)

`ida_scripts/dump_strings.py` walks the pool, maps each record to the
code sites that reference it, and (with `ANNOTATE = True`) writes the
text as an inline comment at each `mov reg,<descriptor_addr>`.

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

`OUT.EXE` `seg003:~0x2150`+ (~140 records): overworld messages — terrain
("GRASSLANDS", "A FOREST", "THE MOUNTAINS", "THE WATER"), travel/raft
("THE RAFT MUST STAY IN THE WATER.", "YOUR RAFT SINKS."), combat
("ATTACKED BY ", "YOUR ATTACK MISSES.", "ENEMY HIT BY BLOW OF "),
shop ("DO YOU WANT TO BUY", "MUSEUM COIN FOR "), death ("YOU DIED!!!",
"THE POWERS OF THE MUSEUM / RESURRECT YOU FROM THE GRAVE!!"), the museum
access code ("World- / Stone- / Ring- "), and the chained-EXE names.

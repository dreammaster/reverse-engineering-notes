# How the DOS build draws — the real drawing code

Everything on screen comes from **one** native routine family in `SYSTEM.INTERP`
(the `CONUNIT` driver, unit 13) plus a thin layer of p-code in the resident
`WIZARDRY` segment.  Nothing else touches video.  This document is the map;
`tools/wizvm.py` + `tools/conunit.py` run the original p-code against a
transcription of the native code and are the executable proof ("the oracle").

The earlier engine UI (`TextScreen`: a 40×24 text grid with UCSD control codes,
Apple-style pixel wireframe, hand-placed frames) was a port of the *Apple II*
model.  The DOS build has none of that: it has a **window compositor** with
z-order, a 40×**25** cell screen, 8×8-px cells in CGA 320×200×4, a cell-glyph
wireframe, and its own window/text procs.  Everything below is from the DOS
code.

## Layers

```
 per-screen p-code (CASTLE, ROLLER, CAMP, RUNNER, COMBAT, ...)      L3
        │  calls
 WIZARDRY text procs: print/menu/prompt/getline (13,22-24,29-31,35,36,39-44,77-81)   L2
 WIZARDRY window manager: open/close/clear/hide/sort (16-21,25-28,37,67,68)         L1
        │  UNITWRITE(CONUNIT, rec, 0, selector, param, 0)
 CONUNIT native driver (SYSTEM.INTERP 0x144B..0x25xx)                               L0
        │  int 10h mode 5 + direct CGA VRAM (B800:0000)
```

`tools/drawcalls.py --md` regenerates `drawing-inventory.md`: every procedure
that draws, directly or through a helper (416 of the 659).

## L0 — CONUNIT driver

`UNITREAD/UNITWRITE(unit, buf, X, Y, Z, W)` — the p-code pushes six values;
for unit 13, **Y is the function selector**, `buf+X` the buffer / window
record, **Z** the parameter (char, string, window record, file block), W unused.
(`pmachine.md` called Y "blocknumber"; the table of selectors below supersedes it.)

| sel | handler | does |
|--:|---|---|
| WR 1 | `0x1AE6` | init: copy config `13E4→13D8` (`word_13DA=200` lines, `13DC=8`, `13DE=1`, `13E0=512`), pick video (CGA path if CRTC 3D4 answers, else Hercules), build tables, returns video type bits in `[buf]` (`0x31` = 200-line CGA) |
| WR 2 | `0x153E` | **layout**: `head := buf`; clear PRIO[]; for each window in the list (skip hidden): paint border (`sub_159A`) + interior (`sub_16F2`); then flush dirty cells to VRAM (`sub_175D`) |
| WR 3 | `0x1577` | **present** one window now: border via `sub_1797` (standard glyphs only), interior via `sub_17FE` |
| WR 4 | `0x14C4` | **putchar**: store word `Z` at the window cursor, draw it if the window is on top, advance cursor (wraps at the right edge, then back to row 0 — **no scrolling**; the p-code clears/scrolls) |
| WR 30 | `0x14A5` | putstring: Pascal string at `Z`, one `putchar` per byte |
| WR 5 | `0x18C1` | **DRAWLINE** (the maze wireframe primitive) — see below |
| WR 13 | `0x1958` | compose monster portraits: up to 4 pic ids at `buf` → 6×5 tile blocks at columns {15 / 11,19 / 7,15,23 / 3,11,19,27}, row 9 of the 36×20 window; loads `200.MONSTERS` records (512 B, pic−1) |
| WR 17 | `0x1950` | zero the attribute bytes of a 36×20 window (720 cells) |
| WR 14 | `0x146B` | load `*.CHARSET` (320 glyphs × 16 B) |
| WR 18 | `0x1496` | remember the `*.MONSTERS` file's start block |
| WR 23 | `0x1AE1`→`0x1EF8` | **title bitmap**: 56 scanlines × 70 B blitted straight to VRAM at cell (2,2) (Z = first block of `200.TITLE`; 0 clears it) |
| WR 19 | `0x2305` | timed pause (polls `int 1Ah`/`int 16h`) |
| WR 24 | `0x235D` | PC-speaker beeps (Z = 0/1/2) |
| WR 26 / 29 | `0x244A`/`0x247B` | Pascal string concat / `POS` (used when `HAS.STROPS` is present) |
| WR 20,22,25,31 | | disk/volume services (drive reset, virtual-disk cache, TOC reads) |
| RD 10 / 11 / 12 / 16 / 21 | | RANDOM / key-buffered? / const 1 / ? / **drive size in blocks (`0x278` = 632)** |

### Window record (heap, built by `WIZARDRY:19`)

```
+0 u16 next      +2 u8 x   +3 u8 y   +4 u8 w   +5 u8 h   (w,h include the border)
+6 u8 cursor x   +7 u8 cursor y      (interior coordinates)
+8 u8 priority   (bit 7 = hidden; 0x3F = emphasis border)       = (prio arg) + 1
+A u16 cells[(w-2)*(h-2)]   lo = glyph, hi = attribute           filled with 0x2020
```

### Screen model

40 × **25** cells (`3E8h` = 1000 everywhere in the driver).  Two shadow arrays
live in the driver's segment: `PRIO[1000]` (0xE900; low 6 bits = priority of the
window that owns the cell, bit 7 = dirty, bit 6 = "old cell was a tile") and
`CELL[1000]` (0xED00; the word last composed).  **Final image = f(PRIO, CELL)**:
a cell with `PRIO==0` is blanked, otherwise `CELL` is drawn.  Priority rule
everywhere: a window writes a cell if `win.prio >= (PRIO & 0x3F)`; later windows
in the list win ties.

Cell word → pixels (`sub_1882`):

| `hi` (attr) | `lo` | draws |
|---|---|---|
| `0x00` | `0x00`/`0x20` | blank |
| `0x00` | other | CHARSET glyph `lo` (8×8 px, 2 bpp, 16 bytes) |
| `0x20` | any | blank |
| `≥0x80` | any | solid block (all pixels 3) |
| `1..0x7F` ≠`0x20` | – | **monster tile**: tile = `(hi&0x1F)−1`, group = `hi>>5 & 3`; 16 bytes from that group's `200.MONSTERS` record at `tile*16` |

Borders (`sub_159A`, called by *layout*): glyphs `1 2 3 / 4 5 / 6 7 8`
(TL T TR / L R / BL B BR).  If `priority == 0x3F` ("emphasis") the corners/edges
are `0D 0A 0E / 0B 0C / 0E 09 0D`.  *present* (`sub_1797`) always uses the
standard set.  There are **no frameless windows** — every window from
`WIZARDRY:19` has a border; hidden ones (bit 7) are skipped entirely.

Video: `int 10h` mode **5** (320×200, 4 colours: black, cyan, red, white); Ctrl
at boot → mode 4, Alt → mode 6 (640×200 mono; the same glyph bytes then read as
16-px 1bpp).  A cell is 2 bytes × 8 rows, CGA bank-interleaved; cell (x,y) is at
`y*320 + x*2` (+0x2000 on odd scanlines).  Hercules (3B4) and the `400.*` assets
are the 400-line alternative path.

### DRAWLINE (`0x18C1`) — exact

`Z` = window record, `buf` = 8-word argument record:
`+0 clipLo  +2 clipHi  +4 col  +6 row  +8 dCol  +A dRow  +C count  +E glyph`.
Interior width `W=w−2`, height `H=h−2`.  Cell = `Z+0xA + 2*(row*W+col)`.
Byte step = `2*((dRow? ±W : 0) + dCol)` (8-bit arithmetic).  Loop: store `glyph` if
`clipLo <= col <= clipHi`; advance; **stop** when col or row goes negative, when
`col == W`, when `row == H`, or the count reaches 0 (count 0 = run to the edge).
It writes into the window's *cell array*; a later *present*/*layout* shows it.
The maze view is `WINDOW1` (36×20 interior); `RUNNER` procs 3, 21-44 are its
callers (`maze.md`).

## L1 — window manager (`WIZARDRY`)

Global 14 is the window-list head (`globals.md` mislabels it `CACHE_LRU`),
global 17 is `WINDOW1`.

| proc | |
|--:|---|
| 19 | `Open(x,y,w,h,prio,refresh,_,_) : ^win` — alloc (18), fill header, cells := `' '`, sort (68), optionally refresh (67) |
| 18 | `Alloc(size)` — first-fit over a free list of released records, else `NEW`; the record gets linked at the list head |
| 20 | `Close(var win, refresh)` — unlink, free (17), refresh |
| 21 | `SetPrio(win, p, refresh)` |
| 25 | `Clear(win, refresh)` — cells := `' '`, cursor := 0,0; refresh (37) or present (WR 3) |
| 26 | `ClearRect(win, x, y, w, h)` + present |
| 27 / 28 | hide (`prio += 128`) / show (`prio mod 128`), then refresh |
| 37 / 67 | `WR 2` layout (67 only if its flag is true) |
| 68 | one bubble pass over the list so it is ordered by ascending `prio mod 128` (hidden bit ignored) — equal priorities keep their order |
| 16 | coalesce adjacent free records |
| 17 | free a record (mark it free, `prio` field := 1) |

## L2 — text procs (`WIZARDRY`)

`35` putc (`WR 4`), `36` puts (`WR 30`; strings may embed `0x81..0x9F` attribute
prefixes), `22` GotoXY (clamped to the window), `24` newline, `23` right-justified
number, `13` write-string + newline, `39` GetStr+puts, `40` GetStr+writeln, `29`
GetLine (underscore cursor, BS/CR/ESC), `30` **menu/prompt reader** (parses
`X)` hotkey strings and `#` party digits), `31` string template expander
(`*`/`+`/`-` prefixes), `41` GetStr+menu, `42` visible-length, `43` centred
boxed message, `44` party-gold total, `77` clamp, `78` backspace.

## The oracle

```
python tools/wizvm.py run --keys 'S\r\r' --shotdir out/ --shot out/final.png
```
runs `SYSTEM.PASCAL` against `WIZ1.DSK` (read from `C:\games\wizard15`; the
in-memory copy hides `HAS.CACHE`, a RAM-page-cache speed-up, and reports the
drive as 632 blocks), shoots the screen at every key wait, and stops when the
script runs out of keys.  `tools/vmtrace.py` traces procedure entries.  Because
it executes the real p-code, any screen reachable by keystrokes can be captured
and its `PRIO`/`CELL` arrays compared with the engine's.

Known oracle limits: the RANDOM generator is the shipped LCG, the key "stir" is
not modelled, `CSP 38` (flush) is a no-op, sounds are ignored.

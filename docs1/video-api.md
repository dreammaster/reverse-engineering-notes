# Video driver API (`VID16.DRV`, `INT 62h`)

`VID16.DRV` is a terminate-and-stay-resident driver for EGA/VGA 640x350x16
planar graphics plus the mouse. Every Chapter 1 program (`SW.EXE`,
`PROG1`–`PROG18`, `INSTALL.EXE`; `PROG0` makes no calls) draws and reads the
mouse only through this driver. It is the natural platform boundary for the
reimplementation: `Yendor1::Vid` should expose one function per row below.

Source: read in full from `yendor1/vid16.asm` on 2026-10-22; names are in
`vid16.idb` (see `ida_scripts/vid_api.py`, `name_vid16.py`). Statements are from
the disassembly unless marked *(inferred)* — those still need confirming against
a client.

## Installation

- The EXE's entry point (`start`) sets the vector for `INT 62h` to the image base
  (`PSP:0100h`, the first byte after the PSP) and exits with
  `INT 21h/AH=31h` (TSR). The ISR is at image offset 0; IDA offsets in
  `vid16.asm` are image offsets.
- Nothing is allocated until the client calls **fn00**; it allocates the
  context block and tile store (below), reads the BIOS ROM 8x8 and 8x14 fonts
  (`INT 10h/1130h`), detects the mouse (`INT 33h/0`, hooks event mask `0Bh` →
  `vid_mouseEventHandler`) and returns the **context segment** in `AX`
  (`FFFFh` = out of memory).
- `SW.EXE` is the program that loads the driver and keeps the segment
  (`word_12880` in `sw.idb`) *(inferred; the loader step itself is Phase 2)*.

## Calling convention

```
mov ah, <function>     ; 00h..2Dh
[mov al, <arg>]        ; only fn0D, fn12, fn2D read AL
int 62h
```

- **Parameters are not passed in registers.** The client writes them into the
  driver's *context block* — the segment returned by fn00 — at fixed offsets
  (`es:[1Ah]`, `es:[1Ch]`, …) before the call. The ISR itself loads `ES` with
  the context segment, so a client's own `ES` is irrelevant to the call, but it
  needs the segment in some register to set the fields (hence the constant
  `mov es, word_12880` before runs of `mov es:[xxh], …`).
- The ISR saves and restores `BX CX DX SI DI DS ES` (and `BP`); **`AX` is
  clobbered** except fn00 (returns the segment / `FFFFh`) and fn02 (returns 0).
  Results are returned in context fields (fn04 → `ctx+30h`).
- `AH > 2Dh`, or an unimplemented number, returns immediately (the table has no
  handler for `18h`, `1A`–`1D`, `22h`, `24`–`27`). `PROG10` makes one call with
  `AH=36h, AL=13h` that therefore does nothing.
- The ISR executes `STI` first, so interrupts are enabled inside calls.

## Display model

- Mode `10h`: 640x350, 16 colours, 4 planes, **80 bytes per scanline**; a pixel
  is addressed as byte `y*80 + x/8`, bit `7 - x%8`.
- **Two video pages**: page 0 at `A000:0000`, page 1 at `A000:8000`. The
  displayed page is switched with `INT 10h/AH=05h` (fn12). `ctx+31h` selects the
  page that drawing calls (`fn03..fn0C`, `fn13`) write to (0 or 1);
  `ctx+32h` is the destination page of fn0D. Rows 350–409 of each page are
  off-screen scratch memory; the driver stages the mouse cursor at y=360.
- Most games draw the next scene on one page and reveal/copy it to the other
  (fn14/fn1E and the `Reveal*` wipes below are the page-0↔page-1 transfers).
- 32x32 pixel tiles are the unit of the map view (a tile is 512 bytes planar-
  packed in the tile store, 4 bytes per plane row).

## Context block layout

Offsets from the context segment. `w` = word, `b` = byte.

| Offset | | Meaning |
|---|---|---|
| `00h` | w | tile slot count (fn00 sets 200) |
| `02h` | w | segment of the tile directory: 200 entries x 6 bytes `{FFFFh, FFFFh, segment}` (one 512-byte tile each) |
| `04h` | w | segment of the tile pixel store: 200 x 512 bytes |
| `08h` | w | busy-wait count after a page flip and per step of the centre/edge wipes |
| `0Ah` | w | busy-wait count per step of the right-to-left wipes (fn16/fn20) |
| `10h` | w | image height in rows (default `20h`) |
| `12h` | w | image width in bytes per plane row (default `4`) |
| `14h` | b | right-edge pixel mask (default `FFh`) |
| `16h`/`18h` | w/w | far pointer (segment/offset) to image data; fn0C reads a 5-byte header (`10h`,`12h`,`14h`) then advances `18h` by 5 |
| `1Ah` `1Ch` | w w | rectangle/point corner 1 (x1, y1) |
| `1Eh` `20h` | w w | corner 2 (x2, y2) |
| `22h` `24h` | w w | destination (x3, y3) |
| `2Ah` `2Bh` | b b | text cell column / row for fn08 (x = col*8) |
| `2Ch` | b | character code (fn08) |
| `2Eh` `2Fh` | b b | colour masks used by fn08 *(inferred: foreground/background plane masks)* |
| `30h` | b | drawing colour (fn03/05/06/07); fn04 result |
| `31h` | b | current drawing page (0/1) |
| `32h` | b | destination page for fn0D |
| `34h` | b | transparent colour for fn13 |
| `35h` | b | font select for fn08: 0 = 8x14 ROM font, nonzero = 8x8 ROM font |
| `38h` `3Ah` `3Ch` | w w w | 8x8 font: height (8), far pointer seg/off |
| `3Eh` `40h` `42h` | w w w | 8x14 font: height (14), far pointer seg/off |
| `64h`–`73h` | 16 b | EGA palette-register map, identity 0..15 at init *(inferred; written, not read by the driver)* |
| `678h` `679h` | b b | map view size for the scroll calls fn0E–fn11 (in 32px steps, see code: width `[678h]*2*…`) |
| `67Ah` `67Ch` | w w | cursor hot-box margin for mode 1 |
| `67Eh` `680h` | w w | map view origin offsets used by fn0E–fn11 |
| `912h` | b | flags set by the game; bits `C0h` switch cursor mode 2 to its alternate (fixed-box) shape selection |
| `952h` | b | mouse present (`INT 33h/0` status; `FFh` = yes) |
| `953h` | b | mouse event flags: bit 7 = moved, left press sets `60h`, right press sets `42h` (consumed by the client, e.g. by clearing) *(inferred use)* |
| `955h` | b | cursor show counter; `FFh` (-1) = hidden, `0` = shown |
| `956h` | b | current cursor shape index (1..12; 0 = save slot) |
| `957h` | b | page the cursor is being drawn on / currently displayed |
| `958h` | b | cursor mode (see fn2D) |
| `95Ah` `95Ch` `95Eh` `960h` | w x4 | mouse clip rect: x min, x max, y min, y max |
| `962h` `964h` | w w | position where the cursor is currently drawn |
| `966h` `968h` | w w | mouse position (x, y) — read by clients |
| `96Ah` `96Ch` | w w | cursor hot-box centre (init `130h`, `8Eh`) |
| `96Eh` | w | segment of the cursor image store (`72h` paragraphs) |
| `970h` | 13 w | offsets of cursor slots, step `85h` bytes; slot 0 holds the saved background, slots 1–12 hold 16x16 cursor images *(the game fills the images in; inferred)* |

## Functions

`x1,y1`/`x2,y2`/`x3,y3` are the corner fields above. "Page" is `ctx+31h`.

### Setup

| AH | Name | Summary |
|---|---|---|
| 00 | `vidInit` | Allocate, read ROM fonts, init mouse. Returns ctx segment in `AX` (`FFFFh` failed). Install once. |
| 01 | `vidShutdown` | Free memory (cursor store, tile store, directory, context), reset the `INT 62h` vector to 0:0. |
| 02 | `vidSetGraphicsMode` | `INT 10h` mode `10h`. Returns `AX=0`. |

### Primitives (draw into `ctx+31h` page)

| AH | Name | Summary |
|---|---|---|
| 03 | `vidPutPixel` | Pixel (x1,y1) in colour `ctx+30h`. |
| 04 | `vidGetPixel` | Colour at (x1,y1) → `ctx+30h`. |
| 05 | `vidLine` | Line (x1,y1)–(x2,y2); Bresenham with horizontal/vertical fast paths; endpoints are sorted internally (the fields are swapped in place). |
| 06 | `vidRect` | Rectangle outline (x1,y1)–(x2,y2). |
| 07 | `vidFillRect` | Filled rectangle, drawn as `y2-y1+1` horizontal lines. |
| 08 | `vidDrawChar` | One ROM-font character at text cell (`2Ah`,`2Bh`), code `2Ch`; font per `35h`. |

### Images

Image data are **planar**: for each of the 4 planes (plane 3 first), `height`
rows of `width` bytes. A 5-byte header `{rows:w, widthBytes:w, mask:b}` precedes
the data in the "Hdr" variants.

| AH | Name | Summary |
|---|---|---|
| 09 | `vidGetImageRaw` | Copy screen rect at (x1,y1) of `10h` rows x `12h` bytes into memory at `16h:18h` (no header). |
| 0A | `vidPutImage` | Draw the image at `16h:18h` (size from `10h`/`12h`/`14h`) at (x1,y1). The `14h` mask is applied to the right edge byte; arbitrary x shifts the data. |
| 0B | `vidGetImage` | Capture rect (x1,y1,x2,y2) to `16h:18h` **with** the 5-byte header. Inverse of fn0C. |
| 0C | `vidPutImageHdr` | Read the 5-byte header at `16h:18h` (sets `10h`,`12h`,`14h`, advances `18h` by 5), draw the image at (x1,y1), then reset `10h/12h/14h` to `20h`,`4`,`FFh`. |
| 0D | `vidCopyTileRow` | `AL` = number of 32-pixel tiles. Copy `AL*4` bytes (all four planes, write mode 1 latch copy) from (x1,y1) on page `31h` to (x3,y3) on page `32h`. |
| 13 | `vidCopyRectKeyed` | Copy rect (x1,y1,x2,y2) → (x3,y3) pixel by pixel, skipping colour `ctx+34h` (sprite blit with colour key). Slow path. |

### Map-view scroll (one 32-pixel step each)

`vidScrollRight`(0E), `Left`(0F), `Up`(10), `Down`(11): shift the map view
region in video memory by 32 pixels using `ctx+678h..680h`. Content moves in the
named direction. Only used by one program.

### Pages and transitions

| AH | Name | Summary |
|---|---|---|
| 12 | `vidShowPage` | `AL` = page to display. If the cursor is shown, erases it from the old page, saves/draws it on the new page (`957h` = new page), then flips (`INT 10h/05h`) and busy-waits `ctx+08h`. With no mouse or the cursor hidden it just flips. |
| 14 | `vidCopyScreen0To1AndShow` | Copy all 350 rows of page 0 to page 1; display page 1. |
| 1E | `vidCopyScreen1To0AndShow` | Copy page 1 to page 0; display page 0. |
| 19 | `vidCopyRows0To1AndShow` | Copy scanlines y1..y2 (full 80-byte rows) of page 0 to page 1 starting at row y3; display page 1. |
| 23 | `vidCopyRows1To0AndShow` | Same, page 1 → page 0; display page 0; sets drawing page 0. |
| 15 / 1F | `vidRevealCenterOut0To1` / `1To0` | Wipe one page onto the other, one 8-pixel column pair at a time from the centre outward. Delay `ctx+08h` per step. |
| 17 / 21 | `vidRevealEdgesIn0To1` / `1To0` | The same from both edges inward. |
| 16 / 20 | `vidRevealRightToLeft0To1` / `1To0` | Column-by-column right-to-left sweep, delay `ctx+0Ah` per column. |

### Mouse

| AH | Name | Summary |
|---|---|---|
| 28 | `vidMouseShow` | Increment the show counter `955h` (starts at -1 = hidden); draws the cursor when it reaches 0. No-op if no mouse. |
| 29 | `vidMouseHide` | Erase the cursor if shown; decrement the counter. |
| 2A | `vidMouseSetBounds` | Set the clip rect to (x1,y1,x2,y2) and clamp the position. |
| 2B | `vidMouseRefresh` | If a move was flagged (`953h` bit 7), redraw the cursor at the new position and clear the bit. |
| 2C | `vidMouseSetPos` | Move the pointer to (x1,y1) clamped to the clip rect. |
| 2D | `vidMouseSetCursorMode` | `AL` → `958h`, recompute cursor shape. Mode 0: arrow (shape 1). Mode 1: shape 2 if inside a hot-box around (`96Ah`,`96Ch`) and the play area, else arrow. Mode 2: directional cursor — the play area is split into regions around the hot-box centre and the shape (3–6, 9–12) points toward the region the pointer is in *(inferred: used for walking by mouse)*. |

The INT 33h event handler (`vid_mouseEventHandler`, mask `0Bh`) maintains
`953h`/`966h`/`968h` asynchronously: movement is added from the motion counters
(`INT 33h/0Bh`), clamped to the clip rect; button presses read
`INT 33h/5` press counts.

## Usage across the programs

2,173 call sites in 20 programs (`PROG0` has none), 36 distinct functions
(`ida_scripts/annotate_int62.py`; every site resolved). By frequency:
`vidCopyScreen0To1AndShow` 458, `vidShowPage` 410, `vidMouseHide` 358,
`vidMouseShow` 316, `vidFillRect` 114, `vidMouseSetCursorMode` 102,
`vidDrawChar` 74, `vidCopyScreen1To0AndShow` 64, `vidMouseRefresh` 42,
`vidRect` 37, `vidCopyRectKeyed` 36; the rest are single digits to ~20.
The scroll calls (0E–11) and `vidCopyTileRow` are used by one program only.
fn21 is implemented but never called.

## Notes for `Yendor1::Vid`

- Keep the original calling contract: a plain struct holding the context fields
  plus one method per function. Clients today set fields directly; the C++
  port can keep a `Context` struct with the same names and offsets (useful while
  both disassembly and port are being cross-checked).
- Model the planar 640x350 two-page display as two 4-plane byte buffers, or
  convert to 8bpp at present time. The page-1 off-screen rows matter (the
  cursor stage lives at y=360; fn19/23 copy full scanlines).
- The busy-wait delays (`ctx+08h`, `ctx+0Ah`) are CPU-speed-dependent loops;
  replace by timed waits.
- Cursor shape images and the mapping from mode/position to shape (fn2D) must
  be reproduced exactly, since programs depend on `966h`/`968h` and `956h`.

## Open questions

- Who fills the cursor image slots (`96Eh:970h…`) and the tile store, and in
  what format *(to confirm in `SW.EXE` / `PROG4`)*.
- Meaning of `ctx+2Eh/2Fh` for fn08, and the exact scroll window semantics of
  `678h–680h` (only `PROG`-one client; trace there).
- `VIDP16.DRV` (817 bytes): a COM-style TSR (image base `0100h`) that hooks
  `INT 1Ch` (timer) and `INT 10h` and writes EGA palette registers
  (`INT 10h/AX=1002h`); likely a palette fade/cycle service. It checks that the
  magic word `1234h` is present in the PSP area `PSP+0D8h` (which `SW.EXE` sets,
  `word_119FE = 1234h`) and exits with 0FFh otherwise, so it is only launched by
  `SW.EXE`. Not yet traced. Its database is loaded as a flat binary at base 0, so
  addresses are off by `0100h`.

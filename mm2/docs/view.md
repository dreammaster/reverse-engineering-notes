# First-person maze view (2PLAY)

Entry points: `draw_view_indoors` (`18744`, towns/dungeons/castles, `g_outdoors == 0`) and
`draw_view_outdoors` (`18D6C`); the dispatcher is resident `13FFC`.  Both draw into video page 1 through
the driver's "draw image" function (`gfx_draw_op13(bank, image, x, y)`), then `gfx_copy_page` shows it.

## Map data used

* Current 16x16 map: walls `DGROUP:59D6` (256 bytes) and flags `5AD6`; the four outdoor neighbours
  are staged at `5BD6/5CD6/5DD8/5ED8` so coordinates 0-15 wrap into the adjacent map
  (`view_sample_row` `1B44E`, `map_cell_at_offset` `1B7C8`).
* A wall byte holds 2 bits per side: **N = bits 6-7, E = 4-5, S = 2-3, W = 0-1** (masks C0h, 30h, 0Ch, 03h;
  `set_facing_masks` `1423E` stores the mask of the side the party faces in `byte_23216` and its shift in
  `byte_23217`: N = C0h/6, E = 30h/4, S = 0Ch/2, W = 03h/0).  Values per side: 0 open, 1 wall, 2 door,
  3 wall with an object/sprite on it (drawn as a wall, plus a queued sprite except at the farthest depth).
  **y grows to the north** (facing N steps `y + 1`).  Wall-value translation table `DGROUP:52B2` (`15F40`)
  maps the low 5 bits of the flag/terrain byte to a wall style.

## Building the visible cell set (`view_prepare_visible_cells` `1B4E0`)

For the facing direction it reads six offsets from `DGROUP:16F8/16FE/1704/170A` (N/S/E/W) and samples
three rows of four cells ahead (`59CA[4]` centre lane, `59CE[4]` left lane, `59D2[4]` right lane),
plus the cell's flag byte (`byte_23218`).  `wall_collect` (`1BEBA`) then turns the 12 sampled bytes and the
facing masks into 20 wall variables `byte_27826..27839`: for each depth 1-4 a **front wall** and a
**left/right side wall**, and for each outer lane a wall seen edge-on.  Doors that would be hidden behind
a wall (`3` beside `1`) are normalised to `1`.

## Collecting walls (`wall_collect` `1BEBA`, reproduced by `tools/mm2_view.py`)

Samples: depth 1 = the party's own cell, depth 2-4 = the cells ahead.  Facing masks: `F` = the facing side,
`L`/`R` = the side to the left/right (N: L = W bits, R = E bits).  Step/lane offsets per facing (`DGROUP:16F8`
N, `16FE` S, `1704` E, `170A` W): N step (0,+1), left lane (-1,0), right lane (+1,0); S the opposite;
E step (+1,0), left (0,+1), right (0,-1); W the opposite.  For depth *n* = 1..4:

* **L(n)** = centre cell `& L`; if zero, **LO(n)** = left-lane cell `& F` (its front wall, visible because the
  side is open).  Same on the right: **R(n)**, **RO(n)**.
* **F(n)** = centre cell `& F`.  If it is non-zero it ends the scan (nothing behind it is visible) after one more
  look: when the left side of depth *n* has neither L nor LO, **LO(n+1)** = left-lane front wall of the next
  cell (same on the right), for *n* = 1..3.
* Fix-up: a `3` in LO(n+1) beside an existing L(n) becomes 1 (n = 1, 2; same right).

## Drawing (`draw_view_indoors`)

Everything is drawn with `gfx_draw_op13(bank, image, x, y)` into page 1 (view area 208x120 at (8,8)):

1. sky (`SKY.16` image chosen by `view_indoor_daylight`, 208x60) at (8, 8), then the floor (`<style>F.16`
   image 0, 208x60) at (8, 68).  At night (`g_day_fraction >= 80h`) with the sky image 0, `sub_14FB2` adds stars.
2. for depth 4 down to 1: L, LO, R, RO, then F (this exact order; the images below are in the style bank
   `TOWN/CAVE/CASTLE.16`, 32 images: 0-15 plain, +10h the door variant of each):

| Piece | Depth index *i* = n-1 | Image | x | y |
|---|---|---|---|---|
| F front | 0..3 | `i` | 32, 64, 88, 104 | 22, 40, 54, 62 |
| L side | 0..3 | `4 + i` | 8, 32, 64, 88 | 8, 22, 40, 54 |
| LO left-lane front | 0..3 | 12, 14, 2, 3 | 8, 8, 40, 88 | 22, 40, 54, 62 |
| R side | 0..3 | `8 + i` | 192, 160, 136, 120 | 8, 22, 40, 54 |
| RO right-lane front | 0..3 | 13, 15, 2, 3 | 192, 160, 136, 120 | 22, 40, 54, 62 |

   (a door, value 2, adds 10h to the image index.)  Image sizes: F 160x92, 96x56, 48x28, 16x10; L/R sides
   24x120, 32x94, 24x56, 16x28; outer pieces 24x92, 24x92, 56x56, 56x56.
3. queued sprites (`view_queue_sprite_a..e`, arrays `DGROUP:6014/6028/603C` = x/y/image; up to 20) from the
   `<style>T.16` (36 images) bank for the value-3 walls, drawn by `view_draw_sprite_list`.  `<style>B.16` holds 16x11
   pieces used for the same purpose (not traced further).

`python tools/mm2_view.py MAP X Y N|E|S|W out.png [town|cave|castle]` renders a view this way (walls, floor and sky
only, no sprites); the output shows plausible streets/corridors, which is how the layout above was checked.

## Outdoors (`draw_view_outdoors` `18D6C`)

The outdoor view is built from **terrain tiles**, not walls.  `sub_15F54` samples the same 3 lanes x 4 depths
(`59CA/59CE/59D2`, centre/left/right) and turns each map byte into a terrain class with
`sub_15F40` (`DGROUP:52B2[byte & 1Fh]`): 0 = nothing (empty), 1 = values 1-2, 2 = 3, 3 = 4, 4 = values 5-12 and 28;
the results go to `5FD8` (centre), `5FDC` (left), `5FE0` (right).  `sub_18CC6` then stores `class - 1` (FFh =
empty) in `54B0[4]` / `54B4[4]` / `54B8[4]`.  The class selects the image bank: classes 1-3 -> the three
`OUTDOOR1-3.16` banks (`word_1DBBE/BC2/BC6`), class 4 -> the terrain bank `word_1DBCA` loaded by `view_load_sky`
from the map attribute byte (`231DA & 0Fh`: 9 desert, 0Ah tundra?, 0Bh swamp?, 0Ch ocean; the names are in the file
list `DGROUP:04CE...`).  `OUTF.16` (floor, `word_1DBB2`) and `OUTB.16` (`word_1DBBA`) supply the ground and the
horizon; the sky is `SKY.16` (`word_1DBD2`), replaced by stars at night (`g_day_fraction >= 80h`).

Drawing order (reproduced by `render_outdoors` in `tools/mm2_view.py`; the output shows sky, cobbled ground and grey mountain
tiles as expected, but could not be compared with the real game):

1. `SKY.16` image 0 at (8, 8) (stars at night), `OUTF.16` image 0 (208x60 ground) at (8, 68).
2. **Horizon strips** (`sub_189B8`, terrain class 4 cells, depth `d` = 0..3, `y = 80h - DGROUP:159E[d]` = 108, 93, 78, 68, bank = the
   terrain bank, 20 images: 0-3 full width 208 px, 4-7 left half, 8-11 right half, 12-15 / 16-19 corner pieces 32/56/80/104 px):
   centre cell -> image `d` at x = 8; left cell -> `d + 4` at x = 8 (or `d + 12` when the centre is also class 4); right cell ->
   `d + 8` at x = 70h (or `d + 16` at x = `1596[d]` = 184, 160, 136, 112).  The drawn cells are then cleared so they are not drawn as tiles.
3. **Tiles** (classes 1-3 = `OUTDOOR1/2/3.16`, 8 images each: 0-3 front faces 160x92, 96x54, 64x35, 32x17, 4-7 side faces):
   the **nearest** centre tile `n` (first depth with a tile) is drawn once: image `15A6[n]` = 0, 0, 1, 2 at x = `15AA[n]` = 40, 40, 64, 88,
   y = `15B2[n]` = 21, 21, 42, 50.  Then for depth `d` = n (or 3 if none) down to 0 the left tile (image `15BA[d]` = 4, 5, 2, 3, x `15C2[d]` =
   8, 16, 32, 88, y `15CA[d]` = 36, 46, 50, 58) and the right tile (images `15D2[d]` = 6, 7, 2, 3, x `15D6[d]` = 176, 152, 136, 120, same y).
   A side tile behind the nearest centre tile (`d == n`, `d != 0`) is skipped when the side cell one step nearer is also a tile, otherwise
   it uses the alternate images 4, 5, 2 / 6, 7, 2 at the tables `15C0`, `15D4` with y from `15B2`.  At depth 1, and at depth 2 when `n == 2`, the
   left tile is moved to x = 8.  *(Two extra x overrides keyed on `byte_22D04/22D08` were not decoded.)*

`view_free_resources` (`1B0F6`), `view_load_style_graphics` (`1B288`) and `enter_map` (`1B5EA`) load and
release the style banks; `map_style_for_id` (`1B410`) chooses the style from the map number.

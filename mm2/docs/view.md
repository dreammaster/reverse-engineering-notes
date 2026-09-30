# First-person maze view (2PLAY)

Entry points: `draw_view_indoors` (`18744`, towns/dungeons/castles, `g_outdoors == 0`) and
`draw_view_outdoors` (`18D6C`); the dispatcher is resident `13FFC`.  Both draw into video page 1 through
the driver's "draw image" function (`gfx_draw_op13(bank, image, x, y)`), then `gfx_copy_page` shows it.

## Map data used

* Current 16x16 map: walls `DGROUP:59D6` (256 bytes) and flags `5AD6`; the four outdoor neighbours
  are staged at `5BD6/5CD6/5DD8/5ED8` so coordinates 0-15 wrap into the adjacent map
  (`view_sample_row` `1B44E`, `map_cell_at_offset` `1B7C8`).
* A wall byte holds 2 bits per side: N = bits 0-1, E = bits 2-3, S = 4-5, W = 6-7 (masks 03h, 0Ch,
  30h, C0h; `set_facing_masks` `1423E` stores the mask of the side the party faces in `byte_23216`
  and its shift in `byte_23217`).  Values per side: 0 open, 1 wall, 2 door, 3 special/secret.
  Wall-value translation table `DGROUP:52B2` (`15F40`) maps the low 5 bits of the flag/terrain byte to
  a wall style.

## Building the visible cell set (`view_prepare_visible_cells` `1B4E0`)

For the facing direction it reads six offsets from `DGROUP:16F8/16FE/1704/170A` (N/S/E/W) and samples
three rows of four cells ahead (`59CA[4]` centre lane, `59CE[4]` left lane, `59D2[4]` right lane),
plus the cell's flag byte (`byte_23218`).  `wall_collect` (`1BEBA`) then turns the 12 sampled bytes and the
facing masks into 20 wall variables `byte_27826..27839`: for each depth 1-4 a **front wall** and a
**left/right side wall**, and for each outer lane a wall seen edge-on.  Doors that would be hidden behind
a wall (`3` beside `1`) are normalised to `1`.

## Drawing order (`draw_view_indoors`)

1. background: sky/ceiling/floor pieces (image 0-3 of the style bank) via `gfx_draw_op13`;
2. for depth 4 down to 1 (far to near): side walls (`view_draw_wall_b`/`_c`) then the front wall
   (`view_draw_wall_a`); the second argument selects the left (`80h` + depth) or right variant;
3. queued sprites (`view_queue_sprite_*`, arrays `DGROUP:6014/6028/603C` = x/y/image; up to 20) such as
   doors' contents and monsters/objects in view, drawn by `view_draw_sprite_list`.

Wall images come from the style banks (`TOWN.16`, `CAVE.16`, `CASTLE.16`; see file-formats.md):
piece index = wall value - 1, +10h for the door variants; x/y positions per depth are in the tables at
`DGROUP:1516..15D6` (`sub_18558/185B4/1867C` add the perspective offsets) -- these tables are the
scaffolding of the perspective and are easiest read from the data in `mm2.idb`.

Outdoors (`draw_view_outdoors`): `sub_189B8` draws the sky/horizon strips per depth; `sub_18CC6` then
draws terrain tiles from `outdoor1-3.16`, `outb/outf.16` and the terrain-type banks (`desert/ocean/
swamp/tundra.16`) for the four depths using the per-cell terrain byte (`byte_54B0..54B8`).

`view_free_resources` (`1B0F6`), `view_load_style_graphics` (`1B288`) and `enter_map` (`1B5EA`) load and
release the style banks; `map_style_for_id` (`1B410`) chooses the style from the map number.

# Indoor view (work in progress)

`drawView` (`1B669`) -> `prepareIndoorView` (`1C195`) walks the visible cells and `renderIndoorView` (`1E407`) draws them.
Tables in DGROUP (named in `names/mm3.tsv`):

* `VIEW_DX` / `VIEW_DY` (`12CAh` / `1382h`): for each facing (N, E, S, W) 46 signed offsets (`0Eh` bytes per row of cells),
  the cells in view in drawing order: index 0/1 = the party's own cell, then the cells one step ahead (and to either side),
  two steps ahead, ... up to four steps ahead (offsets -4..+4 sideways). Facing 0 has dy >= 0, facing 1 dx <= 0 etc.
* `VIEW_MASK` / `VIEW_SHIFT` (`143Ch` / `159Ch`, 88 bytes per facing): for each view slot, the mask/shift that extracts the
  wall type of the relevant cell side from the 16-bit maze word (nibbles `7000h`, `700h`, `70h`, `7`).
* `mazeGetWordRel(dx, dy, mask)` (`1BC15`) reads the cell, `mazeGetFlagsRel` (`1BD67`) the flag byte.

Wall sprites themselves come from the `*.til`/`*.out`/`*.vga` resources; the draw tables of `renderIndoorView` are the next
thing to decode.

## Page slot layout and the automap

A loaded 16x16 page (slot stride `340h`, first slot at DGROUP `C554h`) is:

| offset | size | meaning |
|---|---|---|
| 000h | 200h | 256 cell words (`(y & 15) * 32 + (x & 15) * 2`): wall nibbles N,E,S,W |
| 200h | 100h | 256 cell flag bytes |
| 320h | 20h | 256-bit "visited" bitmap (bit index `(y & 15) * 16 + (x & 15)`), tested/set with `isBitSet`/`setBit` |

Accessors for coordinates relative to the current slot (they add the slot's origin from `2600h/2604h`, resolve neighbouring pages
with `mazeNeighbourSlot` and return `1111h` or 0 outside the 32x32 world): `markCellVisited` (`1B9C5`), `isCellVisited` (`1BA82`),
`mazeGetWordWrap` (`1BB33`). After every step `updateAutomap` (`15736`) marks the party's cell visited when the party has the
Cartographer skill (`checkSkill(4)`), and when Wizard Eye is active (`Party_wizardEye`) draws the overhead map.

Slot offsets for facing 0 (north), `(dx,dy)` (other facings are rotations): 0,1 (0,0) own cell; 2 (1,0); 3 (-1,1); 4,5,6 (0,1) the cell directly ahead (several slots refer to the same cell because the renderer draws several wall faces of it; Jump's wall test uses slot 5); 7 (1,1); 8 (-2,2); 9,10 (-1,2); 11,12,13 (0,2); 14,15 (1,2); 16 (2,2); 17-33 the row three ahead (x -4..4) and 34-43 the row four ahead (x -4..4); the last two entries are fillers.

## Scene description (inferred from `renderIndoorView`, `sub_17439`)

`renderIndoorView` fills a scene block at DGROUP `D66Eh` (the same 4000-byte scratch area that `sprintf` also uses): a 26-byte header of 13 words (`word_35D5E`..`word_35D78`: current sprite sets, view mode, light state, ...)
followed from `D688h` by a list of draw records.  Each record starts with `FFFFh`, then a far pointer to the sprite set (taken from the 4-byte pointer tables at DGROUP `-3AC0h`/`-3B5Ah` style addresses: one per wall type / monster picture),
then words for x, y (e.g. `67h`/`36h` for the monster slot in the centre of the screen), a flag word (`300h | animation state`) and a frame number.
The monsters seen in the first rows (`byte_33311..` and the row bytes `34B92..`) are first collected into 12-byte group records at `A792h` (picture slot, animation phase, picture type -> frame count table `2815h`, flags);
`sub_17439` turns each into one draw record, using `Maze_monAnim`-style phases that `renderIndoorView` advances every frame (modulo the frame counts).  The list is handed to the video module by the code following (not yet followed).
Animated overlays (torch, water) use the small phase counters `word_28810`/`28812`/`byte_2884D` that `renderIndoorView` increments modulo 18/18/3 each frame.

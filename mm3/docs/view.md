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

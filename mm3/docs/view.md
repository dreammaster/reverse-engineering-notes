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

## The draw list and who draws it (decoded from the video module)

**The scene is drawn by the text printer.**  The indoor/outdoor renderers and `drawParty` only build a *draw list* in the 4000-byte scratch buffer at DGROUP `D66Eh`, then call `vdrv_2D_printText` on a short string that contains the text control code `05h` followed by the list's near address written as four hex digits (`sprintf` with `%p`; the strings are `"k%p"` for the party HUD list and similar).  The text engine (`sub_615B6` in the video module, `vdrv` offset 2Dh) dispatches control codes 0-0Dh; code 05h reads the four hex digits, stores the far pointer (string segment : offset) in `dword_60B34` and calls the list interpreter `sub_61BB8`.  (`vdrv_27` is not a screen draw at all: it installs the **mouse cursor** sprite and range, `vdrv_2A` is a particle/starfield transition effect, `vdrv_30` is the module initialiser.)

Control codes of the text engine (character < 20h; `` takes one letter, the numeric ones fixed-width decimal digits):

| code | meaning |
|---|---|
| 01h / 02h | normal / alternate font flag (`byte_29262` = 0 / 80h) |
| 03h + letter | `l` left, `c` centre, `r` right alignment; `t`/`f` (flag `byte_60A15` 0 / 80h); `m` + digit (colour mode); `q`, `k`, `b`, `d` set the "absolute coordinates" flags (`k` is used for HUD lists: do not add the window origin) |
| 04h + 3 digits | draw a filled bar 9 pixels high of that width in the current colour (hit-point/spell-point gauge) |
| 05h + 4 hex digits | **draw list** at that address (see below) |
| 06h | print a space glyph |
| 07h + 3 digits | set the text colour attribute (`word_290FD+1`) |
| 08h + char | print a character with a narrower advance (overlay glyph) |
| 09h + 3 digits | column (x inside the window) |
| 0Ah | tab-like command (`sub_61A1C`) |
| 0Bh + 3 digits | row (y inside the window) |
| 0Ch + 2 digits | text colour index |
| 0Dh | new line |

**Draw list format** (interpreted by `sub_61BB8`, word-granular, 16-bit little endian):

* `FFFFh, offset, segment` -- select the sprite set for the following records: a far pointer to a sprite resource (as returned by `vdrv_21_loadSprites`); the offset word is ignored by the interpreter, only the **segment** is used; **segment 0 ends the list** (`FFFF 0000 0000` is the terminator written by `drawParty`).
* otherwise a 4-word record `x, y, flags, frame` (8 bytes): draw frame `frame` of the current sprite set at (x, y), where x and y are relative to the current window origin unless one of the absolute-coordinate flags `k`/`b`/`d` is set.  A frame has up to two layers (two line-coded images; see `mm3_gfx.py`), both drawn.
* `flags` (`sub_61D70`): bit 15 set = enlarged draw (x2, or x3 when bit 14 is also set); otherwise bits 0-1: 0 normal, 1 horizontally mirrored, 2 a second blit variant (`sub_62180`); bits 8-9 (`100h`/`200h`/`300h`) select a **distance scale** 1-3 from a small table at module offset `0B6Ah` (size reduction for objects one, two, three rows away) and are combined with bit 0 for mirrored scaled sprites.  Monster records are written with `300h | group flags`.

`renderIndoorView`'s record writers produce, in drawing order (back to front): far wall faces, side walls, object sprites, monsters (via `sub_17439`: x/y of the 12-byte monster group slot, flags `300h | slot flags`, frame number from the group's animation phase), and finally the HUD pieces (`sub_1B6D1`).


**Wall faces** (`sub_1DB3D`, 1,200 lines, call-free): the first record selects the current environment's wall sprite set (far pointer `word_34B9E/34BA0`); then, for each visible face, a hard-coded record is emitted when its per-face flag byte (`byte_332E0`, `byte_37388`, `byte_332DA`, `byte_332ED` ... -- hundreds of them, one per face position and type, zeroed by `clearViewFlags` and set by `prepareIndoorView` from the wall nibbles of the cells in `VIEW_DX/DY` order) is non-zero, e.g. `(-40, 40, 2, 6)`: x = -40 (`FFD8h`), y = 40, flags 2, frame 6; `(-40, 40, 2 | byte_28875, 10)` for the animated variant.  Doors, switches and other wall styles select other frames of the same sheet.  The records are therefore a flat, fully unrolled table from (face flag) to (x, y, flags, frame); what remains undecoded is the per-flag correspondence list itself (which cell/side/style sets which flag byte, which frame).
**HUD status icons**: after the scene, `renderIndoorView` appends a row of 8-pixel-high icon records at y = 60 (x = 8, 20h, 38h, 50h, 68h, 78h, 83h, 90h, 98h, B0h, C8h; frames 9-28) for the active party effects (light, protection, levitation, ... each gated by one of the `byte_3xxxx` effect flags that `clearViewFlags` resets and the spell code sets), always in two variants per slot (frames 0Dh/1Ch, 0Bh/1Ah, 9/18h ...).

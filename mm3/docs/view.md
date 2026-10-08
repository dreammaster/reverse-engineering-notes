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
**Far wall faces and HUD pieces**: the rows of 8-pixel-high records at y = 60 (x = 8, 20h, 38h, 50h, 68h, 78h, 83h, 90h, 98h, B0h, C8h) that `renderIndoorView` appends before the object writers are **not status icons** (an earlier note said so): they are the wall faces of the farthest row, drawn from the small `wl4` sheet (24x13 fronts, 16x29 sides, frames up to 13+), with the same style -> flag -> frame scheme (flag bytes such as `byte_34B88`/`byte_33452` are set by the style 1-6 / style 7 cases of the corresponding `prepareIndoorView` block).  The actual HUD pieces are written by `sub_1B6D1` (`1B6D1`): the compass (`checkSkill(6)` Direction Sense; frame `Party_facing + 0Fh`, otherwise frame 0Eh, at x = 110, y = 131) and a secret-door indicator (`checkSkill(10h)` Spot Secret Doors, at x = 197, y = 93).

### Wall styles and face flags (decoded with `tools/mm3_viewflags.py` and `tools/mm3_scenelist.py`)

`prepareIndoorView` is 44 unrolled blocks, one per view slot (the order of `VIEW_DX/VIEW_DY`; the blocks with only style 7 are the extra side faces of a slot).  Each block reads the wall nibble of its cell side and jumps through a 7-way table on the **wall style 1-7** (low three bits of the nibble) that increments flag bytes.  For the visible face blocks the pattern is the same everywhere:

| style | flags set (example: face block 5 = the left-hand near face) |
|---|---|
| 1 | the face's base flag (`byte_332AE`): a plain wall |
| 2 | base + variant A (`byte_332E0`): the closed door, frame 6 |
| 3 | base + variant B (`byte_332FD`): the lamp wall, frame `1 + byte_2884D` |
| 4 | overlay D only (`byte_332E5`) |
| 5 | overlay E only (`byte_332DA`) |
| 6 | overlay F only (`byte_332ED`) |
| 7 | the special flag (`byte_37388`) -- the animated wall/door |

and `sub_1DB3D` turns the flags into records, e.g. for that face (sprite sheet `word_34B9E/34BA0`, x = -40, y = 40, flags 2 = blit variant): base -> frame 0, variant A -> frame 6, overlay D -> frame 8, E -> frame 9, F -> frame 7, special -> frame 10 (with the animation phase `byte_28875` ORed into the flags, and `byte_2884D` selecting alternating frames).  The mirrored right-hand face uses x = 168 and flags 3, the centre face x = 64 and flags 0, further rows other (x, y) pairs and the distance-scale flag bits; the other wall faces of the deeper rows follow the same style -> flag -> frame scheme.  The meaning of the styles (verified against the artwork in the next subsection): 1 plain wall, 2 closed door, 3 wall with a lit lamp, 4 gate, 5 broken wall, 6 open door, 7 archway.
`python tools/mm3_viewflags.py mm3.asm` prints all 44 blocks and `python tools/mm3_scenelist.py mm3.asm sub_1DB3D sub_17F38 sub_1862A sub_18BF1` the emitted records.

## Environment graphics (`sub_430A8` = load map graphics, called when a map is loaded)

`sub_430A8(map - 1)` stores the map number in `Party_map` / `byte_34C1D`, frees the old graphics and loads the new ones.
**Indoor maps (maps 1-40):** an environment number 0-4 comes from the byte table `ENV` at DGROUP `30E2h` indexed by `map - 1`:
`0` town (maps 1-5), `1` cave (6-15), `2` dungeon (16-23 and 29-33), `3` castle (24-28), `4` sci-fi (34-40).
The tile set is loaded by name from `5A3Eh[env]` (`town.til`, `cave.til`, `dung.til`, `castle.til`, and `scifi.til` for environment 4 from the next table), into `word_3407C:3407A`, and four wall sheets are loaded with `sprintf("%swl%u.vga", prefix, n)` where `prefix` is `5A34h[env]` (`twn`, `cav`, `dun`, `cas`, `sci`) and n = 1, 2, 4, 3 (bytes at DGROUP `310Ah`), i.e. `twnwl1.vga`, `twnwl2.vga`, `twnwl4.vga`, `twnwl3.vga`, `cavwl1.vga` ... `sciwl3.vga`; their far pointers go into the per-wall-group pointer table at `-3B5Ah` (indexed by n, 4 bytes each) used by the renderer's `FFFF` sprite-set records.
**Outdoor maps (map 41 and above, `Maze_wrapMode`):** the first seven bytes of the current page's header (`.DAT` `+300h`..`+306h`) are indices into the name table at DGROUP `5A46h`: `0` scifi.til, `1` mount, `2` ltree, `3` dtree, `4` higrass, `5` snotree, `6` snomtn, `7` swmtree, `8` mount, `9` lavamtn, `10` palms, `11` mount, `12` grass, `13` dirt, `14` snow, `15` swamp, `16` lava, `17` desert, `18` road; each non-zero entry loads `<name>.vga` into the seven slots at `-3B56h`; `water.vga` (the water surface) is always loaded, and `day.vga` follows in the table.  This answers the earlier question about page header bytes 00-06: they are the **terrain sprite sheet selectors** of the page (compared between neighbouring pages so only changed sheets are reloaded).
(Resource names recovered from the executable's strings; the `.vga`/`.til` members of `MM3.CC` are identified by the filename hash, see `mm3_cc.py`.)

### Wall sheets verified against the artwork

The 20 wall sheets exist in `MM3.CC` under the hashed names `twn|cav|dun|cas|sci` + `wl1/2/4/3.vga` and decode with `mm3_gfx.py`.  Each sheet holds 13 frames for one **viewing distance**: `wl1` = the adjacent row (front faces 168x85, side faces 24x109), `wl2` = 104x53 / 32x85, `wl3` = 56x29 / 24x52, `wl4` = 24x13 / 16x29, so the renderer's FFFF records switch sheet when going deeper (`wl1` first, then `wl2`, `wl4`...).
Frames of the town sheet (looked at as PNG): 0 plain wall, 1-3 plain wall with a lit lamp (three flicker variants, chosen by the `byte_2884D` counter that counts 0-2), 4 and 5 left and right side walls, 6 closed double door, 7 open door (doorway with red carpet), 8 gate / portcullis, 9 broken wall (rough hole), 10 empty archway (pillars only), 11 and 12 side-face decorations.  With the flag table above: wall style 1 = plain wall (frame 0), style 2 = wall + closed door (6), style 3 = the lamp wall (flag `byte_332FD` emits frame `1 + byte_2884D`, i.e. frames 1-3 cycling), style 4 = gate (8), style 5 = broken wall (9), style 6 = open door (7), style 7 = archway (10, with the animation variant when `byte_28875` is set).  The cave, dungeon, castle and sci-fi sheets keep the same frame numbering with different artwork.

### The four sprite writers: objects, monsters and effects by row

After the wall faces, one writer per viewing distance appends the movable things; the distance is encoded in the scale bits of the record flags (`tools/mm3_scenelist.py` lists them):
| routine | row | flag bits | contents |
|---|---|---|---|
| `sub_18BF1` | 1 cell ahead (the adjacent row) | 0 (full size) | objects (`word`-indexed far pointers at `-3AC0h`, slot pictures loaded by `Map_load`, e.g. at x = 47, y = 8), monsters (far pointers at `-58B0h`, one per monster group slot), spell effect sprites (`word_373CC/373CE`) |
| `sub_1862A` | 2 cells ahead | `100h` (scale 1) | objects at x = 70 (centre), -22 (left, mirrored `|2`), 162 (right), y = 23; monsters; effects |
| `sub_17F38` | 3 cells ahead | `200h` (scale 2) | objects at x = 88, 8, 168 and y = 41; monsters; effects (`512`/`514` = `200h`/`200h|2`) |
| `sub_17439` | 4 cells ahead (the farthest row) | `300h` (scale 3) | the same kinds of records at the smallest scale |
Each kind of record is preceded by an `FFFF` sprite-set record naming where the pictures come from (object pictures: the far-pointer table at `-3AC0h`; monster pictures: the table at `-58B0h`; effect sprites: `word_373CE:373CC`), and each is gated by one of the per-position flag bytes that `scanMonstersAhead`/`prepareIndoorView` set.  Mirroring uses flag bit 1 (`|2`) for the left-hand positions.

### Corrections from tracing the pointers

* The `*.til` tile sets (`town.til`, `cave.til`, `dung.til`, `castle.til`, `scifi.til`) loaded into `word_3407C:3407A` are used only by the **overhead map** (`updateAutomap`, `showOverheadMap`), not by the 3D view; they are the map tile pictures, selected per environment.
* The far-pointer table at `-3B5Ah` (DGROUP `C4A6h`) holds the four wall sheets by their sheet number n (4 bytes each at `C4A6h + 4n`): `word_34B9A/34B9C` = `wl1`, `word_34B9E/34BA0` = `wl2` (the sheet named by the first record of `sub_1DB3D`), then `wl3` and `wl4`.  The wall faces of the deeper rows therefore reference the smaller sheets.
* No routine of the 3D renderer draws a floor or ceiling: the background of the view window (sky, floor) presumably comes from a screen picture drawn earlier (not located), with the wall/object records drawn over it.

### The view background (`sub_43034`, called by `loadMapGraphics`; sets `word_373B6:373B4`)

The picture behind the walls (ceiling and floor) is a sprite set loaded for the map and passed in the scene header (`word_35D62:35D60` in `renderIndoorView`):
* on maps with a day/night cycle (`mapHasDayNight`: towns and open-air maps) it is `day.vga` or `night.vga` (entries 19/20 of the name table at `5A46h`, picked by `Town_closed`, i.e. by the time of day: before 05:00 or from 21:00 night), and `sub_43034` also (re)computes `Town_closed` itself;
* on all other maps it is `sprintf("%s.sky", prefix)` with the environment prefix `5A34h[ENV[map]]` -- `twn.sky`, `cav.sky`, `dun.sky`, `cas.sky`, `sci.sky` (byte table `30E1h`, the same environment numbers as above).
This replaces the earlier guess that the background comes from a screen picture drawn before the view: it is a header field of the scene block (`word_35D62:35D60`), not a list record.  (The *.sky and day/night names are recovered from the executable's strings; their files can be exported with `mm3_gfx.py` using the name hash.)
Check against the archive: `cav.sky`, `dun.sky` and `sci.sky` exist in `MM3.CC`, while `twn.sky` and `cas.sky` do not -- exactly the town and castle environments, which are covered by the day/night maps (`mapHasDayNight`: maps below 6 and 24-28) and use `day.vga`/`night.vga` instead; `day.vga`, `night.vga` and `water.vga` exist too.
Frame counts and extras of the distance sheets (decoded): `wl1` 13 frames, `wl2` 13, `wl3` 15 (frame 14 = an extra 24x31 piece) and `wl4` **31** frames: two groups of 15 small pieces (frames 0-14 and 15-28, the second group being the variants for the other half of the far row, e.g. 24x13 fronts, 16x29 sides, 8x13 and 48x29 corner pieces) followed by frame 29 (216x71) and frame 30 (138x11), full-width strips of the far horizon / end of the corridor.

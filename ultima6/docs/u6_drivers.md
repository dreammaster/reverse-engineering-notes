# Ultima VI drivers

The `*.DRV` files are LZW-compressed (u32 size, then the stream; `tools/lzw_extract.py` unpacks them) raw 16-bit real-mode code
blobs. Each starts with a table of 3-byte `jmp near` entries; the game calls entry *n* through a far pointer whose offset it sets to
`3 * n` (`video_driver_call` in `GAME.EXE`, `dword_293C4` in `U.EXE`) and gets the driver's `retf N` convention back (cdecl, callee
pops). `ida_scripts/create_drivers.ps1` makes one database per driver in `u6/` (`u6_drv_mcga`, `_ega`, `_cga`, `_tandy`, `u6_snd_adlib`,
`_cms`, `_covox`, `_innova`, `_roland`, `_tmus`), `setup_driver.py` switches IDA's binary loader to 16 bits and seeds the analysis with the
entries, `apply_renames_u6_drivers.py` names them.

## Graphics drivers (27 entries each, same numbering in all four)

Entries used by `GAME.EXE` (call counts in parentheses): 0 init (1), 1 (5), 2 (20), 3 (6), 4 (1), 5 (1), 6 (15), 7 (1), 8 (4), 9 (1),
10 (2), 11 (5), 12 (7), 13 (3), **14 (37), 15 (47)**, 16 (1), 21 (1), 25 (5), 26 (1). Read from the MCGA driver:

| Entry | Name | Behaviour |
|---|---|---|
| 0 | `video_init` | saves the BIOS mode, sets mode 13h (320 x 200, 256 colours), returns the data block `cs:0052` |
| 1 | `video_shutdown` | restores the saved mode |
| 2 | `set_color` | the colour for the fill and text routines |
| 6 | `fill_rect` | (x1, y1, x2, y2), corners sorted |
| 7 | `copy_rect` | screen to screen |
| 8 | `draw_bitmap` | (y, x, far ptr): `u16 width, u16 height`, then rows |
| 10 | `draw_char` | (y, x, char): 8 x 8 glyph from the font |
| 14 | (overloaded) | sets the tile cache up when its first argument is >= 0x3000 (reads the tile position table and the kind table through host callbacks), otherwise a drawing variant |
| 15 | `draw_tile` | (y, x, tile): `kind & 3` from the table at `cs:1088` — 0 copies 16 rows of 8 words, 1 copies skipping colour 255, 2 decodes pixel blocks |
| 4, 5, 16 | shape drawing | read the shape header from a size-headed archive (`u16 a, hot x, hot y, d` with width `a + hot x + 1`, height `hot y + d + 1`), clip against a rectangle and draw |

Row start addresses come from a table at `cs:2008` (y * 320); the driver keeps an offscreen buffer pointer at `cs:6E` and a flag at
`cs:61` that redirects drawing to it. Tiles are fetched on demand into a cache (`sub_2249`) through host-supplied read callbacks
(`cs:62`, `7A`, `7E`, `82`), so the driver, not the game, reads `MAPTILES.VGA`/`OBJTILES.VGA` (which is why `GAME.EXE` has no tile file names).
The pixel-block and shape findings are folded into `u6_graphics.md` and the C++ in `src/files/`.

## Sound drivers (4 entries; Roland 5)

`U6ADLIB`, `U6CMS`, `U6COVOX`, `U6INNOVA`, `U6ROLAND`, `U6TMUS` (Tandy music) share one interface: 0 `snd_init(port)` (AdLib: base port 0x388),
1 `snd_silence`, 2 `snd_play(far ptr to music data)` (saves INT 08h, reprograms the PIT and plays from the timer interrupt), 3 `snd_stop`; the Roland
driver adds 4, a per-channel set-up. The timer interrupt handler (`snd_timer_isr`, at `3A0` in the AdLib driver) plays the track; the music format is in [u6_music.md](u6_music.md).

## Martian Dreams: `TM.DRV` and `SUPERTM.DRV`

Martian Dreams (MARTIAN.EXE, `sub_200F0` and `sub_2024B`) loads two video drivers of another kind than Ultima VI's. Each file is a LibN with a size header whose items are LZW blocks, one per display
(item 0 = VGA, word_29658 = 0; 1 EGA; 3 Tandy or CGA; 2 is the same as 4's item). The unpacked block (`tools/extract_md_drivers.py`, the databases `md/md_drv_tm_vga` and `md/md_drv_supertm_vga`,
`ida_scripts/create_md_drivers.ps1`) begins with a table of near entry offsets: word 0 is a flag (`cs:0`) the entries set while they run, word *n* is the entry *n*; the game copies them into its far
pointers (`TM`: words 1-21 become word_287A4 ... dword_28172 and word_2818A, with the game's resource locator `sub_222E8` stored at 0x2E; `SUPERTM`: dword_2817A, word_2817E, dword_28182, word_28186,
word_2818E, the locator at 0xC). A picture is a handle the entry gives to that locator (far call at `cs:0xC`/`cs:0x2E`, answers `dx:ax`): memory that holds the archive of a *.LZC item behind its 4-byte
size, so the entry *k* of its table (at 4 + 4k) is the shape's offset from the start. The screen argument is a 12-byte structure: segment of the pixels, the offset of a table of row offsets, and the
clipping rectangle left, top, right, bottom (`sub_1F26A` sets 0, 0, 319, 199: the whole screen; the window of the scenes is only the 128-line height of the pictures).

`TM` entries used by the scenes: dword_2813A (offset 0xE86 of the VGA block) draw a shape, dword_2814E clear, dword_2814A present, dword_28172 (0x441F) fill a circle (colour, ry, rx, y, x, screen).

`SUPERTM` dword_2817A (offset 0x15E9) draws a shape turned and scaled about its hot spot: the arguments pushed in order are flags (0 or the mirror bits 0x10/0x20), size (256ths: 256 is as it is, the
code keeps `size` below 256 in a decimating loop, above in a replicating one), angle (degrees, -359 to 359, negative added to 360; the sine and cosine are 8-bit tables at 0x10 and 0x6A: the quarter
turn is 255 * sin, the sign and the quarter in the tables at 0x1D2 and 0x33A), frame, the picture's segment and offset, y, x and the screen. The dispatch (`cs:4AC`, `4EC` for the box only) chooses
one of 32 inner loops by quarter-turn code (exactly 0, below 90, exactly 90, below 180), size above 256, mirror flags and the second half-turn; a size of 256 at angle 0 with no flags jumps to the plain
draw at 0x71BE. A shape is a row list (`u16 count` of the bytes in the run, 0 ends; x and y from the hot spot; the run's pixels as plain bytes or as (length << 1 | 1, value) fills inside) and the
loops walk its rows in step with 8-bit accumulators. The game scenes (the photograph on the book, the ship, the cabin that turns, the Mars that grows) use it.

`src/engine/cpu8086.cpp` runs the entry (see approximation 92); `tests/test_md_driver.cpp` compares it with the checksums of the same calls on the Unicorn emulator.

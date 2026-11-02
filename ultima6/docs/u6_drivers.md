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
driver adds 4, a per-channel set-up. The music files (`ULTIMA.M`, `BOOTUP.M`, ..., `MIDI.DAT`) and their format are not analysed yet.

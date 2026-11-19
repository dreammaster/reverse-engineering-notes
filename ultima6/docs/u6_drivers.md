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

## Martian Dreams: the sound driver `STRAX.DRV` and the music

`STRAX.DRV` ("Origin Sound System!") is a LibN with a size header like TM and SUPERTM: item 0 (1192 bytes) is the MPU-401 (Roland) block, item 1 (3838 bytes) the AdLib block, item 2 the third card's. MARTIAN.EXE
chooses by the letters of CONFIG.MD (`sub_156A6`: A = AdLib, `word_28BE6` = 2 and the music flag `word_2B4CA` = 1; R = Roland, 1; N = none) and `sub_21956` loads the block of item `word_28BE6 - 1` and the timbres of `MD.TIM`
(item `word_28BE6 - 1`, 5713 bytes for the AdLib: a byte 0x77 and the OPL patches). The block begins with the table of ten entry offsets (words 1-10: init, load the timbres, reset, timer interrupt, note
(velocity, note, channel), controller (value, controller, channel), pitch bend (second, first, channel), program (program, channel), set the card's port, one more); the game copies them to its far pointers
(`dword_2965E`, `dword_29792`, `dword_2B490`, `word_2B49A`, `dword_2968A`, `dword_2B4A8`, `dword_2B4AC`, `dword_29606`, `word_2B49E`, `word_2961C`); the callee takes its arguments off the stack. The
timer interrupt (offset 0xD49 of the AdLib block, hooked on INT 8 at 60 a second by `sub_21ABC` through `sub_1E2BD(0x3C)`) slides the pitches and ends by calling the vector it replaced (`dword_16`, which the
game's installer `sub_1E226` fills). The driver reads the timer chip's counter (port 0x40) to wait between two writes to the chip (ports 0x388 and 0x389). The channel is the MIDI channel + 1; the drums
are voices 10-26, each with a program of its own (`sub_21ABC` sets them: 0x80, 0x72, 0x83, ...).

The songs are Standard MIDI files (`MDD_MUS.LZC`, 12 for the AdLib, started by `sub_21EB8`; `MUSIC.LZC`, 16, belongs to the other card and is not named by MARTIAN.EXE) read by a sequencer in MARTIAN.EXE's
segment 48: `sub_21CB2` parses the header ("MThd", format 0-2, up to 32 tracks, the division), `sub_21858` starts the tracks, `sub_21038` runs from the timer interrupt (`seg048:12D7`) and plays the events that are
due (`sub_20F94` takes the track whose next event is first and adds the time to a 16.16 count of timer ticks: a tempo of T microseconds and division D makes one MIDI tick T / D * 0x1317 / 1243 of them), channel
events go to the driver after the channel is set up on its first use (`ds:1B26-1B6C`), notes of channel 10 are sent to the drum voices through `ds:1B81` and `ds:1BCE`, and the event 0xFE is Origin's own
control (type 0 and 2: jump back by the 16-bit offset in its data if the game has enabled looping; 1: end; 3: store a marker; 4 and 5: jump back unless or if the byte equals `word_28BEC`). `sub_218FC(n)`
stops the song, over n ticks if n is not 0 (the volume falls by 0x8000 / n a tick). C++: `src/ultima6/md_adlib.cpp` runs the driver on `Cpu8086` and `src/ultima6/md_music.cpp` is the sequencer
(the songs' lengths agree with their tempo maps to a tenth of a second, `tests/test_md_music.cpp`); `tools/md_strax_gt.py` is the Unicorn check of the driver.

GAME.EXE carries the same sequencer (`sub_37CE2` is `sub_21038` with other data addresses) and plays the 16 songs of `MUSIC.LZC` (read by `sub_2FB2D` into the handles at ds:EA90): every turn of the main loop `sub_2FD45` asks
`sub_2FD00` which one goes with the place (level 2 or 3: song 3; any other level above the surface: 2; combat, `globals[0x34]`: 8; else the day, hours 6-17: 0, the night: 1) and `sub_2FC44` starts it (volume
from ds:189E: 215, 215, 225, 225, 230, then 165), fades the old one out over 60 ticks when the song changes, and starts the same song again when it has ended; `sub_2FD5E` plays the story scenes' songs.

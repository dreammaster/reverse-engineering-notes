# Video / timer drivers (`*.DRV`)

Files: `CGA.DRV`, `EGA.DRV`, `TGA.DRV` (Tandy), `HGA.DRV` (Hercules), `MCGA.DRV`, `TIMER.DRV`.  The
resident library loads the driver chosen by `detect_hardware` with `load_video_driver` (`108F2`; names
at `DGROUP:49B9..49DA`, selection table `49E4[word_221BA]`: 0 CGA, 1 EGA, 2 Tandy, 3 Hercules, 4 MCGA)
into a paragraph-aligned block and calls it as a far code segment.

## Layout

* Offset 0: a table of 36 near `jmp`s (3 bytes each): driver function *n* is at offset `3*n`.
* Offset `72h`: table of 200 row start offsets in video memory (EGA: `+28h` per row); the video segment
  is kept in the driver at offset `328h` (`A000` for EGA); other tables/working variables follow.
  The EGA driver programs the sequencer/graphics controller ports directly (`3C4/3CE`).

## Calling convention (`driver_call` `11CDA`)

`driver_call(fn, args...)` looks up a descriptor in `DGROUP:4AED[fn]` (list of arg kinds, 0-terminated, then
a return-kind byte), copies the C stack arguments **in order into AX, BX, CX, DX, SI, DI** (dword
arguments take two registers: offset then segment) and does a far call; the result comes back in AX
(or DX:AX for kind 2).

| Fn | Args (1 = word, 2 = far ptr) | Ret | Resident wrapper / meaning |
|---:|---|---|---|
| 00 | - | - | `gfx_reset`: initialise the adapter |
| 01 | 1 | - | `gfx_select_page` (0/1: draw/display page) |
| 02 | 1 | - | `gfx_set_color` |
| 03 | 1,1 | - | `gfx_plot_point` / move pen |
| 04 | 1,1,1 | - | `gfx_hline` (also used by `gfx_fill_rect`) |
| 05 | 1,1,1 | - | `gfx_vline` |
| 06 | 1,1,1,1 | - | `gfx_line` |
| 07 | 1,1 | word | get pixel |
| 08 | 1x6 | - | `gfx_copy_rect_pages` (rect between pages) |
| 09 | 1,1 | - | `gfx_copy_page` (from page, to page) |
| 0A | 1,1 | - | page-related (`1145E`) |
| 0B | 1x4 | dword | save rectangle -> far pointer (`gfx_window_save_bg`) |
| 0C | 2 | - | restore rectangle |
| 0D / 0E | 1x5 | - | scroll rectangle (`gfx_scroll_rect`) |
| 0F, 10h, 11h | 2,1 | dword | image/resource loaders (`gfx_load_image` uses 10h) |
| 12h | 1,1,1 | - | `114FE`-family draw op |
| 13h | 2,1,1,1 | - | **draw image**: bank far ptr, image index, x, y (mask honoured); see file-formats.md |
| 14h | 2,1,1,1 | - | draw image (alternate mode) |
| 15h | 1 | - | put character (`text_putc` -> font from the window's font pointer) |
| 16h | 2,1,1,1 | - | monster picture set-up: bank ptr, out animation table / length pointers; expands the pieces (see monster format) |
| 17h | 1,1,1 | - | draw monster frame (frame, x, y); frame FFFFh releases the buffers |
| 18h-1Fh, 22h | - | - | no-argument stubs in the EGA driver (jump to `1162`, i.e. `ret`) |
| 20h, 21h, 23h | 2,1 / 1x4 / 2 | - | seen in the table, not called from the resident code |

`TIMER.DRV` installs the timer interrupt handler that decrements `word_22261` (used by `delay_ticks` and
`pc_speaker_tone`); it is loaded by `load_timer_driver` (`11E12`) and freed by `unload_timer_driver`.

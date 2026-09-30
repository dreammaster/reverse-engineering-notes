# The resident game library (mm2.idb)

Most of the 32 KB resident image is a small engine that every overlay calls through thunks
(`thk_*`).  Names below are the ones in `names/mm2.tsv`.

## Layers

1. **MS-C runtime** — `10000–10A00`: `main`, `_cinit`, `exit`, `_aFuldiv`/`_aFulmul` (32-bit helpers),
   `_setargv`.  `main(argc, argv)` reads `argv[1][0]` (video mode letter, default space) and runs
   `1MENU1:init`, `1MENU1:title`, `1MENU2:main options`, `2PLAY:game_main_loop`.
2. **Hardware + video driver** — `detect_hardware` (`1073A`) probes the machine type (BIOS model byte
   via int 15h/C0h and `F000:FFFE`) and adapter (MDA/Hercules/CGA/EGA/VGA/MCGA/Tandy) and picks one of
   `TCGA/EGA/TGA/HGA/MCGA.DRV` (`load_video_driver` `108F2`).  The driver is a code blob whose entry
   table is `fn*3`; `driver_call` (`11CDA`) marshals the arguments into registers
   (`ax,bx,cx,dx,si,di` at `word_22278..`) using a per-function descriptor list at `DGROUP:4A28`.
   Known driver functions (by caller): 0 init, 1 select display page (0/1), 2 set colour, 3 plot,
   4 horizontal line, 5 vertical line, 6 line, 7 get pixel, 8/9 copy rect/page, 0Bh/0Ch save/restore
   rect, 0Dh/0Eh scroll, 10h load image, 13h draw sprite/tile (`gfx_draw_op13`, the workhorse used
   for the 3D view), 15h put character, 16h/17h monster animation.
3. **Window layer** — two tables of 20-byte slots (3 each): *graphics windows* (`DGROUP:9FF6`,
   `gfx_window_*`, pixel rectangles) and *text windows* (`DGROUP:A032`, `text_window_*`, character
   cells).  Current ones are `g_cur_gfx_win`/`g_cur_text_win`.
   Text window: `+0..3` x1,y1,x2,y2 (cells); `+4,+5` cursor; `+6` fg; `+7` bg; `+8` bit 7 = don't
   clear, `&7F == 1` = save/restore the background; `+0A` flags (bits 0-1 alignment 0/1/2 =
   left/right/centre); `+0B` z-order; `+0C..0F` font far pointer; `+10..13` saved background.
   `text_putc`, `text_puts` (word wrap), `text_put_number`, `text_goto_xy`, `draw_frame_*` build
   the UI; the 8x8 font comes from `MM2.CH`.
4. **Files** — `load_file_alloc`, `load_lzw_file`, `lzw_decompress`, `read_file_to_buffer`,
   `write_file_from_buffer`; disk-swap prompts use `g_disk_needed` and a critical-error handler.
5. **Input/time** — `kbd_poll` (non-blocking, returns ASCII or `F0..F3` for arrows, F-keys etc.),
   `wait_key`, `get_key_in_range`, `read_string`, `read_number`, `delay_ticks` (timer driver),
   `rand_range`, `play_music_step` / `play_sound_effect` (PC-speaker sequences).
6. **Game state helpers** — party (`g_party_ids`, `party_contains/remove`, `char_ptr`,
   `party_count_able`), character sheet/party roster screens, spell list, gold/gem/food payment,
   `advance_time`, movement (`party_step_forward/backward`, turning), `check_move_blocked`,
   `start_combat`, monster stat decoding.

## Game state globals (DGROUP, names in the IDBs)

| Address | Name | |
|---|---|---|
| `1DBE2` | `g_map_id` | current map |
| `1DBE3/1DBE4` | `g_party_x/y` | 0..15 |
| `1DBED` | `g_outdoors` | 0 = EVENTSI, 1 = EVENTSO |
| `1DC1F` | `g_facing` | ASCII `N`/`E`/`S`/`W` |
| `1DC1E` | `g_view_mode` | bottom panel layout 0/1/2 |
| `1DC66` | `g_party_ids` | 8 words |
| `1DC76` | `g_party_size` | |
| `1DC1A` | `g_era` | index of the current era (the game has several time periods; 2CAVES asks "What era do you desire (1-8)?") |
| `1DC1C` | `g_day_fraction` | 0..FFh, rolls into a new day |
| `DGROUP:03A2[era]`, `03B6[era]` | | day-of-year (1..180) and year (max 999) per era |
| `1DC7A/1DC7C` | event pointers | current script position / message area |
| `1DC7F` | event `cond` | |
| `1DBEB..1DC80` | redraw / mode flags | |

Time: `advance_time(minutes)` (`150CE`) adds to `word_1DC1C`; each 100h units is a new day: the
day counter of the current era (`DGROUP:03A2[era]`) increments, every character gets one day older
(`age_party_one_day`), special days (3Ch/78h/B4h) reset monthly flags (`byte_1DC44/45`); after day
180 the day resets to 1 and the era's year (`DGROUP:03B6[era]`, max 999) increments.

## Sound

Only the PC speaker is used.  `play_sound_effect(n)` (`157E0`, n = 0-9) walks a `FFh`-terminated list of
`(note, duration)` byte pairs from the pointer table at `DGROUP:5214` (data at `5100..5190`); `note` is a
semitone number (60 = C4) turned into a frequency by the table at `DGROUP:5144` (65 Hz = C2 at index 40 ...
988 Hz at index 87) and `duration` indexes `DGROUP:51F4` ({2000,1000,500,250,125,62,31,15,...} timer
units); `pc_speaker_tone` (`16A88`) programs the 8253 (divisor `1193180/f`) and waits for the timer driver
(`TIMER.DRV`) to count `word_22261` down to zero.  There is no background music; the "anim" waits
(`monster_anim_step`) double as the idle loops while a message is shown.

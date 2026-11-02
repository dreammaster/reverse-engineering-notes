# Ultima VI executables — what is established

Findings from the disassembly of `GAME.EXE` (`u6/u6_game`), `U.EXE` (`u6/u6_u`) and `END.EXE` (`u6/u6_end`). Everything here was
read off the code; where Nuvie was used as a cross-check that is said. Names live in `ida_scripts/apply_renames_u6_*.py`
(one `(address, name, note)` list per database) and are visible in the regenerated `.asm`/`.idc`.

Conventions in the code: Borland C++ large model, cdecl (arguments pushed right to left; the callee often does `retf N`),
`dseg` starts at linear `0x3D850` in `u6_game` so a `ds:XXXX` offset is `0x3D850 + XXXX`. Strings and tables are passed as
immediates (`push ds / mov ax, 57Eh / push ax`); `ida_scripts/offsetize_ds.py` turns these into `offset aSchedule` operands.

## The Origin run-time layer

Small routines shared by `GAME.EXE`, `U.EXE` and `END.EXE` (matched by instruction signature, see tools below):

| GAME.EXE | U.EXE | END.EXE | Name | Behaviour |
|---|---|---|---|---|
| `1020A` | `109F3` | `10671` | `file_open(name)` | INT 21h/3Dh read+write; name is a `ds` offset; returns the handle or -1 |
| `10249` | `10A32` | `106B0` | `file_close(handle)` | |
| `1027D` | | | `file_create(name)` | AH=3Ch |
| `102BD` | | | `file_delete(name)` | AH=41h |
| `104A6` | | | `file_rename(old, new)` | AH=56h |
| `102FB` | `10AE4` | `10762` | `file_read(dest far, count dword, offset dword, handle)` | offset -1 = continue; reads in 0FFF0h chunks; returns the bytes read |
| `103E1` | `10BCA` | `10848` | `file_write(...)` | same arguments |

Every DOS failure stores the error in `word_3DB82`, the kind in `word_3DB84` (1 open, 2 close, 5 read/seek) and calls the hook
at `off_3DB7E` (set to `cs:0430h` in `_main`); the call is retried when the hook clears the error. `require_disk(n)`
(`3A947`) asks for floppy disk *n* (4 = program disk, 8 = save disk) when it is not the current one.

**Memory**: `_main` calls `_farcoreleft`, takes everything with one `_farmalloc` and hands it out with `mem_alloc_zeroed`
(`10CB1`: `size dword`, pointer-variable) — a bump allocator that zero-fills and shuts the driver down if it runs dry. There is
no free.

**Video**: the graphics driver (`EGADRV.BIN`, `MCGADRV.BIN`, `CGADRV.BIN`, `TGADRV.BIN` / `U6*.DRV`) is loaded into memory and
entered through the far pointer `video_driver_call` (`4C508`): the offset is set to an entry number before each `call` (3 = shut
down, 0x24 = set palette, 0x2A = initialise ...). `video_mode` (`3DA18`) is 0 = VGA/MCGA, 1 = EGA, 2 = CGA, 3 = Tandy?, 4 =
Hercules; `_main` builds a per-mode table of the 200 scan-line start offsets (stride 0x140 for VGA, 0x28 for EGA, 0xA0/0x50
interleaved banks at B800h for CGA/Tandy, 0x5A with 2000h bank steps for Hercules).

## Start-up order of GAME.EXE (`_main`)

command-line arguments, `farmalloc` of all memory, driver set-up, `load_tile_attribs`, `load_palette`, then
`load_world_data`, and into `command_loop`. End of game execs `END.EXE`; quitting execs `ULTIMA6.EXE`.

## Data files, as the program reads them

All paths are relative to the game directory; the save files are in `savegame\`.

| File | Reader | Layout |
|---|---|---|
| `ANIMDATA` (194) | `load_tile_attribs` (`1118B`) | u16 count; u16[32] tile to animate; u16[32] first frame; u8[32] AND mask; u8[32] shift. Agrees with Nuvie's `Animdata` |
| `TILEFLAG` (7168) | same | u8[2048] flags1; u8[2048] flags2; u8[1024] block 3 (meaning unknown); u8[2048] flags3 (top two bits = article). Nuvie reads 1, 2 and the last one |
| `U6PAL` (1024) | `load_palette` (`11474`) | VGA only: the first 0x300 bytes, 256 RGB triples of 6-bit values; the remaining 0x100 bytes are not read |
| `SCHEDULE` (3329) | `load_world_data` (`14A6A`) | u16 index[256] and a u16 total (the 0x202-byte header; the total lands in `schedule_count`), then total * 5-byte entries |
| `BASETILE` (2048) | same | u16[1024]: first tile of each object type (a tile is `basetile[type] + frame`) |
| `savegame\objlist` | `load_world_data`, `save_objlist` (`14EDF`) | table below |
| `CHUNKS` (65536) | `load_world_data` | read last by it; not yet analysed |

`savegame\objlist` — one array per attribute, 256 actors each; the file offsets agree exactly with the constants in Nuvie's
`obj_list.h`, which independently confirms both:

| Offset | Size | Memory (ds:) | Name | Notes |
|---|---|---|---|---|
| 0x0000 | 0x100 | A0AB | `actor_obj_flags` | |
| 0x0100 | 0x300 | 6828 | `actor_pos` | 3 bytes: x = b0 + (b1 & 3) << 8; y = (b1 >> 2) + (b2 & 0xF) << 6; z = b2 >> 4 |
| 0x0400 | 0x200 | 3548 | `actor_obj_frame` | u16 = object type (10 bits) \| frame << 10; this array is also the first 256 entries of the object table used everywhere |
| 0x0600 | 0x200 | 4EE4 | `objlist_0600` | purpose unknown (bytes, indexed by object) |
| 0x0800 | 0x100 | 9FAB | `actor_status_flags` | |
| 0x0900 / 0A00 / 0B00 | 0x100 | 8C4A / 3316 / 3433 | strength / dexterity / intelligence | |
| 0x0C00 | 0x200 | 9BF5 | `actor_exp` | u16 |
| 0x0E00 | 0x100 | 66E4 | `actor_hp` | |
| 0x0F00 | 0xE0 | 3236 | `party_names` | 14 * 16 |
| 0x0FE0 / 0FF0 | 0x10 / 1 | 3533 / 8E50 | `party_roster`, `party_size` | |
| 0x0FF1 | 0x100 | far ptr 4669C | `actor_level_ptr` | |
| 0x10F1 | 0x100 | far ptr 46CE1 | unknown | |
| 0x11F1 | 0x100 | far ptr 40D94 | `actor_worktype_ptr` | |
| 0x12F1 / 13F1 | 0x100 | far ptr 47685 / ds:9AF5 | combat mode, magic points (order not settled) | |
| 0x14F1 | 0x100 | 8D4A | `actor_moves` | |
| 0x15F1 | 0x200 | far ptr 46480 | base object type + frame | u16 |
| 0x17F1 | 0x100 | B2EB | `actor_talk_flags` | |
| 0x18F1 | 0x100 | far ptr 40C7F | unknown | |
| 0x19F1 | 0x100 | far ptr 4259C | `actor_movement_flags_ptr` | |
| 0x1AF1 | 0x100 | far ptr 46492 | unknown | |
| 0x1BF1 | rest | 2C4A | `objlist_globals` | quest flag, rest counter, game time, karma, wind, timers ... The program asks for 0x2CCC bytes but a fresh file has only 0x182 (386) |

A fresh game has the Avatar (actor 1) at 307,352,0 and a party of 4 — checked by `tests/test_data_files.cpp`, which exercises the C++ readers
in `src/files/` against the real files.

## Commands and handlers identified from their strings

`command_loop` (`13C64`) is the main loop; `cmd_use` (`35809`) is the Use command, a 1.8 KB dispatcher with 44 object handlers;
`cmd_look` (`302F7`), `cmd_ready`/`cmd_unready` (`1E69B`/`1E988`), `cmd_rest` (`3A1DD`), `cmd_cast` (`259E6`), `search_here`
(`30031`) and a few `use_*` handlers (`use_torch`, `use_fishing_pole`, `use_shovel`, `use_sextant`, `use_food`). The object
"usecode" of Ultima VI is therefore compiled into `GAME.EXE` (segments `seg010` and `seg013`, ~140 functions with messages),
which is what Nuvie's `u6_usecode.cpp` re-implements. `scroll_printf` (`17C83`, 141 callers) is the message output.
Names beyond these are deliberately not guessed yet; `ida_scripts/dump_functions.py` lists every function with the strings it uses.

## How the three games relate at the binary level

`ida_scripts/export_signatures.py` + `tools/match_functions.py` compare functions by position-independent instruction
signatures (unique matches only):

| Pair | Matches |
|---|---|
| Ultima VI `GAME.EXE` vs Martian Dreams `GAME.EXE` | 39 of 607 / 954 |
| Ultima VI `GAME.EXE` vs Savage Empire `GAME.EXE` | 44 of 607 / 654 |
| Martian Dreams `GAME.EXE` vs Savage Empire `GAME.EXE` | 244 of 954 / 654 |
| Martian Dreams `MARTIAN.EXE` vs Savage Empire `SAVAGE.EXE` | 203 of 453 / 342 |
| `U.EXE` vs `END.EXE` / `GAME.EXE` (shared run-time layer) | 166 / 70 |

So the two spin-offs are close relatives of each other and only distant relatives (at the instruction level) of Ultima VI: they
were built from a reworked engine, not the same binary with different data. Expect the diff work (roadmap step 4) to be a real
comparison, with Martian Dreams and Savage Empire sharing a base.

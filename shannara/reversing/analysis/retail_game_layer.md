# Retail `Shannara.idb` (SHANNARA.LE) — relationship to the demo, and the game layer

Headless pass (2026-10-12) over copies of `Shannara.idb`, `Shannara Demo.idb`
and `..\xanth\Xanth.idb` via `idat.exe`. Nothing was run against a live
database (IDA was not open); results were applied to the real IDBs afterwards
with backups in `reversing/backups/`.

## 1. What `Shannara.idb` is

| | Demo (`DEMO.LE`) | Retail (`SHANNARA.LE`) |
|---|---|---|
| Original path | `D:\sources\legend\shandemo\DEMO.LE` | `D:\sources\legend\shannara\SHANNARA.LE` |
| Entry | `0x15E638` | `0x7BC20` |
| Code segment | `0x157000–0x180000` | `0x10000–0x9D000` |
| Data segment | `0x187000–0x1AFA50` | `0xA0000–0xEE1B0` |
| Functions | 1,165 (1,140 named) | 2,286 (879 named, 1,407 `sub_`) |
| Build string | "Shannara CD-ROM Demo" | `1.0.0  11-13-1995` |

**Retail is the full game**, same engine, same toolchain (Watcom + DOS/4GW LE),
but a **different build**: shared functions are frequently a different size
(e.g. `load_pic` 609 vs 388 bytes, `load_palette` 384 vs 190, `init_audio`
1517 vs 1343) and retail uses Watcom register-convention symbol spellings
(`main_`, `printf_`, `memset_` …). So the demo is *not* a byte-identical subset
and addresses/sizes do not transplant directly. Names do transplant — and for
the shared engine, most already had (658 same-named functions, 522 of them the
same size).

### Layout of the retail code segment

```
0x10000 – 0x75000   game layer: ~1,100 UNNAMED functions (≈400 KB) — object model,
                    rooms/NPC logic, verb handlers, message text. <-- the real work
0x75000 – 0x9D000   engine/library: pic/font/region/list/audio/AIL/CRT — nearly all named
```

Only 10 additional demo→retail name transfers were found (bigram-Jaccard of
mnemonic sequences ≥ 0.86, mutual-best, unique): `set_music_volume`,
`reset_mouse_limits`, `_XMI_read_log?`, `load_font_palette?`, `reset_bevel_style?`,
`q_kill?`, `AILXMIDI_start_?`, `_XMI_serve?`, `make_voice?` (+ `unload_pics`, which
could not be renamed because the name is already taken elsewhere in the IDB).
Conclusion: **further demo-side naming work has little left to give retail;**
effort should go into the retail game layer directly.

### Xanth is not a useful comparison here

`Xanth.idb` is `xanth_decoded.exe` — a **16-bit real-mode** RTLink-flattened EXE
(same toolchain family as Gateway), 2,710 functions. A mnemonic-sequence match
against either 32-bit Shannara IDB returns **0** matches. A name transplant would
need a semantic (string/call-graph) matcher. The previously noted
`xanth_vs_Shannara Demo.BinDiff` is **no longer in `..\xanth\`** (only the two
`.BinExport`s, the IDBs and two zips remain), so the BinDiff step on the roadmap
is dropped unless that file turns up.

## 2. Game-layer findings (retail)

Working names use the project's trailing-`?` convention where not fully
verified. Applied via `reversing/scripts/apply_retail_findings.py`.

### 2.1 Object table and attribute bits (high confidence on structure)

- `word_217A0` = number of objects (ids are 1-based; every accessor rejects
  `id <= 0 || id > word_217A0`).
- `byte_20918` = **5-byte records `{u8 type; u32 dataPtr}`** indexed `(id-1)*5`
  (`off_20919` is the pointer half). Types seen: 1–5.
- `sub_1A200` → `object_get_location?`: returns the word at `+0Ah` of the data
  record for types 2 and 5, `+0Bh` for type 3, else 0 (likely the
  parent/container id).
- Attribute flags live in per-object bitmaps. A second **5-byte descriptor table**
  (`byte_208A0` / `dword_208A1`, indexed `(attr-1)*5`) says, per attribute,
  `{u8 bitOffset; u32 typeMask}`; an attribute applies to an object only if
  `(typeMask >> (type-1)) & 1`. The helper family:

| Address | Name | Behaviour |
|---|---|---|
| `0x1A104` | `object_attr_locate?` | `(obj, attr, &bit)` → byte* holding the flag, or 0 |
| `0x1A184` | `object_has_attr` | test the bit (≈299 callers) |
| `0x1A1B0` | `object_set_attr` | set the bit |
| `0x1A1D8` | `object_clear_attr` | clear the bit |

### 2.2 Generic bit arrays (high confidence)

`0x21F30 bitset_test` (≈566 callers), `0x21F80 bitset_set`, `0x21FC4
bitset_clear`: 1-based bit index, LSB-first inside each byte
(`n-1 → byte (n-1)>>3, bit (n-1)&7`, `0` is a no-op/false). These are the
game-flag primitives.

### 2.3 `print_obj` — `0x1C0E8` (≈2,495 callers; real name, from its own error string)

The single most-called routine in the binary (debug string `print_obj: not enough storage` gives its real name; it was `print_obj` in the first pass). A **grammar helper** that turns
(object id, form selector) into a word for `%s` inserts in message text
("You already have%s.", "%s glad that this dark, foreboding figure…"). The
selector (low 3 bits of the first argument; object id in the upper 16)
picks article/pronoun/possessive: `the / some / a / an`, `he / she / it /
they`, `your / their …`, using `object_has_attr` bits (including bit `0x16` =
female) and comparing against the two player-character ids
(`word_ADEB0` / `word_ADEB2`) to substitute "your"/"you". This is why the game
text is full of bare `%s` — the engine assembles sentences from object
attributes, which a reimplementation must reproduce exactly.

### 2.4 Command dispatch and per-object handlers (high confidence on mechanism)

`run_command?` (`0x1B7B0`): `(verb=eax, dobj=dx, iobj=bx)`. Maintains a re-entrancy
counter (`dword_ADE20`) and a repeat guard (`dword_CD518`; after 4 identical
consecutive commands prints *"Perhaps now is not the best time to try..."*),
stores the current command in `dword_CD918` (verb) / `dword_CD924+2` (dobj) /
`dword_CD92C+2` (iobj), then offers the command to object handlers in order
(player, dobj, iobj, phases 1-6) via `call_object_handler?`, stopping when one
returns non-zero, and finally falls back to `run_default_verb_handler?`. Strings
such as *"No input verb"*, *"Take which Object?"*, *"Give orders to..."* confirm a
verb + noun interface (Later engine).

**Handlers are compiled code behind function-pointer tables** (answers roadmap
question 2 — this is the Shannara analogue of Gateway's `proc_table`):

- `0x21AF4 call_object_handler?(obj, arg)`: `h = sub_1A998(obj)` (1-based, max
  `0x280` = 640); table `(h-1)>>9` (0 = `off_AFF58`, 1 = `off_B0758`), slot
  `(h-1)&0x1FF`; calls the DWORD code pointer with `eax = arg`.
- `0x21AC0`: tiny special-case table `word_AFF50` of 6-byte `{u16 obj; u32 fn}` entries.
- `0x1E23C run_default_verb_handler?(verb)`: linear search of 8-byte
  `{u32 verbId; u32 fn}` entries at `dword_1DE98` (79 entries).
- `0x1E110`: a large `switch` on the verb id (0x15-0x4B), with out-of-line chunks.
- `0x1A0C4 object_get_data_ptr?(obj)`: data record + 8 (attribute bitmap base).

**Measured table contents** (read from the IDB): `off_AFF58` = 512 distinct code
pointers (`0x17CA0-0x73B00`); `off_B0758` = 128 valid code pointers then
unrelated data (so the handler count is exactly `512 + 128 = 640 = 0x280`,
639 distinct targets); `dword_1DE98` = 79 `{verbId, fn}` pairs, verb ids
1-80 (78 distinct functions). So there are **640 object handlers and 79 default
verb responses** to reimplement/name - the concrete size of the port's logic
surface.

Consequence for the port: like Gateway, **no interpretable script format** —
each handler is real machine code to be read and reimplemented, though the
object model, attribute bits, flags and the dispatch tables give the skeleton.

### 2.5 `show_dialog?` — `0x26414`

A 6 KB UI routine (≈588 callers): hide mouse, `set_font(10)`, `set_font_color(0xFF)`,
build a text window from the 6-byte entries at `word_BBA0E`, run it with the
mouse pushed/popped. Working name only; not traced.

## 3. Next steps (retail)

1. Name `sub_1A998` (object -> handler index), `sub_1E110`, and the object-list helpers (`sub_1A254/2A0/2EC/348/400/49C/4E8`,
   `sub_10F70`) — `sub_1B470` looks like an object *move/reparent* using a
   child/sibling list.
2. Dump the two handler tables (`off_AFF58`, `off_B0758`, 512 entries each) and the
   verb table (`dword_1DE98`) and name/enumerate the handlers per object.
3. Extract the message strings with their referencing function to map NPC/room
   logic regions (e.g. Allanon lines cluster together).
4. Retail `Shannara.asm` is exported locally (15 MB, git-ignored) for grepping.

## 4. Object model, in full (second pass)

**744 objects**, ids 1..744, in five types: type 1 = 69, type 2 = 192, type 3 = 19,
type 4 = 429, type 5 = 35. Records live in the initialised data segment
(`byte_20918[(id-1)*5]` -> `{u8 type; u32 ptr}`), so their *initial state* is in
the exe; names/descriptions are not:

| Offset | Meaning |
|---|---|
| `+0` (dword) | name: 1-based offset into the **runtime text pool** `unk_CD9D0` (loaded from a data file; `object_get_name?`) |
| `+4` (dword) | description: same pool (`object_get_desc?`), 0 = none |
| `+0Ah/+0Bh` (word) | location (parent id): `+0Ah` types 2,5 (and 1,4 by the shared accessor family), `+0Bh` type 3 |
| `+0Ch/+0Dh` (byte) | slot: which child list of the parent it is in |
| `+0Dh/+0Eh` (word) | next sibling in that list |
| handler index (dword) | type 1 `+10h`, type 2 `+25h`, type 3 `+2Ch`, type 4 `+14h`, type 5 `+2Dh` |

Children are kept as **singly linked lists in typed slots**: a parent holds
`byte_20891[(type-1)*3]` list heads (`object_get_list_head?` / `set`), each child
knows its parent, slot and next sibling; `object_move?` (`0x1B470`) unlinks and
re-inserts at the head. This is a classic IF containment tree (room -> items,
inventory, exits...).

**Verified**: 695 of 744 objects carry a handler index; the indices span exactly
1..640 with 640 distinct values (several objects share a generic handler, e.g.
index 275 serves 25 type-4 objects). Full table: `retail_object_handlers.tsv`
(handler, address, size, owning objects, first strings).

### Handler phases (`call_object_handler?(obj, phase)`)

A handler is `int handler(int phase)` (phase in `eax`), returns 1 when it handled
the event. Phases observed at call sites:

| Phase | Where | Meaning |
|---|---|---|
| 1-6 | `run_command?` | command pipeline: 1 = room/actor-location pre, 2 = actor, 3 = dobj's container, **4 = dobj**, 5 = iobj's container, **6 = iobj** |
| 7 | `sub_1B9C8` (event 0x200) | event/idle notification |
| 8 | `verbdef_07_Look_at` | look-at notification |
| 9, 0x19 | `sub_1DA04` (start / after restore) | room entry / (re)start |
| 0x0A | `main_` after restore | restore notification |
| 0x0D-0x0F, 0x1A | `sub_1EE10`, `sub_6A0C4` | object-level actions (not yet traced) |
| 0x16, 0x17 | `main_`, `sub_11478`, `sub_11054` | init / restart |
| 0x18 | 8 sites incl. `objh_259`, `objh_533` | most common non-command phase (not yet traced) |
| 0x1B | `sub_19DBC` | not yet traced |
| 0x1D / 0x1E | `build_command_line?` (`0x1E304`) | the command being assembled is offered to the direct object's handler (0x1D) and the indirect object's (0x1E) with verb/dobj/iobj already in `dword_CD918` / `CD924+2` / `CD92C+2`; return 2 cancels the command, any other non-zero = handled |

### Verb vocabulary

Verb table = 13-byte records at `0x1DAA8`: `{u8 flags; u32 verbId; char *name; char
*preposition}`, 77 named verbs ("Attack with", "Look at", "Talk to", "Give to",
"Put on" ...); ids 1-80, with **16** and **61** having default handlers but no menu
entry. Default handlers are in `dword_1DE98` (79 `{verbId, fn}` pairs); the same
function serves ids 31 (Break) and 80 (Kiss). See `retail_verb_handlers.tsv`; the
functions are named `verbdef_NN_<Verb>`.

### Main loop (`main_` / `startup`)

`startup`: `openAutorun`, `init_memory` (needs >= 1400 KB free), `init_ini_info` (INI
`LEGEND.INI`; keys `PICDATA`, `VOCDATA`, `SAVEDATA`, root `C:\MOS\`), `set_procs`,
`setShanPtr("shan")`, `init_audio/music/graphics`, `init_lists_(31)`. `main_`:
`startup`, `setjmp(restart_jmp)` (restart codes 1-5; 3 = undo -> prints
`[Undone.]`, 2 = restore, 5 = ...), then the loop `select_pic_window(1)`,
`update_animation`, `get_event(0x1FFF, &ev)`, `dispatch_event?(&ev)`. Phases 0x16/0x17
(and 0x0A after restore) are sent to the player object at (re)start.

### Pictures in the retail build

Retail's pic loader differs from the demo's: `load_palette(picId)` selects file
`picId >> 8` (re-opening `_pic_fp` when it changes) and entry `picId & 0xFF`
from a per-file directory (`dword_D43F4`, 12-byte entries `{u32 offset; u32 hdr; u32
hdr2}`), seeks to the entry's palette/header, then `load_pic` creates the gx buffer
and streams pixel data through `sub_85754` -> `sub_90E90`, a plain read/write
callback copy loop (`pic_file_read` as the reader), i.e. **no LZH in this path**;
the writer callbacks (`loc_855DC`, `loc_85634`, `loc_856C4`) do the blit. Not
validated against real data: **no Shannara data files exist on this machine**.

### Mislabelled names found in the game layer

Library-name matching left false names in the game layer: `_L1OutCode_` (0x1A530) is an
object property getter; `_L0BlockClip_` (0x68AAC) and `find_rgb2?` (0x67774) are object
handlers (626, 625); `set_gr_op` (0x5C714) is a getter. Treat any non-`sub_`
name below `0x75000` with suspicion.

## 5. Object types, rooms and the room transition (third pass)

Relationships measured over all 744 records (`retail_objects.tsv`):

| Type | Count | Evidence |
|---|---|---|
| 1 | 69 | **Rooms/locations.** Hold a child-list head at `+0Ah`; have no location; all 68 entries of the room->picture and room->music tables are type 1. |
| 2 | 192 | Located in rooms (51), in type-5 objects (16 = carried), or unplaced (117). Items and some actors. |
| 3 | 19 | Two child-list slots; located in rooms (8) / unplaced (9). Containers/doors (inferred). |
| 4 | 429 | **Never located**, no parent; handler offset `+14h` is the record end. Scenery/noun props available to commands (inferred). |
| 5 | 35 | Located in rooms (34); three child slots. Characters (inferred). |

`retail_objects.tsv` lists every record (id, type, record address, name/desc text
offsets, location, slot, next sibling, handler index and function, raw bytes).
Record sizes are 36/44/48/24/52 bytes for types 1-5.

**Current room** = high word of `dword_ADE94` (now `_current_room`); initial value
88. The same word is the object that `run_command?` and the lifecycle phases are
sent to first, so **room handlers (`objh_*` owned by type-1 objects) receive phases
1, 2, 9, 0x0A, 0x0D, 0x0F, 0x16, 0x17, 0x19**. Pictures: `get_room_pic_id` consults
`_room_pic_table` (68 rooms; id = `file<<8 | entry`, files 20-34 = 0x14-0x22, `0x32` =
blank) and then hard-coded game-flag overrides; music: `_room_music_table`.
`retail_rooms.tsv` has both tables.

`go_to_room?` (`0x1EE10`, ~85 callers) is the room transition: phase 0x0F to the old
room (non-zero refuses), previous/current room update, reparent the party, lighting
(`update_room_light?` prints "It's light."/"It's dark."), music, phase 0x0D to the
new room, redraw.

Vocabulary helpers: `verb_get_name`, `verb_get_flags`, `object_get_display_name?`.

## 6. Savegame schema (fourth pass; written up late - the table is `retail_save_regions.tsv`)

All persistence goes through one primitive, `save_io_block?` (`0x2405C`: `ptr=eax, size=edx,
handle=ebx, op=ecx`): op 2 = `fp_write`, 0x0A = `fp_read`, 3 = `memmove` INTO an in-memory snapshot
(undo), 0x0B = `memmove` OUT of it. A short transfer longjmps to the error label `unk_CF484`.
`serialize_state?` (`0x240D4`, `op=eax, handle=edx`, 0 ok / -1 error) streams, in this order:

1. 0x20-byte header (`byte_CF464`, the save title) - file saves only;
2. every object's record **prefix** - the first `byte_20892[(type-1)*3]` bytes (33/41/48/24/49 for
   types 1-5), i.e. location, slot, sibling, attribute bitmaps and child-list heads; the rest of each
   record is static;
3. the 92 registered extra regions in `_save_regions` (`{void *ptr; u32 size}`, count
   `_save_region_count`) - the `_game_flags` bitset, party/combat state, journal, per-room state bytes...
   (`retail_save_regions.tsv`; several are large: 3,403 / 2,904 / 1,037 bytes);
4. 4 bytes at `dword_ADDEC`, then 0x30 bytes at `byte_CCBB8`.

Total file size is 32,668 bytes. `save_game?` (`0x24210`) mode 2 writes `JOURNAL.nnn`
(`sprintf("JOURNAL.%03d")`, `SAVEDATA` dir); `load_game?` (`0x243F4`) mode 2 restores one, 3 = undo from
the memory snapshot, 4 = `HELP.DAT`, 5 = `RESTART.DAT`; on success it sends **phase 0x18** to the current
room, clears flag 7 of `unk_CD8AC` and longjmps to `restart_jmp` with the mode (`main_` prints
`[Undone.]` for 3). Consequence for a port: the world state is exactly "object record prefixes + 92 byte
regions", so saves can be implemented as a flat dump of the port's own equivalents - but only
original-format compatibility needs the exact layout above.

## 7. Data files and asset inventory (fifth pass)

Strings embedded in the exe name every external file the retail game opens:

| File | Used for |
|---|---|
| `LEGEND.INI`, `MDI.INI`, `DIG.INI` | configuration / Miles driver lists (INI keys `PICDATA`, `VOCDATA`, `SAVEDATA`, root `C:\MOS\`) |
| `OBJECT.DAT` | object text pool (see below) |
| `SHANSTR.DAT` | compressed message strings (see below) |
| `RESTART.DAT`, `HELP.DAT`, `JOURNAL.nnn` / `*.SAV` | restart snapshot, help/tutorial snapshot, save games (see section 6) |
| `*.FLC` | **Autodesk FLIC animations**: `OPEN1-5`, `STILL06`, `SHR_9/14/1113/1819/2024/2729/3032`, `SHANNARA`, `LECPFIXD`, `book2/3`, `badbook7`, `1455a9`, and numbered clips `7700`...`9595` (about 60). Standard format, so ScummVM's FLIC decoder applies. |
| picture files | opened by number through `load_palette` (file = `picId >> 8`), path from INI `PICDATA` |
| voice / sound / music | INI `VOCDATA`; Miles AIL (`.XMI`/digital) |

**OBJECT.DAT** (`load_object_dat?`): `u16 poolSize; char pool[poolSize]` - the NUL-terminated
object names/descriptions. Object records reference it by 1-based byte offset
(`+0` name, `+4` description).

**SHANSTR.DAT** (`init_string_file?`, `load_string_`, `decode_string?`): message-id =
`(block << 16) | index`. File layout, from the loader:

```
u16  nBlocks
{ u16 stringCount; u32 dataBytes } blocks[nBlocks]        // 6-byte descriptors
u16  compressedLen[stringCount] per block                 // stored block after block, read on demand
u16  N1;  u16 huffTree[N1]                                // binary tree as u16 pairs, root = last pair
u16  K (<=128); u16 phraseOffsets[K]; u16 poolSize; u8 phrasePool[poolSize]
...  per-block compressed string data
```
Each string is a bitstream (LSB first) walked through `huffTree` (a negative entry is a
leaf, symbol = `-v-1`); symbols `>= 0x80` expand phrase `sym-0x80` from the phrase table,
smaller ones are literal bytes, symbol 0 terminates. Decoded strings go into a small LRU
text cache (`dword_CD500`, 32 entries) addressed through `load_string_` (ids with top
byte `0xF0` are cache slot references). Structure only: no data file is available here to
validate the bit-level details.

### Framework function map

`retail_function_map.tsv` lists all 2,347 functions with size, caller and callee counts and a
role. Code volume by role: **object handlers 637 fns / 263 KB**, verb defaults 78 / 11 KB,
named game-layer framework 89 / 42 KB, unnamed game-layer framework 468 / 74 KB, engine
and libraries 1,075 / 149 KB. i.e. roughly two thirds of the game layer is per-object
content logic and one third is framework.

Framework landmarks named in this pass (all working names, see `apply_retail_objects.py`):
input `get_event` / `dispatch_event?` -> `handle_key_event?` (with debug keys),
`handle_click_event?`, `handle_move_event?`, `popup_window_event?`; commands
`run_command?` / `build_command_line?` / `verb_get_name`; rooms `go_to_room?`,
`refresh_room?`, `get_room_pic_id`, `set_room_music?`; party `party_has_member?`,
`party_add_member?`; dialog `speak?` / `show_dialog?` / `load_string_`; combat
`start_encounter?`, `clone_combatant?`, `add_combatants?`, `combat_help_menu?`; UI
`save_restore_dialog?`, `options_dialog?`, `describe_contents?`; cutscenes
`play_cutscene?` (FLC); state `serialize_state?`, `load_game?`, `save_game?`.

## 8. The handler "script API" (what a port must provide)

Counting the calls made by the 715 handler functions (objh_* and verbdef_*), the
engine services they actually use, most used first (handlers using it / total calls):

| Service | Handlers | Calls | Meaning |
|---|---|---|---|
| `wprintf` | 656 | 3,245 | print message text to the game window |
| `print_obj` | 455 | 2,603 | article/pronoun/name insertion (`%s`) |
| `show_dialog?` | 136 | 468 | pop-up/voiced dialog line |
| `bitset_test` / `bitset_set` / `bitset_clear` | 130 / 100 / 48 | 463 / 173 / 68 | `_game_flags` |
| `run_command?` | 127 | 171 | issue a nested command |
| `object_parent_is` / `object_has_attr` / `object_move?` | 100 / 93 / 84 | 227 / 200 / 188 | object tree and attributes |
| `make_voice?` / `make_sound` | 93 / 44 | 150 / 71 | speech and effects |
| `journal_add_entry?` | 65 | 99 | notebook/journal entries |
| `delay?` / `delay_animate?` | 63 / 65 | 137 / 160 | timed pauses |
| `go_to_room?` | 56 | 76 | room transition |
| `disable_pic_regions?` / `setup_pic_regions?` | 53 / 48 | 57 / 50 | room hotspots |
| `speak?`, `party_*`, `refresh_room?`, `flagbyte_*`, `play_music_entry?`, `add_score?`, `start_encounter?`, `play_cutscene?` | 5-35 each | | speech, party, redraw, state bytes, music, score, combat, FLC |

A port's handler runtime therefore needs: a text window with `%s` object-name insertion,
the object tree + attribute bits, the 800-bit flag set, a small per-room state-byte
store, voice/sound/music, a dialog popup, hotspots, the journal and party systems, and
timed delays. Everything else in the handlers is per-object conditionals over those.

Other helpers identified along the way: `move_to_current_room?`, `value_in_list?`,
`load_pic_sequence?`, `journal_*`. The journal table is `_journal_table` (22-byte records).

## 9. Video modes and handler classes (sixth pass)

**Video**: `graphics_mode(n)` accepts 3 and 5 only. Mode 3 = BIOS mode 13h, 320x200x256
(`_video_pitch` 320); **mode 5 = VESA 0x101, 640x480x256** (`_video_pitch` 0x280; via
`VESA_set_mode(0x101)`, else "ERROR: Unable to establish VESA graphics"). `startup` always calls
`graphics_mode(5)`. The main picture viewport is **528x293** (0x210 x 0x125) in mode 5
(264x122 in mode 3). INI keys `XMOUSE` and `NOCYCLE` (disable palette cycling).

**Handler classes** (`retail_handler_features.tsv`, 715 handler functions = 637 object + 78
verb): 403 only print text (wprintf/print_obj, no state change), 274 change state (flags, object
moves, room changes, journal, score, combat, cutscenes), 26 are tiny stubs, 12 other. So
~56 % of the content logic is pure message selection; the remaining ~44 % is the part a
port must get exactly right.

## 10. Handler phases, resolved (seventh pass)

Reading `go_to_room?` and the 63 room handlers' dispatch chains pins down the lifecycle phases
(the `eax` argument of every handler). Room handlers test, in order of frequency: 1 (56 of 63),
0x0E (56), 0x17 (54), 0x0F (53), 0x16 (52), 8 (55), 9 (48), 0x0D (48), 0x18 (30).

| Phase | Sent by | Meaning |
|---|---|---|
| 1-6 | `run_command?` | command pipeline (1 room/actor pre-hook, 2 actor, 3/5 container of dobj/iobj, **4 dobj, 6 iobj**) |
| 7 | event 0x200 handler | notification/idle |
| 8 | `verbdef_07_Look_at` | "look at" notification |
| 9 | `describe_room?` (first visit) | describe room, first time (room attr 5 = "visited" not set) |
| 0x0A | `describe_room?` (revisit) | describe room again (attr 5 set); also sent by `main_` after a restore |
| 0x0B / 0x0C | `describe_room?` | verbosity variants chosen by `byte_ADE9A` (2 -> 0Bh, 0 -> 0Ch) |
| 0x0D | `go_to_room?` | arrived (before description) |
| 0x0E | `go_to_room?` | arrival finished (after description) |
| 0x0F | `go_to_room?` | about to leave; a non-zero return vetoes the move |
| 0x16 | `refresh_room?` | draw hook; non-zero = handler drew the scene itself |
| 0x17 | `main_` / popup event | (re)start / init |
| 0x18 | `load_game?` | game was restored |
| 0x19 | `sub_1DA04` | post-start notification |
| 0x1A | `go_to_room?` | a following object (`unk_AE81E`) has been moved in with the party |
| 0x1B | `sub_19DBC` | not yet traced |
| 0x1D / 0x1E | `build_command_line?` (`0x1E304`) | the command being assembled is offered to the direct object's handler (0x1D) and the indirect object's (0x1E) with verb/dobj/iobj already in `dword_CD918` / `CD924+2` / `CD92C+2`; return 2 cancels the command, any other non-zero = handled |

(Handlers of non-room objects mostly dispatch on 3/4/6 with the verb in `dword_CD918`, as the
verb is not a parameter.) `go_to_room?` also awards the room's stored points once
(`object_get_points?` -> `add_score?` -> `object_set_points?`) and sets attr 5 (visited).

## 11. Room overlays (eighth pass)

A room's clickable/visible objects are drawn as **overlay pictures** on the room picture. Each
room handler owns static tables of 12-byte entries and passes them to
`setup_pic_regions?(table, count)` (on `0x16`, the draw phase) and `disable_pic_regions?` (to
hide):

```
struct Overlay {            // 12 bytes
    s16 obj;                // object shown
    s16 container;          // if non-zero: obj must be inside this object (object_parent_is)
    s16 picId;              // (pic file << 8) | entry   - the overlay sprite
    s16 attr;               // if non-zero: obj must have this attribute bit
    void (*cond)(int obj);  // optional extra visibility test (returns non-zero to draw)
};
```

`overlay_visible?` (`0x1BC20`) evaluates an entry; `setup_pic_regions?` keeps a per-entry "drawn" byte
in `byte_CD51C[128]`, loads each newly visible entry's picture with `load_pic_in` (retrying after
`stop_music` to free memory) and removes entries that stopped being visible. Skipped when the UI
lock `byte_ADE24` is set. `retail_overlays.tsv` lists all 108 distinct tables used by handlers
(e.g. start room 88: object 11 -> picture 0x1409 = file 20 entry 9, object 48 -> 0x1407, ...).

## 12. Picture file container (ninth pass; structure from code, not validated against data)

`load_palette(picId, frame)` -> `sub_77FEC(fileNo)`: a picture id is `(file << 8) | entry`. Each
picture file begins with a **0xC00-byte directory = 256 entries x 12 bytes** (read in one
`read_file`); up to 4 directories are cached (`unk_D13D8` ring, 0xC00 each; their derived
data-length tables in `unk_D03D8`, 0x400 each). Entry layout (copied to `dword_D4438..D4440`):

| Offset | Meaning |
|---|---|
| `+0` u32 | file offset of this picture's data (0 = unused entry; ends the directory scan) |
| `+4` u8 | frame count (the per-frame offset table is `count*4` bytes) |
| `+5` u8 | flags: `0x10` = a 768-byte (0x300) palette follows the frame table; `0x20` / `0x40` select the `Pic.field_5` mode 4 / 3 used by `load_pic` |
| `+6` u16 | stored into `Pic.field_A` (a dimension; the pixel-buffer creation uses it) |
| `+8` u16 | stored into `Pic._nextTime` (animation delay) |
| `+10`, `+11` u8 | stored into `Pic.field_E`, `Pic.field_F` |

Derived: payload length = next entry's offset - this offset - `count*4` - (palette ? 0x300 : 0); for the
last entry the file size is used. `load_palette`: if the requested frame > 0 it seeks to `offset + (frame-1)*4`
and reads two u32 (`dword_CA1C0`, `dword_CA1C4` -> `Pic.field_6`, `Pic.field_8`); if the entry has
a palette it seeks to `offset + count*4` and reads 0x300 bytes into `_default_palette`
(`_palette_loaded = 1`). `load_pic` then allocates the gx buffer (`gxCreateVirtual`) and streams the
frame data through `sub_85754` -> `sub_90E90` (a plain callback copy, `pic_file_read` as reader; the
writer callbacks `loc_855DC` / `loc_85634` / `loc_856C4` blit rows).

Writers: the stream callback pair writes rows with plain `sub_85614` (byte copy; handles VESA bank wrap
through `gxSetCtr`) or, when the XOR flag is set, `sub_81E90` (destination ^= source) - i.e. **pixels are raw
8-bit, and animation frames can be XOR deltas** against the previous frame. A separate sprite blitter
(`sub_83FEC`, called with `ebx == 0x40` for a 1-byte/pixel variant, else 4) walks a `{skip, count}`
run structure; it is the display-side transparency path and was not traced.

Open questions needing real picture files: meaning of the two u32 per frame, the bit layout of
flags 0x20/0x40 (likely pixel format / compression), and whether the frame data is raw or RLE.
The demo build used LZH for the same job (see `lzh_decode_spec.md`); retail does not appear to.

## 13. Video playback and object property accessors (twelfth pass)

**Videos are looked up as `NAME.Q` first, then `NAME.FLC`.** `video_open?` (`0x22D50`) builds both paths with
`_splitpath_`/`_makepath_` from the INI data directory, opens `.Q` (state type `0x2000`) and falls back to `.FLC`
(`0x1000`); `strupr_` is applied to the name. So the in-house Q codec traced in the demo is a first-class
cutscene format in retail, not only a demo artefact, and ScummVM's FLIC decoder is only the fallback. Player state
is a `0x6D0`-byte struct: `+8/+0Ah` current/last frame, `+0Ch` type, `+0Eh` flags (bit 0 palette saved, bit 5
background saved), `+10h` saved-background pic, `+28h` saved palette (`0x300`), `+328h` FLC player / `+32Ch`
Q state, `+338h` FLC frame delay, `+6CCh` optional per-frame callback. Frame pacing is wall-clock based
(`get_clock`): Q delay = `(1000 + fps/2) / fps` ms (fps = Q header byte), FLC delay = header speed; an
adaptive average (`dword_CF450/CF458/CF460`) corrects drift; minimum 10 ms.

| Function | Role |
|---|---|
| `video_open?(vid, name, rect, flags)` | flag `0x20` saves the screen under the video, `0x01` saves the palette |
| `video_play?(vid, frames, flags)` | returns 0 finished / 1 callback stop / 2 error / 3 aborted by input |
| `video_close?(vid)` | closes the stream, restores background + palette |
| `play_video?(name, flags, delay)`, `play_video_wait?(name, flags, delay)`, `play_video_voiced?` | the wrappers handlers call (24 / 49 / 38 callers) |

**Object property accessors** (all take the 1-based object id, switch on the object type via `_object_table`):

| Function | Offsets in the record, by type |
|---|---|
| `object_get/set_prop_a?` | t1 `+14h`, t2 `+19h`, t3 `+20h`, t5 `+21h` (none for t4) |
| `object_get/set_default_verb?` (was prop B) | t2 `+1Dh`, t3 `+24h`, t4 `+0Ch`, t5 `+25h` (none for t1): the verb `build_command_line?` uses when the object is clicked |
| `object_get/set_prop_dword?` | t2 `+11h`, t3 `+14h`, t5 `+15h` |
| `object_get_list_slot?` | index property t1 `+1Dh`, t2 `+21h`, t3 `+28h`, t4 `+10h`, t5 `+29h` into a pool at `unk_AF370` (save region 15, 2,904 bytes) of 0-terminated dword lists; `object_list_append?` adds to one |

What properties A/B/dword *mean* is not established (handlers use them as small per-object variables, e.g. after a
scripted event); `object_carrier?` (`0x19BC0`, 72 callers) maps an item to the character-table entry carrying it
(24 x 8-byte records at `dword_ADB84`, also in the save regions).

## 14. Story cutscenes: `play_cutscene?` (thirteenth pass)

`play_cutscene?` (`0x28858`, 6,258 bytes, 12 callers, all from handlers/`sub_221C4`) is **not** a generic player but the
script of the game's **12 story cutscenes** (`id` 0-11, anything else returns at once). Common prologue: counts the
cutscene in a per-id "seen" word table (`word_A7E88`), computes the 640x480 centering rectangle (`dword_CFC06...`; id 11 uses
the full screen), sends **phase 0x18** to the current room, waits 1 s, stops the music, then starts the cutscene's music with
`play_music_entry?(dword_C4614[id], 0x10)` and jumps through the 12-entry table `jpt_289D6`. Each case is straight-line
code: clips with `play_video_wait?(name, flags)`, narrated clips with `play_video_voiced?(voiceMsgId, name)` (the id is a
`SHANSTR` message that also names the voice file), `show_dialog?`, `make_voice?`/`make_sound` (a sound-effect id doubles
as the voice message id), palette fades, and a few state changes. File names are stored as `"NNNN.flc"` strings, but
`video_open?` discards the extension and tries `NNNN.Q` first (section 13).

| id | Clips (in order) | Notes |
|---|---|---|
| 0 | `9100`, then 9 voiced lines (msgs `0xC7A-0xC82`) alternating `9120` / `9110`, `9130`, sound `0xDE2`, `9140` | party healed 100 % first (`party_heal_percent?(100)`) |
| 1 | `9500`; voices `0xD4A`, `0xD4B` | short |
| 2 | `9500`, `9590`, voiced `0xC83-0xC85` on `9591`, `9592` | heal 100 % |
| 3 | voiced `0xC87-0xC8A` alternating `9220` / `9110` | heal 100 % |
| 4 | `9500`, `9540`, voiced `0xC8D-0xC92` on `9541`, `9542` | |
| 5 | voiced `0xC98-0xC9A` (`9120`/`9110`), `9130`, `9140` | heal 100 % |
| 6 | voiced `0xC9B-0xCA1` over `9420`/`9110`/`9400`, `9130`, `9440` | |
| 7 | dialog/voiced exchange on `9110`, `8550`, `8530`, voices `0xAA0`, `0xAA1`, then **`start_encounter?(14)`** and a fixed HP change on character 3 | cutscene that leads into the boss fight with `enc_hook_14?` |
| 8 | long travel montage: `9500`, then for each of five places a triple (`9590`, `9570`, `9580`, `9560`, `9550` plus their `...2` variants) with a voice line and a dialog each, ends `9595` and a voiced exchange on `9110` | |
| 9 | `8585` | |
| 10 | `8590`, `8560`, `8580` | |
| 11 | full-screen (640x480) narrated slideshow: voices `0xD86-0xD8E` with palette fade out/in and `load_music` between images, then `party_add_member?(4)` and a run of `show_dialog?` | probably the opening sequence (it adds a party member and hands over to play) |

Clip-number families seen: `91xx`/`92xx`/`94xx` (talking-head dialogue scenes, the `...0`/`...1` pairs alternate speakers),
`95xx` (travel/montage), `85xx` (short story beats). Sound-effect ids `0xDE2`, `0xDB2`, `0xDB3` accompany scene
transitions. Reimplementing this function means a data-driven list of `(clip, voiceMsg, dialogMsg, flags)` steps per
id plus the few side effects (healing, `start_encounter?`, party changes) - there are no loops or branching beyond the
`ret`-on-abort checks after each clip.

Related helpers named in this pass: `party_heal_percent?` (`0x196B8`), `character_damage?` (`0x19778`; the character
table is 24 x 8-byte records at `dword_ADB84` with `_char_stats` 14-byte records in the same order), `state_pair_set?`
(`0x19C40`, a 16-entry `{u8 key, u16 value}` list at `byte_CCBB8` that is part of the savegame).

## 15. Dialog descriptor table (fourteenth pass)

Every `show_dialog?(dialogId, ...)`, `play_video_voiced?(dialogId, clip)` and voice call takes a **dialog id** that indexes a
table of **6-byte descriptors** at `word_BBA0E` (`dseg`, 3,403 valid entries, ids 0-3402; the bytes after it are `0x0101`
fill). Dump: `retail_dialog_table.tsv` (index, w0, w1, w2).

| Word | Meaning (from the dump and the `show_dialog?` code) |
|---|---|
| `w0` | layout/speaker: high byte = speaker/portrait index (4-37 seen), low byte = style flags (`0x80` = bit tested into `byte_C2875`); the special values `0x100`/`0x200`/`0x300` select the three dialog layouts that `show_dialog?` caches in `byte_C2871-C2874` |
| `w1` | the line's text message id (a `SHANSTR.DAT` message, section 7) |
| `w2` | continuation: the index of the next descriptor in the conversation; `0xFFFF` ends it, `0x8000` ends it with a special close, and 1,326 entries simply point at `index + 1` |

So a conversation is a linked list of lines, each with its own speaker/portrait and text id; scripts only name the first
entry. `show_dialog?` (`0x26414`, 6,323 bytes, 599 callers) is the interpreter: it hides the mouse, builds the dialog window
for the layout, loads the speaker's portrait animation (`load_pic_sequence?`, `update_animation`), prints the text, starts the
voice (`voice_on`), waits for a click or key, then follows `w2`. A per-dialog state byte (`flagbyte_ptr?(dialogId)`: bit 0 =
enabled, bit 1 = already shown) is part of the savegame state bytes, which is how repeat conversations are suppressed.
Not yet traced: the exact text-layout and portrait-animation rules, and how voice file names are derived (the `make_voice?`
argument is also a dialog id).

## 16. Corrections and details from re-reading the string and picture code (fifteenth pass)

**Picture directory entry (section 12), revised reading of `load_pic`**: `dword_D4438..` is the copied 12-byte entry; the code tests
`word at +6` and `word at +8` for zero ("empty picture": marks the slot active without creating a buffer) and passes them to
`gxCreateVirtual` as the buffer's two dimensions, so **`+6` is most likely the width and `+8` the height**, not an "animation
delay" as section 12 first assumed (the same words are also copied into Pic fields that the demo-derived `Pic` struct names
`field_A` / `_nextTime`, but the retail build lays `Pic` out differently - the struct applied to retail is wrong for these
offsets). Flags: `+5 & 0x10` palette present, `& 0x20` -> pic mode 4, `& 0x40` -> pic mode 3 (else 0). With `_grafx_mode == 5` and
`sub_7858C() == 3` the picture is additionally scaled to 1.6x width (x2 pixel doubling path via `sub_7890C`). Still unvalidated
against real files.

**SHANSTR string decoding (section 7), confirmed from `decode_string?` (`0x1AF0C`-ish)**: `decode_string?(dst, bytesLeft, file,
tree, treeWords, phrasePtrs, phraseCount)`. Bits are read LSB first from the stream (`read_array?` one byte at a time); the
current node starts at the *last* pair of the u16 `tree` (`treeWords - 2`) and `child = tree[node | bit]`; a negative value is a
leaf with symbol `-v-1`. Symbols `>= 0x80` with `sym - 0x80 < phraseCount` copy the NUL-terminated phrase `phrasePtrs[sym-0x80]`
(so phrase text is not NUL-expanded recursively); every other symbol is written as one byte (symbol 0 therefore emits the string's
terminating NUL). Decoding stops when the compressed length is used up *and* the last symbol was 0; the return value is the
number of output bytes. Pure and easy to port - the only unknowns left are the exact file offsets of the tree/phrase sections.

## 17. Data file naming and the voice file (sixteenth pass)

`get_file_name(type, number)` = `sprintf("%s%03d.%s", _shanPtr, number, ext)` with `_shanPtr` = `"shan"` (set by `setShanPtr("shan")`
in `startup`) and `ext` from `_file_path_8[type]`: **PIC, RGN, FNT, MUS, SAV, SAV, VOC, Q** for types 0-7 (`FTYPE_*`).
`get_file_path` then prepends the directory chosen by the INI (`PICDATA` for pictures; `VOCDATA` supplies the voice/sound
directory, `SAVEDATA` the save directory; entries in `_filePaths[type]`) as `"%s%s"`. So the retail files are
`shan020.PIC` ... (picture file number = `picId >> 8`), `shan001.VOC`-style voice files, `shanNNN.SAV` saves, etc. Videos are
the exception: they are opened by their own base name (`9100`, ...) plus `.Q` / `.FLC` (section 13). `open_file` opens mode
`"wb"` for type 5 (SAV write) and `"rb"` otherwise.

**Voice/sound file (`open_file(6)`, kept open in `_voice_fp`)**: `u32 trackCount` followed by `trackCount` u32 absolute
file offsets (`_voice_dir_cache`); track `i` spans `[dir[i], dir[i+1])` (`get_sound_id(id, &offset)` returns the length as
`dir[i+1] - dir[i]`, so the directory needs an end sentinel as its last entry; ids `< 0` or `>= trackCount` give 0). Dialog,
`make_voice?` and `make_sound` ids index it directly. `make_voice?` streams the track in `_voice_buffer_size` chunks through
a Miles AIL sample with double buffering (`_AIL_sample_buffer_ready` / `_AIL_load_sample_buffer`); `make_sound` loads the whole
track into a locked handle and plays it once (`_AIL_set_sample_address`, EOS callback). Sample type / rate come from the
sound driver defaults; the data are streamed as-is to the AIL buffers (the code also counts bytes in 0x7F-0x81, purpose not established), and `get_sound_id` returns 0 when `_sound_supported` is 0. Voices are only played when the INI
enables them (`_voice_enabled`), and `make_voice?` first checks `sub_13A20` (abort/skip input) so a click skips the line.

## 18. Event dispatch (seventeenth pass)

`dispatch_event?(ev)` (`0x1173x`) routes the event record filled by `get_event(mask, &ev)` (`dword type` at `+0`, parameters
further in; word at `+0Ah` carries an id for the audio events). A per-UI-mode hook table (`off_A7E84`, indexed by the high word of
`dword_A7E9A`, entries `funcs_11780[]`) is offered the event first; a return of 1 consumes it. Otherwise by type:

| Event type | Handler |
|---|---|
| `0x01` | `handle_key_event?` (typed keys, debug keys) |
| `0x02`, `0x08` | `handle_move_event?` (mouse movement / hover over regions) |
| `0x04` | `handle_click_event?` (region hit-test via `get_region_`, then a switch on the region kind - kinds seen: **6 = the command-line area -> `build_command_line?`**, **5 -> `click_button_`** (a button widget), 9 -> `sub_123D0`, 0x64 -> picture hotspot path (`get_region_xy_`, game flag test, `sub_105B0`/`sub_10250`; this is where clicks on room overlay regions go), 8 -> `sub_10554` item lookup + `sub_10404`/`sub_10770` selection, 3/4 -> `sub_103DC`, 7 -> `disable_button_?`; kind 1 with sub-kinds 4/5 are the two scroll arrows (subtract/add 5 on `dword_A7EA0`); a right-button click (`dword_CC720 & 2`) cancels the pending command (`sub_10AB8`)) |
| `0x10`, `0x20`, `0x40` | `popup_window_event?` (window open/close notifications; a 0x10 event is swallowed while `word_A7E5C` is non-zero) |
| `0x80`, `0x100` | `sub_10B20(param)`, `sub_132A0(param)` (window refresh / redraw requests) |
| `0x200` | `sub_1B9C8` = idle/notification, sends handler phase 7 |
| `0x400` | `start_music(id = word at +0Ah)` |
| `0x800` | `make_voice?(id)` then `sub_82400` (queued voice playback) |
| `0x1000` | `sub_1FE8C` |

So audio is also driven through the event queue (music and voice requests are posted as events), which is why handlers can
issue them without blocking. The remaining steps from `build_command_line?` to `run_command?` (section 2.4) are not yet traced.

## 19. The turn clock and timed events (nineteenth pass)

`advance_time?(turns, askContinue)` (`0x19DBC`, was `sub_19DBC`) is the game clock. Each turn it (1) sends **handler phase 0x1B** to the
current room (so rooms can react to time passing), then (2) walks the 16-entry **timed-event list** at `byte_CCBB8`
(`{u8 eventId; i16 countdown}`, count `dword_ADDEC`, saved in the savegame): positive countdowns are decremented; an entry at 0
runs `fire_timed_event?(id)` and is removed unless the event rescheduled itself. `schedule_event?(id, delay)` (`0x19C40`, was
`state_pair_set?`) adds an entry. `fire_timed_event?` (`0x1F700`) dispatches ids **1-8** (`funcs_1F709`):
2 `sub_62A70`, 3 `sub_4D5F8`, 4 `sub_4D678` (damages a party member with voiced messages), 5 `sub_61750`, 6 `sub_5BBBC`,
7 `sub_5BC10`, 8 `sub_5BC50` (1 is a null slot). The lethal ones end in `game_over_screen?` (`0x1F580`). Waiting ("%s a while.")
calls `advance_time?` with a turn count and, for long waits, asks "Do you want to continue..." between turns. So the game has
exactly eight scripted timers (hunger-/poison-/cold-style countdowns); porting them means eight small functions plus this list.
Phase 0x1B in the phase table (section 10) is therefore "a turn passed".

## 20. SHANSTR.DAT: complete layout and lookup (twentieth pass; from `init_string_file?`, `load_string_`, `decode_string?`)

Message ids are `(block << 16) | index`; engine calls pass them as `0xF0000000 | id` (e.g. `wprintf(0xF00D0006)` = block 0x0D, string 6),
the `0xF0` top byte meaning "SHANSTR id, not a char*". `init_string_file?` reads, in order:

```
u16  nBlocks
struct { u16 stringCount; u32 dataBytes; } block[nBlocks];     // 6 bytes each
u16  lengths[sum(stringCount)];                                 // compressed length of every string, block after block (position remembered as lenBase)
u16  treeWords;  u16 tree[treeWords];                           // Huffman tree (negative = leaf, symbol = -v-1; root = last pair)
u16  K;                                                          // number of phrases, <= 128
if K > 0 { u16 phraseOffset[K]; u16 poolSize; u8 pool[poolSize]; }   // phrase i = NUL-terminated string at pool + phraseOffset[i]
... string data begins here (dataBase = file position after the phrase pool)
```
Lookup of `(b, i)`: valid if `b < nBlocks` and `i < stringCount[b]`; the length table of block `b` is read from
`lenBase + 2 * sum(stringCount[0..b-1])` (one block's table is cached); the compressed string starts at
`dataBase + sum(dataBytes[0..b-1]) + sum(lengths[b][0..i-1])` and is `lengths[b][i]` bytes long; `decode_string?` (section 16) expands it,
a NUL is appended, and the text goes into a 32-entry LRU cache (`dword_CD3E8`, with a shared text arena `dword_CD500`; ids already
cached are returned without I/O). Bits are consumed LSB-first. This is a complete, portable description - the remaining risk is only
that no real `SHANSTR.DAT` has been available to confirm field widths against actual bytes.
Also noted: `wprintf` formats such ids with `%s` inserts filled by `print_obj` (section 2.3), so a port's text layer is
"decode id -> printf with object-name arguments".

## 21. Validated against the real retail files (twenty-first pass; data in `game/`, not committed)

- **SHANSTR.DAT**: `scripts/shanstr.py` parses it with the section 20 layout: 20 blocks, 7,719 strings, 432-word tree, 128 phrases;
  the computed end of the last block equals the file size (332,731) exactly, and **all strings decode to clean text** ending in exactly one
  NUL. One refinement to the decoder rule: decoding stops after a NUL only once *all input bytes have been loaded* (padding bits in the
  last byte are ignored), which is what the engine's `bytesLeft == 0 && lastSymbol == 0` test does. Strings contain `%s` inserts and
  `\v`/`\r` control characters as expected.
- **OBJECT.DAT**: `u16 6758` + a 6,758-byte NUL-separated pool of 761 names, exactly the file size; entries 1.. are `limbo`, `party`,
  `Jak Ohmsford`, `me`, `Shella`, ... so object id N's name is the string at the record's 1-based offset (matches the object table).
- **FNT fonts** (`scripts/fnt_read.py`): 10 header bytes `[0] ? [1] bits/pixel (1 mono, 8 colour; 0xFE = transparent) [2] bytesPerRow
  [3] firstChar [4] lastChar [5] lineSpacing [6] charHeight [7] charWidth (0xFF = width table) [8] charSpacing (0xFF = spacing table) [9] ?`,
  then optional `u8 widths[lastChar+1]`, `u8 spacing[lastChar+1]`, then `(last-first+1) * bytesPerRow * height` glyph bytes. The size formula
  matches 10 fonts exactly and the three 8-bit fonts (`SHAN015-017`) with a few unread trailing bytes; rendered text is correct.
- **Q videos** (demo): see `q_codec_spec.md` - real files decode completely.

## 22. Picture files, validated on the real data (twenty-second pass)

`PICS.UHA` (UHARC archive, extracted inside DOSBox with the bundled `UHARC.EXE`, a DOS/PMODE-W program) holds the `SHANnnn.PIC` files.
Verified on 3 files (500+ entries) and spot-checked on the rest; `scripts/pic_scan.py` checks the arithmetic on every entry:

- **Directory**: 12-byte entries from file offset 0 until the first entry's data offset (e.g. 219 entries when the first picture starts at
  2,628); `+0 u32 offset`, `+4 u8 hasOffset` (0 or 1 - the "frame count" of section 12 is only ever 0 or 1), `+5 u8 flags`, `+6 u16 width`,
  `+8 u16 height` (confirmed: the first entries are 528x293 = the main picture window, or 640x480 / 1092x293 panoramas), `+10 u8`, `+11 u8`
  (+10 is 2 for a few entries, otherwise 0).
- **Entry data at `offset`**: `{i16 x, i16 y}` placement offset if `hasOffset`; a 768-byte palette (6-bit VGA components, scale x4) if
  `flags & 0x10`; then **exactly `width*height` raw 8-bit pixels** (no compression). `span = next offset - offset` equals
  `4*hasOffset + 768*palette + w*h` for every entry examined.
- **Flags**: bit 0x10 palette; **0x40 = XOR sprite** (index 0 leaves the background unchanged, other pixels are XORed onto the background at
  `(x, y)`; verified: SHAN020 entry 9 XORed onto entry 3 reveals Allanon, smoothness metric 33 vs 93 unXORed); **0x20 = index 254 is
  transparent** (a face portrait with dithered 254 pixels; likely, not proven); the low bits (`0x05`, `0x03`, `0x01`) are always set and unexplained.
- Rooms: an entry 528x293 with a palette is the room backdrop; the following entries are its overlay sprites (these are the `Overlay.picId`
  entries of section 11).

Example result: `SHAN020` entry 0 is the overworld map (Arborlon, Druid's Keep, Hall of Kings, Tyrsis, Leah, Shady Vale), entry 3 the willow
glade with the XOR sprite of Allanon.

## 23. Savegame layout checked against the shipped `HELP.DAT` (twenty-third pass)

`HELP.DAT` (32,660 bytes) is a savegame-format snapshot (loaded by `load_game?(4)`). Against the section 6 schema:

- bytes `0..0x1F` are the 0x20-byte header (all zero here); the **object record prefixes follow immediately and line up for all 744
  objects** (they end at 0x5A40 = 0x20 + 23,072; object 1 is `01 00 00 00 00 00 00 00 00 00 82 02 ...`, identical to the exe's initial
  record, and 488 of the 744 records equal their initial state, the rest differ as expected for a mid-game snapshot);
- the 92 extra regions then decode plausibly in table order: region 2 (high word of `_current_room`) = 0x58 = **88, the start room**; region 9
  (the 25 x 8-byte character table) starts `34 00 00 00 03 00 00 00 35 00 00 00 ...` (character object ids 0x34, 0x35, ...); region 10
  (`_char_stats`, 25 x 14 bytes) starts `03 00 64 00 64 00 3c 00 0a 00 3c 00` = HP 100 / max 100 for the first character (HP at +2, max at +4 as
  assumed); regions 12-14 (the 3,403 + 1,037 + 282 byte per-dialog/state flag bytes) are all `01` = "enabled" as expected;
- **the file is 8 bytes shorter than the 32,668 the current engine expects** (header 0x20 + 23,072 + 9,512 + 4 + 0x30). Dropping the last two
  4-byte regions before the tail (`dword_AE9F4`, `dword_AE9F8`, regions 89 and 90; the latter is set by `start_encounter?`) makes the byte count
  and the tail alignment fit exactly, so this `HELP.DAT` was most likely written by a build that predates those two variables. A port that wants to
  load it should treat a short file as "missing trailing regions keep their defaults".
- The final 52 bytes are `dword_ADDEC = 12` followed by 48 zero bytes (the timed-event list is empty although the count is 12, so that
  count is not simply "number of queued events" at snapshot time).

## 24. Region files (`SHANnnn.RGN`), validated (twenty-fourth pass)

Read from `sub_7D8A4` (picture region loader, called with a pic id; file number = `picId >> 8`, directory entry = `picId & 0xFF`) and verified on
all 12 retail `.RGN` files (`scripts/rgn_read.py`: every region list ends exactly at the next list's offset): **directory of 6-byte
entries `{u16 dataOffset, u16 baseId, u16 count}`** (one per picture entry; ends at the smallest non-zero offset; `dataOffset` 0 = none,
entries with `count` 0 may alias another entry's offset), then at `dataOffset` `count` regions `{i16 id; i16 type; ...}` where type 2 = rectangle
`{x1,y1,x2,y2}` and any other type = polygon with `type` vertices `{x,y}`. `baseId` becomes the id of the whole-picture region (kind 3).
These are the clickable hotspot shapes of the room pictures; the loader adds the picture window offset and, in the 320x200 mode,
scales x by 1/2 and y by 5/12. Region ids are what `handle_click_event?` hands to the hotspot code (region kind `0x64`).

## 25. Music and voice archives, validated (twenty-fifth pass)

- **`SHAN001.MUS` (retail) and `Demo001.mus` (demo)**: `u16 count` followed by `count` entries `{u32 offset; u16 size}` (6 bytes; `get_music_id`
  returns the offset through a pointer and the size in `ax`). `count` includes a terminator entry (`offset = file size`, `size 0`), so the retail
  file holds 78 tracks and the demo 15. Offsets are 4 KB aligned, and **every entry is a complete XMIDI file** (`FORM....XDIR` + `INFO` + `CAT ` `XMID`
  `FORM`...; `size == 22 + 8 + CATsize`, checked for all 93 tracks). They can be fed straight to an XMIDI player; `start_music(id)` and the
  `0x400` event use the id as the index.
- **Demo `Demo001.voc`**: a different, older layout (not the retail `u32 count + u32 offsets` of section 17): byte 0 = track count (7), then
  `u32` values counting **256-byte blocks** (`0x10, 0x10, 0x330, 0x3e0, 0x5a0, 0x850, 0xed0`: block 0x10 = file offset 0x1000 where raw unsigned 8-bit PCM
  starts, last value = end of file 0xED000). Not further decoded.
- **This retail release's voices** are not in the `.VOC` container: `Vocs.zip` holds 3,066 numbered MP3 files (`0002.mp3` ... `3701.mp3`), i.e. the
  dialog/voice ids index MP3s instead of the original PCM archive (a re-encoding made by the CD ripper's `mp3setup`).

## 26. Videos and INI (twenty-sixth pass)

`FLICS.UHA` ... `FLICS7.UHA` unpack (UHARC inside DOSBox) to standard **FLC files** (`0xAF12` magic, 640x480, 8 bpp): names such as `1910.FLC`, `9110.FLC`,
`9120.FLC`, `9220.FLC`, `9400.FLC`, `9420.FLC`, `9541.FLC`, `9591.FLC` - exactly the clip names `play_cutscene?` passes (section 14), so the `.Q`-first
lookup of section 13 falls through to these FLCs in this release. `LEGEND.INI` carries `MOUSE=`, `PICDATA=`, `VOCDATA=`, `SAVEDATA=` and **`FLCDATA=`** (the video
directory), i.e. five directory keys; `video_open?` builds its path from `FLCDATA`.

## 27. The complete dialog pipeline, validated (twenty-seventh pass)

A dialog id (what `show_dialog?`, `play_video_voiced?` and the cutscene script pass) is the index of a 6-byte descriptor `{w0, w1, w2}` (section 15). **`w1` is not a
string index**: it indexes a table of u32 message ids at `dword_B66F0` (`sub_25B60`; dumped to `retail_dialog_text_ids.tsv`, 4,4xx entries; values below 0x8000
index the table, `w1 >= 0x8000` calls a function from a pointer table instead - texts generated at run time). A message id is `0xF0000000 | block << 16 | index` and
resolves through `SHANSTR.DAT` (block 0 alone holds 3,937 strings; blocks 1-19 hold 2-508 each). `w0 >> 8` is the **portrait/talker index** (the party members use
ids 3-9 = their object ids: 3 Jak, 4 Shella, 5 Brendel, 6 Balinor, 7 Davio, 8 Telsek, 9 Geeka; others are NPC portraits, e.g. 0x0D the Shifter and 0x24 the
Warlock Lord in cutscene 0); `w0 & 0xFF` is a style/layout value (0 or 2 in the examples). `w2` is the next descriptor (0xFFFF / 0x8000 = end).
Verified by reconstructing conversations: dialog 0xC7A (first voiced line of cutscene 0) is the Shifter/Warlock Lord exchange ("Forgive me, Master, for not
destroying Jak Ohmsford..." / "YOU FOOL! THIS MORTAL CREATURE SEEKS TO RESTORE THE SWORD OF SHANNARA...") and 0xB7C the narration "You all cross the Ferry to the
island city of Kern...". The voice for a line is the track with the same dialog id in the voice file (section 17). `scripts/dialog_view.py` prints conversations from
real data. Message texts in the earlier doc sections that were called "message id = (block<<16)|index" are the same ids after the `F0` prefix.

## 28. Help menu and the tutorial replay script (twenty-eighth pass)

The `0x1000` event (F1) runs `help_menu?` (`0x1FE8C`): a `choice_dialog?` titled "Help" with five entries (What is an Adventure game?, On-line tutorial, Keyboard
shortcuts, Helpful Hints, "OK, I've had enough"); the text entries page through SHANSTR block 17 (`0xF011000D`...`0xF0110017`) with `show_help_page?`
(`0x1F984`). The tutorial entry calls `tutorial_start?("shan000.hLP", 1)` (`0x1F7BC`): it **saves the player's game with `save_game?(4)` (this is what `HELP.DAT` is - a
runtime backup, so the copy shipped in the release is a stale leftover from development, which explains its slightly older shape, section 23)**, opens the script,
loads the clean `RESTART` snapshot (`load_game?(5)`) and starts the replay.

`SHAN000.HLP` (67,768 bytes, CRLF text, 2,523 lines) is a **recorded input script**, not data: 2,484 event lines `DDDDD TTTT XXX YYY BBB` plus 38 caption lines.
`DDDDD` = delay (mostly 40 ticks between mouse samples), `TTTT` = event type in hex as in the dispatcher (`0008` mouse move: 2,299 lines; `0002` move: 128; `0004` click
with button `BBB` = 1: 18; `4000` = caption follows; `8000` = end), `XXX YYY` = pointer position. A caption line is `x, y, width, text` (e.g. the opening "Welcome to the on-line
tutorial for Shannara..."). The tutorial therefore plays a canned game session through the normal event path with the pointer driven by the file; a port can replay it
the same way (or skip it).

## 29. Room composition validated on real graphics (twenty-ninth pass)

`scripts/compose_room.py ROOM out.png [--all] [--bg ENTRY]` renders a room from the tables alone (`retail_rooms.tsv` for the background,
the handler's `retail_overlays.tsv` table for the sprites, `PIC` frame offsets for positions) and the result is a correct picture,
which checks the whole chain *room object -> picture id -> overlay table -> sprite entry -> position* against real data:

- Room 88 (the start room) draws entry 1 of `SHAN020.PIC` (a forest). `get_room_pic_id` swaps it for **entry 3** once game flag 22
  (`0x16`) is set - the flag test sits in the hard-coded overrides after `_room_pic_table` - and that second background (willow
  and river) is the one the overlay table was written for: Allanon (object 11, entry 9 at (49, 63)), a creature (object 0, entry 10 at
  (400, 48)), a book, a sword and small props line up exactly with the scene.
- **XOR sprites (`flags & 0x40`) must be composited onto the background they were authored for.** Applied to the wrong background
  (entry 1) the result is coloured noise, which is what a single wrong `bg` index looks like. Index 0 is "unchanged", everything else is
  `destination ^= sprite pixel`; the sprite's palette (`flags & 0x10`) is identical to the background's, so no palette switch is needed.
- Frame offsets: the `(x, y)` read from the start of an entry's frame table are the sprite's position in the 528 x 293 picture.
- Overlay visibility (container / attribute / condition function) really is what `overlay_visible?` says: only the overlays whose
  conditions hold in the story state (Allanon, container 88) appear in the normal render, the others need `--all`.

Whole-game check: rendering the base background of all 67 rooms in `retail_rooms.tsv` (room 643 is the blank placeholder) succeeds for every
room and every image is recognisable scene art (castle gates, interiors, forests, mountains, caves, character portraits); a handful of
dark interiors come out nearly black with the background's own palette, as expected for night/cave rooms.

## 30. The world model re-implemented and checked against the real code (thirtieth pass)

`port/src/world.{h,cpp}` ports the object helpers (`object_get_location?`/`set`, slot, next sibling, list heads, `object_move?`, `object_parent_is`,
`object_is_within`, `object_has_attr`/`set`/`clear`, `bitset_*` on `_game_flags`) and `port/src/savestate.*`, `retail_data.*`, `le.*` read the initial
state straight from `SHANNARA.EXE`. To test it without a game session, `reversing/scripts/retail_emu.py` runs the *real* functions of the executable under
unicorn: it parses the LE container (page map, object table) **and applies its fixups** (the object table's record pointers are stored unrelocated in the file:
`01 58 09 01 00` = type 1, offset 0x10958 in the data object; every internal-reference fixup adds the target object's base), maps the objects at their link
addresses and calls the functions with Watcom register arguments. `port/tests/world_fuzz.py` then executes thousands of random operations on both (invalid ids
and attributes 0-24 included; moves are restricted to avoid cycles, which the original does not check) and compares every result and a hash of all saved object
state plus the flag set after every step: **12,000 operations, 0 mismatches.**

Facts established on the way: there are exactly **24 attribute descriptors** (`0x208A0`-`0x20918`; attribute ids 25+ would read the object table as descriptors
and are undefined); `sub_10F70` (the UI "selection" helper called at the end of `object_move?`) is the only external dependency of the tree operations and
is stubbed as "current selection word at `0xA7E54`"; the executable keeps its object table and most static tables in the *code* object (`0x10000-0x9CCE8`), the
object records themselves in the data object, and the BSS tail of the data object (above `0xCD000`, e.g. `_game_flags`) is zero at start.

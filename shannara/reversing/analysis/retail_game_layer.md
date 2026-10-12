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
| 0x1B, 0x1D, 0x1E | `sub_19DBC`, `sub_1E304` | not yet traced |

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
| 0x1B, 0x1D, 0x1E | `sub_19DBC`, `sub_1E304` | not yet traced |

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

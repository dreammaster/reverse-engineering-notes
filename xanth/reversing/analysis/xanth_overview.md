# Companions of Xanth vs. the Shannara engine - what carries over

Companions of Xanth is an early game on Legend's "new adventure game system" (its tutorial calls it exactly that); Shannara (1995) is the same engine some generations later. This file records what was measured, not guessed. Everything here was checked against `xanth/game/` (retail data) and `xanth/xanth.idb`
(the flattened 16-bit executable).

## 1. Data formats: the same family, validated on Xanth's files

| File | Result |
|---|---|
| `OBJECT.DAT` | same layout as Shannara (`u16 poolSize` + NUL-separated names: `You`, `me`, `Grundy`, `headman`, `Kim`, `Nada`, ...), exact file size |
| `XANTHSTR.DAT` | **parses with Shannara's `shanstr.py` unchanged**: 80 blocks, 5,542 strings, 426-word Huffman tree, 128 phrases, computed end == file size (210,146) |
| `*.FNT` | same 10-byte header + width/spacing tables + 1 bpp glyphs; all 6 fonts match the size formula (`fnt_read.py`) |
| `*.RGN` | same directory + region lists (`rgn_read.py` layout; every list ends exactly at the next offset) |
| `*.HLP` | same recorded-input tutorial script (`00050 0008 165  62   0` lines + captions; the opening caption welcomes the player to the "guided tour") |
| `*.PIC` | same 12-byte directory (`pic_scan.entries`), 320x200 mode (264x122 picture window), **but the pixel data is PKWARE DCL "implode"-compressed** (flags 0x23/0x33/0x43/0x53; XANTH.EXE contains the PKWARE library). `scripts/dcl_explode.py` (a port of blast.c) decodes every entry tested to exactly w*h bytes (XANTH_00/01/02/05/90 = 558 entries); pictures render correctly (`xanth_pic.py`) |
| `*.VOC` | `u16 count` (1327) + `u32` absolute offsets (all 4 KB aligned; the last entry is the file size 190,251,008, so 1,326 tracks, a few of them empty), each track unsigned 8-bit PCM (`0x80` silence) padded with `0x80` to the next 4 KB; the sample rate is not in the file |
| `*.MUS` | **different** from Shannara's XMIDI archive: a 120-slot directory of 12-byte records `{u32 offset; u16 size; u16 a, b, c}` (first record's offset = 1,440 + 1 = start of data), 24 slots used in `XANTH_01/02`, 22 in `_03`, 16 in `_04`. Two records per tune - the engine picks one by a sound-device bit (`word_8912E`), i.e. one MIDI arrangement per device class - each pointing at a **standard MIDI file** (format 1, 120 ticks). Even record: `size` = exact SMF length and `(a,b,c)` small numbers (e.g. 1,3,4; not decoded); odd record: `size` = SMF length + 4, `a=b=c=0`. Verified for every used record in all four files (`scripts/xanth_mus.py`) |
| `*.RS` | RealSound PC-speaker clips (bark, doorbell, ...): 32-byte header (`"STEVE"`, u8 kind 1/2, u8 0x48, u32 length at +7 = file size - 32, constant 21-byte table `CC CC CC CB 33 33 33 33 33 33 8C CC ...`) then 8-bit samples that are plain unsigned PCM of 0..0x44 (centre about 0x20) |
| `XANTH.EXE` + `XANTH.OVL` | RTLink overlay executable (16-bit Microsoft C); `xanth.idb` is the flattened image made with Paul's RTLink tool |

Picture compression across the family: Xanth = PKWARE DCL; Shannara demo = LHA -lh4- (`shannara/reversing/analysis/lzh_decode_spec.md`, decoder
`shannara/reversing/scripts/lzh_lh4.py`, written from that spec and **not yet validated** because no available data uses it); Shannara retail = raw 8 bpp.

## 2. Code: different compiler, same source

Xanth is 16-bit real-mode Microsoft C with overlays; Shannara retail is 32-bit Watcom under DOS/4GW. Instruction-level diffing (BinDiff, mnemonic matching) finds
nothing, as established earlier. What survives recompilation is the source structure: referenced strings, constants, call graph, data references and link order.
`shannara/reversing/scripts/match_engines.py` (+ `extract_func_features.py`, `match_data.py`) uses exactly those:

1. **seeds**: functions that reference the same unique string in both binaries (27 seed pairs: `print_obj`, `serialize_state?`, `go_to_room?`, `help_menu?`, `load_object_dat?`, the verb defaults, ...);
2. **call-graph propagation**: align the ordered callee lists of matched pairs and pair the callees in between; pair sole callers;
3. **link-order gap filling**: between consecutive anchors that are increasing in both address spaces, align the unmatched functions by size (+ constants / library-call overlap);
4. **data anchors**: global variables are paired from matched functions' data-reference lists; a matched global used by one unmatched function on each side pairs those functions.

Later stages added to the matcher: RTLink stub resolution (`call rtlink_thunk; jmp target` is followed so call lists see the real callee), co-occurrence voting
(functions that call the same set of already matched functions), *refine* (replace a pair when a better-supported alternative exists) and *signature groups* (callee/data
signature groups paired by link order). Current result: **391 function pairs kept** (27 string seeds, 255 tier B, 109 tier C; 98 further pairs dropped as too weak) and
**101 data pairs**; hold-out (half of the seeds withheld) gives 18 correct, 3 wrong, 4 missed. The first, simpler run (333 pairs) mispaired several tiny high-fan-in
helpers (`object_has_attr` landed on `wputchar`'s function etc.); the apply script now resets all earlier transferred names before writing the new ones. Quality checks of that first run:

- *hold-out*: withholding half of the string seeds and letting propagation rediscover them gives 19 correct, 2 wrong, 4 missed (about 90 % precision);
- *independent string evidence*: of 55 matched pairs that share a string, 52 agree and 3 are explained by strings living in neighbouring functions;
- *Xanth's own human names*: 97 matches landed on functions that already carry Paul's names, and the large majority agree in meaning (`verbdef_01_Take` = `cmdTake`,
  `init_string_file?` = `Resources_LoadText`, `decode_string?` = `decompressString`, `set_font` = `Font_LoadFont`, `fade_in_palette` = `Screen_FadeIn`, `make_xwindow` =
  `Windows_AddWindow`, `fp_open/read/seek` = `DosFile::open/read/seek`, the object accessor family, the mouse/window/region library ...). About a dozen are wrong
  (e.g. `dispatch_event?` vs `show_options_dialog`, `init_graphics` vs `process_cmd_line`) and are therefore left as comments, never renames;
- the globals pair with Paul's names too (`_xoffset` = `Image_OffsetPos`, `_w_cx` = `Windows_xpos`, `_font` = `Font_activeFont`, `_mouse_pointer_x?` = `Mouse_rawMousePos`, ...).

## 3. What was written into the IDBs

- `xanth.idb` (current numbers are in section 6): **115 functions and about 20 globals renamed** from `sub_xxxx` / `word_xxxx` to the Shannara names with a trailing `?` ("transferred by matching, not
  individually verified"), plus an evidence comment (Shannara address, tier, how it was matched, sizes) on 200+ items. Existing Xanth names are never overwritten -
  those functions only get the Shannara equivalent as a comment. Reproducible from `analysis/function_matches.tsv`, `data_matches.tsv` and `xanth_names.tsv`; apply with
  `bash reversing/scripts/apply_to_idb.sh xanth.idb reversing/scripts/apply_xanth_names.py` (a backup is kept in `reversing/backups`).
- `Shannara.idb`: 16 still-unnamed functions got a comment with Xanth's name (`apply_xanth_hints.py`; e.g. `sub_77FEC` = `PicFile::loadIndex`, `sub_7E2EC` = `Mouse_update`,
  `sub_7B980` = `MouseCursor_SetSmallArrow`, `sub_82400` = `Events_GetKey`).

## 4. What does not carry over (and why that is fine)

Of Shannara's ~560 unmatched non-library functions the largest groups are the **combat engine, the Q video codec, Miles AIL audio, the cutscene/video player and the
journal** - systems Xanth does not have. The framework that is shared (objects, verbs, `print_obj`, string and dialog loading, windows, fonts, regions, mouse, palette,
picture files, saves, timers, help) is what a ScummVM engine for both games needs first, and Xanth is the smaller, simpler game to get it right on. Not matched yet but
expected to exist in Xanth: party handling, `call_object_handler?`, the flag bitsets, `describe_contents?`, the memory manager entry points, the list system.

## 5. BinDiff

Not needed for this step: the custom matcher produced 333 pairs from 27 string seeds, and BinDiff cannot use manual seeds or the string and data evidence. It is still
worth running later as an *independent cross-check* (pairs it finds that disagree with ours are the interesting ones); both IDBs would need a fresh BinExport after the renames.

## 6. Found by reading the Xanth code (2026-10-30)

**Object handler model is identical to Shannara's.** `call_object_handler` (0x1FB8A): `handler_index = object_get_handler_index(obj)`; if `1..0x1C0` it does a far call
through `object_handler_table[handler_index-1]` with the phase as the argument. The table (448 far pointers, linear 0x34F80) points at 10-byte RTLink stubs
(`call rtlink_thunk / jmp target`); all 448 entries are named `objh_NNN` (stub `objh_NNN_stub`) by `scripts/apply_xanth_handlers.py`, same numbering convention as
Shannara (640 handlers there, split in two 512-entry halves; here a single table). A handler takes a phase argument and dispatches on it and on `selectedCommand` (= Shannara's verb) - `objh_010` compares the phase with 4 and 6, then the verb - the same shape as in
Shannara (phase meanings not yet checked against Shannara's) - so the Shannara handler digest and phase documentation (`retail_game_layer.md`) apply directly.

**Savegame layout** (`serialize_state`, matched by string; Xanth 0x41B1B vs Shannara 0x24100), written/read through `save_io_block(ptr, size, count, FILE*, mode)`:

| Part | Shannara retail | Xanth |
|---|---|---|
| header | 0x20 bytes, only for modes 2 (write) / 0xA | same |
| objects | `object_count` entries of 5 bytes `{u8 type; ptr}`; blob size = `size_table[(type-1)*3]` | `object_count` (`word_8580C`) entries of 6 bytes `{u8 type; ?; far ptr}` from offset 0x6C of `object_table_seg`; blob size = byte `[type*3-1]` of `object_sizes_seg` |
| regions | `_save_region_count` x `{ptr, size}` (8 bytes) | `save_region_count` (`word_85336`) x `{far ptr, u16 size}` (6 bytes) from offset 0x116 of `save_regions_seg` |
| tail | `dword_ADDEC` (4 bytes) + a fixed 0x30-byte block | four words (`8B9AE`, `8B9B0`, `8B9AA`, `8B9AC`, refreshed from `8912C/8911A/89128/8912A` first in modes 2/3), then `u16 event_count` and `event_count * 4` bytes (`event_list`) |

So the saved state is "every object's state blob + the registered global regions + the scheduled-event list", and the object-type size table is what a reimplementation
needs to reproduce to read original saves. `schedule_event` caps the event list at 16.

Names and comments established this way are in `analysis/manual_names.tsv` (`scripts/apply_xanth_manual.py`).

## 7. Conversations are compiled into the executable (2026-10-30)

`Conversation_Show(convNum, picNum)` (Paul's name; handlers call it through `show_compendium`, 189 call sites in the digest) is the full-screen conversation scene: fade out,
load the background picture `picNum` (`0x5A04` = file `XANTH_90`, entry 4; the window rectangle comes from the `CompendiumBounds` table), then run a tiny dialogue graph. The graph
is **not in a data file**: it is in the exe's data segments (`seg068`/`seg069`), and the text of every line is a SHANSTR message id into `XANTHSTR.DAT`. Layout (decoded from
`Conversation_Show` and `Conversation_GetEntry`; extractor `scripts/dump_conversations.py`, viewer `scripts/xanth_conversations.py`, result `analysis/xanth_conversations.tsv`):

| Item | Layout |
|---|---|
| simple line (`convId < 0x40C`, 1,036 of them) | `{u16 strIdx; u16 next}` at `dword_3B314[id*4]`; enabled flag = bit 0 of `Conversation_ENTRIES1[id]`; `next` = another line, a menu (`0x8000 | n`) or `0xFFFF` (end) |
| menu (`convId = 0x8000 | n`, 400 of them) | `{u8 count; u8 b1; u16 first; u16 callback}` at `Conversation_LOOKUPS[n*6]`; option `j` = `{u16 strIdx; u16 next}` at `unk_3C858[(first+j)*4]`, enabled flag = bit 0 of `Conversation_ENTRIES2[first+j]` (211 options start disabled); per-menu flag in `ENTRIES3[n]`; 1,300 options in all |
| callback | `dword_3DCA4[callback]` (82 distinct functions) is called with `(convId, first+j)` after the player picks option `j`, then the walk continues at that option's `next`; this is where the game reacts (enable follow-up options, set flags, move objects) |
| text | `strIdx < 0x8000`: `seg068[strIdx*4]` is `0xF0BBIIII` (SHANSTR block `BB`, index `IIII`); `strIdx >= 0x8000`: `seg069[(strIdx-0x8000)*4]` is a far pointer to a function that builds the text (66 simple lines do this) |
| state | `ConversationEntry` flag bytes are what `conversation_set_enabled(convId, subId, on)` flips (bit 0); the byte arrays are runtime state (the savegame saves object/region blobs, so they are probably registered as save regions - not checked) |

The first simple line (`line 0 -> menu 0`) is the opening phone call (the caller's answers are menu options). Text view with the lines resolved (local only, `xanth/game_out/conversations.txt`) is produced by
`xanth_conversations.py conv.json XANTHSTR.DAT ../../game_out`. This replaces what Shannara does with its 6-byte dialog descriptors (`retail_dialog_table.tsv`): the same idea (portrait/state + message id + next), Xanth just keeps the whole graph in the exe.

Other facts established while naming the handler helpers: `selectedCommand` is the verb (enum `CMD` in `analysis/xanth_enums.tsv`: 92 verbs, `TAKE` = 68, `LOOK_AT` = 19, `TALK_TO` = 69, ...), `dobj` / `iobj` (`word_8B9C4` / `word_8B9C2`) the command's objects, `wait_ticks_or_click` the
pause between scripted lines, `event_pending` / `cancel_event` / `schedule_event` the 16-slot timed-event queue (4-byte records, id first), `describe_contents` the "... holding ..." printer, `lock_combination_correct` the dial puzzle (bytes `8B9D2..8B9D8` = 2,1,3,1,2,1,1).
Handler digest: `analysis/xanth_handler_digest.txt` (528 functions as path-structured pseudo-code, `scripts/extract_xanth_digest.py`).

## 8. Object table (2026-10-30)

498 objects (`object_count`), ids 1..498, initial state in the exe's data (`scripts/dump_objects.py` -> `scripts/xanth_objects.py` -> `analysis/xanth_objects.tsv`, which also resolves names from `OBJECT.DAT`).
Table: 6-byte entries `{u8 type; u8 pad; u16 recOffset; u16 recSegment}` at `seg_8A9E4*16 + 0x66 + id*6` (savegame loop starts at `+0x6C`); records are packed in a data segment with sizes
**18 / 28 / 32 / 14 / 32 bytes for types 1..5** (record spacing; every accessor offset fits). Types: **1 = rooms** (70), **2 = portable items** (326), **3 = containers/furniture** (69; things that hold items, e.g. desk, sink),
**4 = scenery props** (14: door, path, bush, ocean), **5 = characters** (19: You, Grundy, headman, Kim, Nada, ...).

| Field | Type 1 | Type 2 | Type 3 | Type 4 | Type 5 |
|---|---|---|---|---|---|
| name offset (word, 1-based into `OBJECT.DAT` pool) / alt name | +0 / +2 | +0 / +2 | +0 / +2 | +0 / +2 | +0 / +2 |
| location (parent object) | - | +8 | +8 | - | +6 |
| slot (byte; which child list of the parent) | - | +0Ah | +0Ah | - | +8 |
| next sibling | - | +0Ch | +0Ch | - | +0Ah |
| handler index (`objh_NNN`) | +0Ah | +1Ah | +1Eh | +0Ch | +1Eh |
| default verb | - | +16h | +1Ah | +8 | +1Ah |
| points | +8 | +12h | +16h | - | - |
| `prop_a` | +0Ch | +14h | +18h | - | +18h |

Checks: all 492 objects with a handler use indices 1..448 and 448 distinct values - exactly the size of `object_handler_table`; every location is a valid object of the expected type (items in rooms or containers,
characters in rooms); characters default to verb `TALK_TO` (69). The model is Shannara's (rooms -> containers -> items as typed child lists, `object_move`), with 16-bit records. The `bitset_test/set/clear` flag primitives of
Shannara do not exist here: Xanth keeps its game flags as plain bytes/words (`byte_852B3`, `byte_852C0`, ...), which the handlers read and write directly (visible all over the digest).

`call_object_handler` is driven by `run_command` exactly as in Shannara (phases 1-2 for the actor/room objects, 3/4 for one command object and its container, 5/6 for the other); in Xanth the iobj gets phases 3 and 4 and the
dobj 5 and 6 (checked in `run_command`). Shannara's `run_command?` is the same (dx = dobj is stored in `dword_CD92C` and gets phases 5/6; bx = iobj in `dword_CD924` gets 3/4); the earlier Shannara note that listed 4 = dobj and 6 = iobj was wrong and has been corrected. Handler bodies agree: e.g. a boulder's phase-4 PUT branch tests `dobj` (the thing being put on it), its phase-6 branches act on `dobj` = itself.
Digest headers (`xanth_handler_digest.txt`) now list the objects that use each `objh_NNN`.

## 9. Room exits (2026-10-30)

152 exits over the 70 rooms (`analysis/xanth_exits.tsv`; `scripts/dump_exits.py` + `scripts/xanth_exits.py`), read from `sub_1EB3A` (the movement routine behind walk/enter). A room record (type 1) has `+0Eh` = exit count and `+10h` = 1-based index of
its first exit; exit `i` is 4 bytes `{u8 direction; u8 kind; u16 arg}` at `seg_8AA12*16 + 0x10FA + 4*(i-1)`. Direction ids (word table beside the clock code): 1 north, 2 northeast, 3 east, 4 southeast, 5 south, 6 southwest,
7 west, 8 northwest (9 up, 10 down inferred). Kinds: **1** = plain exit (`arg` = target room, 72 of them); **2** = door (`arg` indexes a table of `{targetRoom, doorObject}` at `seg_8AA14*16 + 0x84E`; passes when the door has
attribute 0xE = open, otherwise "... isn't open"; 3 of them); **3/4** = computed (77 of kind 4): `sub_1FB15(arg, 0xF)` calls `exit_function_table[arg]` (56 far pointers, named `exit_fn_NN`) which returns the target room, 0 = handled
(it printed something), `0xFFFF` = blocked; with any other phase value the same functions answer the default target (the digest shows `phase==0xF` / `phase:other`). Walking into a room (`go_to_room`) is shared by exits, `cmdGoto` and the
story scripts (58 explicit `go_to_room` calls in handlers, mostly to the special room 90 "screen" used for cutscenes/conversations with `word_85268` as the return selector).
Also named: `current_room` (object id of the player's room), `player_obj` (initially 1 = You), `actor_obj` (who performs the current command).

## 10. Voice (XANTH_01.VOC) (2026-10-30)

Xanth is a CD-talkie: `voc_init` (0x23D80) opens `XANTH_01.VOC`, reads the `u16` track count (1327) and the `u32` offsets into `voc_offsets`; track *N* is the recording of conversation line *N*
(`voc_track_length(convId)` = `offsets[N+1] - offsets[N]`, > 0 means the line is voiced). `Conversation_Show` sets `voice_track` to the line's convId (or 0xFFFF), shows the text only when voice is off or
`word_8B9DA` says so, and calls `voc_play_track(track, callback)`. Scripted sequences use explicit tracks above the 1,036 conversation lines (0x47C, 0x4C0, 0x525-0x528, 0x52D, ...) up to the last track 1325.
Each track is raw unsigned 8-bit PCM at the 4 KB-aligned file offset, padded with 0x80. **The sample rate is not stored in the file.** Validation of the line -> track mapping against the data: 932 of the 1,036 simple lines have a non-empty track, and over 860 lines clip length (trailing `0x80` padding stripped) correlates with the
resolved text length with r = 0.91; the median is about 1,430 bytes per character, i.e. about 22 kHz for normal speech (roughly 15 characters per second) - so 22050 Hz is the likely rate (the SoundBlaster driver programs it from a table that
is not yet traced). A reimplementation needs no executable data to find clips - only the convId -> line mapping from `xanth_conversations.tsv`.

## 11. Game state = object records + 205 save regions (2026-10-30)

`serialize_state` writes, after the 0x20-byte header, (a) every object's record prefix, (b) the **save-region table**: 205 `{far ptr, u16 size}` entries (`save_regions_seg + 0x116`, count `save_region_count`), 4,522 bytes in all
(`analysis/xanth_save_regions.tsv`, from `scripts/dump_save_regions.py`), then the scheduled-event list. The regions are the whole mutable world besides the object records:

- 2,736 bytes are the **conversation flag arrays** (`Conversation_ENTRIES1/2/3` = 1,036 + 1,300 + 400 bytes, bit 0 = line/option enabled), so conversation progress is saved state;
- `current_room`, `word_8B9DA`/`word_8B9DC` (the *show text with voice* / *voice on* options - options travel inside savegames), the run-time words around `0x85216-0x8526E`;
- about 130 single **flag bytes** at `0x852B2-0x85334` (`byte_852B3`, `byte_852C0`, ... - the puzzle and story flags that the handlers test; no bitset helpers in this engine) and ~30 flag words;
- `unk_34628` (1,250 bytes, next to the object-type tables: probably the typed child-list heads of the containment tree), `word_81A26` (64 bytes), `unk_87C50` (180 bytes), `unk_87C3E` (18), `unk_8823C` (12), `byte_8B9D2` (the 7-byte lock combination).

Everything a savegame needs is therefore enumerable from two tables, which is what makes a faithful, save-compatible reimplementation tractable. Naming the ~130 flag bytes from the handler digest (where each one is set) is the next big readability step.

## 12. Object attributes (2026-10-30)

`object_has_attr/set_attr/clear_attr(obj, attr)` work on a bit array inside the object record: `object_attr_locate` uses a 4-byte descriptor per attribute id at `0x33A1C + 4*id` = `{u8 bitOffset; u8 pad; u16 typeMask}`
(the attribute exists only if `(typeMask >> (type-1)) & 1`), the bit is `record + 4 + bitOffset/8`, bit `bitOffset%8`. 23 attributes are defined (ids 1-23; 24 has an empty mask). The initial settings of every object are in the new
`attrs` column of `xanth_objects.tsv` (dump: `dump_objects.py`). Meanings, from the default verbs (`cmdOpen`, `cmdTake`, `cmdLookAt`, `go_to_room`) and from which objects carry them:

| id | meaning | evidence |
|---|---|---|
| 2 | proper name (no article: "Kim", "You") | on 11 of 19 characters, exactly the named ones; `print_obj` article logic |
| 5 | "has been handled / scored once" | set by `cmdTake`, `cmdOpen`, room entry after the first points are awarded |
| 7 | normal visible item (listed in room descriptions) | 90 items, 11 containers |
| 8 | animate/other-grammar flag - on all 19 characters and 26 items | consulted in `cmdGive`/`print_obj` |
| 9 / 21 | room properties (61 / 39 rooms; 21 = enclosed: `cmdOpen` on the current room says "isn't closed") | rooms only |
| 10 | generic per-object story flag (set 157 times, cleared 38, tested 611 times by object handlers: switches, "done" bits) | no fixed meaning |
| 11 | surface: contents are "on" it ("On the desk you see ...") | `cmdLookAt`, `go_to_room` ("on" / "in") |
| 12 | portable (`cmdTake` refuses without it) | letter, bracelet, Post-It |
| 13 | contents visible while closed (transparent) | `cmdLookAt`, `cmdOpen` |
| 14 | open | `cmdOpen` sets, `cmdClose` clears; 327 tests |
| 15 | openable | `cmdOpen` ("you can't open that" without it); envelope, window |
| 17 | container | 38 containers, 14 items |
| 22 | female | exactly the female characters (Kim, Nada, Jenny, Metria, ogress, ...) - pronoun choice in `print_obj` |
| 23 | person/humanoid (speaks) | 16 of 19 characters |
| 1, 3, 4, 18, 19, 20 | not identified yet | |

Also named: `follower_obj` (the object that follows the player between rooms: `go_to_room` moves it into the new room and gives it handler phase 0x1A). Room-entry sequence in `go_to_room`: handler phase 0xF for the old room (leaving; non-zero cancels), `current_room` changes,
the player and followers are moved, `update_room_light`, `set_room_music`, phase 0xD (entering, before the description), `describe_room`, phase 0xE (after the description); the first visit awards the room's points (`object_get_points` -> `add_score`) and sets attribute 5.

## 13. Room music (2026-10-30)

`set_room_music(room)` looks the room up in a 66-entry table (`analysis/xanth_room_music.tsv`, `scripts/dump_music.py`) of `{u16 room; u16 musicId; u8 volume; u8 loop}` (all volume 127, looped) and calls `play_music_entry(musicId, volume, loop)`.
**`musicId = (file number << 8) | tune`**: `0x105` = tune 5 of `XANTH_01.MUS`, `0x204` = tune 4 of `XANTH_02.MUS`, ...; the tune numbers used (1-12, 1-12, 1-11, 1-8) fit the tune counts of the four archives exactly (12/12/11/8 tunes = 24/24/22/16
directory records). Rooms 0x22 (bedroom), 0x40 and 0x51 bypass the table and play `0x307` (volume 0x7F, loop); rooms not in the table fall back to `0x302` / `0x307` depending on a sword flag. `play_music_entry` first checks the sound-device bits
in `word_8912E` (bits 1-2 and 5-6, mapping to the two MIDI arrangements per tune) and remembers the current tune in `word_8911A` / `word_8912C` (both saved in the savegame tail).

## 14. Room pictures (2026-10-30)

`describe_room(room)` asks `sub_1DB4E` for the room's picture and redraws the scene (`RoomItem_Draw(0x20)`) only when it differs from the current `picNum`. The table (`analysis/xanth_room_pics.tsv`, `scripts/dump_roompics.py`) has 68 entries
`{u16 room; u16 picNum}` at `seg044:0x7F8`, **`picNum = (XANTH_nn.PIC file number << 8) | entry`** (all 68 resolve to existing entries; the same format is used by `Conversation_Show(convNum, picNum)` and the other `show_compendium` calls, e.g.
`0x5A04` = `XANTH_90.PIC` entry 4). Room 0x1F0 chooses between two pictures (0x123E / 0x123F) by whether object 0x8B is held. Checked visually: room 34 (bedroom) -> `XANTH_02.PIC` entry 0 renders as the window, pen holder and computer that the
object table puts in that room. Pictures are 264x122 (the picture window) and carry their own palette.

## 15. Scenes: pictures + clickable regions (2026-10-30)

`XANTH_nn.RGN` has Shannara's layout (`rgn_read.py`: 6-byte directory `{u16 dataOffset; u16 baseId; u16 count}` per entry of the same-numbered `.PIC`; rectangles and polygons in picture-window pixels), and in Xanth the **region id is simply the object id**
and `baseId` the room's object id: `XANTH_02.RGN` entry 0 (the bedroom picture, `baseId` 34) has regions 35 desk, 45 window, 48 bookshelf, 46 pen holder, 47 pencils, 39 personal computer, 40 computer screen ... exactly the objects the object table places in room 34. Joining the room
picture table, the region files and the object table gives **716 hotspots over the 70 rooms** (`analysis/xanth_scenes.tsv`, `scripts/xanth_scenes.py`; three rooms share a picture entry with a different `baseId`). Clicking in a scene is therefore a point-in-polygon test
that yields an object id, which feeds `run_command(verb, dobj, iobj)`. With the object table, exits, handlers, conversations, voice, music and save state, the data a ScummVM `AdventureEngine` needs for the Xanth world is now fully located.

## 16. Cutscenes, RealSound (2026-10-30)

There is no video: `showCutscene(n)` (Paul's names throughout) is a 11-way jump table of code-driven sequences built from `Conversation_Show`, pictures and `wait_ticks_or_click`: 0 `intro` (title + `showIntroConv`), 1-4 `demonConv1..4`, 5 `demonEndgame`, 6 `meetingKim`,
7 `kimMeetsMerman`, 8 `kimRafting`, 9 `kimGapChasm`, 10 `merCouple`. Triggers seen in handlers: censer (149) -> demonConv1, telephone (65) -> demonEndgame, spring (127) -> kimMeetsMerman, mirror (213) -> kimRafting, sewer (412) -> merCouple.
`*.RS` files are loaded by `realsound_load` (only for certain sound-device bits of `word_8912E`; played through `sub_23BB1`) in chunks of at most 0xFFF0 bytes, up to 7 chunks per file. The earlier 'bit/nibble pattern' reading was wrong: the `CC`/`33` runs are part of a constant 32-byte header (the same bytes are the template `unk_3EC20` that `voc_play_track` -> `sub_23D57` memcpy's in front of every voice buffer, kind byte 6 there), and the data from offset 0x20 is smooth unsigned PCM (BARK: 0..68, mean 32; THUNDER: 0..68; the largest sample step is 58) whose maximum equals the header byte 0x48 = 72: the samples are PC-speaker PWM duty values for a timer period of 72 PIT ticks, i.e. about 1,193,182/72 = 16.6 kHz (the kind byte 1 or 2 is probably a repeat factor, so 8.3 kHz is possible; not confirmed because the player code behind `word_891BA` (`sub_10C98` selects start/stop routines per device 0-4) sits in a driver segment the IDB has not disassembled). A port can play them as 6-bit PCM at 16572 Hz (scale by 4).


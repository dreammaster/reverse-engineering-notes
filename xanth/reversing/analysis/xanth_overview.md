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
| `*.RS` | RealSound-style PC-speaker clips (bark, doorbell, ...): header `53 54 45 56 45 02` ("STEVE" + version 2) then two words (`0x48`, `0x46` in `BARK.RS`) and packed 4-bit-looking sample data; format not decoded (PC-speaker only, low priority for a ScummVM port) |
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
dobj 5 and 6 (checked in `run_command`), whereas Shannara's notes list 4 = dobj and 6 = iobj - one of the two readings is the wrong way round; worth re-checking in Shannara's `run_command` when its handlers are read.
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

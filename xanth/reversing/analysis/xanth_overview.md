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

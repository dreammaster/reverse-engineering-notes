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
| `*.VOC` | `u16 count` + `u32` absolute offsets (4 KB aligned, last = file size 190,251,008), unsigned 8-bit PCM (`0x80` silence); 1,327 tracks in `XANTH_01.VOC` |
| `*.MUS` | **different** from Shannara's XMIDI archive: starts `A1 05 00 00 ...` and contains 24 standard MIDI (`MThd`) files with a directory scheme not yet decoded |
| `*.RS` | small sound-effect files (bark, doorbell, ...), not studied |
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

Result: **333 function pairs** (27 string seeds, 163 tier B, 81 tier C, 62 dropped as too weak) and **98 data pairs**. Quality checks:

- *hold-out*: withholding half of the string seeds and letting propagation rediscover them gives 19 correct, 2 wrong, 4 missed (about 90 % precision);
- *independent string evidence*: of 55 matched pairs that share a string, 52 agree and 3 are explained by strings living in neighbouring functions;
- *Xanth's own human names*: 97 matches landed on functions that already carry Paul's names, and the large majority agree in meaning (`verbdef_01_Take` = `cmdTake`,
  `init_string_file?` = `Resources_LoadText`, `decode_string?` = `decompressString`, `set_font` = `Font_LoadFont`, `fade_in_palette` = `Screen_FadeIn`, `make_xwindow` =
  `Windows_AddWindow`, `fp_open/read/seek` = `DosFile::open/read/seek`, the object accessor family, the mouse/window/region library ...). About a dozen are wrong
  (e.g. `dispatch_event?` vs `show_options_dialog`, `init_graphics` vs `process_cmd_line`) and are therefore left as comments, never renames;
- the globals pair with Paul's names too (`_xoffset` = `Image_OffsetPos`, `_w_cx` = `Windows_xpos`, `_font` = `Font_activeFont`, `_mouse_pointer_x?` = `Mouse_rawMousePos`, ...).

## 3. What was written into the IDBs

- `xanth.idb`: **115 functions and about 20 globals renamed** from `sub_xxxx` / `word_xxxx` to the Shannara names with a trailing `?` ("transferred by matching, not
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

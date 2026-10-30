# Xanth reversing roadmap

Goal: use Companions of Xanth (the older, smaller game on the same engine) as the first ScummVM target and as a cross-reference that shortens the Shannara work.
Workflow is the same as `shannara/reversing/notes/roadmap.md`: work on a copy of the IDB (`scripts/apply_to_idb.sh`), keep backups in `reversing/backups/`, game data in
`xanth/game/` (git-ignored), exports such as `xanth.asm` git-ignored.

## Done (2026-10-30)
- Data formats cross-checked (`analysis/xanth_overview.md`): OBJECT.DAT, XANTHSTR.DAT, FNT, RGN, HLP identical; PIC = same directory + PKWARE DCL pixels
  (decoder `scripts/dcl_explode.py`, reader `scripts/xanth_pic.py`); VOC directory.
- Function/data matching against Shannara retail and name transfer into xanth.idb (333 pairs, 98 data pairs, 115 renames); 16 hints back into Shannara.idb.

## Next
1. `*.MUS` directory (24 standard MIDI files, header `A1 05 00 00`), `*.RS` sound effects, `XANTH_01.VOC` PCM clips: decode and verify.
2. Grow the match: manual seeds, then read the unmatched Xanth framework functions directly (party, flag bitsets, handler dispatch, memory manager, lists).
3. Xanth object table and savegame layout (compare with Shannara's 5-byte table + `_save_regions`); handler model (compiled code behind tables as in Shannara?).
4. Rename Xanth structs from the Shannara ones (Pic, Region, Font, MemoryBlock) once layouts are confirmed (16-bit sizes differ).
5. Re-run the whole pipeline (`extract_func_features.py` on both IDBs -> `match_engines.py` -> `match_data.py` -> `make_xanth_names.py` -> apply) whenever either IDB gains names.

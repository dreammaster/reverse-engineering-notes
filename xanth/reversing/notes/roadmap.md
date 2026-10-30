# Xanth reversing roadmap

Goal: use Companions of Xanth (the older, smaller game on the same engine) as the first ScummVM target and as a cross-reference that shortens the Shannara work.
Workflow is the same as `shannara/reversing/notes/roadmap.md`: work on a copy of the IDB (`scripts/apply_to_idb.sh`), keep backups in `reversing/backups/`, game data in
`xanth/game/` (git-ignored), exports such as `xanth.asm` git-ignored.

## Done (2026-10-30)
- Data formats cross-checked (`analysis/xanth_overview.md`): OBJECT.DAT, XANTHSTR.DAT, FNT, RGN, HLP identical; PIC = same directory + PKWARE DCL pixels
  (decoder `scripts/dcl_explode.py`, reader `scripts/xanth_pic.py`); VOC directory.
- Function/data matching against Shannara retail and name transfer into xanth.idb (333 pairs, 98 data pairs, 115 renames); 16 hints back into Shannara.idb.

- `*.MUS` directory decoded and verified on all four files (`scripts/xanth_mus.py`); `*.VOC` directory/track layout verified; `*.RS` identified (header "STEVE", 4-bit-ish samples).
- Matcher refined (stub resolution, co-occurrence, refine, signature groups): 391 pairs kept, 101 data pairs; earlier wrong `?` renames reset and re-applied.
- Handler model confirmed identical; all 448 `objh_NNN` named (`scripts/apply_xanth_handlers.py`); savegame layout compared (see overview section 6).

## Next
1. Read the Xanth handlers and port the Shannara handler digest/story-event extraction to Xanth (`extract_handler_digest.py` assumes the 32-bit table; needs a 16-bit far-pointer variant).
2. Still unmatched/unverified: flag bitset helpers (`bitset_test/set/clear` not found by idiom search - may be inlined), party handling, `get_event`, memory manager (`new_handle`, `lock_handle`, ...), list system, `describe_contents?`, `show_dialog?`, `speak?`.
3. `*.RS` sample format and `*.VOC` sample rate / which game event plays which track (voice index linkage, as `retail_game_layer.md` section on voice for Shannara).
4. Rename Xanth structs from the Shannara ones (Pic, Region, Font, MemoryBlock) once layouts are confirmed (16-bit sizes differ).
5. Re-run the whole pipeline (`extract_func_features.py` on both IDBs -> `match_engines.py --cooc 3 0.9` -> `match_data.py` -> `make_xanth_names.py` -> apply) whenever either IDB gains names; the matcher treats a trailing `?` on the Xanth side as automatic.

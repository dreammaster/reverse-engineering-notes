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

- Conversation system (compiled-in dialogue graph, 1,036 lines + 400 menus) and the 498-object table decoded; handler digest (528 functions) with owning objects.

- Exits (152, kinds 1-4, 56 computed `exit_fn_NN`), voice (VOC directory = convId; clip length vs text r=0.91, ~22 kHz), save regions (205 regions / 4,522 bytes), 82 conversation callbacks and 85 dynamic line texts named and in the digest (751 functions); `analysis/xanth_flags.tsv` cross-references every saved variable with the handlers that write/test it.

- Scenes (716 hotspots, region id = object id), room pictures (68), room music (66), object attributes (23), cutscene list: all data needed to *render and navigate* the Xanth world is located (overview sections 8-16).

## Next
1. Write per-room/NPC story summaries from the digest (puzzles, flag logic) and name the remaining ~80 flag bytes meaningfully (they currently carry `flag_ADDR_<first writer>` labels).
2. Resolve the handler phase order question (iobj 3/4, dobj 5/6 in Xanth vs the opposite in Shannara's notes) by reading Shannara's `run_command?`.
3. Decode `*.RS` and the SoundBlaster sample-rate table (low priority); trace `play_music_entry` -> device selection for the two MIDI arrangements.
4. Port the verified decoders (DCL, PIC, RGN, MUS directory, SHANSTR, VOC directory) to C++ and sketch the engine around `run_command` / `call_object_handler` / `Conversation_Show`.
5. Re-run the pipeline (`extract_func_features.py` -> `match_engines.py --cooc 3 0.9` -> `make_xanth_names.py` -> apply scripts) whenever either IDB gains names; apply order: `apply_xanth_names.py`, `apply_xanth_handlers.py`, `apply_xanth_manual.py`, `apply_xanth_flags.py`.

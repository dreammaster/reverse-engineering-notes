# Shannara (Legend Entertainment, 1995) - reverse engineering

Goal: understand the engine well enough to re-implement the game (ScummVM style). The retail game is a 32-bit DOS/4GW (Watcom C) executable;
the demo (`DEMO.LE`) is a smaller build of the same engine.

## Layout

| Path | What |
|---|---|
| `reversing/analysis/retail_game_layer.md` | the master write-up (33+ sections): object model, handlers, verbs, rooms, savegame, data files, formats, oracle, recompiler |
| `reversing/analysis/*.tsv` | generated tables: 744 objects, 640 handlers, 78 verbs, rooms/pictures/music, overlays, save regions, function map, events |
| `reversing/analysis/*_spec.md`, `retail_combat.md` | codec specs (Q video, LHA -lh4-), combat |
| `reversing/notes/roadmap.md` | status and next steps (read first) |
| `reversing/scripts/` | IDAPython rename/export scripts (`apply_*.py`, `unfold_functions.py`, `dump_insns.py`), readers, the unicorn oracle (`retail_emu.py`, `retail_game.py`), the recompiler (`transpile_handlers.py`) |
| `reversing/scripts/pseudo_c.py` | readable pseudo-C (calls with resolved arguments, conditions as expressions) for any function set - local reading aid for hand-porting (`game_out/handlers_pseudo.c`, git-ignored) |
| `port/` | C++17: resource readers, world model, `print_obj`, savegame, **transpiled engine core**, tests - see `port/README.md` |
| `Shannara.idb`, `Shannara Demo.idb` | IDA databases (git-ignored); `Shannara.asm` local text export (git-ignored, 15 MB) |
| `game/`, `demo/`, `game_out/` | the game's own files and local extractions (git-ignored) |

## Pipeline in one picture

```
SHANNARA.EXE --LE loader + fixups--> memory image
   |-- IDA (names, structures, scripts/apply_*.py)             -> analysis docs and TSVs
   |-- retail_emu.py / retail_game.py (unicorn, UI stubbed)    -> "oracle": real responses to commands
   '-- dump_insns.py -> transpile_handlers.py -> port/gen/*.cpp -> engine_tool / play_tool (native, no emulator)
                                  verified against the oracle: port/tests/engine_diff.py (~30,000 random operations, 0 mismatches)
```

## What is established (all checked against the real files / real code)

- Formats: SHANSTR.DAT, OBJECT.DAT, PIC/RGN/FNT, MUS (XMIDI), Q video, FLC, savegame/HELP.DAT, voice archive; C++ readers with differential tests.
- Game logic: 744 objects (rooms, items, characters...), 640 compiled object handlers + 78 verb defaults reached through function-pointer tables, lifecycle phases,
  room overlays, party/journal/score systems, the 80-verb vocabulary.
- A native C++ engine core (statically recompiled game layer) that reproduces the original's text and state changes, and a text front end (`play_tool`).

## Reproduce

```
bash reversing/scripts/apply_to_idb.sh Shannara.idb reversing/scripts/apply_retail_objects.py --export Shannara.asm   # names (idempotent)
python reversing/scripts/make_retn_map.py Shannara.asm game_out/retn_map.txt
# dump_insns.py over the game-layer functions (see port/README.md) -> game_out/insns_all.json
python reversing/scripts/transpile_handlers.py game_out/insns_all.json port/gen game_out/retn_map.txt
bash port/build_engine.sh
python port/tests/verify.py && python port/tests/engine_diff.py 3000 1
port/build/play_tool.exe game/SHANNARA.EXE game
```

Caveats: no combat, save/load, cutscene or dialog popups in the headless engine (stubbed); generated C++ is derived from the game code and is kept out of git;
see the roadmap for the list of open items.

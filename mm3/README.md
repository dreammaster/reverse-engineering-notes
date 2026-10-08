# Might and Magic III: Isles of Terra (DOS) -- reverse engineering

* `ida_scripts/rebuild_all.ps1` rebuilds `mm3.idb` from the installed game (unpack -> IDA -> names, structs, video module, strings).
  `names/mm3.tsv` holds every name/comment (`export_names.py` / `apply_names.py`).
* `tools/` -- `unpack_mm3.py` (EXE), `mm3_cc.py` (`.CC` archives, LZHUF), `mm3_monsters.py`, `mm3_events.py` (event script disassembler),
  `mm3_map.py` (ASCII map), `mm3_chars.py` (roster listing), `mm3_gfx.py` (sprites/screens -> PNG), `mm3_music.py` (song walker), `mm3_samples.py` (digitised sounds -> WAV), `mm3_viewflags.py` and `mm3_scenelist.py` (renderer flag tables), `mm3_fx.py` (AdLib effect streams).
* `docs/` -- `xeen-comparison.md` (MM3 vs Xeen), `mm3-re.md` (packing, `.CC`, graphics codec; original notes), `exe-layout.md` (unpacking, overlays), `overview.md` (code map,
  Xeen comparison), `data-files.md` (saves, maps, events, party block), `character.h`, `items.md`, `spells.md`, `monsters.md`, `town.md`,
  `view.md`, `video-module.md`, `engine-loop.md`, `text-files.md`, `music.md`, `rules.md` (class requirements, experience, HP/SP, prices, damage and saving throws, training/inn/tavern/guild/smithy).

* `src/` -- a **playable C + SDL2 port** that loads the original data files at run time (nothing of the game is in this repository except the
  data you copy to `data/`): `cd src && make mm3game && ./mm3game ../data` (`../data` = a folder with `MM3.EXE`, `MM3.CC`, `MM3.CUR` and the derived
  `DGROUP.BIN`/`IMAGE.BIN` made by `tools/mm3_dgroup.py` / `tools/mm3_image.py`).  Mouse and keyboard as in the DOS game, music and effects through the original
  AdLib driver (run in an 8086 interpreter) and an OPL2 emulator, saved games in `./mm3-saves`.  See `src/README.md` for the architecture: the
  game logic is statically translated from the disassembly (`tools/asm2c.py`) and is being replaced piece by piece with readable C that is
  verified against the translation (`src/logic.c`, `src/rules.c`; `tests/difftest.sh`).

The program is the same engine generation as Xeen (Borland C++ 1991, VROOMM overlays): the ScummVM Xeen source
(`engines/mm/xeen`) is the best commentary for most of the code; the documents say where MM3 differs.

## Status (what is understood)

Decoded and documented (details in `docs/`): executable packing and overlays; `.CC`/`.CUR` archives; the maze files (cells, header bytes, events with all value modes, monster/object records, passwords); the character record
and its rules (stats, HP/SP, experience, skills, equipment, saving throws, damage); items (prices, generation, class restrictions, smithy stock); the 77 spells (cost, school lists, attack parameters, many effects); monsters
(table, special attacks, targeting, to-hit, movement, loot, status-spell susceptibility); the town buildings; time, day/night, resting; the exploration loop and key table, combat round, music/sound entry points.
Known gaps: in the renderer, the full frame list of the far-row wall sheet (the draw-list format, its consumer -- the video module's text printer -- the wall style/flag/frame scheme and the environment graphics loading are decoded); the details of the creation screen (its buttons, roll and record are documented); the musical meaning of the individual effect streams (they decode with `mm3_fx.py`); how hirelings are recruited.

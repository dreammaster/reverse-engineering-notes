# Might and Magic III: Isles of Terra (DOS) -- reverse engineering

* `ida_scripts/rebuild_all.ps1` rebuilds `mm3.idb` from the installed game (unpack -> IDA -> names, structs, video module, strings).
  `names/mm3.tsv` holds every name/comment (`export_names.py` / `apply_names.py`).
* `tools/` -- `unpack_mm3.py` (EXE), `mm3_cc.py` (`.CC` archives, LZHUF), `mm3_monsters.py`, `mm3_events.py` (event script disassembler),
  `mm3_map.py` (ASCII map), `mm3_chars.py` (roster listing), `mm3_gfx.py` (sprites/screens -> PNG), `mm3_music.py` (song walker).
* `docs/` -- `mm3-re.md` (packing, `.CC`, graphics codec; original notes), `exe-layout.md` (unpacking, overlays), `overview.md` (code map,
  Xeen comparison), `data-files.md` (saves, maps, events, party block), `character.h`, `items.md`, `spells.md`, `monsters.md`, `town.md`,
  `view.md`, `video-module.md`, `engine-loop.md`, `text-files.md`, `music.md`, `rules.md` (class requirements, experience, HP/SP, prices, damage and saving throws, training/inn/tavern/guild/smithy).

The program is the same engine generation as Xeen (Borland C++ 1991, VROOMM overlays): the ScummVM Xeen source
(`engines/mm/xeen`) is the best commentary for most of the code; the documents say where MM3 differs.

## Status (what is understood)

Decoded and documented (details in `docs/`): executable packing and overlays; `.CC`/`.CUR` archives; the maze files (cells, header bytes, events with all value modes, monster/object records, passwords); the character record
and its rules (stats, HP/SP, experience, skills, equipment, saving throws, damage); items (prices, generation, class restrictions, smithy stock); the 77 spells (cost, school lists, attack parameters, many effects); monsters
(table, special attacks, targeting, to-hit, movement, loot, status-spell susceptibility); the town buildings; time, day/night, resting; the exploration loop and key table, combat round, music/sound entry points.
Known gaps: the renderer's record writers (`sub_17439`, `sub_17F38`, `sub_1862A`, `sub_18BF1`, `sub_1DB3D`) and the video module's scene consumer, per-object chest loot,
`moveMonsters`' exact pathing, character creation screens, the AdLib effect data tables.

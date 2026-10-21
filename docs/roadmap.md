# Roadmap

Long-running plan for the Wasteland / Fountain of Dreams reverse-engineering
effort. Keep this current across sessions.

## Phases

1. **Fountain of Dreams (FoD) analysis** — fully document `FOD.EXE` and
   `KEH.EXE` via the headless IDA pipeline (names, types, comments,
   structs), plus the on-disk data formats. See [overview.md](overview.md).
2. **FoD stand-alone C++ engine** — portable (SDL-or-similar backend behind a
   thin platform layer so it can later be lifted into ScummVM) that plays the
   whole game from the original data files.
3. **Wasteland 1 analysis** — `wasteland1/Wasteland.idb` and
   `Wasteland Launcher.idb` (no `.asm`/`.idc` exports yet). Reuse FoD names
   via BinDiff / signature matching where the engine code is shared.
4. **Wasteland 1 stand-alone C++ engine.**
5. **Feasibility review of one combined engine** (shared core, per-game data
   tables/scripts).
6. *(Paul, separate thread)* convert the engine source to a ScummVM engine.
   Prior abandoned attempt lives in `scummvm/` — a useful reference for data
   loaders (`fod/data/*`), image decoder, and view structure.

## Phase 1 status (2026-10-21)

| IDB | Role | Funcs named | Structs |
|---|---|---|---|
| `fod/…FOD.idb` (FOD.EXE) | Party creation / main menu | 49 / 130 | 8 |
| `fod/…KEH.idb` (KEH.EXE) | Main game engine | 105 / 355 | 10 |

- [x] Headless pipeline (`ida_scripts/run_ida_script.ps1`,
      `batch_run_and_export.py`), `identify.py`, `rank_unnamed_functions.py`.
- [ ] FOD.EXE: finish naming the remaining 81 `sub_XXXXX`; document
      `Disk1`/party-member save layout.
- [ ] KEH.EXE: name the remaining 250; map/script interpreter, combat,
      inventory, skills, UI.
- [ ] Data formats (`docs/file-formats.md`): DISK1-4 (savegame/state),
      xMAP, xPICT, xANI, TILES, FONT, BORDERS, ARCHTYPE, GLOBALS, PACKETS,
      SERVICES, WEAPONS, HDSPCT, xSCR (scripts/text?). Cross-check with the
      old `scummvm/fod/data/*` loaders and `wasteland1/docs/wasteland_suite`.

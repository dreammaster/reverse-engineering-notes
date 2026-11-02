# Roadmap

Check items off and add notes as they are resolved. See [overview.md](overview.md) for the layout and tooling.

## 1. Disassembly infrastructure

- [x] Headless IDA pipeline (`ida_scripts/`), databases for all eight game executables, `.asm`/`.idc` exported.
- [x] Unpack Ultima VI's EXEPACKed `GAME.EXE` (`tools/unexepack.py`) so it can be analysed.
- [x] Martian Dreams `GAME.EXE` overlays: IDA's loader already loads all 17 (`stub079-095`, `ovr079-095`); verified 95-100%
      function coverage and correct relocation, format documented in overview.md. (The first version of this item wrongly
      claimed nothing was loaded.)
- [x] Databases for the ten `*.DRV` drivers (LZW-compressed raw code), entries named: `u6_drivers.md`
- [ ] Databases for `INSTALL.EXE`/`MAKEMODE.EXE` and the small `*.BIN` loaders (low priority).
- [ ] Per-database `apply_renames_*.py` / struct scripts as functions are identified.

## 2. Analysis of Ultima VI (using `nuvie/` as the reference)

Suggested order: file formats and loaders first (they can be verified against the data), then the game loop, then the scripted
parts that Nuvie moved into Lua/C++ (usecode, conversations, intro, character creation, ending).

- [x] Origin run-time layer (DOS file I/O, bump allocator, disk prompt) named in `GAME.EXE`, `U.EXE`, `END.EXE`
- [x] `GAME.EXE`: ANIMDATA, TILEFLAG, BASETILE, U6PAL, SCHEDULE and `objlist` layouts verified against the code (C++ readers in `src/files/`)
- [x] `GAME.EXE`: MAP/CHUNKS lookup and caching, `objblk*` loading and saving, object arrays, LZW (C++ in `src/files/`, checked against the shipped files)
- [ ] `GAME.EXE`: object list ordering (`sub_1BE84`), egg/NPC spawning, `objlist` globals block, new-game set-up (`U.EXE`)
- [x] `U.EXE`: menu flow, character creation (gypsy quiz, new-game objlist patch), see `u6_intro.md`; C++ in `src/ultima6/gypsy.*`
- [ ] `U.EXE`: drawing/animation of the title, introduction and gypsy scenes, story text pages, character transfer formats, music loading
- [x] Conversation script files and opcode table: `u6_conversation.md`, `src/files/converse.*` (evaluator and text codes still open)
- [ ] `GAME.EXE`: main loop, commands, combat, magic, usecode handlers, conversation evaluator
- [x] Graphics/text data formats (tiles, shapes, archives, fonts, palettes, descriptions): `u6_graphics.md`, `src/files/` (graphics drivers still to disassemble)
- [x] Music: `*.M` track format and the AdLib player: `u6_music.md`, `src/files/music.*`
- [x] `END.EXE`: how it is started (arguments, time played), structure; see `u6_ending.md` (scene scripting not analysed)

## 3. Stand-alone C++ implementation (`src/`)

- [x] Skeleton: engine namespace `Ultima6` with `Ultima6::Ultima6`, `Ultima6::MartianDreams` and `Ultima6::SavageEmpire`
      for game-specific code (main menu, intro, character creation, ending), game detection, tests, MinGW Makefile.
- [x] `src/files/`: readers for ANIMDATA, TILEFLAG, BASETILE, U6PAL, SCHEDULE and `objlist` with tests against the real data.
- [x] `src/files/`: `lzwDecompress`, `MapData` (MAP + CHUNKS), `ObjBlock`.
- [x] `src/files/`: `LibN`, `Shape`, `TileSet`, `LookData`, `Font`, `IntroPalettes`.
- [x] `src/ultima6/gypsy.*`, `src/engine/random.h`: the gypsy quiz, Borland generator and the new-game character patch.
- [x] `src/ultima6/`: `World` (all data, objects, inventories), `MapRenderer`, `Movement` (port of sub_25D7F), `TextWindow`, `stats_view` (party panel), `Conversation`
      (interpreter for the original scripts) and `Talk`, `ActorManager` (hourly schedules, walking), `Game` (key handler, clock, Talk/Look/Get/Rest, party follows);
      `src/engine/`: framebuffer, headless and Win32 backends. `build/u6.exe <gamedir>` plays the start of the shipped game. See `u6_screen.md`.
- [ ] Not yet: Use/Drop/Move and the inventory view, day and night (lighting), combat, magic, creature AI and spawning, object-use handlers (doors, levers, ...), eggs,
      ships and horses, save/load, main menu, introduction, character creation screens, ending, sound and music playback, mouse.

## 4. Martian Dreams and Savage Empire

- [x] First findings: `spinoffs.md` — world data formats are shared and verified on both games' files.
- [ ] Diff each game's executables against Ultima VI's (start with the shared engine code of MD/SE), work out the LZC item flags (0xE0, 2 and 1 all occur; LZMAP's single item is the raw MAP), add to the stand-alone engine behind the per-game namespaces.

## 5. ScummVM integration, 6. Nuvie replacement

- [ ] Not started.

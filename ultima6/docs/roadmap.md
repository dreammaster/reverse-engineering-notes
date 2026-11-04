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
- [x] Also: `Combat`, `Creatures` (eggs, hunting), `Magic` (spell table, spellbook, reagents, first spells), inventory view, light and darkness, save/load, front end
      with character creation, containers, skiffs and rafts. See [u6_systems.md](u6_systems.md) for the state of each system.
- [x] Since then: the Use command (`use.cpp`), Move, moon gates and the Orb, books and signs, shrines, fountains, buckets, fishing, the ending, the introduction (slide show), more spells.
- [x] Also since: wind and rafts/balloons, combat mode (key X), retreat/guard/work/tangle worktypes, field/Peer/Tremor and other spells (about 85 % of the 64 listed), gems, telescope, crystal ball, silver horn, cannon, Drop.
- [ ] Not yet (U6): the drawbridge crank (`sub_335D7`), multi-part ships and ship cannons, `wt_combat_rear/flank/berserk/like/unfriendly`, `wt_walk_path`, the remaining spells (Detect Magic/Trap, Trap, Untrap,
      Help, Animate, Wing Strike, Web, Enchant, Gate Travel, Seance, X-ray, Slime), alcohol, the rest of the original's mouse interface (party panel, inventory view), sound and music, the animated introduction and title
      sequence, and replacing every entry of [approximations.md](approximations.md) (see its numbered list).

## 4. Martian Dreams and Savage Empire

- [x] First findings: `spinoffs.md` — world data formats are shared and verified on both games' files.
- [x] LZC archives decoded (`LibN::get`), tile sets, palettes, frames and WOU fonts load; the shared `World` / `Game` draw both games' maps with their message windows (`build/u6.exe md/game/MARTIAN`).
- [ ] Per-game tables from each executable (movement classes, creatures, usable objects, spells, texts), `TALK.LZC` scripts, portraits (`MDFACES.LZC`, `FACES.LZC`), party panel layout and graphics, menus, intros,
      character creation, endings, Martian Dreams' own mechanics (oxygen, flight) and Savage Empire's (tribal rules), the LZC items whose use is still unknown.

## 5. ScummVM integration, 6. Nuvie replacement

- [ ] Not started.

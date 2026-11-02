# Roadmap

Check items off and add notes as they are resolved. See [overview.md](overview.md) for the layout and tooling.

## 1. Disassembly infrastructure

- [x] Headless IDA pipeline (`ida_scripts/`), databases for all eight game executables, `.asm`/`.idc` exported.
- [x] Unpack Ultima VI's EXEPACKed `GAME.EXE` (`tools/unexepack.py`) so it can be analysed.
- [x] Martian Dreams `GAME.EXE` overlays: IDA's loader already loads all 17 (`stub079-095`, `ovr079-095`); verified 95-100%
      function coverage and correct relocation, format documented in overview.md. (The first version of this item wrongly
      claimed nothing was loaded.)
- [ ] Database for `INSTALL.EXE`/`MAKEMODE.EXE` and the `.DRV`/`.BIN` driver blobs (low priority).
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
- [ ] `GAME.EXE`: main loop, commands, combat, magic, usecode interpreter, conversation interpreter
- [x] Graphics/text data formats (tiles, shapes, archives, fonts, palettes, descriptions): `u6_graphics.md`, `src/files/` (graphics drivers still to disassemble)
- [x] `END.EXE`: how it is started (arguments, time played), structure; see `u6_ending.md` (scene scripting not analysed)

## 3. Stand-alone C++ implementation (`src/`)

- [x] Skeleton: engine namespace `Ultima6` with `Ultima6::Ultima6`, `Ultima6::MartianDreams` and `Ultima6::SavageEmpire`
      for game-specific code (main menu, intro, character creation, ending), game detection, tests, MinGW Makefile.
- [x] `src/files/`: readers for ANIMDATA, TILEFLAG, BASETILE, U6PAL, SCHEDULE and `objlist` with tests against the real data.
- [x] `src/files/`: `lzwDecompress`, `MapData` (MAP + CHUNKS), `ObjBlock`.
- [x] `src/files/`: `LibN`, `Shape`, `TileSet`, `LookData`, `Font`, `IntroPalettes`.
- [x] `src/ultima6/gypsy.*`, `src/engine/random.h`: the gypsy quiz, Borland generator and the new-game character patch.
- [ ] Everything else.

## 4. Martian Dreams and Savage Empire

- [ ] Diff each game's executables against Ultima VI's, add to the stand-alone engine behind the per-game namespaces.

## 5. ScummVM integration, 6. Nuvie replacement

- [ ] Not started.

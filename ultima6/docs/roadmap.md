# Roadmap

Check items off and add notes as they are resolved. See [overview.md](overview.md) for the layout and tooling.

## 1. Disassembly infrastructure

- [x] Headless IDA pipeline (`ida_scripts/`), databases for all eight game executables, `.asm`/`.idc` exported.
- [x] Unpack Ultima VI's EXEPACKed `GAME.EXE` (`tools/unexepack.py`) so it can be analysed.
- [ ] Martian Dreams `GAME.EXE` carries a 70,672-byte Borland `FBOV` overlay block after the MZ image (IDA reports it as
      "Overlays: base=422B0" but loads nothing from it in batch mode; 171 `CD 3F` overlay thunks exist in the image). Work out
      how the overlay code is loaded and add it to `md_game.idb` as its own segments.
- [ ] Database for `INSTALL.EXE`/`MAKEMODE.EXE` and the `.DRV`/`.BIN` driver blobs (low priority).
- [ ] Per-database `apply_renames_*.py` / struct scripts as functions are identified.

## 2. Analysis of Ultima VI (using `nuvie/` as the reference)

Suggested order: file formats and loaders first (they can be verified against the data), then the game loop, then the scripted
parts that Nuvie moved into Lua/C++ (usecode, conversations, intro, character creation, ending).

- [ ] `U.EXE`: data-file loaders, LZW, main menu, intro, character creation
- [ ] `GAME.EXE`: map/chunk/object loading, tile and animation tables, savegame format
- [ ] `GAME.EXE`: main loop, commands, combat, magic, usecode interpreter, conversation interpreter
- [ ] `END.EXE`: ending sequence

## 3. Stand-alone C++ implementation (`src/`)

- [x] Skeleton: engine namespace `Ultima6` with `Ultima6::Ultima6`, `Ultima6::MartianDreams` and `Ultima6::SavageEmpire`
      for game-specific code (main menu, intro, character creation, ending), game detection, tests, MinGW Makefile.
- [ ] Everything else.

## 4. Martian Dreams and Savage Empire

- [ ] Diff each game's executables against Ultima VI's, add to the stand-alone engine behind the per-game namespaces.

## 5. ScummVM integration, 6. Nuvie replacement

- [ ] Not started.

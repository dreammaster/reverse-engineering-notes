# Engine 5.0.1, build 1189, built 2019 (the reconstruction target)

| | |
|---|---|
| Engine release | 5.0.1 |
| Build number | 1189 |
| Build date | 8 Feb 2019 (the Linux build; the GOG Windows build of Deponia 1 says 5.0.1 / 1189 / 30 Apr 2019: the same release compiled again) |
| Project format version | 0xBA = 186 (what `TVisionaireGame` passes to `TVisionaire`; the schema of a savegame says version 186, revision 308) |
| Optimisation | `-O2` |
| Lua | ScummVM's Lua 5.1 stands for LuaJIT |
| Games of this build | **Deponia 1** (Linux, GOG 4.0.1386, executable `Deponia`; confirmed completable in the stand-alone player) |
| Compare with | `../vis-5.0.1-2017` (the same release built in 2017 at `-O0`: Deponia 4), `../vis-5.0.3-2025`, `../vis-4.2.5-2015` |

This tree is the reference the others were copied from. What is known about the game and about where the build differs from the
others is in `VERSIONS.md`, the reasons for the code in `NOTES.md`; `COMPARE.md` and `COVERAGE.md` are the comparison with the
original executable and the coverage of the tests. The two workarounds for known bugs of the original game (the mayor's door in the
Rathaus, the Viaduct tunnel) are in `TGObjectManager.cpp` and `TGCharacter.cpp`, marked `WORKAROUND:`; they only act for the game
`Deponia` and name the object, area and scene they are for.

Build and run: `SDL=1 tools/build.sh 5.0.1-2019`, then `deponia1.bat` (it starts `vis-5.0.1-2019_sdl.exe` in the game's folder with
its password). Tests: `python tools/tbuild.py <test>.cpp` (the sources are in `tests/vis`).

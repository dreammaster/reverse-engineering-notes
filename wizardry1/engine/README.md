# Wizardry I — standalone C++ engine

A dependency-free reimplementation of the DOS "Ultimate Wizardry Archives" release, ported screen by screen
from the game's own p-code and checked bit-exact against a Python p-machine oracle (`../tools/wizvm.py`).
The platform-independent core (`wizcore`) is meant to back a ScummVM engine later; the `wiz1` executable is
the standalone driver.  **Read [`../docs/handoff.md`](../docs/handoff.md) first** for the project state and the
workflow; this file is the module map.

## Two generations of code

| generation | files | status |
|---|---|---|
| **DOS-faithful** (current) | `*_dos.cpp/.h`, `castle.*`, `shops.*`, `dos.*`, `console.*`, `dos_flow.h`, `combat_cutil.cpp`, `combat_act.cpp`, `title.*` | transcribed from the DOS p-code; verified against the oracle; what `wiz1 dosgame` runs |
| legacy (Apple-derived) | `textscreen`, `town_ui`, `maze_ui`, `maze3d`, `camp_ui`, `combat`, `combat_ui`, `roller_ui`, `rewards`, `inn`, `temple`, `cemetery`, `equip`, `specials`, `runner`, … | earlier port with an invented UI; kept for its ctest tests only; slated for removal |

Shared data/rules layer (used by both): `ucsd_volume`, `string_pool`, `scenario`, `types`, `rng`, `character`,
`party`, `roster`, `shop`, `roller` (rules), `font`, `bitmap`, `monster_art`, `surface`, `platform` (+ SDL).

## DOS-faithful modules

| module | p-code segment(s) |
|---|---|
| `dos.{h,cpp}` | WIZARDRY text/window/input procs (`Dos`): print, menus, `GetKey`, `GetNum`, `Delay`, string substitution |
| `console.{h,cpp}`, `console_replay.cpp` | CONUNIT driver: window compositor, wireframe line drawing, portrait blits, CGA VRAM (`Console`) |
| `dos_flow.h` | WIZARDRY:1 — the XGOTO dispatcher used by `dosgame` and `castle-test` |
| `title.{h,cpp}` | title screen + animation |
| `castle.{h,cpp}` | CASTLE: hub, Gilgamesh's Tavern, Adventurer's Inn |
| `shops.{h,cpp}` | SHOPS: Edge of Town, Boltac's, Temple of Cant; leaving the maze, victory, party wiped |
| `roller_dos.{h,cpp}` | ROLLER: Training Grounds |
| `maze_dos.{h,cpp}` | RUNNER (+ UTILITIE:9–14 maze entry): view, HUD, movement, squares, encounters |
| `camp_dos.{h,cpp}` | CAMP: sheet, trade/drop/pool/use, camp spells |
| `specials_dos.{h,cpp}` | SPECIALS: scripted squares, I)NSPECT, DUMAPIC |
| `combat_dos.{h,cpp}`, `combat_cutil.cpp`, `combat_act.cpp` | COMBAT, CINIT, CUTIL, SWINGASW, CASTASPE |
| `rewards_dos.{h,cpp}` | REWARDS: experience, treasure, chests, camp jobs |
| `utilities_dos.{h,cpp}` | UTILITIE camp side: derived scores, E)QUIP, R)EORDER, MALOR |
| not ported | GAMEUTIL (XGOTO 28/29, Edge-of-Town U)TILITIES), KANJIREA |

## Build

Windows / MSVC (uses the CMake + Ninja that ship with VS 2022):

```powershell
engine\build.ps1            # -> engine\build\wiz1.exe
```

Or any CMake toolchain: `cmake -S engine -B engine/build -G Ninja && cmake --build engine/build`.
SDL2 is optional: set `WIZ_SDL_DIR` (default `C:/dev/scummvm_libs_2015`); without it only the headless
commands are built.

## Run

```bash
# the game (SDL window); <dataDir> = extracted/wiz1, <stateDir> keeps roster.dat / party.dat / shop.dat / maze.dat
wiz1 dosgame extracted/wiz1 mystate

# headless, scripted -- what the regression harness uses (keys use \r and \e escapes; gridDir gets a .grid
# dump of the composed screen at every key wait)
wiz1 castle-test extracted/wiz1 'GAAC\rLEMLN' gridDir
wiz1 title-test  extracted/wiz1 'S' gridDir
#   env: WIZ1_RNG="s0,s1,s2,s3" (hex)  WIZ1_PATCH="slot:word:value,..."  WIZ1_TOWN_PARTY=n

# data-layer inspection
wiz1 files    /path/to/WIZ1.DSK
wiz1 toc      extracted/wiz1/SCENARIO.DATA
wiz1 monsters extracted/wiz1/SCENARIO.DATA extracted/wiz1/ASCII.KRN
wiz1 items    extracted/wiz1/SCENARIO.DATA extracted/wiz1/ASCII.KRN
wiz1 str      extracted/wiz1/ASCII.KRN 13010 5001 20355
wiz1 rng      1 16
wiz1 roster   extracted/wiz1/SCENARIO.DATA
```

## Test

```bash
ctest --test-dir engine/build
```

52 tests.  The legacy ones (`town_flow`, `maze_*`, `combat_*`, `camp_*`, …) are self-contained.  Two need
Python and the game disk and report *skipped* without them: `console_replay` (Console vs the oracle's driver
calls) and `castle_screens` (`tools/screens_check.py`: 50 oracle-vs-engine scenarios covering the title, town,
Training Grounds, maze, camp, specials, combat and rewards).

# src23: Yendorian Tales Book I, Chapters 2 and 3, as portable C

A decode-and-reimplement of everything the two games do that does not depend on DOS: the file formats, the game rules, the screens, the cinematics and the front end's key bindings. Plain C99,
no platform code in the library; `tools/explore_sdl.c` is a complete playable front end for SDL2 and shows how a platform drives the modules. The game data files are not part of this
repository: the tests and tools read `WORLD.DAT`, `PICTURES.VGA` and the executable (`SW.EXE` for Chapter 2, `REGISTER.EXE` for Chapter 3) from `../yendor2/game` and `../yendor3/game`
(or the directories in `YENDOR2_GAME_DIR` / `YENDOR3_GAME_DIR`), and the tests that need them skip themselves when they are absent. No game text or art is copied into the sources.

## Build and test

CMake (a static library of every module, 89 test programs, the tools):

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

or, with gcc and Python only (each test's header comment holds its own gcc line; `run_all.py` builds and runs them all, `python tests/run_all.py <filter>` a few):

    python tests/run_all.py

`explore_sdl` needs SDL2 (CMake builds it when `find_package(SDL2)` succeeds; its header has the gcc line):

    ./explore_sdl <2|3> <game dir> [scale]

Keys are the original's (arrows, F1-F4 member sheets, F5/F9 save and load, F8 the clue book, M map, P paper dolls, Esc the pause dialog); `EXPLORE_TITLE=1` starts on the title menu,
`EXPLORE_SPLASH=1` plays the boot title first, `EXPLORE_INTRO=opening|story|ending|credits|splash` a cinematic, `EXPLORE_SHAREWARE=1` the unregistered program, `EXPLORE_KEYS` /
`EXPLORE_SHOT` a scripted run with a screenshot (see the header). `tests/test_soak.c` feeds random keys through every screen of both games.

## Layout

* `gameui.h` is the whole game as a platform-independent front end: `gameUiKey`, `gameUiDraw`, `gameUiAnimate`, `gameUiStatus` and a `GameUiHost` of callbacks (effect, music, cinematic, save
  image). A new platform only has to supply input, a 320 x 200 palette screen, a clock, sound and files.
* `session.h` is the engine core (movement, combat, spells, items, talk, shops, saves, time) with no display in it; `introstory.h` the cinematics as scripts; `creationwizard.h` character
  creation; the `clue*.h` modules the clue book; every other header documents the original routine it reimplements (with the IDA address).
* `tools/` has renderers (`render_view`, `intro_story`, `intro_sheet`), a sound dumper (`cmf_wav`), a headless walker and a parser fuzzer.

The formats, the per-game differences and the open questions are in `../docs23/` (start with `porting-guide.md`, `file-formats.md`, `module-index.md` and `roadmap.md`).

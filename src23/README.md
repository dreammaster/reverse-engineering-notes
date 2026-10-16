# src23: Yendorian Tales Book I, Chapters 2 and 3, as portable C

A decode-and-reimplement of everything the two games do that does not depend on DOS: the file formats, the game rules, the screens, the cinematics and the front end's key bindings. Plain C99,
no platform code in the library; `tools/explore_sdl.c` is a complete playable front end for SDL2 and shows how a platform drives the modules. The game data files are not part of this
repository: the tests and tools read `WORLD.DAT`, `PICTURES.VGA` and the executable (`SW.EXE` for Chapter 2, `REGISTER.EXE` for Chapter 3) from `../yendor2/game` and `../yendor3/game`
(or the directories in `YENDOR2_GAME_DIR` / `YENDOR3_GAME_DIR`), and the tests that need them skip themselves when they are absent. No game text or art is copied into the sources.

## Build and test

CMake (a static library of every module, 90 test programs, the tools):

    cmake -S . -B build
    cmake --build build
    ctest --test-dir build --output-on-failure

or, with gcc and Python only (each test's header comment holds its own gcc line; `run_all.py` builds and runs them all, `python tests/run_all.py <filter>` a few):

    python tests/run_all.py

The playable game is `yendor` (the `explore_sdl` target; it needs SDL2, and CMake builds it when `find_package(SDL2)` succeeds -- otherwise pass `-DSDL2_DIR=<SDL2>/lib/cmake/SDL2`).
Just the game: `cmake --build build --target explore_sdl`, giving `build/tools/yendor` (with `SDL2.dll` beside it on Windows). Point it at a game folder; the chapter is found from the executable in it:

    build/tools/yendor ../yendor3/game
    build/tools/yendor --scale 4 --no-splash /games/yendor2
    build/tools/yendor --help

It starts like the original (InitGame): the boot title, then the introduction (Escape in either skips what is left of them), then, when the folder has a save (`SAVGAME1` .. `SAVGAME6`, the original's own
files, so the DOS game's saves load here), the dialog with LOAD / NEW GAME / DOS -- NEW GAME, after its question, goes to the title menu; with no save it goes to the title menu at once and, after E, to
the dialog with SAVE / DOS / ANIMATION / RETURN. A new game has no party: A assembles one from the four heroes (E beeps until then); C creates a hero, I plays the introduction, R the credits, Ctrl+Q leaves.
`--no-splash` skips the boot title and the introduction, `--no-title` goes straight into the game with no start-up at all, `--game 2|3` forces the chapter, `--shareware` plays the unregistered program,
`--no-sound` does what it says; `--clock-ms N` sets the pace of the game clock (default 5000: a game minute every five seconds as in the original, so the sky changes at 06:00 and 18:00 over about nine minutes each; 0 stops it). The old form `explore_sdl <2|3> <game dir> [scale]` still works.

Keys are the original's (arrows, F1-F4 member sheets, F8 the clue book, M map, P paper dolls, S shoot (Chapter 2: the party's weapons fly down the corridor), Space or the right button on the view use what is ahead, D the pause dialog with its six save slots, Esc backs out of a screen and, on the main screen, leaves; F5 cycles the top-right panel -- plate, minimap, resources; F7/F9 are an
extra quick save and load to `explore.sav`);
the mouse works as in the original (the title menu, the roster's check boxes and DONE, character creation, the clue book, the pause dialog, conversations and shops, and on the main screen the walk pad, the icon
row, the map panel and the party panels; the right button backs out of a screen); `EXPLORE_BOOT=1` runs the start-up above in a scripted run, `EXPLORE_TITLE=1` starts on the title menu,
`EXPLORE_SPLASH=1` plays the boot title and the introduction first, `EXPLORE_INTRO=opening|story|ending|credits|splash` a cinematic, `EXPLORE_SHAREWARE=1` the unregistered program, `EXPLORE_KEYS` /
`EXPLORE_SHOT` a scripted run with a screenshot (see the header). `tests/test_soak.c` feeds random keys through every screen of both games.

## Layout

* `gameui.h` is the whole game as a platform-independent front end: `gameUiBoot`, `gameUiKey`, `gameUiDraw`, `gameUiAnimate`, `gameUiStatus` and a `GameUiHost` of callbacks (effect, music, cinematic, save
  image, the six save slots). A new platform only has to supply input, a 320 x 200 palette screen, a clock, sound and files.
* `session.h` is the engine core (movement, combat, spells, items, talk, shops, saves, time) with no display in it; `introstory.h` the cinematics as scripts; `creationwizard.h` character
  creation; the `clue*.h` modules the clue book; every other header documents the original routine it reimplements (with the IDA address).
* `tools/` has renderers (`render_view`, `intro_story`, `intro_sheet`), a sound dumper (`cmf_wav`), a headless walker and a parser fuzzer.

The formats, the per-game differences and the open questions are in `../docs23/` (start with `porting-guide.md`, `file-formats.md`, `module-index.md` and `roadmap.md`).

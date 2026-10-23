# yemu scripts behind the reference runs

Each script is run on the original game with, for example (PowerShell, from the repository root; `game_state2` is a copy of
`yendor1/game` whose `CURRGAME/PARTY.DAT` has byte 25h set to 2):

    src1/build/tools/yemu.exe <gameDir> SW.EXE --fonts src1/fonts --script <script> --out <dir> --dump <dir> --log62 <dir>/trace.txt

and the test named below is pointed at `<dir>` through its environment variable.

| script | game dir | test (variable) |
|---|---|---|
| `prog3_menu.txt` | party state 2 | `test_prog3` (`YENDOR1_REFP3_DIR`) |
| `new_game_to_menu_about.txt` | unmodified (new game) | `test_prog1`/`test_prog2`/`test_sw` (`YENDOR1_REF2_DIR`) |
| `prog6_bookmarker.txt` | party state 2 | `test_prog6` (`YENDOR1_REFP6_DIR`) |
| `prog0_save.txt`, `prog0_restore.txt` (game dir with differing current/saved game) | party state 2, `SAVEGAME.A\PARTY.DAT` original | `test_prog0` (`YENDOR1_REFP0_DIR`: before/after file copies, see the test) |
| `prog0_new_game.txt` | party state 1, saved game original | `test_prog0` |
| `prog5_create.txt` | party state 2 | `test_prog5` `create` (`YENDOR1_REFP5_DIR/create`: trace, snapshots, `in/` and `out/CURRGAME` party files) |
| `prog5_three_characters.txt` | party state 2 | makes the game used by the next ones (three characters AAA, BBB, CCC; "game_chars") |
| `prog5_assemble.txt` | game_chars | `test_prog5` `assemble`; its output is the "assembled" game (game_asm) |
| `prog5_view.txt`, `prog5_remove_assigned.txt` | game_asm | `test_prog5` `view`, `remove_assigned` |
| `prog5_remove.txt` | game_chars | `test_prog5` `remove` |

For the PROG5 runs add `--work <dir>` (its CURRGAME holds the files after the run) as well as `--dump`/`--log62`.

## PROG4 and the programs it starts (`test_prog4`, `YENDOR1_REFP4_DIR`)

`test_prog4` takes one directory per scenario under `$YENDOR1_REFP4_DIR`: `<name>/trace.txt`, `trace.txt.events`, `06_PROG4_start.ydmp` and
the party files before the run in `<name>/in/CURRGAME` (copy them from the game directory the run starts with). Make one with

    src1/build/tools/yemu.exe <game_asm> SW.EXE --fonts src1/fonts --script <script> --out <dir> --dump <dir> --log62 <dir>/trace.txt --work <workdir> --clock 4660

(`--clock 4660` makes INT 21h/2Ch return DX = 1234h so the random generator's seeds are the same in the port). `trace.txt.events` is written
by yemu next to the trace: every key it delivered with the number of driver calls made so far (pointer refreshes excluded); the test
delivers each key when the port has made as many calls. The comparison runs through the children PROG4 starts (their calls are in the trace
after PROG4's start marker). Scenarios: `idle` (script `wait 4, key 0D, wait 8, key 0D, wait 8, quit`), `walk` (`prog4_walk.txt`), `keys`
(`prog4_keys.txt`: every command letter once: look, spells, talk... reaches PROG11, PROG5 (in-game), PROG6, PROG8, PROG7). The game directory is
the "assembled" party of three characters (`game_asm`, see prog5_assemble above). `YENDOR1_ONLY=<name>` runs one scenario,
`YENDOR1_DUMP=<file>` writes the compared call lines of both sides, `CALL_LOG=1` prints every call to stderr.

Keys in the reference runs: yemu writes `K <program> <driver calls so far> <polls of the program since its last driver call> <byte>` to
`trace.txt.events` whenever a program reads a key with INT 21h/06h. A translated program (`Machine16`) is given each key at exactly that
poll (`System::pollKeyFor`), which matters because such programs count their waiting in loop iterations (PROG4's NPC sweeps); a hand-ported
program gets its next key once the port has made as many driver calls. `Shared::DefaultLauncher` tells the System which kind of program runs
(`programStarted/Ended`). `tests/test_game.cpp` replays a whole run from the start of SW.EXE this way (`YENDOR1_REFG_DIR`: `trace.txt`,
`trace.txt.events`, `in/CURRGAME`, `in/SAVEGAME.A`).


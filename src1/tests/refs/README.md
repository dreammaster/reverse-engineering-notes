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

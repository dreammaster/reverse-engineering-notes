# Main loop (exploration)

`_main` (`14BE3`, with copy-protection junk bytes interleaved in the prologue) parses the command line (`C`/`S` options), initialises
overlays (`__OvrInitEms`, `__OvrInitExt`: "Using Expanded/Extended Memory"), opens `mm3.cc`, loads the timer/sound drivers and
runs the intro (`introSequence`), then the title/roster menu (`rosterMenu`) and `exploreLoop`.

`exploreLoop` (`3E982`, `Engine_mode` 1):

1. rebuild maze pages (`mazeUpdateSlot`), run the event for the current square (`runMazeEvent`), `drawView`;
2. `getCommand` (`254BA`) returns a command code; a 36-entry table at overlay-04 `16D4h` (handlers at `+48h`) dispatches it:
   * `F0h`/`F1h` turn left/right (adjust `Party_facing`), `F2h` step forward (tests the wall word with `mazeGetWordRel`, calls
     `chargeStep` to spend time, then moves `Party_x`/`Party_y`), `F3h`/`F4h`/`F5h` other movements (back, strafe),
   * ASCII keys: `C` cast (`spellsDialog(0)`), `R` rest (`rest`), `B` bash (`sub_2863F`), `D` dismiss (`dismissCharacter`),
     `I`, `M`, `Q`, `S`, `V`..., `ESC` -> `controlPanel`; `C9h`-`D0h` select party member 1-8 (the F-keys),
   * after every command `runMazeEvent` is run again for the new square; when it returns 1 or 2 the loop ends.
3. at the end the same function plays the death sequence (`death.vga`, `mm3theme.m`) when the party is dead.

`Engine_mode`: 0 = ?, 1 = exploring, 2 = combat, 3 = spell menu, 9 = script in progress (see `addTime`, `runMazeEvent`).

## Exploration key table (decoded from overlay 04, table at `16D4h`, handlers at `+48h`; by the calls each handler makes)

| command | handler does |
|---|---|
| `S` / 0 | `sub_19174` -- shoot / fire a ranged attack ahead (the routine that `spellAttackAhead` also uses) |
| `C` / 1 | `spellsDialog` (cast) |
| `R` / 2 (and `%`-code 25h goes to `13E8h`) | `rest` |
| `B` / 3 | bash the door/wall ahead (`chargeStep` + wall test) |
| `D` / 4 | `chargeStep`, `dismissCharacter` |
| `V` / 5 | `showOverheadMap` (after `sub_282CA`) |
| `M` / 6 | `showOverheadMap` (map) |
| `I` / 7 | `sub_2818A` + `chargeStep` |
| space (20h) | pass one step of time (`chargeStep` only) |
| `Q` / 8 | `characterStatsDialog` / `controlPanel` |
| ESC (9) | `controlPanel` |
| `C9h`-`D0h` | select party member 1-8: `characterInfoDialog` (then `moveMonsters`, `runMazeEvent`) |
| `F0h`/`F1h` | turn left / right |
| `F2h`, `F3h`, `F4h`, `F5h` | forward, back and the two strafes (each tests the destination wall with `mazeGetWordRel` and spends time in `chargeStep`) |

Letters are the upper-case key codes (`B`=42h, `C`=43h, `D`=44h, `I`=49h, `M`=4Dh, `Q`=51h, `R`=52h, `S`=53h, `V`=56h); the small numbers 0-9 are the
same commands as delivered by the mouse/button panel.  The assignment of `I`/`V`/`Q` to their dialogs comes from the callees only and is uncertain.

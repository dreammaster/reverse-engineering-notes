# Running the engine and its controls

```
mingw32-make            # build/u6.exe
mingw32-make test       # the unit tests (a few minutes: the fuzz test plays random keys on all three games)
build/u6.exe u6/game --play                     # Ultima VI from the shipped saved game (without --play: the original's menu, creation, transfer)
build/u6.exe md/game/MARTIAN                    # Martian Dreams
build/u6.exe se/game/SAVAGE                     # Savage Empire
build/u6.exe u6/game --play --keys "{up}tl" --shot out.png --scale 3    # headless: a key script and a screenshot
```

The game files are only ever read; saves go to `u6save`, `mdsave`, `sesave` (or `--save dir`). Setting `U6_NO_MUSIC` silences sound.

## Keys (as in the original)

| Key | Command |
|---|---|
| arrows, `7 9 1 3` | walk (eight ways) |
| `A` | attack (with each weapon in the hands; a cursor asks for the target) |
| `C` | cast (circle digit and the syllables, then a target) |
| `T` | talk (a direction) |
| `L` | look (a cursor) |
| `G` | get (a direction) |
| `D` | drop (inventory pick, quantity, a place) |
| `M` | move an object (a direction, then where to) |
| `U` | use (a direction; in Martian Dreams the recipes of the object ask for partners with a cursor) |
| `R` | rest (camp) |
| `B` | combat mode on / off |
| `I` | inventory (arrows select, Enter readies / unreadies / uses, `D` drops, `M` hands over, Esc leaves) |
| `1`-`8` | that member walks alone (solo mode); in the inventory they show that member |
| `0` | party mode (everyone must be within eight squares) |
| space | pass a turn |
| ctrl-S | save (asks `Save game?`) |
| Enter / Esc | confirm / give up in cursors and prompts |

The mouse clicks the command icons under the map, steps towards a clicked square and (right button) gives a question up.

## State

[roadmap.md](roadmap.md) lists what is missing; [approximations.md](approximations.md) lists every place that is not the original's exact behaviour.

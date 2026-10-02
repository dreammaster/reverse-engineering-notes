# MM2 in C (intermediate goal toward a ScummVM engine)

Plain C99 ports of the verified Python tools, built with MinGW (`C:\mingw32\bin`), plus a small SDL2 viewer.

| File | Port of |
|---|---|
| `mm2_lzw.c` | `tools/mm2_lzw.py` |
| `mm2_files.c` | `tools/mm2_data.py` (MAP, ATTRIB, EVENTS, generic LZW files) |
| `mm2_gfx.c` | `tools/mm2_gfx.py` (image banks, masks, EGA palette) |
| `mm2_view.c` | `tools/mm2_view.py` (indoor and outdoor first-person views) |
| `mm2_map.c` | map styles, movement blocking (docs/file-formats.md, docs/view.md) |
| `mm2_data.c`, `mm2_tables_gen.c`, `mm2_rules.c` | items/monsters/spells/roster, numeric tables from the EXE (`tools/gen_c_tables.py`), experience/training rules |
| `mm2_state.c`, `mm2_events.c` | saved-state addressing by DGROUP offset; event script interpreter with a host callback |
| `mm2_combat.c`, `mm2_party.c` | party/monster attack formulas, spell damage roll, character creation (checked against the shipped characters) |
| `mm2_text.c`, `mm2_png.c` | 8x8 font (MM2.CH) and a stored-deflate PNG writer |
| `main_shot.c` | `mm2_shot MAP X Y N|E|S|W out.png`: renders a view without SDL (`mingw32-make shot`) |
| `mm2_game.c` | game session: map, movement, triggers, event host (messages, teleports, fights) |
| `mm2_battle.c`, `mm2_reward.c` | battle state (ranks, initiative, status wear-off), monster loot and victory treasure |
| `mm2_smith.c`, `mm2_town.c`, `mm2_spells.c` (+ screens in `mm2_ui.c`: `mm2_shot shop temple|guild|smith TOWN out.png`) | blacksmith stock/prices, temple and guild costs, spell names/costs/damage/healing |
| `mm2_party.c` (training), `mm2_inn.c`, `mm2_ui.c` | level-up (hit points, thievery, spell levels and spells), the inn rules, and the inn / training hall / character sheet screens (`mm2_shot inn|sheet|train ...`); in the explorer: **C** = character sheet, training hall door = train with 1-8 |
| (inn) | inn rules (party of 6 + 2 hirelings, towns, hireling gating) and its text screen; `mm2_shot inn TOWN out.png [ids]` renders it |
| `mm2_time.c` | calendar (day fraction, days, years, eras), aging, and the rest command; in the explorer **R** rests |
| `mm2_tavern.c`, `mm2_strings.c` | tavern (feeding, drinks, specialties, tips, rumours) and the STR.DAT building texts (real tavern text is shown); `mm2_shot tavern TOWN SUBMENU out.png` |
| `mm2_fight.c` | a whole fight (turn order, party attack/cast/block/run, monster turns, damage, kills, loot, victory/defeat) and the battle screen with the monster picture; in the explorer an event fight starts a battle, **F** starts a test fight (needs a party) |
| `mm2_treasure.c` | treasure sharing (gold, gems, items into backpacks); event opcode 42 places treasure (**S** collects it) and opcode 20 clears one-shot triggers; victory loot is shared automatically |
| `mm2_monpic.c` | monster pictures (EGA/CGA) |
| `main_sdl.c` | SDL2 explorer: arrows move/turn, PgUp/PgDn change map, Y/N answer prompts; runs the event scripts (messages, teleports) |
| `tests/test_main.c` | data + render regression tests; render hashes come from the Python renderer |

```
set PATH=C:\mingw32\bin;%PATH%
mingw32-make test        # builds and runs the tests (no SDL needed)
mingw32-make check-sdl   # syntax-checks main_sdl.c against the SDL2 headers
mingw32-make mm2         # links build/mm2.exe against SDL2 (SDL_PREFIX, default C:/dev/SDL2/SDL2-2.32.10/i686-w64-mingw32) and copies SDL2.dll
```

The game directory is `$MM2_DIR` or `D:/GOG Games/Might and Magic 2`.  SDL2 2.32.10 (MinGW development package from libsdl.org) is unpacked in `C:\dev\SDL2`; `mm2.exe` links and runs
(smoke-tested with `SDL_VIDEODRIVER=dummy`).

## Review markers

Everything I was unsure about in the original is marked `TODO(review)` in the source with the original routine and its IDA
address (the overlay `.asm` files are in `../ovl/`, the resident code is `../mm2.asm`).  List them with:

```
grep -n "TODO(review)" *.c
```

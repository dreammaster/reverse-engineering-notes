# Stand-alone C++ engine — status

Directory `engine/` (CMake, C++20, no external dependencies; Windows front-end uses plain Win32/GDI + winmm).

| Piece | Files | State |
|---|---|---|
| LZSS, archive TOC, map, script bank, tiles, font, GLOBALS/WEAPONS/ARCHTYPE/PACKETS/SERVICES/HDSPCT | `lzss`, `archive`, `map`, `script`, `gfx`, `data` | done, verified against the real data |
| Save state (`Disk1`, party `Member`), save/load files | `state`, `savegame` | done (`DISK<n>` as the original; `MAPS<n>` is our container for changed maps) |
| World (stats, items, map load + live map cache, persistent map changes, RNG, time/upkeep, encounters, monster movement, loot) | `world` | done; RNG is a port of KEH `rngByte` |
| Script VM (`doScript`, 76 opcodes) | `vm` | all opcodes implemented; second pass verified against the disassembly for stats/compare/IF/skill checks/damage/hazards/monster ops |
| Combat (`runCombat`, rollAttack/resolveAttack, enemy AI, flee/stun/shriek/reload/unjam/doctor/trade, XP, level-ups) | `combat`, `combat_fight`, `progress` | ported; combat text goes through the dialog window (no dedicated combat screen yet) |
| Party handling (item menu, ingest, trade, ammo, unjam, doctor) | `party` | ported |
| Shops and paid services | `shop` | ported |
| Screens: main map view, dialog window, character sheet (3 pages), roster (reorder/banish) | `screen`, `charsheet`, `game` | done |
| Field commands (use item / skill / weapon on a cell), loot pickup, monster conversations (hire/attack/leave/use), scripted fights | `game`, `vm` | done |
| FOD.EXE: title picture, party creation (add/remove/edit/play) | `creator` | done (no animated welcome portrait) |
| PC-speaker sound effects | `sound`, `sounddata.h` | rendered to PCM, played by the Win32 front-end |
| Front-ends | `platform/win32_main.cpp` (`fodplay`), `tools/fodtool.cpp` (`fodrun`) | working |

Run: `cmake -G Ninja -S engine -B engine/build && ninja -C engine/build`, then
`engine/build/fodplay.exe fod/game [savedir] [--new] [--default-party]`
(arrows move, ESC waits, E engage, A roster, F1-F5 character sheets, Q quit/save).
Without a save the FOD party creator runs first.

Test commands (`fodrun <gamedir> ...`): `maps`, `render`, `script`, `run`, `screen`, `sim <png> <keys>`
(U/D/L/R arrows, a-e = F1-F5, `_` space, `!` ESC), `creator <png> <keys>`, `duel`, `fight <map> [seed]`,
`shop <map> <idx> <keys>`, `scriptall` (runs every script of every map), `savetest`, `pict <file> <png>`,
`borders <n> <png>`, `items|weapons|professions|enemies`.

Not done yet: combat screen UI (party status rows, enemy picture panel), shop/NPC portraits and picture sets
(`*PICT` sub-pictures), map animations (`*ANI`), the animated FOD welcome portrait (`GANI`), blast-weapon
party attacks and the combat-time use-item command, KEH.EXE credits/ending pictures, the Tandy/CGA/EGA video modes.

Note: this machine's antivirus deletes freshly linked dynamically linked test
executables; the CMake files link statically and use neutral target names.

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
| Front-ends | `src/fod/platform/win32_main.cpp` (`fodplay`), `src/fod/tools/fodtool.cpp` (`fodrun`) | working |

Run: `cmake -G Ninja -S engine -B engine/build && ninja -C engine/build`, then
`engine/build/src/fod/fodplay.exe fod/game [savedir] [--new] [--default-party]`
(arrows move, ESC waits, E engage, A roster, F1-F5 character sheets, Q quit/save).
Without a save the FOD party creator runs first.

Test commands (`fodrun <gamedir> ...`): `maps`, `render`, `script`, `run`, `screen`, `sim <png> <keys>`
(U/D/L/R arrows, a-e = F1-F5, `_` space, `!` ESC), `creator <png> <keys>`, `duel`, `fight <map> [seed]`,
`shop <map> <idx> <keys>`, `scriptall` (runs every script of every map), `savetest`, `pict <file> <png>`,
`borders <n> <png>`, `items|weapons|professions|enemies`.

Also done: animated KANI/FANI/DANI portraits (shops, services, NPC talk, combat with up to three enemies), ending
screens (op 74), the GANI welcome portrait, hourly mutation events, combat-time skill/item use.

Not done yet: a dedicated combat screen layout (party status/name windows as in the original, F-key command entry),
blast-weapon party attacks, the Tandy/CGA/EGA video modes.

Note: this machine's antivirus deletes freshly linked dynamically linked test
executables; the CMake files link statically and use neutral target names.

Layout: `engine/src/shared` (byte reader, file reading, RGB image + PNG writer: used by both games), `engine/src/fod`
(Fountain of Dreams: library, `tools/`, `platform/`), `engine/src/wasteland1` (Wasteland 1: library `wl`, tools incl. the
recompiler, front-end). The games depend on `shared` only. Binaries land in `engine/build/src/<game>/`.

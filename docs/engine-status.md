# Stand-alone C++ engine — status

Directory `engine/` (CMake, C++20, no external dependencies; Windows front-end uses plain Win32/GDI).

| Piece | Files | State |
|---|---|---|
| LZSS, archive TOC, map, script bank, tiles, font, GLOBALS/WEAPONS/ARCHTYPE | `lzss`, `archive`, `map`, `script`, `gfx`, `data` | done, verified against the real data |
| Save state (`Disk1`, party `Member`) | `state` | done (new-game defaults from FOD.EXE) |
| World (stats, items, map load, persistent map changes, RNG) | `world` | done; RNG is a port of KEH `rngByte` |
| Script VM (`doScript`) | `vm` | ~60 of 76 opcodes; monsters/groups/loot/skill-improvement/NPC-menu verbs simplified or stubbed (logged) |
| Screen composition (borders, 19x9 map view, overlays, party names, clock, dialog frame) | `screen` | done for the main screen |
| Game loop (`movePartyStep`, dialog text, yes/no, key input) | `game` | exploration works headless and in Win32 |
| Front-ends | `platform/win32_main.cpp` (`fodplay`), `tools/fodtool.cpp` (`fodrun`: maps, render, script, run, screen, sim) | working |

Run: `cmake -G Ninja -S engine -B engine/build && ninja -C engine/build`, then
`engine/build/fodplay.exe fod/game` (arrows move, Q quits) or
`fodrun.exe fod/game sim out.png RRDDL` for a headless run that writes a screenshot.

Not done yet (in priority order): combat (`runCombat` and the attack maths), random
encounters/monster movement, character sheet / roster / inventory screens, shops and
services, FOD.EXE-style party creation, saving/loading, sound, animation (`*ANI`),
pictures (`*PICT`, welcome portrait), remaining script verbs.

Note: this machine's antivirus deletes freshly linked dynamically linked test
executables; the CMake files link statically and use neutral target names.

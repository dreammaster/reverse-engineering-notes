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
| Combat (`runCombat`, rollAttack/resolveAttack, enemy AI, buildCombatants, flee/stun/shriek/reload/unjam, XP) | `combat`, `combat_fight` | ported; text-only (no combat screen); doctor/use-item/trade commands, blast-weapon party attacks, loot packets (PACKETS) not yet |
| Time / upkeep (`passTime`, `damageMember`), random encounters, group spawning | `world` | ported; monster/group roaming (`moveMapEntities`) not yet |
| Front-ends | `platform/win32_main.cpp` (`fodplay`), `tools/fodtool.cpp` (`fodrun`: maps, render, script, run, screen, sim) | working |

Run: `cmake -G Ninja -S engine -B engine/build && ninja -C engine/build`, then
`engine/build/fodplay.exe fod/game` (arrows move, Q quits) or
`fodrun.exe fod/game sim out.png RRDDL` for a headless run that writes a screenshot.

Test commands: `fodrun <game> duel <map> <group>`, `fodrun <game> fight <map> [seed]` (forces a random encounter and auto-fights it).

Not done yet (in priority order): monster movement, combat screen UI, scripted fights (request 6/8), character sheet / roster / inventory screens, shops and
services, FOD.EXE-style party creation, saving/loading, sound, animation (`*ANI`),
pictures (`*PICT`, welcome portrait), remaining script verbs.

Note: this machine's antivirus deletes freshly linked dynamically linked test
executables; the CMake files link statically and use neutral target names.

# Engine 5.0.3, build 1192, built 2025

| | |
|---|---|
| Engine release | 5.0.3 |
| Build number | 1192 (0x4A8) |
| Build date | 18 Feb 2025 (the newest of the builds looked at) |
| Project format version | 0xC0 = 192 (the data of the game of this tree is of the old format, 0xB2 = 178, and is upgraded when it is loaded) |
| Compiler / Lua | GCC 4.8.5 / 7.3.1 (Red Hat); **Lua 5.4.0** (`third_party/lua54`; the scripts do not compile in Lua 5.1) |
| Games of this build | **The Night of the Rabbit** (Linux, GOG 2.2.334, executable `rabbit`) |
| Compare with | `../vis-5.0.1-2019` (the tree it was copied from; the scene objects are `TGObject`...: the later 2022 build of Nine Noir Lives has a component model, see `../vis-unidentified-ninenoirlives`) |

`VERSIONS.md` has what differs from the 5.0.1 build and the log of the work (plugins with Lua action parts, conditions with any number
of sub-conditions, decibel volumes, the sound engine: `AUDIO.md`).

Build and run: `SDL=1 tools/build.sh 5.0.3`, then `nightoftherabbit.bat`.

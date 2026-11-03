# The Night of the Rabbit Linux build

Looked at 3 Nov 2026: `D:\Games\ScummVM\Dev\games\Visionnaire\NightOfTheRabbit-Linux` (the GOG installer, unpacked: `data/noarch/game`, `meta`). The listing was made with `idat64.exe -A -B` on a copy of `rabbit` (`build/nrida/rabbit.asm`, 169 MB; not kept in the repository).
Each statement says whether it rests on the executable (E), the data (D) or a run of the reconstruction (R). This is a first look: the quick part of the recipe in `VERSIONS.md` and the symbol and data model comparisons. The function by function comparison has not been done.

| | |
|---|---|
| Game version | 2.2.334 (`version.txt`) |
| Executable | `rabbit`, 15,709,504 bytes, sha1 `a2be06819343`, file date 14 Mar 2025, 10,714 functions, 15,545 symbols (E) |
| Engine | **5.0.3, build 1192 (0x4A8), built Feb 18 2025** (the wide strings of `Init` and the immediate of its `logexpanded` call) (E). The newest of the builds looked at: Deponia 1 is 5.0.1 / 1189 (Feb 2019) |
| Compiler (`.comment`) | GCC 4.8.5 and GCC 7.3.1 (Red Hat, i.e. built on CentOS/RHEL 7), and one 4.6.3 (Ubuntu/Linaro) entry; Deponia's executables have only Ubuntu/Debian 4.6.3 / 4.7.2 entries. Optimization level not checked (E) |
| Password of `data.vis` | `vBdK3x8pXRGNhg`, a narrow string, found with `tools/find_vis_password.py` (E, D) |
| `data.vis` | 69,007,934 bytes (D) |
| Libraries | no Box2D symbols (Deponia 1: 1,876), HarfBuzz 563 symbols (Deponia 1: 929), **Lua 5.4** (see below), SDL2 3,325 symbols (Deponia 1: 2,476), Spine 228 (as Deponia 1), Assimp 1,243, Steam 2, glm 3 (E) |

## Lua 5.4

`strings` finds `Lua 5.4` and the executable defines `lua_arith`, `lua_compare`, `lua_rotate`, `lua_callk`, `lua_pcallk`, `lua_isinteger`, `lua_tointegerx` and `luaopen_utf8`: **the scripts of this game run in the standard Lua 5.4**, not in Lua 5.1 / LuaJIT as in Deponia 1, 2 and 4 (E).
The data uses it: running the reconstruction (ScummVM's Lua 5.1) on this game's scripts gives `')' expected near '&'` (Lua 5.3's bitwise operators, e.g. in the scripts `(30,307)`, `(30,308)`, `(30,328)`, `(30,382)`, `(30,409)`), and the scripts that fail to compile leave functions undefined later
(`attempt to call global 'IsNotEmpty'`, `attempt to index global 'ButtonManager'`) (R, D). So **a ScummVM engine for this game needs Lua 5.3/5.4 syntax and integers** (ScummVM's `common/lua` is 5.1); this is the main thing that separates it from the Deponia builds.

## The data model: a superset of Deponia 1's

`tools/extract_xml_names.py` and `extract_schema.py` on the listing (they read it as it is): **842 XML names (ids up to 882), 44 record classes, 619 fields**, against Deponia 1's 799 names (up to 840), 41 classes and 583 fields. Nothing of Deponia 1's is missing (0 fields and 0 names only in Deponia 1; of the 799 ids in common, the names of 3, 144 and 521 differ only through the extractor, as for Deponia 2, and 840 is Deponia 1's maximum marker, here the real name `GameOverlayVisibility`).
Added: 42 names (ids 841-882: `GameOverlayColor`, `ActionSavedValue`, `GameSaveNameOffsetY`, `ConditionConditions`, `ConditionConditionsNegate`, `ObjectValue*`, `AnimationPauses`, `AudioBusOutputBus`, `GameDefault{Speech,Sound,Music}Bus`, `GameWalksoundBus`, `CharacterAudioBus`, `GameCursor`, `GameCustomActionParts`, `Plugins`, `Plugin*`, `GamePlugins`, `EventAction`, `EventStartBus`, `EventStopBus`,
`OutfitModelTurnSpeed`, `BlockContainers`, `Block*`, `GameBlockContainers`, `ButtonVisibility`, `InterfaceScale`, `InterfaceRotationCenter`), three record classes (**`TTPlugin`, `TTBlock`, `TTBlockContainer`**) and 36 fields (`TTGame` 11, `TTBlock` 5, `TTEvent` 3, `TTObject` 3, `TTPlugin` 3, `TTCondition` 2, `TTInterface` 2, and one each in `TSAction`, `TTAnimation`, `TTAudioBus`, `TTBlockContainer`, `TTButton`, `TTCharacter`, `TTOutfit`) (E).
So the order of the data models is Deponia 2 (753 names, 38 classes) < Deponia 4 / Deponia 1 (799, 41) < Night of the Rabbit (842, 44), each a superset of the one before; a merged engine needs one schema (the newest) with the fields' versions.

## The code compared with Deponia 1 (symbols; `SYMDIFF-D1.md`)

- 7 classes only in this build: `TConsoleGUI` (144 symbols, with `Draw` and `ProcessSDLEvent(SDL_Event&)`), `TFontTexture`, `TTBlock`, `TTBlockContainer`, `TTPlugin` (the new record classes), `TextPart` and `TextScrollPane`.
- 11 classes of Deponia 1 that it does not have under that name: `TBuffer`, `TFramebuffer`, `TGraphicsOGL`, `TGraphicsSubSystem`, `TSpritePartTexture`, `TTexture` (what replaced them was not read; `TFramebufferGL` is in both, here with `DetachTexture()` added), `TGEventInfo`, `TKeyboardEventHandler`, `TProjectFileWriter`, `TSoundItem`, `TextChangedEvent`.
- New methods of classes both have, to be read: `TArgument::GetBoolList` / `Add(bool)` / `Set(vector<bool>)` (a bool list argument), `TCAnimation::ProjectTime` / `MaximumProjectTime` / `SetTime`, `TDataGroup::RemoveLink`, `TGAction::GetRunningAction` and `actionPartHook` (a hook before an action part), `TFreetypeFont::StringLengthShaped`, `TCPDebuggerClient::Activate` / `ProfileAreas` / `StopAllHooks`, `TConsole::bWaysystemDisplay`, the log call `wxLog::logexpanded(int, wchar_t const*, ...)` takes a level first.
- 2,065 free functions and globals only here, 2,157 only in Deponia 1 (many are library code and template instances).
- `T*` classes: 190 here (177 in Deponia 2, 194 in Deponia 1, 214 in Deponia 4), 167 common with Deponia 2 and 183 with Deponia 1 (`tools/compare_binaries.py`). Of the functions both Deponia 1 and this build have (8,806), 26% have the same size.

## The reconstruction on this game's data (R)

`GAME_PASSW=vBdK3x8pXRGNhg` and `tests/deponia1/realdatatest.exe` run in the game folder: the directory is read and the tables load (`PreLoad`, the same `table 21 not in sync` message as in every game), the interfaces load. With `src/deponia1` the run stops at `lua_dump not supported` (the Shader Toolkit script, as for Deponia 4); with the build of `src/deponia4`,
which replaces `string.dump` by an error, the run goes on to the scripts and fails on Lua 5.3 syntax (above). So the data format is read by the reconstruction as it is; **Lua 5.4 is what stops it.**

## Open points

1. A function by function comparison with Deponia 1 (`tools/crosscompare.py`, `origdiff.py`, `emucompare.py`): which functions changed in 1189 -> 1192 and in the years after. Not done.
2. Read the new classes (`TConsoleGUI`, `TFontTexture`, `TextScrollPane`, `TextPart`, the plugin and block records) and the graphics layer (what replaced `TGraphicsOGL`).
3. The Lua API: the registered functions and hooks against Deponia 4's (the scripts of this data call `ButtonManager`, `IsNotEmpty`, `prompts_*` visobjs); which of them need Lua 5.4.
4. Whether the optimizer level is `-O2` (read a tail-call function).
5. The `rabbit` folder has `configtool` and `configure` (a launcher), `goggame-*.info`; the game itself is only `rabbit` + `data.vis` + `scenes`, `characters`, `videos`, `lua`.

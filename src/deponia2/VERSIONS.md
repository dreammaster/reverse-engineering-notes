# Deponia 2: versions of the engine and of the game

This folder holds notes only: the sources of the reconstruction are in `src/deponia1/` (the Deponia 1 Linux build). What
makes the other builds different is collected in [`/VERSIONS.md`](../../VERSIONS.md); this file has the details of Deponia 2.

## Deponia 2 Linux (GOG installer, unpacked)

Looked at: `D:\Games\ScummVM\Dev\games\Visionnaire\Deponia2-Linux` (the MojoSetup installer: `data/noarch/game`, `meta`,
`scripts`), compared with Deponia 1 Linux (GOG, game version 4.0.1386).

| | |
|---|---|
| Game version | 3.3.2351 (`version.txt`) |
| Executable | `Deponia2`, ELF x86-64, 22,364,946 bytes, packaged 18 Dec 2015, sha1 `6d2042d8c5a2...`, GCC 4.7.2 (Debian) and 4.6.3 (Ubuntu) |
| Launcher | `start` (a script like Deponia 1's, with `GAME_NAME="Deponia 2"`; the save and log folder is `$XDG_DATA_HOME/Daedalic Entertainment/Deponia 2/`) |
| Engine build string | "Engine Version: %s (Build %d from Build date: %s)" with **4.2.5, build 1185, built Dec  6 2015** (the arguments of the `wxLog::logexpanded` call in `Init` at `0x5ae255`: the wide string `4.2.5` at `0xe287b8`, the integer `0x4A1`, the date string `Dec  6 2015`), found 23 Oct 2026 by the references to the format string in the code |
| Debug information | sections `.debug_info` (1.9 MB) and the dynamic symbol table are kept, as in Deponia 1; no `branchPillars` source paths in the assertion texts |
| `data.vis` | 55,904,517 bytes, `VIS3`, 2,629 entries, project version 2629 (Deponia 1: 81 MB, 3,109 entries) |
| Password of `data.vis` | `aqy4rnsmsd8jTw` (a wide string in the executable, found with `tools/find_vis_password.py`) |
| Same password for the volumes? | yes: `scenes/scene.vs000` opens with it (3,005 entries, 3,004 Opus sounds and 1 WebP) |
| Volumes | 94 `scenes/scene.vsNNN`, 166 `characters/character.vcNNN`, 57 `videos/video.vvNNN` (Deponia 1: 74, 84, 42) |
| Videos | Matroska (EBML header `matroska`), as in Deponia 1; not decoded yet |
| Sound | Opus in Ogg, as in Deponia 1 |
| `config.ini` | the keys of Deponia 1 without `Device` and `LockCursor` |
| `libs64` | ffmpeg 56 libraries, zlib, Galaxy; no `libopenal`, no `libpng12` (Deponia 1 ships them) |
| `lua/` | the same nine files as Deponia 1: `bannerapi`, `io`, `json`, `luacurl-wrapper`, `md5`, `voices_DE/EN/ES/IT` |

### How the executable differs from Deponia 1's

See the matrix in `/VERSIONS.md`. In short: an older build of the same code base. 2,506 of its 2,582 `T*` methods have a
signature that Deponia 1 has too, but only 46% of the functions in common have the same size. It has no shader,
framebuffer or texture classes (`TGraphicsSubSystemGL`, `TShader`, `TFramebuffer`, `TTexture`, `TBuffer`), no TCP
debugger, no clipboard and no tooltips; it has `TAlphaTexture` and five more methods in `TGraphicsOGL` that Deponia 1
dropped. Its helper functions take `wxString` where Deponia 1's take `std::string`. The command line option texts
`name of start scene`, `debugger adress` and `profiling mode` are not in it; `password` is.

### What the reconstruction does with it

Done with `tools/` and the tests of `tests/deponia1`, run from the game folder (the engine writes a `userdata` folder
there: delete it afterwards).

- Without the password `data.vis` is refused: "Invalid header" (the directory does not decrypt to `HDR`).
- With the password (`GAME_PASSW=aqy4rnsmsd8jTw tests/deponia1/realdatatest.exe`, or `deponia1_sdl.exe -w -p aqy4rnsmsd8jTw
  data.vis`): the game data loads as "Deponia 2", window 1920x1080, the images and sounds of the scenes load, and the
  SDL2 player shows the language select. Nothing past it has been tried.
- `realdatatest` reports "freetype: 0 fonts made" and a failure of its `made >= 12` check: Deponia 2 has no TrueType
  font (Deponia 1's check is for its Chinese, Korean and Arabic ones), so that is the test, not the engine.

### Open points

- **The lightmap of scene 1001** (`data/assets/scenes/ch1/1001/1001_lightmap.webp`) is named in the data as
  `#s#000#00002#`, entry 2 of `scenes/scene.vs000`, which is an Opus sound; the log says "wrong format, error 3" and
  the lightmap is 0x0. The only WebP in that volume (480x270, plausible for a lightmap of a 1920x1080 scene) is entry
  1838. Either Deponia 2 numbers its entries another way for some names, which would be a difference in the
  engine, or the name in the data is a mistake of the game's that the original survives. Not decided.
- ~~Which build of the engine this is~~: 4.2.5, build 1185, Dec  6 2015 (see the table at the top).
- Whether the rest of the data (actions, scripts, dialogs, characters) loads and plays as in Deponia 1: only the load
  was checked.
- Videos (57 Matroska volumes): not decoded with `TMatroskaMoviePlayer` yet.


## Deponia 2 compared with Deponia 4 and Deponia 1 (22-23 Oct 2026)

Done with the tools that `src/deponia4/VERSIONS.md` describes, on the executables only (`tools/origdiff.py`, `tools/symdiff.py`, `tools/globaldiff.py`, `tools/emucompare.py`, the extractors of the XML names and the schema on a new IDA listing, `Deponia2.asm`, made with
`idat64.exe -A -B`; it is not kept in the repository). The reports: [`SYMDIFF-D1.md`](SYMDIFF-D1.md), [`ENGINECALLS-D4.md`](ENGINECALLS-D4.md), [`GLOBALDIFF-D4.md`](GLOBALDIFF-D4.md).

**The assumption that the changes of Deponia 2 are a subset of Deponia 4's does not hold; it is the other way round.** Deponia 4's logic is that of Deponia 1 (their differences are the text path, a few graphics details and the build options). Deponia 2 is the *older* engine (packaged Dec 2015) and
everything that is new in Deponia 4 / Deponia 1 is new relative to it: the order of the builds is Deponia 2 -> Deponia 4 (Sep 2017) -> Deponia 1 (Feb 2019), and the step from Deponia 2 to Deponia 4 is the big one. Function by function, Deponia 2 against Deponia 1 (both `-O2`): of 12,984 functions in both, 6,633 have the same calls, texts and shape and 6,351 differ
(Deponia 4 against Deponia 1: all the differences are optimization or the points of `src/deponia4/MERGE.md`).

### The build

| | Deponia 2 | Deponia 4 / Deponia 1 |
|---|---|---|
| Compiler (`.comment`) | GCC 4.6.3 (Ubuntu/Linaro) and 4.7.2 (Debian), `-O2` | GCC 4.4.7, 4.7.2 and 4.6.3; Deponia 4 `-O0`, Deponia 1 `-O2` |
| JIT assembler in the file | DynASM 1.3.0 | DynASM 1.4.0 (DynASM belongs to LuaJIT: most probably LuaJIT 2.0 against 2.1; not checked further) |
| Class layouts | `TConsole` inside `TGameControl` at `+0x838` | `+0x938` in both |
| Libraries | no Box2D, no HarfBuzz, no glm symbols (Deponia 1 has none either: inlined); 217 Lua/tolua symbols | Box2D (the `b2` Lua module); glm in Deponia 4 and 1 (inlined in Deponia 1); 345 / 339 Lua/tolua symbols; HarfBuzz in Deponia 1 only |

### Data model: additive

`tools/extract_xml_names.py` and `extract_schema.py` on `Deponia2.asm` (the same `-O2` code patterns as Deponia 1, so they read it directly): **Deponia 2 has 753 XML names (ids up to 794), 38 record classes and 543 fields; Deponia 4 and Deponia 1 have 799 names (up to 840), 41 classes and 583 fields.**
Every name, class and field of Deponia 2 is in the newer builds with the same id, type, kind, link and version (0 differences, 0 fields only in Deponia 2; the five name differences are the extractor's: ids 3, 58, 144, 521, and the last id 794, which is a maximum marker in Deponia 2). What the newer builds add:

- three record classes: `TTAudioBus` (audio busses), `TTShader` (shaders) and `TTEvent` (events, no fields of its own in the schema), with the tables `AudioBusses`, `Shaders` and `Events`; the links `GameAudioContainerLinks`, `GameAudioMixLinks`, `GameShaderLinks`, `SceneAudioBusses`;
- 32 fields in 11 existing classes: `TTGame` 8 (`GameContainers`, `GameBuildRules`, `GameIconFile`, `GameBuildOptions`, `GameShaderLinks`, `GameSmoothScrolling`, `GameAudioMixLinks`, `GameAudioContainerLinks`), `TTButton` 7 (`ButtonRotation`, `ButtonRotationCenter`, `ButtonShaderSet`, `ButtonScale`, `ButtonMatrixId`, `ButtonScaleX`, `ButtonScaleY`),
  `TTAnimation` 3 (`AnimationModelFile`, `AnimationModelName`, `AnimationEndDirection`), `TTCharacter` 3 (`CharacterShaderSet`, `CharacterLightmapActive`, `CharacterHarmonizeWalkAnimations`), `TTObject` 2 (`ObjectScaleX/Y`), `TTOutfit` 2 (`OutfitModelFiles`, `OutfitTurnAnimations`), `TTParticleContainer` 2 (`ParticleContainerSettings`, `ParticleContainerPreviewScene`),
  `TTScene` 2 (`SceneShaderSet`, `SceneAudioBusses`), `TTDialog` 1 (`DialogCharacter`), `TTInterface` 1 (`InterfaceMatrixId`), `TTValue` 1 (`ValueFloat`).

So the newer data format reads Deponia 2's data as it is (a field that is not in the file keeps its default); a merged engine needs no second schema, only the older build's behaviour where it differs (below). The `UpdateVersion` strings of the newer builds (`.vc<3i>`, `.vi<3i>`, `.vs<3i>`, `.vv<3c>;copy` ...) are the upgrade steps for projects of older versions.

### What the newer builds added (strings and symbols; each is absent from Deponia 2's executable, checked on the bytes)

- **The Lua hook API**: `AnimationStartedHook`, `AnimationStoppedHook`, `CharacterDirectionHook`, `GetActionTextHook`, `KeyEventHandler`, `MouseMoveHandler`, `ObjectRenderHook`, `TextPositionHook`, `TextRenderHook`, `TextStartedHook`, `TextStoppedHook`, `TextTextHook`, `SceneMousePositionHook`,
  the `EngineEventHandler` / `engineEvent` / `movieEvent` events, `controller.lua`, the global `sha1`, and the savegame events `LoadingSavegameSuccess` / `LoadingSavegameFailed`. (Deponia 2 has `MouseEventHandler` and `MainLoop` only of the strings checked.)
- **The Lua graphics API**: 94 registered functions in Deponia 2, 140 in Deponia 4. Added: `graphics_addDrawFunc` / `removeDrawFunc` (Deponia 2 has `setDrawFunc`), `bindFramebuffer`, `createFramebuffer`, `createBuffer`, `createIndexBuffer`, `drawIndexed`, `shaderCallback`, `shaderDrawCallback`, `shaderUniform`, `shaderUse`, `lightmapCallback`,
  `drawAnimation`, `instantiateAnimation`, `getAnimationSize` / `InnerSize`, `getCharacterTextPosition`, `getScrollPosition` / `setScrollPosition`, `lockAspectRatio` / `unlockAspectRatio`, `fontColor`, `fontDimension`, `fontLineHeight`, `performLinebreaks`, `movieOpen`, `openedVideos`, `noise` / `noise2`, `evalTween`,
  `box2Doffset` / `box2dnew`, `drawSpriteWithNineRect`, `getMatrix1Properties`, `setupOffsets`, and the modules `luaopen_Particles`, `luaopen_b2` (Box2D), `luaopen_jit_util`, `luaopen_jit_profile`, `luaopen_table_new`, `luaopen_table_clear`, `luaopen_utf8`. The 51 script commands (`Cmd*::Do`) are the same in both.
- **Graphics layer**: the classes `TGraphicsSubSystem`, `TGraphicsSubSystemGL` (39 methods), `TShader`, `TShaderConstantBuffer`, `TFramebuffer(GL)`, `TTexture(GL)`, `TBuffer(GL)`, `TSpritePartTexture`; Deponia 2 has `TAlphaTexture` and draws through `TGraphicsOGL` alone. The shader files (`yuv.bin`, `stipple.bin`, `text.bin`, `textured.bin`, `particles.bin`, `lightmap.bin`, `color.bin`, `color2.bin`, `commitShift.bin`, `sht_*.bin` ...) and the `renderpass` configuration.
- **Debugging and editor aids**: `TCPDebuggerServer` / `Client` (the Lua debugger over TCP), `TVisionaireClipboard`, `Tooltip` / `TooltipHolder`, `TreeItem`, `TextChangedEvent`, `TKeyboardEventHandler`; the console commands `LOAD` and `SAVE` (`TConsole::ParseCommand` and `ExecuteCommand` calling `LoadGame(int)`, `LoadGame(TMSavegame*)` and `SaveGame(int)`) and option completion (`FillOptions`).
- **Game controller**: `HandleControllerButtonHit`, `HandleControllerButtonRelease` and `HandleControllerAxis` call `HandleKeyEvent` in the newer builds (a button becomes a key event: `ConvertControllerButtonToSymKey`, `1000001 + button`); Deponia 2's handlers do not call it. `TMasterControl::StartEarthquake` / `StopEarthquake` call `TGameController::HapticStartRumble` / `HapticStopRumble` in Deponia 2 and not (within three levels) in Deponia 4.
- **Platform SDKs**: Deponia 2 asks Steam for `STEAMREMOTESTORAGE_INTERFACE_VERSION013`; Deponia 4 and 1 for `...014`, `STEAMAPPS_INTERFACE_VERSION008`, `SteamUser019`, `SteamUtils008` and `SteamController005`, and they have the Galaxy overlay (`TGalaxySDKOverlayInitListener`).
- **Files and containers**: `TComposedFile::InitEntries` reports "Invalid header. Should be VIS3, but was %s" and "Could not open"; `TComposedFileManager::GetComposedFileInfo` / `GetComposedFile` are split out of `Open` / `Export` / `GetMemoryFile`; `TPictureWebP::ReadHeader` / `ReadData` go through `TMemoryBuffer::Decrypt`; `TPictureIO::LoadHeader`.
  Deponia 2 only: the temporary movie extension `evv.vid`, `TComposedFile::AddFile` / `WriteToDisk`, the multichannel OpenAL formats in `TSoundFFMPEG::Play` (the newer builds have a `Vis Audio Thread` and `soundengine`).
- **Scripting data**: `TXMLNames::AddXMLName` fills the name table in the newer builds; Deponia 2 has map classes of its own for the same lookups (`TXMLNameMap`, `TStringIntMap`, `TArgFlagArrayMap`, `TArgFlagSyntaxPtrMap`, `TCommandSyntaxPtrMap`, `TGSceneActionAreasIntHash`). `TSText::SetTextIntern` calls `TTText::GetTextProperties`, `ReplaceValues` and `InvalidTag` in the newer builds.
- **Characters and animation**: `StartTalkAnim`, `StartWalkAnim`, `StartRandomAnim` and `StartStandingAnim` all call `StartCharacterAnim(TVisObjRef&, TCharacterAnimEnum, bool)` in the newer builds (Deponia 2 has `StopCharacterAnim(bool)` and another `StartCharacterAnim`); `SetCurrentOutfit`, `SetCurrentCommentSet`, `SetWaySystem(TVisObjRef const&, bool)`, `GetAllPointsNextTo`, `IsConnected`.
- **Savegames**: `TMSavegame::SaveGame` calls `SaveSnapShot`; the snapshot name `vtp_savepic%d.webp#g#-01#00001#`.

### Behaviour that changed, shown by running the code

`tools/emucompare.py` on the 55 free functions and static methods with number arguments that can be run in both executables (`GetAngle`, `CompositeEnums`, `lerp`, `EaseLinear`, `BlendColors`, `angleDist`, `diff`, `mirror`, `Rand`, `TGCharacter::GetOppositeDirection`, `TData::GetDataSize`, `Easing::*`, `Noise1234::noise` ...): **53 give the same result on every input**; two differ:

1. **`GetAngle(float x, float y)`** (the direction of a vector in degrees): both return `atan(-y/x)` in degrees plus 90, 180, 270 or 360 by quadrant, and 90 or 270 for `x == 0`. The newer builds then take the **result modulo 360**; Deponia 2 does not, so it can return **360** where the newer builds return 0.
   55 of 400 test inputs differ, e.g. `x = 10^6, y = 2`: 360 against 0. It is used for the directions of characters (`kCharacterDirection`), their turning and some script commands. A merged engine should use the modulo (the newer behaviour); a direction of 360 against 0 only matters where something compares directions for equality.
2. **`CompositeEnums(int)`**: the blend modes of the scripts. Deponia 2's table is `0, 1, 0x300 ... 0x308` (the OpenGL blend factors `GL_ZERO`, `GL_ONE`, `GL_SRC_COLOR` ... `GL_SRC_ALPHA_SATURATE`; `GL_ONE` for a number over 10), the newer builds return the engine's own numbers 1 to 11 (2 for a number over 10), which the shader layer maps to GL. Same intent, another interface.

### Methods run in both executables (Deponia 2 against Deponia 4)

`tools/emucompare.py ... --methods --trials 12` (the method mode, see `src/deponia4/VERSIONS.md`): **1,174 methods of the `T*` classes ran in both, 1,011 give the same results and the same writes on every input, 161 differ** (882 more were skipped, as before).
A difference is a moved member (a class whose layout changed), a changed piece of logic, or the noise the method mode has (library calls the optimizer inlined, uninitialised timer bytes, addresses). By class:

- **No difference in any method that ran: 100 classes (535 methods)**, among them `TCAnimation` 28, `TCharHolder` 27, `TGameControl` 24 (the methods that do not touch the console), `TPaintControl` 24, `TDataGroup` 20, `TTypeData` 18, `TTypeGroup` 16, `TFieldValue` 13, `TLink` 13, `TMemoryBuffer` 10, `TTAction` 10, `TGInterface` 9,
  `TGameController` 9, `TSoundInterface` 9, `TSceneControl` 8, `TGText` 7, `TSText` 6, `TTimer` 6, `TFreetypeFont` 5, `TGObjectManager` 5, `TComposedFile` 4 and the record classes `TT*` of the data except `TTButton`, `TTDialogPart` and `TTObject` (one method each differs). So the data classes, the animation, the typed-data layer and the plain accessors of the objects are the same code in 2015 and 2017.
- **Most of their methods differ: `TConsole` 17 of 20** (it moved from `+0x838` to `+0x938` inside `TGameControl` and gained the load/save commands), `TPreloadedPicManager` 2 of 2, `TSpriteHandle` 5 of 7, `TFontManager` 4 of 6, `TSteamSDK` 4 of 6, `TGraphicsOGL` 6 of 11, `TPictureIO` 5 of 10, `TTable` 10 of 21, `TFile` 10 of 27, `TPictureMEM` 6 of 16,
  `TMatrix4` 4 of 11, `TMasterControl` 6 of 21, `TVedFile` 3 of 9; fewer of `TVisionaire` 5 of 33, `TGCharacter` 5 of 34, `TManagedObject` 5 of 36, `TVisObjRef` 3 of 35, `TArgument` 3 of 40, `TVisionaireObject` 2 of 26.
  These are the classes a merged engine has to look at per build (Deponia 2 against the others) first; the ones in the first list need nothing.

### What this gives the work on Deponia 2

The other thread works on playing Deponia 2 with the reconstruction. From this comparison: the list of what Deponia 2's executable cannot do that the reconstruction (Deponia 1's code) does (the hook API, the extra graphics Lua functions, shaders and framebuffers, audio busses, the console load/save, controller to key events), so those paths can be off for Deponia 2;
the data model needs nothing (it is a subset); the one mathematical difference found is `GetAngle` (360 against 0), and `CompositeEnums`. Not compared: the methods of the classes whose layout changed (`TGameControl`, `TConsole` ... run in the emulator they differ at every offset), the conditions of the functions the emulator cannot run, `TGraphicsOGL` internals. The text path was looked at only through the calls of `TFreetypeFont::RenderString_i`: like Deponia 4's it goes **character by character** (`decode_utf8_char`, `GetChar`) with no `itemize` and no HarfBuzz, and draws with GL calls of its own (`GLShader::use`, `SetFloat4`, `glBindBuffer`, `glDrawElements`; it takes the scale as an `int`). So the shaping is the newest addition of all: Deponia 1 (2019) only.

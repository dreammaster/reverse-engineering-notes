# Deponia 4 (Deponia Doomsday): versions of the engine and of the game

`src/deponia4/` started as a **copy of `src/deponia1/`** (taken at commit 4ca6cb5) so that the Deponia 4 build can be
reconstructed as a second code base next to the first, with this file as the running log of what differs. The aim is to merge the
two into one engine later, so every difference found goes in the log below, with how it is known (executable, data, or a run).
The matrix of all the builds is in [`/VERSIONS.md`](../../VERSIONS.md); the reasons for the code are in
[`../deponia1/NOTES.md`](../deponia1/NOTES.md) (not copied: it describes the code both share).

Nothing in this folder has been changed from the Deponia 1 sources yet, apart from this file and the generated reports
(`SYMDIFF.md`, `FUNCDIFF.md`, `ENGINECALLS.md`). Where a difference below is marked **to do**, the source here still does what Deponia 1 does.

## The build

Looked at: `D:\Games\ScummVM\Dev\games\Visionnaire\Deponia4-Linux` (the GOG installer, unpacked: `data/noarch/game`, `meta`).

| | |
|---|---|
| Game | "Deponia 4" in the launcher (`GAME_NAME` of `start`): **Deponia Doomsday**; game version 1.2.0267 (`version.txt`, `changes.txt`: patch of 2017-10-20, "Engine Update to Visionaire 5.0.1") |
| Executable | `Deponia4`, ELF x86-64, 27,476,740 bytes, sha1 `7d77245bba56`, dated 15 Oct 2017; GCC 4.7.2 (Debian) and the same libraries as Deponia 1 (`readelf -p .comment`) |
| Engine build | `5.0.1`, built **Sep  6 2017** (the wide strings `Engine Version: %s (Build %d from Build date: %s)`, `5.0.1`, `Sep  6 2017` are in `.rodata`; the build number is not found yet). Deponia 1's is 5.0.1 / 1189 / Feb  8 2019: **Deponia 4's engine is 17 months OLDER than Deponia 1's**, whatever the numbers of the games say |
| Symbols | `.symtab` is kept (69,377 defined symbols, 32,621 of them dynamic functions), `.debug_info` holds only the DWARF of the statically linked SDL2 (not of the engine), as in Deponia 1 |
| Optimization | **`-O0`**, with frame pointers: nothing is inlined, every `std::vector::end()` is a call (see below). Deponia 1 is `-O2` |
| `data.vis` | 78,066,964 bytes, `VIS3`, 2,416 entries; password `RHgPi6dmqvu6uT`, a narrow string (found with `tools/find_vis_password.py`, as in Deponia 1) |
| Volumes | 168 `scenes/scene.vsNNN`, 329 `characters/character.vcNNN`, 66 `videos/video.vvNNN` (Deponia 1: 74, 84, 42) |
| `config.ini` | the same keys as Deponia 1 (`diff` of the key names is empty); its comments describe `Device = {DX9\|DX11\|OGL}` and `UseTextureForWidescreen` |
| `libs64` | ffmpeg 56 (`libavcodec.so.56` ...), zlib, Galaxy, `libopenal.so`; **no `libpng12.so.0`** and no `libopenal.so.1` (Deponia 1 has both) |
| `lua/` | `bannerapi`, `io`, `json`, `luacurl-wrapper`, `md5`, `voices_DE`, `voices_EN`; **no `voices_ES`, `voices_IT`** (Deponia 1 has them) |
| Command line | the option texts are the same as Deponia 1's, narrow (`password`, `name of start scene`, `debugger adress`, `profiling mode`) |

## How the files here were made

```
# 1. the disassembly (IDA Pro 8.3; the 64-bit idat64 is needed for an ELF64, idat refuses it)
idat64.exe -A -B -L<log> Deponia4          # in a COPY of the executable; writes Deponia4.asm and Deponia4.i64, 4 minutes
#    -> Deponia4_Linux.asm / Deponia4_Linux.i64 in the repository root, next to Deponia_Linux.asm (both are ignored by git)

# 2. the symbols
python tools/symdiff.py <Deponia> <Deponia4> -o src/deponia4/SYMDIFF.md

# 3. the reconstruction against the new executable (at -O0, which is what the original is)
python tools/crosscompile.py deponia4 -O0 -X -fno-omit-frame-pointer --tag o0
python tools/crosscompare.py deponia4 --tag o0 --original <Deponia4>          # build/cross/deponia4-o0-compare.tsv
python tools/crossreport.py build/cross/deponia4-o0-compare.tsv -o src/deponia4/FUNCDIFF.md

# 4. the same two executables against each other, no reconstruction involved (needs both original-*.pkl caches of step 3)
python tools/origdiff.py build/cross/original-Deponia.pkl build/cross/original-Deponia4.pkl --classes -o build/cross/orig-D1-D4.tsv   # (meaningless for -O2 against -O0: use --engine)
python tools/origdiff.py build/cross/original-Deponia.pkl build/cross/original-Deponia4.pkl --engine build/cross/orig-engine-D1-D4.tsv   # -> ENGINECALLS.md

# 5. the data schema (from the asm; run in a folder of its own, the tools write manifest/*.tsv relative to the working directory)
python tools/extract_xml_names.py Deponia4_Linux.asm manifest/xml_names.tsv ; python tools/extract_schema.py Deponia4_Linux.asm
```

Tools changed or added while doing this: `tools/symdiff.py`, `tools/origdiff.py`, `tools/crossreport.py` (new);
`tools/asmdata.py` (a function called `align` in the listing was taken for the `align N` directive), `tools/crosscompare.py`
(all the `wxLog::log*` overloads are one call, see the logging finding).

## How to read the comparison with the reconstruction

`FUNCDIFF.md` lists, per class, the functions whose calls / numbers / texts / shape are least like the original's. A low score is
one of three things, and only the third is a difference of Deponia 4:

1. **Code the reconstruction does not have** (the editor: `TTable::ImportNewObjects`, `MergeObjects` ... 264 instructions against 7).
   Look at the two instruction counts in the report.
2. **Noise of the compilers.** The reconstruction is clang, the original GCC 4.7.2; the reconstruction was written from an `-O2`
   listing, so what GCC had inlined there (an accessor, `ClearCurrentText`, a `wxString::Format`) is inline in the source, and in the
   `-O0` original it is a call. `TGameControl::ClearTexts` is an example: the original ends with `call TGameControl::ClearCurrentText()`, the
   reconstruction with the inlined body of it. The same goes for `std::to_wstring` (reconstruction) against `wxString::Format<int>` (original) in `SavegameExists`/`SaveGame`.
3. **A real difference.** Separated from 2 by `tools/origdiff.py --engine` (see "The call graph of the engine"): very few so far.

At `-O2` the reconstruction scored 0.95-1.00 for 4 functions of Deponia 4 and under 0.5 for 1,720; at `-O0` it is 3,113 and 930
(of 7,043 functions in both). The `-O0` build is the right yardstick for Deponia 4. (Deponia 1 compared with itself: 1,001 under 0.5 of 2,801.)

## Findings

Each: what, how it is known, what it means for the code here.

### The engine is the same code base; the game data format is the same

- **Classes** (symbols): all 194 `T*` classes of Deponia 1 are in Deponia 4; the 20 "new" ones in `SYMDIFF.md` (`TRect`, `TLinkRef`, `TMappedId`,
  `TUndoData`, `TSignalSlot`, `TLetter` ...) are small helpers whose constructors and accessors were inline in Deponia 1 and are symbols at `-O0`.
  So **no class was added or removed between the builds**. The `T*` methods only in Deponia 4 (267) are the same kind: inline accessors
  (`TPictureIO::GetScaleX`, `TGEventInfo::GetCommand` ...).
- **Game data** (executable): the table of XML names read from `TXMLNames::InitXMLNamesIntern` and `TVisionaireGame::InitXMLNames` has the same
  799 ids with the same names in both (`extract_xml_names.py`; the three differences, ids 3, 58 and 521, are the extractor reading a shared
  wide string at `-O0`, not the engine); the schema (`extract_schema.py`) is the same 41 record classes with the same 583 fields, ids, types, links
  and versions. The extractor does not read the `TTypeGroup` arguments of `-O0` code (it looks for registers loaded by `mov`; here they go through the stack): `schema_groups.tsv` of Deponia 4 is empty in those columns, not different. **to do**: teach it `-O0`.
- **Data** (run): `realdatatest` of the reconstruction (unchanged Deponia 1 code) opens `data.vis` with the Deponia 4 password, decodes the 2,416
  directory entries, loads the project, the interfaces and the scene volumes.
  - `assertion failed: ObjectsHash.size() == Objects.size() (datastruct/table.cpp:650)` and `table 21 not in sync, recreating index` are logged while loading the project. The
    Deponia 1 data logs exactly the same two lines (run of the same test), so this is not a difference of the builds.
  - The run **ends (the process disappears, no `0 failures` line) right after** `lua_dump not supported: Handling of precompiled LUA scripts has been removed in ScummVM`, while
    loading the first font (`SourceHanSansCN-Medium.otf`). The Deponia 1 data passes this point. Not diagnosed: it may be the data (a script run at load that Deponia 1's does not have)
    or a difference in the engine. **to do**: run it under gdb (`tools/tbuild.py realdatatest.cpp -g`).

### Compilation: Deponia 4 is an unoptimized build

- (executable) `TGameControl::DisplayConsole` is `add rdi, 938h; jmp TConsole::Draw` in Deponia 1 and `push rbp; mov rbp, rsp; sub rsp, 10h; mov [rbp+var_8], rdi ...; leave; retn`
  in Deponia 4. Every function has a frame, locals live on the stack, nothing is inlined, and template instantiations and accessors are separate symbols (69,377 against 28,746 symbols).
- Consequence for reading the listing: the `-O0` code is a close transcription of the source, easier to read than Deponia 1's, and a good place to check how Deponia 1's inlined
  code was meant. Consequence for the byte comparison: the `-O0` build is where GCC 4.7.2 and a plain transcription agree most.
- (executable) The same member offsets: the `TConsole` inside `TGameControl` is at `+0x938` in both builds (`DisplayConsole` adds `938h` to `this`), so at least that much of the class layout is the same. Not checked for the others. **to do**: compare the offsets class by class from `DisplayConsole`-like accessors.

### Logging

- (executable) Deponia 1 logs with the varargs `wxLog::logexpanded(wchar_t const *fmt, ...)` at 676 call sites. Deponia 4 uses `wxLog::logexpanded` at 65 and the typed
  template `wxLog::log<T...>(wxString const &fmt, T...)` (`wxLog::log(wxString const&)` 172 sites, `log<wchar_t const*>` 161, `log<char const*>` 71, `log<int>` 26 ...) for the rest.
  It is the same statement with another overload; the older build (Deponia 4) has moved most of its call sites to the typed form and the newer one (Deponia 1) is back to the varargs one at every site.
  **to do**: the `wxLog::log<T...>` family has to exist in `WxStub.h` (it is a `wxString` format with typed arguments) to compile Deponia 4's `-O0` call shapes; it makes no difference to the behaviour.
  `tools/crosscompare.py` now counts all of them as one call so that this does not hide other differences.

### Libraries linked in

(symbols per library, `SYMDIFF.md`) Deponia 1 / Deponia 4: HarfBuzz 929 / **0**, glm 0 / **85**, Box2D 1,876 / 2,316, Assimp 1,337 / 2,645, FreeType 952 / 1,223, wxWidgets 288 / 488, Lua+tolua 339 / 345,
Spine 228 / 266, libpng+zlib 479 / 536, SDL2 2,476 / 2,476 (the same SDL2 build).
Most of the larger counts are the `-O0` instantiations. Two are not: **no HarfBuzz in Deponia 4** and **glm in Deponia 4** (85 `glm::` symbols; Deponia 1 has none and has `TMatrix3`/`TMatrix4` constructors inline: what uses glm is not looked at).

### Text rendering (fonts)

(symbols) Deponia 1 has `TFreetypeFont::GetCharIndex(unsigned int)` / `CreateCharIndex(unsigned int, TFreetypeFont *)` and `TFontManager::PrintText` / `PrintTextLines` /
`TCFont::PrintText` / `PrintTextLines` taking a `std::vector<GLCharBuffer*> *` (a buffer of the glyphs drawn; what it is for is not read yet); Deponia 4 has none of the three and the same functions
without the `GLCharBuffer` parameter (`TFontManager::PrintText(wxString const&, TextAlignmentEnum, wxPoint const&, float)`, `TCFont::PrintText(..., int)`).
Deponia 4 has 12 more `TFreetypeFont` methods (`SetFontColor`, `SetBackgroundColor`, `SetSpacing`, `SetLCDFiltered`, `LockHeight`, `GetLineHeight` ...), which at `-O0` are most probably the inline accessors of Deponia 1.
Reading (from the symbols and the library only, the text code is not read): HarfBuzz and the glyph buffers are a **later addition** to the engine; Deponia 4 draws text without them. `TFreetypeFont`, `TFontManager`, `TCFont`, `TTText`, `TGText` are where Deponia 4 is plainer.
**to do**: `src/deponia4` still has Deponia 1's HarfBuzz text path; strip it back to the `-O0` listing (`TFreetypeFont` has 7 of its 8 functions under 0.5, `TTText` 8 of 15, `TFontManager` 9 of 10).

### Platform SDKs and the client interface

(symbols, scores) `TGameClientSDK` has 8 methods only in Deponia 4 (`InitSDK(wxString const&, wxString const&)`, `GetGalaxySDK`, `GetSteamSDK`, `GetGalaxyStatus`, `GetSteamStatus`, `DeleteCloudSavegame(wxString const&)`, `Update`):
the layer that owns both the Steam and the Galaxy objects is explicit in Deponia 4. `TSteamSDK` (9 of 9 functions unlike) and `TGalaxySDK` (9 of 9) both differ. `CSteamID` is in the symbols of Deponia 4 (Steam: 1 symbol; Deponia 1: 0).
`changes.txt`: "GOG: Galaxy Overlay Support", "Steam: Achievements ..."; `libGalaxy64.so`/`libGalaxyPeer64.so` ship with this build. **to do**: read `TGameClientSDK`, `TSteamSDK`, `TGalaxySDK` in the listing.

### Tweens, tooltips, text boxes, the debugger

`Tween`, `Tooltip`, `TooltipHolder`, `TextBox`, `TCPDebuggerClient`, `TSyntaxDoc`, `TExampleDoc` are in both builds (they are in the Deponia 1 sources); the symbol differences are inline helpers.
Their scores: `TCPDebuggerClient` 2 of 2 unlike, `TextBox` 21 more symbols, `Tween` 3 of 3 unlike, `TConsole` 4 of 4. **to do**: read them.

### The call graph of the engine: no function with another logic found so far

Method (executable, `tools/origdiff.py --engine`, no reconstruction involved; the two original executables against each other): for every function that both define, the calls to engine
functions (T* methods and the engine's own free functions) are compared. Deponia 4 is `-O0`, so what Deponia 1 has inlined is a call in Deponia 4: Deponia 4's calls are therefore
read together with what they call, three levels down. Result in [`ENGINECALLS.md`](ENGINECALLS.md):

- Deponia 1 makes a call that Deponia 4's function does not make (even three levels down): **51 of 2,923 functions**. Reading them: calls through a vtable in Deponia 4 that GCC devirtualized in Deponia 1
  (`TMSavegame::Delete`/`Exists`, `TVisionaire::CleanUp`, `TGameControl::LoadGame(TMSavegame*)`), comparison functions that `std::sort` calls through a pointer (`cmpName`, `cmpOrder`, `CmpLinkReferences`), `x_assert`,
  `toUTF` (from the inlined `wxString(char const*)`) and the like. No missing call that looks like logic.
- Deponia 4 makes a call to a function over 200 bytes that Deponia 1's function does not: 86 functions. Examples read in the listing: `TManagedObject::HandlePostExecution` calls `TGCharacter::ShowComment` at **two** sites in Deponia 4
  and one in Deponia 1: the `-O0` code is the same two-way test (`!ReceiveItem(info)` then `info.character`, or no `ReceiveItem` test) that GCC merged; `TGameControl::HandleMouseUp` has the same sequence in both
  (scene mouse position hook through `LuaExecuteFunction`, `SelectSavegame`, the inverse matrix as `TMatrix3 * TVector3D`, `IsCurrentObjectEmpty`, `SetValue(destination)`) as the reconstruction has it. The rest are helpers (`TData` `GetRefV*`,
  `TMemoryBuffer::AppendData`, `TGInterface::UpdateAlpha`, the `TConsole::Print*` and `Parse*` helpers) that `-O2` inlined.
- So, apart from what the other sections list (fonts, logging, platform SDKs), **the logic of the engine in Deponia 4 is that of Deponia 1** as far as the order of its engine calls shows. What this cannot see: a changed constant, a changed condition, a call
  that was added to a function whose `-O2` form inlined the callee, and every function that calls no engine function.

### Numbers, texts, field ids and source lines: the same

Four checks of the two original executables against each other (`tools/origdiff.py --consts`, `--args`, `tools/assertlines.py`, and one-off scripts over the same caches); the `-O0`/`-O2` noise is treated as in the call graph section.

- **Numbers (immediates), all `T*` code together**: 1,028 distinct numbers of 0x30 or more in Deponia 1, 1,030 in Deponia 4; 35 only in Deponia 1, 37 only in Deponia 4. Read: the line numbers of the asserts of `graphicslib/graphicsOGL.cpp` (they moved, see below),
  and values that a `-O0` compiler writes out as separate cases. Example: `TGameControl::ConvertControllerButtonToSymKey` is a table (`CSWTCH_876`) in Deponia 1 and a 15-case `switch` in Deponia 4, both giving `1000001 + button` for buttons 0-14 and -1 otherwise.
  Nothing in either build is a number the other has no counterpart for (no new field id, event, command or limit).
- **Field ids**: the pairs (`TVisObjRef::GetInt/GetLink/SetValue/...`, id) over the whole executable: 657 in Deponia 1 against 666 in Deponia 4; the pairs only in one are the wrappers `TVisionaireObject`/`TDataGroup` (the same id goes through the other class in the other build,
  one inlines the other) and the way a `wxPoint`'s two numbers get loaded. `TTGame::OnCreate` and `TCAnimation::Start`, read in both listings, write the same fields in the same order with the same values.
- **Texts**: the string literals that `T*` code refers to are 1,758 (Deponia 1) and 1,739 (Deponia 4); the 17 only in Deponia 1 and the 3 only in Deponia 4 are standard library exception texts (`basic_string::substr`, `vector::reserve`: inlined range checks in Deponia 1), format strings that live in a helper in the other build (`%ld` is
  in `CONVTOSTR(long const&)` in Deponia 4) and names that `-O0` stores through a local (`mouseDown`, `touchUp` ... are in `TMasterControl::MovieEvent` of both). Of the log messages (`wxLog::log*`/`logexpanded` call sites) 730 are in Deponia 1 and 706 in Deponia 4; the functions that differ
  are `TData::*` (Deponia 1 has an `x_assert(value != NULL)` with its log inlined in 11 functions, Deponia 4 has it in the `GetRefV*` helpers) and `AudioDevice::AudioDevice` (9 against 3). **No log message or text of the game was added or removed.**
- **Lines of the source files** (`tools/assertlines.py`): the engine's `x_assert(cond, "expression", file, line)` has the line of the source in it. 39 source files have such asserts in both builds; in **38 of them every assertion is at the same line** in both
  (`datastruct/table.cpp` 966, `vstables/visionaireGame.cpp` 2072, `vsplayer/main/mainSDL.cpp` 872 ...), so nothing was added to or removed from those files above their last assert between Sep 2017 and Feb 2019. The files where the lines moved (`cfont.cpp` is one of the 39; the tool leaves `graphicsOGL.cpp` out because several of its asserts share an expression, it was read with `--detail`):
  - `vscommon/cfont.cpp`: Deponia 1 is 4 lines longer before line 0x155 (341) and 5 more before 0x2a2 (674) (`eAlignment == eAlignLeft` at 0x159 against 0x155, `false` at 0x2ab against 0x2a2). The same file as the text printing (`TCFont::PrintText`) that differs by the `GLCharBuffer` parameter.
  - `graphicslib/graphicsOGL.cpp`: Deponia 1 is 5 lines longer before 0x4f3 and 2 more before 0x94e (`false` at 0x4f8, 0x52a, 0xa92 against 0x4f3, 0x525, 0xa8b; `m_drawValid` at 0x6d1, 0x955, 0x9f7 against 0x6cc, 0x94e, 0x9f0). Deponia 4 has asserts of its own in `EnableMipmaps(int)` and `SetEmptyTexture(int, int, int)` (0x504, 0x50f,
    `assert(false)` with `__PRETTY_FUNCTION__`: functions that must not be called) where Deponia 1's are empty (1 instruction, a `ret`).
  Since these asserts are spread over `baselib`, `datastruct`, `graphicslib`, `guilib`, `mmedialib`, `vscommon`, `vsplayer` and `vstables`, **the sources of the two builds are the same files, line for line, apart from `cfont.cpp` and `graphicsOGL.cpp`**.

So the conclusion of the three comparisons (call graph, numbers/texts, source lines) is that, apart from the fonts/text path (HarfBuzz, glyph buffers), `TGraphicsOGL`, the logging overloads, the platform SDK layer and the build options, **Deponia 4 and Deponia 1 are one code base**. The merge needs
`#if`s (or runtime flags) for those, not two versions of the engine. `src/deponia4` can stay a copy only until the text path is cut back; after that it can be dropped in favour of a build option.

### Other classes with the most functions unlike the reconstruction

From `FUNCDIFF.md` (functions under 0.5 / functions in both): `TTable` 32/49 (most of it editor code the reconstruction does not have), `TArgument` 27/55, `TVisionaire` 19/80, `TSoundBase` 17/39,
`TMatrix4` 16/21, `TVisionaireObject` 16/95, `TGameControl` 15/98, `TGAnimation` 14/50, `TComposedFile` 12/19, `TGAction` 11/31, `TXMLWriter` 11/32, `TComposedFileManager` 10/17,
`TGScene` 10/38, `TFontManager` 9/10, `TMovie` 8/18, `TTText` 8/15. `TGAction::Execute(bool, t_SkipCutsceneInfo*)` is 10,083 instructions in the original against 464 in the reconstruction (the `-O0` form of the dispatcher).
None of these has been read yet; the next step is to take them in that order.

## Open points / next steps

1. Conditions (a changed `<` or `&&` is not a number or a call): not covered by any check so far. The unlike functions of `FUNCDIFF.md` that call no engine function and have no assert (`TArgument`, `TMatrix3/4`, `TVector3D`, geometry helpers) are the place to look.
2. Why the run of the reconstruction on the Deponia 4 data stops after `lua_dump not supported`, and the `table 21` assertion.
3. The build number of the engine (the integer passed to `wxLog::logexpanded("Engine Version ...")`).
4. Compile the sources here at `-O0` for real (`tools/pbuild.py deponia4`, `tests/`): the tests (`tests/deponia1`) still build `src/deponia1` (`tools/tbuild.py` has the folder fixed); parametrize it before the two code bases diverge.
5. Deponia 2 (packaged Dec 2015, in the other thread) is older than this build: most of what Deponia 4 differs from Deponia 1 in should be a stage between Deponia 2 and Deponia 1.
   When Deponia 2 is looked at, use `tools/symdiff.py` and `tools/crosscompare.py --original` as above, and record in which build each difference appears (the engine order is Deponia 2 -> Deponia 4 (Sep 2017) -> Deponia 1 (Feb 2019), not the order of the games).

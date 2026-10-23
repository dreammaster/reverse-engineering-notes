# Deponia 4 (Deponia Doomsday): versions of the engine and of the game

The list to work from when merging is in [`MERGE.md`](MERGE.md).

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
| Engine build | `5.0.1`, built **Sep  6 2017** (the wide strings `Engine Version: %s (Build %d from Build date: %s)`, `5.0.1`, `Sep  6 2017` are in `.rodata`; the build number is **1189**: `Init` calls `wxLog::log("Engine Version: %s (Build %d from Build date: %s)", "5.0.1", 0x4A5, "Sep  6 2017")` at `0x89341b`, found by the references to the three strings in the code; **the same number as Deponia 1's**, whose build date is Feb 2019: the number was not raised between the two). Deponia 1's is 5.0.1 / 1189 / Feb  8 2019: **Deponia 4's engine is 17 months OLDER than Deponia 1's**, whatever the numbers of the games say |
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
After the text path was changed to Deponia 4's form, `src/deponia4` itself (157 files, `tools/crosscompile.py deponia4 -O0 -X -fno-omit-frame-pointer --tag o0`, 23 Oct 2026) scores 3,075 functions at 0.95-1.00 and 932 under 0.5 of 6,994 in both. The text classes stay low (`TFreetypeFont` 0.32, `TCFont` 0.47, `TFontManager` 0.39, `TTText` 0.16) for a reason of design, not a wrong text path: the reconstruction draws each glyph as a sprite of the graphics backend, where the original's `RenderString()` is a short wrapper (59 instructions) around `RenderString_i(x, y, begin, end, scale, vector<region>*, int, GLCharBuffer*)` and puts glyphs into textures and GL batches; the reconstruction's `RenderString()` does the per-character loop itself (371 instructions).

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
  - The run **ended right after** `lua_dump not supported: Handling of precompiled LUA scripts has been removed in ScummVM`, while loading the first font. **Cause (found with gdb, `tools/tbuild.py realdatatest.cpp -g`, a breakpoint on the shim's `error()`):
    not a crash and not a difference of the engine, but of the game data and of ScummVM's Lua.** `TGameControl::InitScripts()` runs the game's scripts; one of them is the **"Shader Toolkit" module** (`--[VISMODULE,id,version,chksum]`, Simon Scheckel's, 39,296 bytes, chunk name `(30,150)`), which
    contains a `DataDumper` that runs at load: `for each function in _G, string, table, math, io, os, coroutine, package, debug: if not pcall(string.dump, f) then c_functions[f] = name`. ScummVM's `lua_dump` is `error("lua_dump not supported ...")`, which in ScummVM (and in the test shim) ends the
    program, where in Lua `pcall(string.dump, <C function>)` just returns false. Deponia 1's data has no such script: none of the 73 chunks that `LuaDoString` runs there (listed with a breakpoint under gdb) contains `VISMODULE` or `Shader Toolkit`; Deponia 4's run listed 72 chunks before it stopped, the last one this module.
    The original does not need a workaround: `.comment` has `DynASM 1.4.0`, which is LuaJIT's assembler, so the engine most probably links LuaJIT (not checked further).
    **Fixed in `src/deponia4/vscommon/scripting/visLua.cpp`** (a `WORKAROUND:`): `InitLua` replaces `string.dump` by a function that raises the Lua error `unable to dump given function` (the module only uses `pcall` on it to find the C functions; for Lua functions it now also gets
    "false", so it takes them for C functions, which only matters for the save functionality of that module). For the merge: this belongs in the common `InitLua` for ScummVM, not only for Deponia 4. Another caller of `lua_dump` is `traverseTable` in `scriptingutils.cpp` (the script variables of the debugger/savegame view,
    a function value is stored as base64 bytecode): it is not reached in this run, and would end the program the same way in ScummVM. **to do** for the same reason.
  - With the workaround `realdatatest` built from `src/deponia4` (`python tools/tbuild.py realdatatest.cpp -g --game deponia4`, new option) **runs to the end** on the Deponia 4 data: "Deponia 4", 1920x1080, 168 scene / 328 character / 66 video volumes, 400 pictures decoded without failure, 60 Opus files opened without failure.
    Its one failing check (`made >= 12` fonts: 3 fonts made) and the lightmap line are the test's own, written for Deponia 1's data: Deponia 1 has the twelve fonts of its Chinese, Korean and Arabic text, Deponia 4 has three (`SourceHanSansCN-Medium.otf` among them); `1001_lightmap.webp#s#000#00002#` is a name of **Deponia 1's** data that the test loads: Deponia 4's `scenes/scene.vs000` has **0 entries** (18 of its 168 scene volumes are empty; Deponia 1's has 1,445, Deponia 2's 3,005), so "Error reading header" is right, not a difference of the engine and not the open point of Deponia 2 (that one is a name that exists in Deponia 2's data; see there).

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
Most of the larger counts are the `-O0` instantiations (glm is one: Deponia 1 uses it too, `circleLineSegmentIntersection(glm::tvec2<float> const&, ...)` has it in its signature, but `-O2` inlined every glm function so none is a symbol; the users of glm in Deponia 4 are `Model`, `Camera`, `Light`, `Mesh` and `Animator` (the 3D models read with Assimp), `TGraphicsOGL::SetOffset` and the way system geometry: `TGWaySystem::CalculateWayTriangles`, `closesPointOnTriangle`, `toVec2`). One is not: **no HarfBuzz in Deponia 4**.

### Text rendering (fonts): what Deponia 4 does differently

Read in both listings (`TFreetypeFont::GetChar`, `CreateChar`/`CreateCharIndex`, `RenderString_i`, `SetSpacing`, `LockHeight`, `TCFont::OnEvent`, `TGraphicsOGL::ResizeScene`/`SetOffset`) and checked against the callers of the library functions in both executables. Everything not listed here is the same in the two builds:
the width of a character (a space is `size / 4.2 + letter spacing`, a tab two spaces plus the spacing, CR and LF nothing), the stroker border (`FT_Stroker` + `RenderSpans`), the fallback font for a character the font lacks (a second `TFreetypeFont` made on demand,
warning `character %s / %d not in %s`), the shader hook (`shader_list`, uniforms `mvp_mat`, `texel`, `color`, `offset`), and the whole of `TCFont`, `TFontManager` and `TTText` as far as the call graph, the numbers and the texts show (no engine call, number or text that one has and the other has not; the
functions differ in size only by what `-O0` does and by the points below).

1. **No text shaping in Deponia 4.** In Deponia 1 `TFreetypeFont::RenderString_i` turns the string into a wide string (`toUTF`), cuts it into runs of one script (`itemize(wxString&)`), and for each run makes a HarfBuzz buffer
   (`hb_buffer_set_direction`, `hb_buffer_set_script`, `hb_language_from_string`/`hb_buffer_set_language`, `hb_buffer_add_utf32`), a font (`hb_ft_font_create`), calls `hb_shape` and reads `hb_buffer_get_glyph_infos`/`_positions`; the glyph indices
   go through `TFreetypeFont::GetCharIndex(uint)` (337 bytes). This is the **only code in the executable that calls HarfBuzz**. Deponia 4 reads the string with `decode_utf8_next_char`, one character at a time, and asks `GetChar(code)`: no runs, no direction, no ligatures or letter forms, no right-to-left lines.
   Right-to-left or joined scripts (Arabic) cannot be laid out correctly by Deponia 4's code; Latin, Cyrillic, Chinese, Japanese and Korean need none of it.
2. **The glyph cache is keyed by character in Deponia 4, by glyph index in Deponia 1.** Deponia 1: `HashMap<int,int>` (character to glyph index, `FT_Get_Char_Index`) and `CreateChar` (864 bytes) calls `CreateCharIndex(glyph index)` (3,512 bytes, `FT_Load_Glyph`). Deponia 4: a single `CreateChar(code)` (8,698 bytes at `-O0`)
   with `HashMap<int,Character>` keyed by the character and `FT_Load_Char`. Same bitmaps, same metrics logic.
3. **A distance field path that is switched off.** Deponia 4's `CreateChar` contains the oversampled glyph code (render at 72x72 units with 10% padding via `FT_Set_Char_Size`, `distance_map` (edtaa3), `resize`/`interpolate` back down) but guards it with a local that is set to 0 in the line before it is tested
   (`mov [var_59], 0; cmp [var_59], 0; jz`): it never runs. Deponia 1 has `distance_map`, `edtaa3`, `resize` and `interpolate` in the executable and no caller of them. No difference in what is drawn.
4. **The height lock of the game's fonts (a behaviour change).** Each `TFreetypeFont` has a flag (Deponia 1 `+0xF7`, Deponia 4 `+0xD7`) that stops the highest glyph seen so far (`+0xCC` / `+0xAC`, which sets the top of a line) from being raised when a higher glyph is made
   (`if (!flag && top > maxBearingY) maxBearingY = top` in `CreateCharIndex`, six sites in `CreateChar`). In **Deponia 1 `TCFont`'s constructor and `OnEvent` for the letter spacing (field 0xFA) measure the text "abc" first (`TFreetypeFont::StringLength`, which makes those glyphs) and then set
   the flag together with the spacing**: the line height of every game font is fixed from "abc" on. In **Deponia 4 the flag is a method, `LockHeight()`, called only by `SkinManager::LoadSkin` and `CssSkinnedObject::RefreshStyle` (the editor's GUI fonts)**; `TCFont` only calls `SetSpacing(int)`, which stores the spacing.
   So in Deponia 4 the top of a line follows the highest glyph created so far; a text that first shows a taller glyph (a capital with an accent, a CJK character) than the ones shown before can move the lines of that font from then on. This reads like a fix of Deponia 1: **to do**: confirm with a run (`renderscene` with two texts) once the text path is built.
   (Done in `src/deponia4`: `TFreetypeFont` has `SetSpacing()` and `LockHeight()` and a lock flag that `glyph()` honours; `TCFont` only calls `SetSpacing()`.)
5. **A cache of the rendered game text in Deponia 1.** `TGText` has a `std::vector<GLCharBuffer*>` (made in its constructor, destroyed in its destructor) in Deponia 1; `TGText::Draw` hands it to `TFontManager::PrintTextLines`, `TCFont::PrintTextLines` makes a `GLCharBuffer` per line and passes it to `RenderString`, which can draw from it
   again. In Deponia 4 `TGText` has none and `PrintTextLines`/`PrintText` have no buffer parameter: the text is laid out again every frame. (The GUI widgets `Label`, `ListBox`, `TextBox`, `Tooltip`, `Button` keep `GLCharBuffer`s in both builds; Deponia 4 has `GLCharBuffer::Clear()` as a method.) Only speed.
6. **The `outline` shader and `borderColor`.** Deponia 1 has an extra shader file name `outline.bin` (loaded in `TGraphicsOGL::ResizeScene` when the shader object at `+0x6B0` is not valid yet) and sets the uniform `borderColor` (the border colour as r, g, b and the alpha of the font) on the text shader in `RenderString_i` when the font has a border. Deponia 4 has neither string; its shader names have `sht_color.bin`
   which Deponia 1 lacks. `SetOffset` updates `mvp_mat` of 7 shader objects in Deponia 1 and of 5 in Deponia 4. The other 50 shader names are in both.
7. **Layout of `TFreetypeFont`**: Deponia 1's is 0x130 bytes, Deponia 4's 0x110 (the allocation of the fallback font in `CreateChar`): Deponia 1 inserted 0x20 bytes ahead of the border flag (`+0xD8`/`+0xB8`; in the reconstruction the HarfBuzz font `_shaper`). Spacing `+0xF0`/`+0xD0`, valid `+0xF4`/`+0xD4`, size `+0x108`/`+0xE8`, fallback font `+0x110`/`+0xF0`.
8. Signatures: Deponia 1 `TFontManager::PrintText(wxString const&, TextAlignmentEnum, wxPoint const&, float, GLCharBuffer*)`, `TFontManager::PrintTextLines(..., std::vector<GLCharBuffer*>*)`, `TCFont::PrintText(..., int, GLCharBuffer*)`; Deponia 4 the same without the buffer.
   Deponia 4 has `TFontManager::GetFonts()` (an accessor) that Deponia 1 does not.

**`src/deponia4` now has Deponia 4's text path** (22 Oct 2026): `TFreetypeFont` has no HarfBuzz (no `itemize`, `hb_*`, `_shaper`), keeps its glyphs by character (`FT_Load_Char`) and draws a string one character after the other (pen moved by the advance times the scale and the spacing times the scale; a space or tab by
its advance, which has the spacing in it, plus the spacing again; `StringLength` counts a space by the cached advance and only by `size / 4.2` while that is 0); `TCFont::createFreetypeFont` no longer measures "abc" and only calls `SetSpacing()`; `PrintText`/`PrintTextLines` of `TCFont` and `TFontManager` have no `GLCharBuffer` parameter and `TGText` no buffer
vector. Not touched: the `outline` shader and the `borderColor` uniform (the reconstruction draws glyph sprites, there is no such shader here), the fallback font (not done in either), the `GLCharBuffer` class (the GUI widgets have it). Checked (23 Oct 2026): **all 71 test programs of `tests/deponia1` other than the two that need game data, built from `src/deponia4` (`--game deponia4`), end with 0 failures**, as do `ftfonttest` and `realdatatest` (up to its own Deponia 1 checks). The sources compile; `fonttest`, `texttest`, `texttest2`, `ttexttest`, `cmdtexttest` and `talktest`
(built with `--game deponia4`) give 0 failures; `realdatatest` from this build makes 12 TrueType fonts from Deponia 1's data (0 failures) and 3 from Deponia 4's. `tests/deponia1/ftfonttest.cpp` (new; run from the Deponia 4 game folder with `GAME_PASSW`, `tools/tbuild.py ftfonttest.cpp --game deponia4`) makes the TrueType font of the data (`SourceHanSansCN-Medium.otf`, size 32) and checks, with a backend that records the draws: the space advance is `int(size / 4.2 + spacing)` when the spacing is set before it is first asked for (`size / 4.2` without), a tab two spaces plus the spacing, `StringLength` with the spacing after every character, the pen between two glyphs (a, spacing, space, spacing; at scale 1 and 2), a new line, the top of a line moving down by the difference of the bearings when a taller glyph is made (x, then H), both glyphs of "xH" on one baseline, and `LockHeight()` keeping the line still (and `CreateFont()` letting go of it). 0 failures; checked that it fails (3 checks) when the lock is ignored and when the spacing is left out of the space advance. The Deponia 1 order (measure "abc" first, spacing after) gives a space without the spacing: that is what check 1 tells apart.
For the merge, 1 and 5 are build options (HarfBuzz yes/no), 4 is a bug fix that the newer behaviour should win, 6 follows the shaders that ship with the data.

### Platform SDKs and the client interface

(symbols, scores) `TGameClientSDK` has 8 methods only in Deponia 4 (`InitSDK(wxString const&, wxString const&)`, `GetGalaxySDK`, `GetSteamSDK`, `GetGalaxyStatus`, `GetSteamStatus`, `DeleteCloudSavegame(wxString const&)`, `Update`):
the layer that owns both the Steam and the Galaxy objects is explicit in Deponia 4. `TSteamSDK` (9 of 9 functions unlike) and `TGalaxySDK` (9 of 9) both differ. `CSteamID` is in the symbols of Deponia 4 (Steam: 1 symbol; Deponia 1: 0).
`changes.txt`: "GOG: Galaxy Overlay Support", "Steam: Achievements ..."; `libGalaxy64.so`/`libGalaxyPeer64.so` ship with this build. Read (22-23 Oct 2026): every function of `TGameClientSDK`, `TSteamSDK`, `TGalaxySDK` and the Galaxy listener classes, strings and calls, in both builds: **no string differs anywhere**; the calls differ only where Deponia 1 inlined (`GetSteamSDK`/`GetSteamStatus`/`GetGalaxyStatus` are accessors in Deponia 4, `TGalaxySDK::IsActive` is a call there), where Deponia 4's `-O0` shows constructors of the listener classes and of `wxDynamicLibrary`, and the logging overload. The platform layer is the same code.

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

### Floating point constants: the same

`tools/floatconsts.py` reads the values that functions load from `.rodata` (`movss 0x...(%rip),%xmm0`), which the numbers check does not see. Over all `T*` code: 62 distinct values in Deponia 1, 54 in Deponia 4; 12 only in Deponia 1, 4 only in Deponia 4. All of them explained:

- **`TGraphicsOGL::SetOffset(float, float)`, the `depthView` branch (a 3D camera: `m_rotate_x/y`, `m_scale`)**: Deponia 4 calls the glm functions (`glm::perspective(45, 1.0, 0.3, 10)`, `glm::lookAt`, `glm::ortho`, `operator*=`); Deponia 1 has the same product with the
  glm code inlined, which is where its constants `1.79259`, `-1.06186`, `-0.618557` come from (checked: `1/tan(45/2)` with 45 taken as **radians** = 1.792591, `-(10+0.3)/(10-0.3)` = -1.061856, `-2*10*0.3/(10-0.3)` = -0.618557, i.e. `perspective` with fovy 45, aspect 1.0, near 0.3, far 10).
  The same projection. The `0.0001` that only Deponia 1 loads in this function is not explained (probably an epsilon of an inlined glm `normalize`/`lookAt`). Whether `depthView` is ever set in a shipped game is not known (it is a global of the graphics layer).
- **Compiler**: `1/65536` (`1.52588e-05`, `TParticleEmitter::IncTime`) against a division by 65536, `0.125`/`0.25` against a division by 8 and 4 (`ConvertYUVToRGBA`): GCC `-O2` makes a multiplication of a division by a power of two.
  `99999.9` (`TParticleEmitter` constructor) is in Deponia 1 as the integer immediate `47C34FF3h`. `1e6` in `TMovie::GetTime` is the inlined `get_clock()` of the ffplay code (`av_gettime_relative`) that Deponia 4 calls. `75` in `TPictureWebP::Write` is libwebp's default quality inside its inline
  `WebPConfigInit`; both builds then set the quality of the encoded picture to **85** (Deponia 4 loads `85.0` from `.rodata`, Deponia 1 has `config.quality = 85` as an immediate; the reconstruction has 85).
- `TDiagnostic::DrawDiagram` (40, 960): the diagnostics drawing, the same values as immediates in Deponia 1.

### Conditions and results: the same, checked by running the code

A flipped `<` or `&&` is neither a call, a number nor a text, so three more checks were made (all of the two originals, Deponia 1 `-O2` and Deponia 4 `-O0`).

- **`tools/origdiff.py --conds`** (comparisons with a constant, as the split of the numbers that they make, so `x > 16` and `x >= 17` are alike): 8,941 functions in both have such a comparison, 5,142 differ: not usable, `-O0` against `-O2` turns switches into jump tables or bit tests and
  ranges into one unsigned compare. (`TTAction::IsImmediateExecutionType` is a chain of 16 compares in Deponia 4 and a pile of `lea/cmp/bt` tricks in Deponia 1; read by hand the sets are the same 16 values.)
- **`tools/condcompare.py`** compares the reconstruction compiled at `-O0` (clang) with Deponia 4's `-O0` (GCC): both are transcriptions of the source, so only the compilers differ. 344 of 7,043 functions differ in their tests (127 with a jump table left out); with `--first` (Deponia 1) a test counts only when the
  Deponia 1 original does not have it either: 63 functions left. Read and found alike: `TVisionaireObject::Release` (the assert is `0 < RefCount < 3000` in both: `(RefCount - 1) <= 2998` unsigned in Deponia 1, `RefCount != 0 && RefCount <= 2999` in Deponia 4), `TGameControl::MoveScene` (the same angle ranges, 91..269 and 90..270; the reconstruction
  writes them as the unsigned range tests of Deponia 1), `TComposedFile::InitEntries` (a count of 0 to 99,999), `TTText::ReplaceValues`, `FindObjectByNameRelative`, `TSoundFFMPEG::SetStats` (volume 0..100, pan -100..100), `TVisionaire::BinaryLoad` (the tests of the ids 0x347 and 0x3E7 are in a function of their own in Deponia 4, inline in Deponia 1). The rest of the 63 are switch lowerings
  (`THCharacter::OnEvent` and the other `OnEvent`s, `HapticNewEffect*`) and `== true` / `!= -1` tests; they were not all read.
- **`tools/emucompare.py`** runs the same function of both executables in an x86-64 emulator (Unicorn) on the same arguments and compares what it returns (in the register of its return type, which the sources give) and, for a method, what it wrote into a 4 KB block of random small numbers standing for `this`.
  This does not care how the compiler wrote a test. It finds a threshold moved by one (checked: patching `cmpl $0x167` to `$0x166` in a copy of `GetOppositeDirection` makes it report 1 of 479 inputs). Result:
  - **50 free functions and static methods** with number arguments, 150-479 inputs each, **the same on every input**: `EaseLinear`, `lerp`, `BlendColors`, `angleDist`, `diff`, `mirror`, `Rand`, `RandF`, `CompositeEnums`, `TGCharacter::GetOppositeDirection`, `TData::GetDataSize`, `TSText::GetPauseLength`, `TGAction::ConvertToEvent`, the 40 easing functions
    (`Easing::*`), `Noise1234::noise` (1 to 4 floats), `SimplexNoise1234::noise`. 22 more were skipped (they call out of the image).
  - **546 methods of the `T*` classes** with number arguments (60 random argument sets each, the same object block in both): **541 the same**, including their writes into the object, e.g. `TTypeData::IsFittingSaveGameType`, `TTypeGroup::IsFittingSaveGameType` (first reported different: the test passed a "bool" of 56, which only Deponia 4's code reads as
    two different truth values), `TVector3D::Length`. The 5 that differ are not code differences: `TCursorControl::TCursorControl` (+0x48 is the pointer to a second table of virtual functions), `TStandardPaths::TStandardPaths` (+0x8, an address), `TTAction::GetTypeGroup` (returns the address of a static). 891 methods were skipped (they call through a pointer that the block does not have, or take objects).
  - **Layout**: a class whose members moved between the builds would differ on every run; none shows, which also says that the members these 546 methods use are at the same offsets in both builds.
  - **Extended emulator** (22-23 Oct 2026): the calls into the libraries (PLT stubs) are noted by name and answered (`new`/`malloc`, `memcpy`, `memset`, `strlen`, the compares and the math library really; anything else 0), a call through a pointer that is not there lands on a `ret` (noted as a virtual call),
    unmapped pages are made on demand (and let go after each call), every pointer or reference argument gets a block of its own (the first 8 bytes of a reference point at an empty string), and each call has a time limit. Two kinds of blocks alternate (small numbers 0-8, and only 0 and 1, since a `bool` member holding 2 is read differently by GCC
    `-O0` (`xor 1`) and `-O2` (`test`)). Compared per call: the return value in the register of its type, every block after the call (addresses are tokens), and the sequence of library calls (with the allocator, string, math, clock and mutex calls left out).
    `--trials 40`: **1,338 methods ran: 1,267 the same on every input, 2 differ only in blocks with numbers over 1 (the `bool` effect: `TPictureMEM::InsertPixel`, `TGWaySystem::GetNextPosition`), 69 differ, 1,031 were skipped.** Mutation check: `cmp $2` made `cmp $3` in a copy of `TTypeData::IsFittingSaveGameType` is reported (12 of 60 inputs; at 10 inputs it was missed, hence `--trials`).
    **Final run (23 Oct 2026, after stubs for `clock_gettime`, `time`, `sincos`, `av_malloc*`, `fwrite`, `rand`, the single-precision math fix, and with the `std::allocator` constructors, the number of virtual calls and `__dynamic_cast` dropped from the call sequence, `bool` results compared as true/false, the address of a static of the image taken as equal, and an input that does not run (a loop over a random size, a fault) left out instead of the whole method; a method is skipped when three inputs in a row fail): 67 free functions, all the same on every input; 1,587 methods ran: 1,562 the same, 3 differ only in blocks with numbers over 1, 22 differ, 782 skipped** (404 hit the time limit of image 1 on a random size, 266 read unmapped memory through a pointer that the block does not hold, 35 call through one, 23 ask for too much memory, 10 throw).
    The 22 left, read or classified: results that are the address of a static (`TArgument::GetRect/GetPoint/GetSprite`), iterators and structures returned through a hidden pointer or a `wxPoint` copy (`TGWaySystem::CheckPosition`, `TPolygonList`/`TVList::rbegin/rend`, `GetDataObject`, `GetDialog`, `GetActiveCommand`), `void` functions whose "return value" is a leftover register (`TFile::WriteFromBuf`, read in both listings: the same `+0x48 += written` unless the flag is set; `TGameControl::DisplayConsole`, `TConsole::Draw`), the socket helpers of the debugger (`TCPDebuggerClient::Connect`, `TCPDebuggerServer::Bind/GetClientIP`: `inet_aton`/`htons` inlined differently), `TSpriteCache::AddToCache` (`towlower` inlined in the string helper), `TVisionaire::GetListSize` (read in both listings: index tested against the size of the vector at `+0x90` the same way, Deponia 4 through `GetTableConst`; the 7 against 6 is `TTable::GetCount` reading memory that the emulator fills with an address of the image) and members that the stubbed library did not fill. No logic difference among them.
    The 69 of the earlier run, by what differs: **34 only in the sequence of library calls** (the optimizer inlined `std::allocator` constructors, `sin`+`cos` into `sincosf`, `fflush`, the socket helpers; the results and memory are the same); **29 only in a block** (the timer members at `+0x90`, `+0x88`, `+0x298`, `+0x318` of `TManagedObject`, `TGDialog`, `TGScene`, `TGCharacter` hold uninitialised stack bytes of `clock_gettime`, which the stub does not fill;
    `TGText` (the removed buffer vector moved its members); iterators and structures returned through a hidden pointer; stubbed library effects); **8 in the returned value** (addresses of statics `TTAction::GetTypeGroup`, `TArgument::GetRect/GetPoint/GetSprite`; `TMovie::Initialize`, where `av_mallocz` answers 0 and Deponia 1's `-O2` code, which assumes the placement `new` result is not null, goes on while Deponia 4's tests it;
    functions that end in a tail call of a `void` function, `TGameControl::DisplayConsole`, `TSpriteCache::RemoveFromCache`; `TFile::WriteFromBuf` over a stubbed `fwrite`). Read in both listings and found alike: `TCAnimation::GetPauseCompletion`, `TConsole::Draw`, `TMovie::Initialize`, `TDataGroup::SetTemporary` (sets bit 1 of the byte at `+0x4B`; first reported through a tokenizing mistake of the tool), `TPictureMEM::InsertPixel` (the same
    byte order for the flag at `+0x72`), `TGWaySystem::CheckPosition`, `TSpriteCache::RemoveFromCache`. **None of the 69 showed a different piece of logic; they were not all read.**
  Not covered by any of the three: the functions that the emulator could not run (about a third; a `wxString` or `TVisObjRef` argument needs a real object, a call that fails when its stubbed library answers 0), class values passed or returned by value, and what a stub answers differently from the library.

So the logic of the engine is the same where it can be checked mechanically: calls, numbers, texts, the lines of the source, floating point constants, the order of the engine's calls, and now 590 functions run on the same inputs. The differences are the ones in the other sections.

### Other classes with the most functions unlike the reconstruction

From `FUNCDIFF.md` (functions under 0.5 / functions in both): `TTable` 32/49 (most of it editor code the reconstruction does not have), `TArgument` 27/55, `TVisionaire` 19/80, `TSoundBase` 17/39,
`TMatrix4` 16/21, `TVisionaireObject` 16/95, `TGameControl` 15/98, `TGAnimation` 14/50, `TComposedFile` 12/19, `TGAction` 11/31, `TXMLWriter` 11/32, `TComposedFileManager` 10/17,
`TGScene` 10/38, `TFontManager` 9/10, `TMovie` 8/18, `TTText` 8/15. `TGAction::Execute(bool, t_SkipCutsceneInfo*)` is 10,083 instructions in the original against 464 in the reconstruction (the `-O0` form of the dispatcher).
None of these has been read yet; the next step is to take them in that order.

## Open points / next steps

1. Extend the emulator comparison to the functions that take objects: build the objects the way the engine does (a `TVisObjRef` with a record of the data, a `wxString`) in the emulated memory, or run the functions in a emulated image with the libraries stubbed. The 891 methods that were skipped are the first candidates.
2. How `#s#000#00002#` (the lightmap of scene 1001) is resolved in Deponia 2 and Deponia 4 (see the data finding); the `table 21` assertion is the same in Deponia 1.
3. ~~The build number of the engine~~ found: 1189 (see the table at the top).
4. Compile the sources here at `-O0` for real (`tools/pbuild.py deponia4`): `tools/tbuild.py <test>.cpp --game deponia4` builds a test of `tests/deponia1` against `src/deponia4` (objects in `build/tests-64[g]-deponia4`); the suite has been run on it (23 Oct 2026, `-g` builds of `src/deponia4` as it is committed, all tests of `tests/deponia1` except `realdatatest` and `ftfonttest`, which need the game folder and were run on their own): 71 tests, 0 failures each. Every source file of `src/deponia4` compiles and links through `tools/tbuild.py` (`tools/pbuild.py` has no include paths for the libraries in `third_party`, so it stops at the first file that needs libvpx, libwebp, FreeType or opusfile; use `tbuild.py`). Still open: the byte comparison with the Deponia 4 executable (`tools/crosscompile.py --tag o0`, `-O0`, was done on the Deponia 1 sources only).
5. Deponia 2 (packaged Dec 2015, in the other thread) is older than this build: most of what Deponia 4 differs from Deponia 1 in should be a stage between Deponia 2 and Deponia 1.
   When Deponia 2 is looked at, use `tools/symdiff.py` and `tools/crosscompare.py --original` as above, and record in which build each difference appears (the engine order is Deponia 2 -> Deponia 4 (Sep 2017) -> Deponia 1 (Feb 2019), not the order of the games).

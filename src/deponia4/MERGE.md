# Merging `src/deponia4` into `src/deponia1`: what differs and what to do with each

This is the working list for the merge into one engine. Every row says what is different between the Deponia 1 build (engine 5.0.1, 8 Feb 2019, `-O2`) and the
Deponia 4 build (engine 5.0.1, 6 Sep 2017, `-O0`), where it is in the code, how it is known, and what the merged engine should do. The evidence, and the checks
that found nothing else, are in [`VERSIONS.md`](VERSIONS.md); the generated reports are `SYMDIFF.md`, `FUNCDIFF.md` and `ENGINECALLS.md`.

**The short version:** the two builds are one code base. Source files that carry line numbers (the asserts) are line for line the same except `vscommon/cfont.cpp` and
`graphicslib/graphicsOGL.cpp`; the data format (XML names, 41 record classes, 583 fields) is the same; the order of the engine's calls, the numbers, the texts, the floating
point constants and the results of about 1,330 functions run in an emulator on the same inputs are the same. What differs is a text path, a few graphics details and the build.

## Differences that need a decision in the merged engine

| # | What | Deponia 1 | Deponia 4 | How known | In the merged engine |
|---|---|---|---|---|---|
| 1 | Text shaping | `RenderString_i` cuts a string into runs of one script (`itemize`), shapes each with HarfBuzz (direction, script, language `ar`), glyphs by index | one character at a time (`decode_utf8_next_char`, `GetChar`), no shaping | calls of `hb_*` (only in `RenderString_i`), listings | a switch in `TFreetypeFont` (`shaping`): on for Deponia 1, off for Deponia 4. Needed for Arabic/right-to-left and ligatures only. `src/deponia1` has the shaped path, `src/deponia4` the plain one |
| 2 | Glyph cache key | `HashMap<int,int>` character to glyph index, glyphs by index (`GetCharIndex`, `CreateCharIndex`) | glyphs by character (`CreateChar`, `FT_Load_Char`) | listings, symbols | follows 1; the same bitmaps and metrics |
| 3 | Height of the game's fonts | `TCFont` measures "abc" (`StringLength`), then sets the lock flag and the spacing: the line top (`_maxBearingY`) is fixed from "abc" | `TCFont` only calls `SetSpacing()`; `LockHeight()` is called by the editor GUI's fonts only: the line top follows the highest glyph made so far | listings of both `TCFont` constructors/`OnEvent`, `CreateChar`; `tests/deponia1/ftfonttest.cpp` (Deponia 4's behaviour) | **Deponia 1's**: it removes a line that moves when a taller glyph first appears. Keep as a setting only if a Deponia 4 data set shows text that depends on the old behaviour |
| 4 | Width of a space with letter spacing | measured with spacing 0 (because of 3): `int(size / 4.2)`, then the spacing is added | `int(size / 4.2 + spacing)` and the spacing again | listings, `ftfonttest` | follows 3 |
| 5 | Rendered text cache | `TGText` keeps `std::vector<GLCharBuffer*>` (glyph vertices per line); `TFontManager::PrintTextLines` and `TCFont::PrintText/PrintTextLines` take the buffer | none: the text is laid out every frame | symbols (signatures), `TGText` constructor/destructor | speed only; the reconstruction does not batch glyphs, so keep Deponia 4's signatures (no buffer). A GL backend can add the cache |
| 6 | Text shader | extra shader file `outline.bin` (shader object at `+0x6B0`, loaded in `ResizeScene`), uniform `borderColor` set in `RenderString_i`; `SetOffset` updates 7 shaders | no `outline.bin`; has `sht_color.bin`; `SetOffset` updates 5 | strings, listings | only for a GL backend: load the shaders the data names. Nothing in the reconstruction |
| 7 | `TGraphicsOGL` | `EnableMipmaps(int)` and `SetEmptyTexture(int,int,int)` are empty; `graphicsOGL.cpp` has 5 + 2 more lines before 0x4F3 / 0x94E | both contain `assert(false)` ("must not be called") | symbols, assert line numbers | keep the empty functions (they are called from nowhere that matters) |
| 8 | `cfont.cpp` | 4 more lines before line 0x155 and 5 more before 0x2A2 | | assert line numbers | probably the `GLCharBuffer` argument and the spacing priming of 3 and 5 (the lines were not located) |
| 9 | Logging | `wxLog::logexpanded(fmt, ...)` at every site | typed `wxLog::log<T...>(fmt, T...)` at most sites | symbols (676 and 65 sites of `logexpanded` against 172 + 161 + 71 + ... of `log`) | keep one API in `WxStub.h`; no behaviour |
| 10 | Platform SDK layer | `TGameClientSDK` methods inline `GetSteamSDK` etc. | explicit accessors (`GetSteamSDK`, `GetGalaxySDK`, `GetSteamStatus`, `GetGalaxyStatus`, `Update`, `InitSDK`, `DeleteCloudSavegame`) | every string and call of the three classes compared: no string differs; the calls differ only in inlining and constructors (the listener classes have explicit constructors at `-O0`) | no difference in behaviour; Deponia 4's accessors are inline helpers of Deponia 1's code |
| 11 | Libraries | HarfBuzz 1.8.8 linked; `libpng12.so.0`, `libopenal.so.1`, `voices_ES/IT.lua` shipped | no HarfBuzz; none of those three | symbols, game folders | HarfBuzz follows 1; the rest is packaging |

## Differences that are in the data or the harness, not the engine

| What | Detail |
|---|---|
| Password | Deponia 1 `ykgT6QsrRNv9XR`, Deponia 4 `RHgPi6dmqvu6uT`, both narrow strings |
| Shader Toolkit script | Deponia 4's data runs a Lua `DataDumper` at load that calls `pcall(string.dump, f)`; ScummVM's `lua_dump` is fatal. `InitLua` in `src/deponia4` replaces `string.dump` by a function that raises a Lua error (`WORKAROUND:`). **Put it in the merged `InitLua`** (and in `scriptingutils.cpp`'s `traverseTable`, which also calls `lua_dump`, not reached yet) |
| Lightmap test | `realdatatest` loads `1001_lightmap.webp#s#000#00002#`, a name of Deponia 1's data; Deponia 4's `scene.vs000` has no entries, so the test line is Deponia 1's own (the open question about Deponia 2's data with this name is in `src/deponia2/VERSIONS.md`) |
| Fonts | Deponia 4 has one TrueType font (`SourceHanSansCN-Medium.otf`), Deponia 1 twelve; `tests/deponia1/realdatatest.cpp` checks for 12 |
| Volumes | Deponia 4: 168 scene, 328 character and 66 video volumes; Deponia 1: 74, 84, 42 |

## What is the same (checked, no merge work)

- the 194 `T*` classes and their methods (Deponia 4's 214 include 20 helper classes whose members Deponia 1 inlines), the XML name table, the record schema;
- the order of engine calls of every function that both define (51 of 2,923 functions differ for Deponia 1, none a logic difference; devirtualized calls, sort comparators, `x_assert`, `toUTF`);
- the numbers (1,028 against 1,030 distinct constants in `T*` code; the rest are assert line numbers and `-O0` case labels), the texts (1,758 against 1,739 literals; standard library exception texts and format helpers), the floating point constants, the order of the field ids written by the constructors read;
- the line numbers of the asserts in 38 of the 39 source files that have them;
- the class layouts as far as the 1,338 methods that ran in the emulator on random objects use them (a moved member shows as a difference; only `TFreetypeFont` and `TGText` did).

## Not checked

Conditions in functions the emulator could not run (about a third of the methods: those needing a real `wxString`, `TVisObjRef` or object graph), the build number of Deponia 4's engine, the details of
`TGraphicsOGL`'s other methods, the shaders in the data, and anything that only shows in play (`src/deponia4` has been run on the data up to the end of loading, not played).

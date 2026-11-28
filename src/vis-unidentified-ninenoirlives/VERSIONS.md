# Nine Noir Lives (2022 release; Silvernode Games)

Looked at on 19 Nov 2026. Folder: `D:\Games\ScummVM\Dev\games\Visionnaire\NineNoirLives-Linux` (GOG, game id 1805490116, build id 57807033147516547). Nothing of the engine is reconstructed here yet; this file is the log of what is known and how.

## The build
- Player: `visplayer` (the other games' players are named after the game), 71,170,704 bytes, sha1 `0593af8adec6`, ELF64 x86-64, **not stripped**, `.comment`: GCC 4.8.5 (Red Hat) and GCC 7.3.1 (`-O3 -std=c++1z -fvisibility=hidden -fPIC`; so an optimized build like Deponia 1 and the Rabbit, not the `-O0` of Deponia 4). Built on the same Jenkins machine as the others (`/home/simon/Documents/jenkins/vis-repo/...`). Lua **5.4.0** (as the Rabbit).
- Start script `start` runs `./visplayer`, log in `$XDG_DATA_HOME/Silvernode Games/Nine Noir Lives/messages.log`; `config.ini` has `FILE = NineNoirLives.vis`, `FULLSCREEN`, `RESOLUTION`, `INTRO`, `LANGUAGE`, `LOGLEVEL` (the Rabbit's `configure`/`configtool` are gone).
- The engine version string ("Engine Version: %s (Build %d ...)") is **not found** as a narrow or wide string; the version is still to be found (`InitVersionedIds`, `TVisionaireGame::InitWithVersion`/`UpdateVersion` exist).

## Debug information: yes, but not for the engine (checked in the executable)
- `file` says "with debug_info, not stripped": there are `.debug_info` (25 MB), `.debug_abbrev`, `.debug_line`, `.debug_str`, `.debug_loc`, `.debug_ranges`, `.debug_aranges`, and a full `.symtab` (66,775 entries; 55,156 defined symbols; 34,179 functions) with the mangled names, as in the other Linux builds.
- **All 105 compile units of the DWARF are third-party code** (Assimp 97, poly2tri 5, Open3DGC 2, irrXML 1, built from `vis-repo/src/assimp-cmake/...` with `-g`). The engine's own classes (`TGameControl`, `TVisionaireObject`, `TComposedFile`, ...) have **no** DWARF: no struct layouts, member names or line numbers for them. So the invented member names stay invented, as for the other games; what is new against them is only the readable symbol table (and `symbols.txt` in the game folder, a `readelf -s` dump of the same table, which ships with the game).

## The engine is a newer generation than the Rabbit's (5.0.3)
`python tools/symdiff.py rabbit visplayer` (`build/nnl_symdiff.md`, not kept in the repository): 
- Libraries (symbols: Rabbit / Nine Noir Lives): Assimp 1243/1985, **Box2D 0/538**, FreeType 944/937, HarfBuzz 563/1006, Lua 495/497, SDL2 3325/3396, libpng/zlib 475/511, curl 60/83, wxWidgets 275/298; Spine is `spine::` in both (nm: 896 symbols in this build; `symdiff` does not see the namespace). New in the symbol table: **antlr4 (1,178) with `IliosParser`/`IliosTreeCreator`/`LuaParser`/`LuaLangVisitor`** (a parser for a Lua dialect or for the scripts of the editor), **asmjit (143)**, p2t.
- 15 classes only here: `TButton`, `TItem`, `TObject`, `TTComponent`, `TTCurve`, `TCurveCache`, `TSpritePartTexture`, `TPictureSVG`, `TMXRenderer` (Tiled maps?), `TTokenStream`, `TernaryNode`, `TokenRange`, `Transition`, `TransitionTag`, `TGraphicsSubSystem`. 23 classes only in the Rabbit, among them `TGObject`, `TGItem`, `TGActionArea`, `TGCommand`, `TGParticleEmitter`, `TGParticleSystem`, `TGPlaceHolder`, `TGScrollButton`, `TDataGroup`, `TFontTexture`: the scene objects are a different set of classes (a component model: `TObject` + `TTComponent`, `TCurveCache`...), so this is not a Deponia-style `TGObject` engine with additions. 438 methods only here and 373 only in the Rabbit (of the classes both have); 10,528 free functions/globals only here (mostly the new libraries).
- The `TCFont` of this build has full-size text drawing (`DrawFullSizeText`, `RenderTextAtFullSize`, `fullSizeFonts`) and `PrintText` takes a `TMatrix3`.

## The data: VIS5, not VIS3
- `NineNoirLives.vis` 380,922,350 bytes starts `VIS5` and a big endian count 33,010 (`0x80F2`); the directory after it is encrypted (a repeating pattern of printable bytes, as the key stream of the VIS3 directories, but not the VIS3 layout: `tools/find_vis_password.py` stops at "not a VIS3 container"). The volumes (`Scene000.vs`..., 110 of them; `Character000.vc`..., 31; `Interfaces.vi`; `Lang_English.vt`) start `VIS5` + `FFFFFFFF` and then **the first file's own bytes** (a PNG, an Ogg stream) with parts that look altered (`IHDR135.402` in a PNG header: the size fields are not binary), so only part of each entry is scrambled. The videos `Video000.vv`... (31) are plain MP4 (H.264, `ftypisom`), no container, no encryption.
- Where to read next: `TComposedFile::Init/InitEntries/Open`, `TComposedFileManager::DecryptComposedFile`, `TComposedFileManager::InitMainContainer` (the symbols exist; IDA listing needed: `idat64 -A -B` as in `VERSIONS.md`).
- The password is not in the executable's strings the way the other games' is (not found by `find_vis_password.py`; that tool needs the VIS5 layout first).

## Next steps
1. IDA listing of `visplayer` (`build/nnl/…`), then the VIS5 reader (directory, key, the partly scrambled entries) so the data can be listed.
2. The data model: `tools/extract_xml_names.py`/`extract_schema.py` against `manifest/` to see how far the Rabbit's fields carry over.
3. Decide how much of `src/vis-5.0.3-2025` can be the base (the 23/15 class difference says the scene-object layer has to be written new).

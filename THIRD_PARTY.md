# Third-party libraries of the original (Deponia 1 for Linux, engine 5.0.1 build 1189, 8 Feb 2019)

How this was found: the strings and symbol names of the 64-bit ELF `Deponia` (the GOG Linux release), the libraries shipped
next to it (`libs64/`, `documents/licenses/readme.txt`), and what the game data (`data.vis`, 3109 files) actually uses
(`tests/deponia1/datainventory.cpp` lists it; run it from the game directory). The versions marked ~ are inferred from
which functions or files are present, not read from a version string.

## What is statically linked into the executable

| Library | Version | Evidence | Deponia 1 uses it? |
|---|---|---|---|
| LuaJIT | 2.1.0-beta3 (built with `LUAJIT_ENABLE_LUA52COMPAT`) | the string `LuaJIT 2.1.0-beta3` | The scripts use plain Lua 5.1 only (no `ffi`, `bit`, `goto`), plus `os.time`, `os.date`, `io.open/close` and one `require("lfs")` |
| libwebp (decoder and encoder) | ~0.5.0 | `WebPPictureSmartARGBToYUVA` is there (0.5.0), no SSE4.1 variants (0.5.1+); `WebPEncode*` too | **Yes: every picture is WebP** (15029 animation sprites, 88 more in actions) |
| libopus | 1.1.x | the string `opus 1.1`, `opus_custom_decoder_ctl` | **Yes: all 1244 sounds and the speech are `.opus`** |
| libvorbis / libogg | libvorbis 1.3.5 (20150105) | `Xiph.Org libVorbis I 20150105` | The videos' audio track is Vorbis |
| zlib | 1.2.5 | `deflate 1.2.5`, `inflate 1.2.5` | Yes (data containers); the shipped `libz.so.1` is a different one |
| libpng | 1.6.20 (3 Dec 2015) | the string `libpng version 1.6.20 - December 3, 2015` | A decoder exists; the game data has no PNG |
| FreeType + HarfBuzz | FreeType 2.8.0 (`FREETYPE_MAJOR/MINOR/PATCH` of the library), HarfBuzz 1.8.8 (`hb_version_string` returns "1.8.8"; it has `hb-ucdn.cc`, `hb-ft.cc`) | the version strings and the file names | Only for the TrueType/OpenType fonts of Chinese, Korean and Arabic (12 fonts); the other languages use bitmap fonts |
| SDL2 | 2.0.4 (has `SDL_WarpMouseGlobal`, not `SDL_SetWindowResizable`) | symbol set | the window, input, joysticks; ScummVM replaces it |
| GLEW | 1.11.0 | the string `1.11.0`, `glew.c` | the OpenGL backend; ScummVM replaces it |
| Box2D | 2.3.x (chain shapes, rope, motor joints) | `b2*.cpp` names | The engine binds it for Lua (`tolua_b2_*`); **no script of the game uses it** |
| Assimp | 3.x (Collada, FBX, OBJ, X, IRR, Assbin importers) | `*Loader.cpp` names | **No**: no 3D model animation in the data |
| Spine runtime (C) | ~3.4 (has path constraints) | `spine/*.c` names | **No**: no Spine animation in the data |
| libcurl | 7.45.0 | the string `libcurl 7.45.0` | only the cross promotion banner (`CrossAppPromotion.lua`) |
| PCRE | 8.36 (2014-09-26) | the string | the `rex_pcre` Lua module; the scripts do not `require` it |
| LuaFileSystem | 1.6.3 | the string | one script (`VED_Archimedes.lua`, a developer tool) |
| LuaSocket | 2.0.2 | the string | the script editor's debugger |
| rapidxml 1.13, rapidjson, jpgd, mysql++, tolua++ | - | symbol names | XML projects (editor), JSON of audio busses (none in the data), JPEG (none in the data) |

## Shared libraries that the game shipped (`libs64/`)

FFmpeg 2.8.x (`libavcodec.so.56`, `libavformat.so.56`, `libavutil.so.54`, `libswresample.so.1`, `libswscale.so.3`,
`libavfilter.so.5`, `libavdevice.so.56`), OpenAL (`libopenal.so.1`), libpng12, zlib 1.2.8 (`libz.so.1`), and the GOG Galaxy SDK
(proprietary, not needed). FFmpeg plays the 42 `videos/video.vvNNN` files: **Matroska with VP8 video and Vorbis audio**.

## What the data needs, in short

Picture decoder (WebP), sound decoder (Ogg Opus), video decoder (Matroska + VP8 + Vorbis), Lua 5.1 with `os` and `io`, the
engine's own container and project formats. Everything else in the table is for the editor, the other Visionaire games, or the
platform layer that ScummVM provides.

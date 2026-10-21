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
| Engine build string | the format string is the same as Deponia 1's ("Engine Version: %s (Build %d from Build date: %s)"); the values are not in the executable's strings, so they have not been found |
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
- Which build of the engine this is: the version and the date of the build are not found. The `Engine Version` text is
  built from values that are not in the strings of the file.
- Whether the rest of the data (actions, scripts, dialogs, characters) loads and plays as in Deponia 1: only the load
  was checked.
- Videos (57 Matroska volumes): not decoded with `TMatroskaMoviePlayer` yet.

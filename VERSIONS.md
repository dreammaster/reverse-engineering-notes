# Versions of the engine and of the games

The Visionaire engine was shipped in many builds: one per game, per platform and often per patch. The reconstruction in
`src/deponia1/` is of **one** of them (the Deponia 1 Linux build). This file tracks what is known about the others and where
they differ; the details of each game are in its own folder (`src/<game>/VERSIONS.md`), the reasons for the code are in
`src/deponia1/NOTES.md`.

Evidence comes from the executables (dynamic symbols, strings, section sizes), from the game data (`data.vis` and its
containers) and from running the reconstruction on the data. Say which of the three a statement rests on.

## The builds that have been looked at

| Game | Platform | Executable | Engine build | Project version in `data.vis` | Status |
|---|---|---|---|---|---|
| Deponia 1 | Linux (GOG, game version 4.0.1386) | `Deponia`, 18.9 MB, sha1 `f4cad9601c77` | 5.0.1, build 1189, built 8 Feb 2019 | 3109 | **The reconstruction target.** Plays through the start, the tutorial and chapter 1 headless and in the SDL2 player ([details](src/deponia1/VERSIONS.md)) |
| Deponia 2 | Linux (GOG, packaged Dec 2015) | `Deponia2`, 22.4 MB, sha1 `6d2042d8c5a2` | older than the above (no build string recovered yet) | 2629 | Data loads with the Deponia 2 password; the SDL2 player reaches the language select ([details](src/deponia2/VERSIONS.md)) |

Not yet looked at: the Windows and macOS builds of either game, other Visionaire games (Chaos on Deponia, Goodbye
Deponia, Deponia Doomsday, Edna & Harvey, The Whispered World, ...), and Linux builds of other patch levels.

## What the Linux builds have in common

- The same `VIS3` container for `data.vis` and for the volumes (`scenes/scene.vsNNN`, `characters/character.vcNNN`,
  `videos/video.vvNNN`): `"VIS3"`, the big endian count, then an encrypted directory of `count * 16 + 6` bytes
  (`"HDR"`, the entries, `"END"`) that is XORed with the first 16 hexadecimal digits of the MD5 of the game's password.
- The same code base: the `T*` class names, and most of their method signatures, are the same (see the matrix).
- The same libraries around it: ffmpeg 56 (`libavcodec.so.56` and the rest), zlib, GOG Galaxy.

## What differs between builds (matrix)

| | Deponia 1 Linux | Deponia 2 Linux |
|---|---|---|
| Executable size | 18,926,351 | 22,364,946 |
| `.text` size | 8,449,712 | 9,544,024 |
| Dynamic functions / symbols | 11,665 / 16,521 | 8,437 / 12,431 |
| `T*` classes / methods | 194 / 2,896 | 177 / 2,582 |
| Password of `data.vis` | `ykgT6QsrRNv9XR`, a narrow string | `aqy4rnsmsd8jTw`, a wide (UTF-32) string |
| Command line option texts | has `password`, `name of start scene`, `debugger adress`, `profiling mode` | has `password`; has no `name of start scene`, `debugger adress` or `profiling mode` |
| Graphics layer | `TGraphicsSubSystemGL`, `TShader`, `TFramebuffer`, `TTexture`, `TBuffer` | only `TGraphicsOGL` and `TAlphaTexture`; no shader or framebuffer classes |
| Debugging aids | TCP debugger server and client, clipboard, tooltips | none of them |
| Libraries in `libs64` | with `libopenal.so`, `libpng12.so.0` | without them (the system ones) |
| String types of the helpers | `std::string` (`LuaDoString`, `LuaExecuteFunction`), `x_assert(bool, char const *, char const *, int)` | `wxString` for the same, `x_assert(bool, wchar_t const *, wchar_t const *)` without the line |

How much the shared code differs: of 7,082 functions that both executables define under the same symbol, 46% have the same
size. Where they differ most (functions of another size / functions in common): `TVisionaireObject` 87/98, `TGameControl`
65/93, `TConsole` 54/59, `TVisionaire` 48/85, `TGCharacter` 45/71, `TTable` 39/49, `TGraphicsOGL` 31/41.
`tools/compare_binaries.py` prints this for any two builds.

## How to look at a new build

1. `python tools/compare_binaries.py <known build> <new build>`: the sizes, hashes and the table above for the pair.
2. `python tools/find_vis_password.py <new executable> <its data.vis>`: finds the password of the data (every string of
   the executable, narrow and wide, is tried against the directory of the container; the entries must lie inside the
   file, because the key is made of hexadecimal digits only and a check of `HDR`/`END` alone lets false hits through).
3. Run the reconstruction on its data with the password: `tests/deponia1/realdatatest.exe` with `GAME_PASSW=<password>`
   from the game folder, or the SDL2 player with `-p <password>`. The first thing that fails is the first difference.
4. Write what was found in `src/<game>/VERSIONS.md` (create the folder for a new game) and add a row to the tables above.

Keep the runs out of the game folders: the engine writes a `userdata` folder next to `data.vis` (remove it, or run
from a copy).

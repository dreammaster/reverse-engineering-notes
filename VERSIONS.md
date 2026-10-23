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
| Deponia 4 (Doomsday) | Linux (GOG, game version 1.2.0267) | `Deponia4`, 27.5 MB, sha1 `7d77245bba56` | 5.0.1, built 6 Sep 2017 (**older** than Deponia 1's), compiled with `-O0` | 2416 entries | Data loads with the Deponia 4 password; a copy of the reconstruction lives in `src/deponia4/`, nothing changed in it yet ([details](src/deponia4/VERSIONS.md)) |

The engine builds are not in the order of the games: Deponia 4's engine (6 Sep 2017) is older than Deponia 1's (8 Feb 2019); Deponia 2 was packaged in Dec 2015 (its build date is
not recovered). Deponia 4 is compiled without optimization (`-O0`; Deponia 1 is `-O2`, Deponia 2 not checked), which changes what every comparison of code means (see `src/deponia4/VERSIONS.md`).

Not yet looked at: the Windows and macOS builds of either game, other Visionaire games (Chaos on Deponia, Goodbye
Deponia, Deponia Doomsday, Edna & Harvey, The Whispered World, ...), and Linux builds of other patch levels.

## What the Linux builds have in common

- The same `VIS3` container for `data.vis` and for the volumes (`scenes/scene.vsNNN`, `characters/character.vcNNN`,
  `videos/video.vvNNN`): `"VIS3"`, the big endian count, then an encrypted directory of `count * 16 + 6` bytes
  (`"HDR"`, the entries, `"END"`) that is XORed with the first 16 hexadecimal digits of the MD5 of the game's password.
- The same code base: the `T*` class names, and most of their method signatures, are the same (see the matrix).
- The same libraries around it: ffmpeg 56 (`libavcodec.so.56` and the rest), zlib, GOG Galaxy.

## What differs between builds (matrix)

| | Deponia 1 Linux | Deponia 2 Linux | Deponia 4 Linux |
|---|---|---|---|
| Executable size | 18,926,351 | 22,364,946 | 27,476,740 |
| `.text` size | 8,449,712 | 9,544,024 | 9,512,752 |
| Dynamic functions / symbols | 11,665 / 16,521 | 8,437 / 12,431 | 32,621 / 37,504 (`.symtab`: 69,377) |
| `T*` classes / methods | 194 / 2,896 | 177 / 2,582 | 214 / 3,140 (the 20 more are inline helpers of Deponia 1) |
| Password of `data.vis` | `ykgT6QsrRNv9XR`, a narrow string | `aqy4rnsmsd8jTw`, a wide (UTF-32) string | `RHgPi6dmqvu6uT`, a narrow string |
| Command line option texts | has `password`, `name of start scene`, `debugger adress`, `profiling mode` | has `password`; has no `name of start scene`, `debugger adress` or `profiling mode` | as Deponia 1 (all four, narrow) |
| Graphics layer | `TGraphicsSubSystemGL`, `TShader`, `TFramebuffer`, `TTexture`, `TBuffer` | only `TGraphicsOGL` and `TAlphaTexture`; no shader or framebuffer classes | as Deponia 1 (`TGraphicsSubSystemGL`, `TShader`, `TFramebuffer`, `TTexture`, `TBuffer`; no `TAlphaTexture`) |
| Debugging aids | TCP debugger server and client, clipboard, tooltips | none of them | as Deponia 1 (`TCPDebuggerServer`/`Client`, clipboard, `Tooltip`) |
| Libraries in `libs64` | with `libopenal.so`, `libpng12.so.0` | without them (the system ones) | with `libopenal.so`, without `libpng12.so.0` |
| String types of the helpers | `std::string` (`LuaDoString`, `LuaExecuteFunction`), `x_assert(bool, char const *, char const *, int)` | `wxString` for the same, `x_assert(bool, wchar_t const *, wchar_t const *)` without the line | as Deponia 1 (`std::string` helpers, `x_assert(bool, char const *, char const *, int)`) |

The differences of Deponia 4 from Deponia 1 that need a decision when the engines are merged are collected in `src/deponia4/MERGE.md`: the text path (HarfBuzz shaping, the height lock of the game's fonts, the rendered-text cache), the outline shader and a few graphics details; everything else that could be checked mechanically is the same.

Deponia 4 sits with Deponia 1 on every row of this table where Deponia 2 does not: the two share the graphics layer, the debugging aids and the `std::string` helpers, so the
newer generation of the engine starts between Deponia 2 and Deponia 4. Further rows that only Deponia 4 and Deponia 1 differ in (`-O0` against `-O2`, HarfBuzz, typed `wxLog::log<T...>` against
`wxLog::logexpanded`, the glyph-buffer variants of the text printing) are in `src/deponia4/VERSIONS.md`.

How much the shared code differs (Deponia 1 compared with Deponia 2): of 7,082 functions that both executables define under the same symbol, 46% have the same
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

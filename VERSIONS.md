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
| Deponia 2 | Linux (GOG, packaged Dec 2015) | `Deponia2`, 22.4 MB, sha1 `6d2042d8c5a2` | 4.2.5, build 1185, built 6 Dec 2015 | 2629 | Data loads with the Deponia 2 password; the SDL2 player reaches the language select ([details](src/deponia2/VERSIONS.md)) |
| Deponia 3 | Linux (GOG, game version 3.3.3335) | `Deponia3`, 22.4 MB, sha1 `5792a699a8c4`: **Deponia 2's executable** (34 bytes differ: the build id and the password) | 4.2.5, build 1185, built 6 Dec 2015 | 2925 entries | Data loads with the Deponia 3 password (`kML8jCwHeNtdY3`, wide); the test of the real data stops only at Deponia 1's own expectations ([details](src/deponia3/VERSIONS.md)) |
| The Night of the Rabbit | Linux (GOG, game version 2.2.334) | `rabbit`, 15.7 MB, sha1 `a2be06819343` | **5.0.3, build 1192, built 18 Feb 2025** (the newest), GCC 4.8.5 / 7.3.1 (Red Hat), **Lua 5.4** | 842 XML names, 44 record classes, 619 fields (a superset of Deponia 1's) | Data format read; the scripts stop at Lua 5.3 syntax (`&`), which ScummVM's Lua 5.1 does not have ([details](src/nightoftherabbit/VERSIONS.md)); first look only |
| Deponia 4 (Doomsday) | Linux (GOG, game version 1.2.0267) | `Deponia4`, 27.5 MB, sha1 `7d77245bba56` | 5.0.1, build 1189, built 6 Sep 2017 (**older** than Deponia 1's, the same build number), compiled with `-O0` | 2416 entries | Data loads with the Deponia 4 password; a copy of the reconstruction lives in `src/deponia4/`, nothing changed in it yet ([details](src/deponia4/VERSIONS.md)) |

The engine builds are not in the order of the games: Deponia 4's engine (6 Sep 2017) is older than Deponia 1's (8 Feb 2019); Deponia 2 was packaged in Dec 2015 (its build date is
not recovered). Deponia 4 is compiled without optimization (`-O0`; Deponia 1 is `-O2`, Deponia 2 not checked), which changes what every comparison of code means (see `src/deponia4/VERSIONS.md`).

Not yet looked at: the Windows and macOS builds of these games, other Visionaire games (Chaos on Deponia, Edna & Harvey, The Whispered World, ...), and Linux builds of other patch levels. (Chaos on Deponia is Deponia 2, Goodbye Deponia is Deponia 3, Deponia Doomsday is Deponia 4.)

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

Deponia 2 is the older engine that differs most: its data model is a subset of the newer builds' (every field has the same id and type; the newer builds add 3 record classes and 32 fields), and it has none of the Lua hook API, 46 of the Lua graphics functions, the shader layer, the console load/save or the controller key events (details in `src/deponia2/VERSIONS.md`; the one change of a function found by running both is `GetAngle`, which the newer builds take modulo 360).

The differences of Deponia 4 from Deponia 1 that need a decision when the engines are merged are collected in `src/deponia4/MERGE.md`: the text path (HarfBuzz shaping, the height lock of the game's fonts, the rendered-text cache), the outline shader and a few graphics details; everything else that could be checked mechanically is the same.

Deponia 4 sits with Deponia 1 on every row of this table where Deponia 2 does not: the two share the graphics layer, the debugging aids and the `std::string` helpers, so the
newer generation of the engine starts between Deponia 2 and Deponia 4. Further rows that only Deponia 4 and Deponia 1 differ in (`-O0` against `-O2`, HarfBuzz, typed `wxLog::log<T...>` against
`wxLog::logexpanded`, the glyph-buffer variants of the text printing) are in `src/deponia4/VERSIONS.md`.

How much the shared code differs (Deponia 1 compared with Deponia 2): of 7,082 functions that both executables define under the same symbol, 46% have the same
size. Where they differ most (functions of another size / functions in common): `TVisionaireObject` 87/98, `TGameControl`
65/93, `TConsole` 54/59, `TVisionaire` 48/85, `TGCharacter` 45/71, `TTable` 39/49, `TGraphicsOGL` 31/41.
`tools/compare_binaries.py` prints this for any two builds.

## How to look at a new build

Quick look (minutes):

1. `python tools/compare_binaries.py <known build> <new build>`: the sizes, hashes and the table above for the pair.
2. `python tools/find_vis_password.py <new executable> <its data.vis>`: finds the password of the data (every string of the executable, narrow and wide, is tried against the directory of the container; the entries must lie inside the file, because the key is made of hexadecimal digits only and a check of `HDR`/`END` alone lets false hits through).
3. The engine build: the references to the wide string "Engine Version: %s (Build %d from Build date: %s)" in the code give the version, the build number and the date (`Init`; e.g. Deponia 2: 4.2.5 / 1185 / Dec  6 2015). `readelf -p .comment` gives the compiler; check `-O0` or `-O2` by reading a tail-call function such as `TGameControl::DisplayConsole`.
4. Run the reconstruction on its data with the password: `tests/deponia1/realdatatest.exe` with `GAME_PASSW=<password>` from the game folder, or the SDL2 player with `-p <password>`. The first thing that fails is the first difference.

Full comparison (what was done for Deponia 4 and Deponia 2; `src/deponia4/VERSIONS.md` has the reasons and the pitfalls):

5. Make the cache of each executable: `python tools/crosscompare.py <game> --original <executable>` writes `build/cross/original-<name>.pkl` (it also compares with the reconstruction of `<game>`; the table it overwrites is regenerable).
6. `python tools/symdiff.py <first> <second> -o out.md`: classes, methods, free functions and libraries that only one has. A `-O0` build has every inline function as a symbol: read the additions with that in mind.
7. `python tools/origdiff.py <first.pkl> <second.pkl> --engine out.tsv`: calls of the engine that one build makes and the other does not (it reads a `-O0` second build together with what its functions call, three levels down); `--consts`, `--args` and `--conds` are for the numbers, field ids and tests, but between `-O0` and `-O2` they drown in noise (`--conds` especially).
8. `python tools/globaldiff.py <first.pkl> <second.pkl>`: texts and numbers used anywhere in the `T*` code that the other build does not use; `tools/floatconsts.py` for the floating point constants; `tools/assertlines.py` for the line numbers in the asserts, which says in which source files lines were added or removed.
9. `python tools/emucompare.py <first exe> <second exe> [--methods] [--trials N]`: runs the same functions of both executables in an emulator (Unicorn) on the same inputs and compares the results: the check that does not care how the compiler wrote a test. Needs `pip install unicorn`. `tools/condcompare.py` compares the reconstruction compiled at `-O0` with an `-O0` original.
10. The data model: make an IDA listing (`idat64.exe -A -B` on a copy of the executable; plain `idat.exe` refuses an ELF64), then `tools/extract_xml_names.py` and `tools/extract_schema.py` on it (they read `-O2` code; run them in a folder of their own, they write `manifest/*.tsv` there) and compare with `manifest/`.
11. Write what was found in `src/<game>/VERSIONS.md` (create the folder for a new game) and add a row to the tables above.

Keep the runs out of the game folders: the engine writes a `userdata` folder next to `data.vis` (remove it, or run from a copy).

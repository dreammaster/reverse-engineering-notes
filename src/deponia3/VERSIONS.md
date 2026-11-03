# Deponia 3 (Goodbye Deponia) Linux build

Looked at 3 Nov 2026: `D:\Games\ScummVM\Dev\games\Visionnaire\Deponia3-Linux` (the GOG installer, unpacked: `data/noarch/game`, `meta`). The reconstruction in `src/deponia1` and the logs of the other games are the reference; this file says what is known by running the tools on the executable and the data.
Each statement says whether it rests on the executable (E), the data (D) or a run of the reconstruction (R).

| | |
|---|---|
| Game version | 3.3.3335 (`version.txt`) |
| Executable | `Deponia3`, 22,364,946 bytes, sha1 `5792a699a8c4`, file date 18 Dec 2015 (E) |
| Engine | **the same executable as Deponia 2's** (see below): 4.2.5, build 1185, built 6 Dec 2015; GCC 4.6.3 / 4.7.2 `-O2`, DynASM 1.3.0 (E) |
| Password of `data.vis` | `kML8jCwHeNtdY3`, a wide (UTF-32) string, found with `tools/find_vis_password.py` (E, D) |
| `data.vis` | 75,826,529 bytes, `VIS3`, 2,925 entries (D) |
| Volumes | 131 scene volumes (`scenes/scene.vs000...`), 265 character volumes, 70 video volumes (D) |
| Game name / window in the data | `Deponia 3`, 1920x1080 (R) |
| Libraries in `libs64` | ffmpeg 56, zlib, GOG Galaxy; no OpenAL or libpng (the system ones), as Deponia 2 (E) |
| Languages in `lua` | `voices_DE`, `voices_EN`, `voices_ES`, `voices_IT` (D) |

## The executable is Deponia 2's

`python tools/compare_binaries.py Deponia2 Deponia3`: the same size (22,364,946 bytes), 12,431 symbols and 8,437 functions in both, **every function the same size**, 177 `T*` classes and 2,582 methods with the same signatures.
A byte comparison of the two files: **34 bytes differ, in two places**: the GNU build id of the ELF notes (`0x2BC-0x2CF`, 20 bytes) and the password, a 14 character wide string at `0xA258D8-0xA2590C` (Deponia 2 `aqy4rnsmsd8jTw`, Deponia 3 `kML8jCwHeNtdY3`). Everything else, code and data, is the same.
So **everything that `src/deponia2/VERSIONS.md` says about Deponia 2's executable holds for Deponia 3's**, and Deponia 3 needs no executable analysis of its own; only the data is new (the sizes above, the game's name, its scripts).
Against the newer builds (Deponia 4 / Deponia 1) Deponia 3 differs exactly as Deponia 2 does: 177 against 214 classes, the data format a subset, no Lua hook API, no shader layer (`src/deponia2/VERSIONS.md`).

## The reconstruction on Deponia 3's data (R)

`tests/deponia1/realdatatest.exe` (built from `src/deponia1`) run in the game folder with `GAME_PASSW=kML8jCwHeNtdY3`: the directory is read, the game loads and initializes (`LoadAndInitGame`, `game name 'Deponia 3'`), the interfaces and scripts load, the pictures give their sizes, 60 of 60 sounds open.
The same two messages as for Deponia 1, Deponia 2 and Deponia 4 appear: `assertion failed: ObjectsHash.size() == Objects.size()` and `table 21 not in sync, recreating index` (the data is as the original's own is). The two failures of the test are Deponia 1's own expectations:
`1001_lightmap.webp#s#000#00002#` is a name of Deponia 1's data (here "Error reading header", as in the other games), and the test wants 12 TrueType fonts where Deponia 3 has none (`freetype: 0 fonts made`; Deponia 4 has one, Deponia 1 twelve, so the fonts of Deponia 3 are all picture fonts).
No `lua_dump` stop as in Deponia 4's data (that is the Shader Toolkit script of the newer data).

## Open points

1. The data against the model of Deponia 2: the typed values and the version of the project (UpdateVersion) were not compared field by field; the loading above shows that nothing in it is refused.
2. A run of the SDL2 player on this data (Deponia 2 reaches the language select).
3. The texts of the data (voices, languages) and the start scene were not looked at.

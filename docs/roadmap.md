# Status and roadmap

## Where things stand

The request was a C implementation "that can play both games". That is a
large project: the executables are ~430 KB of hand-written, unlabeled 16-bit
assembler (≈35 000 lines of disassembly listing in `seg000`/`seg001`; the rest of the image is data) with
their own script language, planar-VRAM blitters and sound drivers. This first
pass concentrated on the groundwork that everything else depends on, and on
building oracles so later work can be *verified* rather than guessed.

| Area | State |
|---|---|
| `mjm` container, PKWARE DCL, RNC1 | **Done** — Python + C, identical output for all 101 decodable files, unit-tested |
| `.PC1` pictures | **Done** — Python + C, matches DOSBox pixel-for-pixel (visually) |
| Game detection / path resolution (Cadaver 1 vs Payoff, `DATA`/`TATA`, `?` disk placeholder, case-insensitivity) | **Done** (`src/engine/gamefiles.c`) |
| SDL2 front end | Shows the title and scroll pictures of either game. **No gameplay.** |
| Emulator harness + DOSBox capture | **Done** — runs the real game to the level-1 screen (see `docs/tools.md`) |
| `FONT2.SPR` | Layout known (8×8, 4 plane bytes/row), colour mapping not pinned |
| `*.LNG` text | Container known, **string encoding not decoded** |
| `LEVn.MP1/2/3`, `ANDATA.DAT`, `PCMAN.DAT`, `ROUTINEn.TAD` | Loaded into known places in memory; **formats not decoded** |
| Isometric renderer, objects, ACL interpreter, collision/AI, input, sound | **Not started** |
| Cadaver 2 (Payoff) code differences | Not analysed (data differs only in `TATA/` and RNC-packed copies) |

## What was learned about the engine (for the next steps)

* **Video**: EGA-style planar 320×200×16 (mode 0Dh). The VRAM (256 KB) is used
  as storage: several screen-sized pages (the room is rendered into a buffer
  larger than the 320×136 viewport and scrolled), plus the sprite sheets in the
  last ~600 rows (column-planar layout), all blitted with the VGA latches and
  AND/OR masking. The bottom panel is a fixed picture (inventory frames, "DAY n",
  location name).
* **Memory after loading level 1** (measured in the harness): `LEV1.MP1`
  decompressed at `2917:0000`, `ENG1.LNG` at `6C28:000A`; other blocks of `MP1`,
  `MP2`, `ANDATA` are copied into other segments.
* **Files are loaded through one coroutine-based reader** (`1000:C7D1`, header
  at `1000:C8F1`), so every `mjm` file passes the same code: a good hook point
  for "what is this file used for".
* **Game start-up**: read `CADAVER.INI` → load sound driver (`BEEPER.BIN`) →
  decompress `ANDATA.DAT` and `PCMAN.DAT` → language prompt (`1/2/3`) →
  "restore game?" prompt (`0`–`9`/Esc; saves live in `data\mike_save`) → level
  files for the current disk/level → `ENGn.LNG`.

## Renames

Every routine/variable/string identified goes into `renames/<game>/<stem>.tsv`
(see `renames/README.md`) so it can be applied to the local IDBs; currently
`cad` 50 rows, `intro` 5, Cadaver 2 `game` 38 and `intro` 5, including the
mjm/DCL loader chain, the 50 Hz timer ISR, keyboard helpers, VRAM latch copy,
the PC1 decoder and the file-name strings. New findings add rows as we go.

## Suggested order of work

1. **Language strings** — `text6_decode` (`1000:75CA`, 6-bit packed text via the
   table at `1F17:F853`) is understood but is *not* fed directly from the
   `.LNG` buffer (see `formats.md` §5). Find who fills `ds:9738`/`ds:9B20`
   (hook writes to that range, or break on the first `text6_decode` call with a
   non-empty source — it only happens once text is displayed, e.g. after
   moving the player or picking something up), and what the `.LNG` file is
   converted into on the way.
2. **Level data** — diff harness memory/VRAM before and after the level load
   to attribute each part of `MP1/MP2/MP3` to a destination (tile graphics →
   VRAM pages, room geometry → tables). `MP2` begins with a `(size,offset)`
   record table (see `docs/formats.md`) — start there. Validate by rendering
   the same room in the C port and comparing to harness frames
   (`tools/emu_run.py`, `tools/dosbox_capture.sh`).
3. **Renderer** — port the planar blitters to chunky C (masked blits by
   reading the AND/OR planes), driven by the object list. Match the harness
   frame pixel-for-pixel for level 1's first room.
4. **Object/ACL layer** — decode the opcode table (jump table in `seg000` near
   `1000:7AE0`) and the `ROUTINEn.TAD` tables; run the scripts in C.
5. **Player, input, timing** — keyboard map from `CADAVER.INI` (arrow keys,
   `J`/`G` for fire in Cadaver 1/2), 18.2 Hz tick, movement/collision.
6. **Sound** — the `*.BIN` drivers ("AdLib Music by Sound Images") are raw
   code blobs with call stubs at offsets 0 and 4; either port them to an OPL
   emulation or run them under a tiny x86 interpreter.
7. **Cadaver 2** — diff `GAME.EXE` against `CAD.EXE` function by function
   (the data directories are shared, so most of the engine should be identical)
   and add the `TATA/` levels and Roland support.

## Notes for a ScummVM-style engine module

`overview.md` states that the long-term target is a ScummVM engine module. The
`src/engine/` code is deliberately free of platform calls (only `gamefiles.c`
touches the filesystem; `main.c` is the sole SDL user), so it can be ported to
ScummVM's `Common::File`/`Graphics::Surface` layer later.

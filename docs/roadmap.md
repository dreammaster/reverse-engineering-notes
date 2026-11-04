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
| `*.LNG` text | **Done** — 6-bit packed text, Python + C, all 41 files verified, in-game strings confirmed in the emulator |
| `PCMAN.DAT` (player sprites) | **Done** — directory + 5-plane records, Python + C, all 48 frames, viewer in the SDL app |
| Palette | **Done** — 16 ST words, per level in `MP1` |
| `LEVn.MP2` | Sections, room lists and the thing table decoded and named (Python + C, unit-tested on all 9 levels); thing record fields only partly understood |
| `ANDATA` blocks | Container known, contents not |
| `LEVn.MP1`, `MP3`, `ROUTINEn.TAD` | Mostly undecoded |
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

1. ~~Language strings~~ — **done** (`tools/lng.py`, `src/engine/lng.c`, `formats.md` §5).
2. **Level data** — structure of `MP2` (four sections) and `ANDATA` (six blocks)
   is known; `MP1`, `MP3` partially. Next: find the *consumers* — hook reads of
   the expanded tables (`ds:3DDA` rooms, `ds:5194`, `ds:5E13`, `ds:65E4`) with
   the harness while walking the player (`Emu.scan_queue`) and histogram
   `(cs, ip)`; name each field from how the code uses it. Compare compact records
   (file) with expanded ones (memory) to learn the expansion rules. `ANDATA`
   block 0 (8-byte entries) should give the sprite frame rectangles for
   `PCMAN.DAT`, whose placement in VRAM is already known.
3. **Renderer** — the sprite format and the blit algorithm are known (formats.md §6.1/6.2); what is missing is *where room backgrounds and creature/object sprites come from* (they are not in the VRAM sprite area: only `PCMAN.DAT` is). — port the planar blitters to chunky C (masked blits by
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

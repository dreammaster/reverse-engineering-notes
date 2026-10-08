# MM3 C reimplementation (work in progress)

Goal: a portable C + SDL re-creation of Might and Magic III that loads the original game files at runtime
(no assets are in this repo).  The Python tools in `../tools/` are the reference; the docs in `../docs/` say where
each rule comes from.

| File | What |
|---|---|
| `cc.c/.h` | `.CC` archive reader: TOC cipher, filename hash, LZHUF decoder (port of `tools/mm3_cc.py`) |
| `maze.c/.h` | `MAZEnn.DAT` pages (walls, flags, header), `MAZEnn.EVT` event records, `TEXTnn.MAZ` strings |
| `ccdump.c` | list/extract a `.CC`; `make crosscheck` diffs it against `tools/mm3_cc.py` on the real archives (`DATA=../data`) |
| `party.c/.h`, `character.h` | `MAZE.PTY` party state and the 303-byte character record / `MAZE.CHR` roster |
| `events.c/.h` | event operand decoder (opcodes, `(mode,value)` pairs, goto lines); every shipped record decodes except the documented MAZE60 stray byte |
| `gfx.c/.h` | master palette and sprite codec (literal/skip/fill, two-layer frames), blit; `make gfxcheck` diffs all 415 graphics members against `tools/mm3_gfx.py` |
| `viewer.c` | first SDL2 front end: `make viewer && ./viewer ../data CREATE.RAW` (arrow keys step sprite frames; `--shot x.bmp` for headless screenshots with `SDL_VIDEODRIVER=dummy`) |
| `dgroup.c/.h`, `mapbin.c/.h` | run-time access to the game's lookup tables (DGROUP dumped from `MM3.EXE` by `tools/mm3_dgroup.py`, git-ignored) and the `MAZEnn.BIN` monster/object lists (`make mapbintest`: 64 maps, 2,604 monsters, 1,483 objects, ids in range) |
| `tests/` | `make test` -- round-trips a synthetic ciphered/LZHUF archive built by `gen_testdata.py` (which reuses the Python reference) and checks the parsers |

Verified on the real game files (`../data`): all 558 `MM3.CC` and 240 `MM3.CUR` members extract byte-identical to the Python tool, and
`make realtest` parses every `MAZEnn.DAT`/`.EVT` (10,831 events, records tile each file exactly).

Not done yet (planned order): event interpreter (execution; operands are decoded), rules (combat, spells, town), game screens on top of the SDL viewer (UI frames, text/font), first-person renderer (draw-list format still partly undecoded, see `docs/view.md`).

Open question found while porting: `docs/data-files.md` gives facing 0 = north, 1 = south, 2 = east, 3 = west, while
the view tables are described as N, E, S, W; settle this before writing movement code.

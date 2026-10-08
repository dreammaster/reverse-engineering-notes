# MM3 C reimplementation (work in progress)

Goal: a portable C + SDL re-creation of Might and Magic III that loads the original game files at runtime
(no assets are in this repo).  The Python tools in `../tools/` are the reference; the docs in `../docs/` say where
each rule comes from.

| File | What |
|---|---|
| `cc.c/.h` | `.CC` archive reader: TOC cipher, filename hash, LZHUF decoder (port of `tools/mm3_cc.py`) |
| `maze.c/.h` | `MAZEnn.DAT` pages (walls, flags, header), `MAZEnn.EVT` event records, `TEXTnn.MAZ` strings |
| `ccdump.c` | list/extract a `.CC`; `make crosscheck` diffs it against `tools/mm3_cc.py` on the real archives (`DATA=../data`) |
| `tests/` | `make test` -- round-trips a synthetic ciphered/LZHUF archive built by `gen_testdata.py` (which reuses the Python reference) and checks the parsers |

Verified on the real game files (`../data`): all 558 `MM3.CC` and 240 `MM3.CUR` members extract byte-identical to the Python tool, and
`make realtest` parses every `MAZEnn.DAT`/`.EVT` (10,831 events, records tile each file exactly).

Not done yet (planned order): event interpreter, party/character data (`docs/character.h`), `MAZE.BIN` monsters/objects,
rules (combat, spells, town), SDL front end (VGA palette, `.VGA`/`.OUT` sprite codec from `tools/mm3_gfx.py`),
first-person renderer (draw-list format still partly undecoded, see `docs/view.md`).

Open question found while porting: `docs/data-files.md` gives facing 0 = north, 1 = south, 2 = east, 3 = west, while
the view tables are described as N, E, S, W; settle this before writing movement code.

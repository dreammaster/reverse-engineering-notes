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
| `font.c/.h` | the text font (`FO.T`, id 8D92h, found through the video module's init code): glyph drawing with descenders, widths, simple control codes |
| `gen/rules_gen.c`, `rules_host.c` | the game's character rules (`ifProc` event conditions, HP/SP/AC, stats, skills, conditions), translated from the disassembly and checked against the original overlay code in the emulator (`make rulescheck`, 1,800 random calls identical) |
| `ui_text.c/.h` | **text engine and windows**: hand-written port of the video module's `printText` / `openWindow` / `closeWindows` (word wrap, centre/right alignment, scrolling, colours, bars, overlay glyphs, parchment window frame with saved background), fuzzed against the original module in the emulator (`make uifuzz`, ~700 random cases identical) |
| `viewer.c` | first SDL2 front end: `make viewer && ./viewer ../data CREATE.RAW` (arrow keys step sprite frames; `--shot x.bmp` for headless screenshots with `SDL_VIDEODRIVER=dummy`) |
| `dgroup.c/.h`, `mapbin.c/.h` | run-time access to the game's lookup tables (DGROUP dumped from `MM3.EXE` by `tools/mm3_dgroup.py`, git-ignored) and the `MAZEnn.BIN` monster/object lists (`make mapbintest`: 64 maps, 2,604 monsters, 1,483 objects, ids in range) |
| `evtrun.c/.h` | event script runner: the control flow of `runMazeEvent` (lines, If/JumpRnd goto, CallEvent/Return, AlterEvent, Exit) with a host interface for the game effects; `make evttest` replays the real MAZE01 pit script |
| `recomp.h`, `view_host.c`, `view_glue.c`, `gen/` | the **3D view**: the original renderer, mechanically translated from the disassembly (`tools/asm2c.py`, see `gen/README.md`) and verified draw-list-exact against the original code in an emulator (`make viewcheck`); the glue fills its data segment from the maps (walls, monsters, objects) and composites its draw lists with the real sprite sheets, including the video module's mirrored / window-clipped / distance-scaled blits (`gfx.c: mm3_blit_ex`; bit 15 'enlarge' is not implemented) |
| `mm3view.c` | SDL first-person viewer: `make mm3view && ./mm3view ../data 1 2 5 2 [--time MINUTES]` (map x y facing; arrows move; outdoor maps 41+ work too, `make walktest` crosses page boundaries) |
| `tests/` | `make test` -- round-trips a synthetic ciphered/LZHUF archive built by `gen_testdata.py` (which reuses the Python reference) and checks the parsers |

Verified on the real game files (`../data`): all 558 `MM3.CC` and 240 `MM3.CUR` members extract byte-identical to the Python tool, and
`make realtest` parses every `MAZEnn.DAT`/`.EVT` (10,831 events, records tile each file exactly).

Not done yet (planned order): event effects (`If` tests, give/take, teleport, town buildings: the host side of `evtrun`), rules (combat, spells, town), game screens on top of the SDL viewer (UI frames, text colours/windows/alignment), monster movement (`moveMonsters` is a stub in `view_host.c`), UI screens.

Facing (settled from `exploreLoop`'s turn and step tables): 0 = north (+y), 1 = south (-y), 2 = east (+x), 3 = west (-x);
turning left goes 0 -> 3 -> 1 -> 2 -> 0, turning right 0 -> 2 -> 1 -> 3 -> 0.  Wall nibble sides are N, E, S, W from the top nibble
(`MM3_SIDE_*`); `mm3_facing_*` in `view_glue.c` has the helpers.

## Running the recompiled game
`make mm3game && ./mm3game ../data [--intro] [--at MAP,X,Y,FACING] [--headless --keys HEX,... --shot f.bmp]`
Keys are BIOS codes (scan<<8|ascii, e.g. 4800 = Up, 1E41 = 'A', 3920 = Space).  Debug environment: `MM3_TRACE=1` (key log),
`MM3_TEXTLOG=1` (every string given to the text engine), `MM3_SHOT_EVERY=N`/`MM3_SHOT_PREFIX` (screenshots while the game polls
the keyboard), `MM3_IDLE=N` (headless idle limit).  Scripts: `tests/gamefuzz.sh`, `tests/combatfuzz.sh`, `tests/combatwin.sh`.
Combat commands (table in the combat overlay): A attack, B block, C cast, F fight, I info, O, Q quick reference, R run, U use.

## Hand-written replacements for translated routines (the hybrid step)
`rules.c`/`rules.h` are readable C versions of the character rules (`statBonus`, `getAge`, `itemScan`, `conditionMod`, `getStat`,
`getCurrentLevel`, `getMaxHP`, `getMaxSP`, `getArmorClass`); `make rulesdiff` fuzzes them against the translated originals on random
characters (200,000+ checks, bit-exact, including the originals' 16/32-bit quirks).  `game_rules.c` exposes them as hosts and the
names are listed in `gen/hosts.txt`, so `make regen_game` leaves the translated copies out of the game.  The same recipe applies to
any other routine: write the readable function, add a differential test against `gen/*_gen.c`, then list it in `gen/hosts.txt`.

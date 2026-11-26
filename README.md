# Cadaver / Cadaver: The Payoff — reverse engineering notes and a C reimplementation

Notes, tools and a C/SDL2 reimplementation of the DOS games *Cadaver* (1990) and *Cadaver: The Payoff* (1991) by The Bitmap Brothers.
The game data in `cadaver1/` and `cadaver2/` is copyrighted and only present so the sessions can read it.

| Where | What |
|---|---|
| `src/` | The C implementation (`cadaver` SDL2 front end, `engine/` library, `test_formats`). See `src/README.md` for the file list, keys and options. |
| `docs/formats.md` | Everything learned about file formats, the script language (ACL), the native TAD routines, verbs, the rucksack, the map, creatures, sound, … with the evidence |
| `docs/roadmap.md` | What works and what does not |
| `docs/tools.md`, `tools/` | Python tools: decoders, the Unicorn-based emulator harness used as an oracle (`dosemu.py`, `emu_call.py`), verifiers, the test vector generator |
| `disasm/`, `unpacked/`, `ida_scripts/` | IDA listings of the unpacked executables / sound drivers and the headless IDA pipeline |
| `renames/` | Rename tables (`tools/renames.py fill|check|port`) to re-apply the names to local IDA databases |

## Build and run

```bash
cmake -S src -B build && cmake --build build
ctest --test-dir build --output-on-failure          # needs the retail data next to the sources
./build/cadaver -g cadaver1                         # title scroll, Space starts level 1 (= --scene game)
./build/cadaver -g cadaver2                         # Space = Cadaver, P = The Payoff
```

Keys: arrows (+ Insert/Delete/PgUp/PgDn diagonals) move, End = fire (verb menu / jump / cast / throw), Space or Return = rucksack, H = hold, F1 = map, `I` = status, S / L = pay-to-save / load slots 0-9, F5/F9 = quick save / load.
Headless runs (`--sim`, `--screenshot`) are described in `src/README.md`; `sh tools/check_all.sh` builds, runs the tests, the smoke run over every level and the rename-table checks.

## State in one paragraph

All file formats are decoded and verified against the original running in an emulator; the walk scene plays levels of both games with scripts, timers,
creatures (the original TAD routines on an 8086 interpreter), verbs, chests, the rucksack (small view and scrolling grid), held and thrown things, spells,
potions and throwing weapons with projectiles, map, jumping, level progression, death, save slots and sound from all three PC drivers (the original
`BEEPER.BIN`, `ADLIB.BIN` and `SBLASTER.BIN` on the interpreter, with a small OPL2 synthesiser). Open items: a few spell / potion effects, the intro's story pages,
the original save format, Roland music (exported as MIDI by `tools/roland_midi.py`, not played) — see `docs/roadmap.md`.

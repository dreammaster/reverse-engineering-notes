# The Wizardry I–V engine

All of Wizardry I through V (Apple II originals, and the DOS *Ultimate
Wizardry Archives* repackaging) run on one engine: an **Apollo / RWI UCSD
p‑System**.  Each game is:

```
WIZn.COM   — a tiny DOS loader; hooks INT 18h as the disk callback and boots
             the p-System off the disk image (there is no DOS or BIOS INT 13h
             in the running game)
WIZn.DSK   — a linear 512-byte-block image of a UCSD p-System volume
             (NOT a FAT floppy).  Contains:
               SYSTEM.INTERP   — the x86 p-machine (©1987 RWI), ~16 KB
               SYSTEM.PASCAL   — the game, as linked p-code (~16 segments)
               ASCII.KRN       — the keyed, ciphered string pool (names, messages)
               SCENARIO.DATA   — the scenario database (chars, monsters, items,
                                 spells, maze, rewards)
               200/400.CHARSET, 200/400.TITLE, 200.MONSTERS, HAS.*, KANA.* …
SAVEn.DSK  — a full bootable volume image; the save IS SCENARIO.DATA written
             back in place (there is no separate PLAYER.DATA)
```

## What's shared, and how much

`SYSTEM.INTERP` extracted from each DOS disk:

| disk | `SYSTEM.INTERP` MD5 | note |
|---|---|---|
| WIZ1.DSK | `1b83a5e540a9c4556729b79cd5222c8c` | |
| WIZ2.DSK | `1b83a5e540a9c4556729b79cd5222c8c` | **identical to WIZ1** |
| WIZ3.DSK | `1b83a5e540a9c4556729b79cd5222c8c` | **identical to WIZ1** |
| WIZ4.DSK | `0ad1552cdd9316f943fa9935a04bec95` | different (revised p-machine) |
| WIZ5.DSK | `9459ba0c1ece8616fb408ff09ba746e8` | different again |

So **Wiz I / II / III are the exact same interpreter** — the opcode dispatch,
the CSP table, the SBIOS (disk / keyboard / video) interface, the runtime
layout, and the `RANDOM` generator at file offset `0x221E` are byte‑for‑byte
common.  Everything in [`pmachine.md`](pmachine.md), [`pcode.md`](pcode.md)
and [`rng-validation.md`](rng-validation.md) transfers to Wiz II and III
unchanged.  Wiz IV and V need their interpreters re‑examined.

`SYSTEM.PASCAL` differs per game (each has its own linked p-code), but the
**segment layout lines up** — II and III were built on the Wiz I engine and
are essentially large extra scenarios, so most procedures should match Wiz I
close enough to name by structure.  The `SCENARIO.DATA` container
([`file-formats.md`](file-formats.md)) and the `ASCII.KRN` cipher are the
same mechanism across the family; the record *contents* and string *keys*
are per game.

## Per-game work

| game | sources | plan |
|---|---|---|
| **I** | Apple Pascal reconstruction in `wizardry1/sources/` | reference port — done through a standalone C++ engine (`wizardry1/`) |
| **II** | none | derive by diffing WIZ2 p-code / `SCENARIO.DATA` against Wiz I |
| **III** | Apple Pascal reconstruction (`wizardry3/`) | same approach as Wiz I |
| **IV / V** | Apple sources exist for some | later; different interpreter |

## Toward one ScummVM engine

The intent is a single engine that carries the common core (p-machine-derived
game logic, the data formats, the RNG, the town / maze / combat / camp
state machines) and keeps per-game code down to: the scenario tables, the
maze, the scripted content, and whatever small rule tweaks each title added.
Wiz IV's puzzle-heavy "play the monsters" inversion and Wiz V's additions are
the largest expected deltas.

## The reusable tooling (currently under `wizardry1/tools/`)

- `ucsd_disk.py` — list / extract / dump any UCSD p-System volume
- `pcode_dis.py` — disassemble any p-code codefile (segments, procs, PATs, full opcode/operand decode)
- `strpool.py` — the `ASCII.KRN` cipher + `GetStr` range-tree reader
- `scenario.py` — the `SCENARIO.DATA` TOC + record grid
- `globals.py` — the `+289` DOS/Apple global-offset map generator

These are game-agnostic; only the paths they're pointed at are Wiz1's.

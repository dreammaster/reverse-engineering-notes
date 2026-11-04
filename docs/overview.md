# Cadaver (DOS) — Disassembly Overview

Working notes for the reverse-engineering of the two DOS games *Cadaver*
(1990) and *Cadaver: The Payoff* (1991) by The Bitmap Brothers, with the
goal of a clean C/C++ reimplementation for a ScummVM engine module — the
same approach as the sibling `ultima` projects (`C:\dev\ultima`).

The retail game folders (`cadaver1/`, `cadaver2/`) are copyrighted and are
committed only temporarily, so that a cloud session can read the data
files; they are meant to be removed from the repository again.

## Layout

| Path | Contents |
|---|---|
| `cadaver1/`, `cadaver2/` | Retail game files exactly as shipped (temporary commit) |
| `unpacked/<game>/` | Decompressed executables and sound drivers (see below) — IDA works from these |
| `disasm/<game>/*.asm`, `*.idc` | IDA exports, one pair per executable/driver. The `.idb`s are gitignored |
| `ida_scripts/` | Headless IDA pipeline (`create_idb.ps1`, `run_ida_script.ps1`, `batch_run_and_export.py`, `identify.py`, `init_binary16.py`) |
| `tools/` | `unpack_exes.ps1` (regenerates `unpacked/`), `rnc1.py` (RNC method 1 decompressor) |

## Executables

Both games' executables are compressed, so IDA initially only shows the
decompression stub. Cadaver 1 uses **PKLITE 1.12**, Cadaver 2 uses Rob
Northen's **Pro-Pack** (RNC method 1).

### Cadaver 1 (`cadaver1/`)

| File | Packer | Unpacked | IDB | Funcs | Role (best guess, to confirm) |
|---|---|---|---|---|---|
| `CAD.EXE` | PKLITE | 431,816 | `cad` | 357 | The game. Started by `CADAVER.COM` (`RUNME.BAT` runs `cadaver`) |
| `INTRO.EXE` | PKLITE (-e) | 8,566 | `intro` | 40 | Intro / title sequence |
| `CADAVER.COM` | PKLITE | 7,500 | `cadaver_com` | 60 | Launcher / sound & video setup reader (`CADAVER.INI`) |
| `SETUP.COM` | PKLITE | 55,320 | `setup_com` | 182 | Configuration program (writes `CADAVER.INI`) |

### Cadaver 2 (`cadaver2/`)

| File | Packer | Unpacked | IDB | Funcs | Role (best guess, to confirm) |
|---|---|---|---|---|---|
| `GAME.EXE` | Pro-Pack 2.14 | 432,424 | `game` | 372 | The game |
| `INTRO.EXE` | Pro-Pack 2.08 | 10,227 | `intro` | 49 | Intro / title sequence |
| `CADAVER.EXE` | none | 1,056 | `cadaver` | 3 | Tiny launcher: execs `game.exe`, `intro.exe`, reads `cadaver.ini` |
| `SETUP.EXE` | none | 54,717 | `setup` | 178 | Configuration program ("installation program (c) 1992 The Bitmap Brothers"), 6 KB header |

All are 16-bit real-mode MS-DOS programs and look hand-written in
assembler (`push ax/cx/dx/bx/bp/si/di` register-save prologues rather
than a C compiler's), so expect little help from FLIRT signatures.
Function counts are from IDA's first auto-analysis only; nothing has been
named, typed or reviewed yet (`ida_scripts/identify.py` reports the
progress).

### Sound drivers

`ADLIB.BIN`, `BEEPER.BIN`, `SBLASTER.BIN` (and `ROLAND.BIN` in Cadaver 2)
are raw 16-bit code blobs ("AdLib Music by Sound Images (c) 1991" /
"Roland LAPC1 Music by Sound Images" in the strings). They start with two
near-call/`retf` entry stubs (offsets 0 and 4) and are loaded by the game
to some segment's offset 0. Cadaver 2's copies are RNC1-compressed;
`ADLIB.BIN` and `BEEPER.BIN` decompress to byte-identical copies of
Cadaver 1's, `SBLASTER.BIN` differs (11,053 vs 11,136 bytes). They are
disassembled as raw binaries based at `1000:0` (`init_binary16.py` forces
the segment to 16-bit; IDA's headless Binary loader picks 32-bit).

## Pipeline

1. `tools\unpack_exes.ps1` — runs UNP 4.11 (`C:\dev\unpackers\UNP.EXE`)
   under the DOSBox debug build (`C:\dev\dosbox-0.74-3\...\dosbox.exe`,
   with SDL's dummy video/audio drivers so it runs headless) over every
   `.EXE`/`.COM`, runs `tools\rnc1.py` over Cadaver 2's `.BIN`s (it
   verifies the RNC header's CRC16 of the unpacked data), and copies the
   rest unchanged into `unpacked/`. DOSBox segfaults on exit; harmless.
2. `ida_scripts\create_idb.ps1 -Game cadaver2 -Exe GAME.EXE -Stem game`
   — creates `disasm\cadaver2\game.idb` with `idat.exe` (IDA Pro 8.3,
   32-bit `idat`, 16-bit idbs), runs `identify.py` and exports the first
   `.asm`/`.idc`. `-Binary` for the sound drivers.
3. `ida_scripts\run_ida_script.ps1 -Idb cadaver2\game -ScriptName x.py` —
   runs a script against an existing IDB, then re-exports `.asm`/`.idc`
   and saves (`-NoExport` for read-only report scripts). The IDA GUI must
   be closed. Output is traced in `ida_scripts/batch_run_and_export.log`
   (gitignored; `idat.exe`'s console output is not reliable).

The two games contain same-named executables (`INTRO.EXE`, `CADAVER.*`,
`SETUP.*`), so pass `-Idb cadaver1\intro` / `cadaver2\intro`. Bare stems
work when unambiguous (`game`, `cad`).

## Not yet looked at

- Everything: no function has been named, no structs/enums defined.
- The `DATA/`, `TATA/` directories and the numbered files (`1`, `2`, …),
  `*.DAT`, `*.PC1` (DEGAS-style compressed pictures?), `FONT2.SPR`,
  `FLAME.DAT` file formats.
- Cadaver 1 `SETUP.COM`/Cadaver 2 `SETUP.EXE` embed an installer script
  (see `FILE.TXT`) that is probably irrelevant for the engine.
- The 430 KB unpacked game images are mostly data; the code and the data
  segments (`seg000`…`seg010`) have not been separated or renamed.

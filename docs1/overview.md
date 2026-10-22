# Yendorian Tales Book I, Chapter 1 — overview / session log

Append-only: correct a stale entry with a new dated note, not a silent rewrite.

## 2026-10-22 — IDA databases created for every executable

### The game directory

`yendor1/game/` is a full install of the shareware Chapter 1 (EGA and SVGA
video drivers present). Unlike Chapters 2/3, which are a single large `SW.EXE`,
Chapter 1 is a set of **20 separate programs**: a small `SW.EXE` launcher and
`PROG0.EXE` … `PROG18.EXE` (plus `INSTALL.EXE` and two video drivers — 23
inputs in all), which are presumably chained/exec'd by `SW.EXE`
(not yet traced — that is the first thing to establish). Data files:
`WORLD.DAT`, `TOWN.DAT`, `MINE.DAT` (256 KB each), `FILE00`–`FILE06`
`.16`/`.PAL` (16-colour graphics + palettes), `CURRGAME/` and `SAVEGAME.A/`
(`AREA.DAT`, `NPC.DAT`, `PARTY.DAT`, and `BATPIC.DAT` in CURRGAME), and the
video drivers `VID16.DRV` / `VIDP16.DRV`. `YENDOR1.TXT` and `REGISTER.TXT` are
the shareware readme / order form.

### Databases

One IDA database per executable, named by the lower-cased stem and kept
directly under `yendor1/` (`sw.idb`, `prog4.idb`, …). Every input except
`VIDP16.DRV` is a 16-bit MS-DOS MZ executable and was loaded with IDA 8.3's
default DOS EXE loader. Function counts are `proc` lines in the freshly
generated listing — nothing has been named or analysed yet, so treat them as
rough.

| Program | Size (bytes) | Funcs | Notes |
|---|---:|---:|---|
| `SW.EXE` | 15,217 | 43 | main startup executable |
| `INSTALL.EXE` | 12,953 | 56 | installer |
| `PROG0.EXE` | 6,120 | 12 | |
| `PROG1.EXE` | 13,676 | 50 | |
| `PROG2.EXE` | 14,598 | 53 | |
| `PROG3.EXE` | 11,128 | 37 | |
| `PROG4.EXE` | 99,387 | 300 | largest program |
| `PROG5.EXE` | 35,685 | 88 | |
| `PROG6.EXE` | 12,480 | 47 | |
| `PROG7.EXE` | 25,423 | 57 | |
| `PROG8.EXE` | 54,287 | 102 | |
| `PROG9.EXE` | 53,627 | 106 | |
| `PROG10.EXE` | 90,624 | 204 | |
| `PROG11.EXE` | 54,181 | 109 | |
| `PROG12.EXE` | 55,030 | 44 | few functions for its size — mostly data? |
| `PROG13.EXE` | 58,490 | 44 | ditto (12–14 look like siblings) |
| `PROG14.EXE` | 64,136 | 44 | ditto |
| `PROG15.EXE` | 45,639 | 106 | |
| `PROG16.EXE` | 12,972 | 58 | |
| `PROG17.EXE` | 12,208 | 43 | |
| `PROG18.EXE` | 15,814 | 56 | |
| `VID16.DRV` | 6,695 | 15 | video driver; MZ format |
| `VIDP16.DRV` | 817 | 1 | flat code blob, see below |

`SW.EXE`'s own database: entry `1000:0` (`start`), md5
`a5faa33223e3db48efd0866b0cf3b18d`, 21 segments (IDA splits an MZ file into
one segment per relocation target), 43 functions, 1 named (`start`).

**`VIDP16.DRV` is not an MZ file.** It begins `E9 C5 02` (`jmp 02C8h`) with no
header at all; IDA's default loader guesses "MS-DOS SYS-file (device driver)"
and annotates it with bogus Strategy/Interrupt routine labels. It is loaded
with the Binary loader instead (16-bit x86, base 0), and
`ida_scripts/seed_vidp16.py` marks the entry code at offset 0.

### Pipeline (`yendor1/ida_scripts/`)

Derived from the `yendor2`/`yendor3` pipeline, with the database name made a
parameter since there are 23 inputs. Run from inside `ida_scripts/` with the
PowerShell tool (IDA 8.3 at `C:\Program Files\IDA Pro 8.3\idat.exe`):

- `create_idb.ps1 <EXE>` / `-All` [`-Force`] — builds the database from
  `game\<EXE>` in one `idat.exe` pass (`-c -o`), then exports `<name>.asm` and
  `<name>.idc`. Refuses to overwrite an existing `.idb` without `-Force`, since
  a database with renames in it is the real work product. Non-MZ inputs get the
  Binary loader automatically.
- `run_ida_script.ps1 <name> <script.py> [-NoExport]` — runs a script headlessly
  against `yendor1\<name>.idb`, then re-exports. `-NoExport` is **not** a dry
  run (same caveat as the other games).
- `batch_run_and_export.py` — the driver; logs to the (gitignored)
  `batch_run_and_export.log`.
- `identify.py` — read-only report; `export_only.py` — no-op target used for the
  initial build; `seed_vidp16.py` — see above.

Nothing in `yendor1/game/` is modified: the databases are written under
`yendor1/`, and `yendor1/game/` is gitignored along with `*.idb`.

### Next

1. Disassemble `SW.EXE` properly (43 functions): establish how it launches the
   `PROGn.EXE` programs (DOS `exec`/overlay, command-line, shared memory?).
2. Identify what each `PROGn.EXE` is (title, town, mine, combat, …) from its
   strings, then name functions per program.
3. Decode the data files (`WORLD.DAT`/`TOWN.DAT`/`MINE.DAT`, `.16`/`.PAL`,
   `CURRGAME` records).

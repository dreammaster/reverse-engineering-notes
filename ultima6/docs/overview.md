# Ultima 6 engine — disassembly and re-implementation notes

Goal (in order): disassemble and document the DOS executables of Ultima VI,
Martian Dreams and The Savage Empire; write a stand-alone C++ implementation
in [`../src`](../src) that matches the originals; add the two spin-offs to it;
move the result into ScummVM's Ultima engine; and finally replace the old Nuvie
engine (reference copy in [`../nuvie`](../nuvie)) with it.

[roadmap.md](roadmap.md) lists what is done and what is next; [u6_game.md](u6_game.md) records what the Ultima VI executables have been
shown to do so far.

## Layout

| Path | Contents |
|---|---|
| `u6/`, `md/`, `se/` | per game: `*.idb` (git-ignored), `*.asm`, `*.idc` (tracked, regenerated from the idb) |
| `u6/game/`, `md/game/`, `se/game/` | the original installed games (git-ignored, never modified; Martian Dreams and Savage Empire keep the real game files in the `MARTIAN/` and `SAVAGE/` sub-folder) |
| `ida_scripts/` | headless IDA 8.3 pipeline and the per-database rename/struct scripts |
| `tools/` | helper tools (`unexepack.py`, `match_functions.py`) |
| `src/`, `tests/`, `Makefile` | the stand-alone C++ engine and its tests |
| `nuvie/` | ScummVM's old Nuvie engine, kept as a reference only |

## Executables and databases

| Database | Executable | Compiler | Role |
|---|---|---|---|
| `u6/u6_ultima6` | `ULTIMA6.EXE` (18 KB) | Microsoft C 5/6 (`TANDY.OBJ`) | launcher: reads `CONFIG.U6`, runs `TANDYPIN.EXE`, then `U.EXE` |
| `u6/u6_u` | `U.EXE` (110 KB) | Borland C++, large model | title/main menu, intro, character creation; loads the music (`*.M`), drivers and graphics sets; chains to `GAME.EXE` |
| `u6/u6_game` | `GAME.EXE` (205 KB, **EXEPACK**) | Borland C++, large model | the game proper (loaded from the *unpacked* `GAME_UNP.EXE`) |
| `u6/u6_end` | `END.EXE` (40 KB) | Borland C++ | ending sequence |
| `md/md_martian` | `MARTIAN\MARTIAN.EXE` (118 KB) | Borland C++ | Martian Dreams menu/intro/character creation |
| `md/md_game` | `MARTIAN\GAME.EXE` (341 KB) | Borland C++, 17 VROOMM overlays | Martian Dreams game proper (70 KB `FBOV` overlay block, see [Martian Dreams overlays](#martian-dreams-overlays)) |
| `se/se_savage` | `SAVAGE\SAVAGE.EXE` (76 KB) | Borland C++ | Savage Empire menu/intro/character creation |
| `se/se_game` | `SAVAGE\GAME.EXE` (270 KB) | Borland C++ | Savage Empire game proper |

Not databased yet: `INSTALL.EXE`, `MAKEMODE.EXE`, `T2*.EXE`/`TANDYPIN.EXE`
(video-mode/Tandy helpers) and the `*.DRV`/`*.BIN` driver blobs in Ultima VI.

## Facts established so far

* **Chain**: `ULTIMA6.EXE` → `U.EXE` → `GAME.EXE` → (`END.EXE` on victory, or back to `ULTIMA6.EXE`). Strings in `GAME.EXE`
  confirm the `end.exe` / `ultima6.exe` exec targets and the `savegame\objlist` / `objblkaa` save files.
* **Ultima VI's `GAME.EXE` is Microsoft EXEPACK-compressed.** The stub's first bytes disassemble as `push dx / inc dx` because they
  are the `RB` signature; IDA only sees a data blob plus the unpacker. `tools/unexepack.py` reverses it (packed data is decoded
  backwards from the stub, the untouched head of the file is carried over, the relocation table sits after the unpacker code).
  The result (`u6/game/GAME_UNP.EXE`, 3328 relocations, 3169 of them far calls — a good sanity check) starts with the normal
  Turbo C startup and is what `u6_game.idb` is built from. The spin-offs' `GAME.EXE` files are not packed.
* IDA's Borland run-time signatures resolve 60-120 library functions per database automatically; everything else starts as
  `sub_XXXXX` and is named through the scripts below.
* All executables are 16-bit real-mode large-model programs; each Borland module is its own code segment (50-100 `segNNN` per
  database).

## Martian Dreams overlays

Martian Dreams' `GAME.EXE` is the only executable of the three games with Borland overlays: a 70,672-byte `FBOV` block follows
the MZ image. IDA's DOS loader already handles it: `md_game.idb` has 17 `stubNNN` segments (the thunk tables, with a struct for the
header) and 17 `ovrNNN` segments (the overlay code, relocated), the thunks are patched into `jmp`s to the overlay functions, and
the overlay code is 95-100% covered by functions with every call into the resident program landing on a function start. (An
earlier version of this repository's notes wrongly said the overlays were not loaded; a hand-written loader was tried, found to
reproduce IDA's result exactly, and dropped.) The format, decoded while checking that:

* `FBOV` block: `'FBOV'`, size of the overlay data (dword), file offset of the segment table (dword), number of segment-table
  records (dword, 102), then the overlay data: per overlay its code followed by its relocation table.
* **Segment table**: 8-byte records `(paragraph, size, kind, extra)` for every program segment, including the 17 stub segments
  (kind 3) and DGROUP (record 96).
* **Stub segment**: 0x20-byte header (`CD 3F`, memory-swap word, file offset of the overlay data (dword), code size, relocation
  table size in bytes, thunk count, previous-stub link, 16 bytes of run-time state), then 5-byte thunks `CD 3F <offset in the overlay
  code> 00`. Callers far-call a thunk; the overlay manager loads the overlay, patches the thunk into a `jmp` and runs it.
* **Overlay relocations**: a list of word offsets into the overlay code. The word found there is **not a paragraph but the byte
  offset of a record in the segment table**; the real segment is that record's paragraph plus the load segment. (Overlay far calls
  use `0098:1872` where the resident program uses `1B09:1872`, and record 19 is `1B09`; overlays call each other through the stub
  records.)
* Stub order = overlay order; the 17 overlays hold 153 thunk entry points and 1.2-8.2 KB of code each (`ovr079` 1,197 bytes ...
  `ovr095` 7,833 bytes).

For the re-implementation the overlays don't matter beyond this: they are ordinary code that the C++ port simply links in.

## Headless IDA pipeline

Modelled on `ultima1/ida_scripts`; IDA Pro 8.3 (`idat.exe`), no GUI. The GUI locks the `.idb`, so close it before running.

```powershell
cd C:\dev\ultima\ultima6\ida_scripts
.\create_all.ps1                               # create any missing database (never overwrites without -Force)
.\run_ida_script.ps1 -Idb u6_game -ScriptName identify.py -NoExport   # read-only report
.\run_ida_script.ps1 -Idb u6_game -ScriptName apply_renames_u6_game.py # apply names, re-export .asm/.idc, save
```

* `run_ida_script.ps1` finds the database by stem (`u6_game`) or path, refuses to run on a locked one, runs the script through
  `batch_run_and_export.py`, then exports `<stem>.asm` / `<stem>.idc` next to the `.idb` and saves it. `-NoExport` skips the
  export for read-only report scripts. `-Create -Exe <path>` builds a new database instead.
* `batch_run_and_export.py` logs every step to `batch_run_and_export.log` (git-ignored); `idat.exe`'s console output in `-A`
  mode is not reliable.
* Convention (as in ultima1): one accumulating `apply_renames_<stem>.py` per database holds every confirmed name with a note;
  re-running it is idempotent, so the `.asm`/`.idc` can be regenerated at will.
* Gotchas: `idat.exe` relaunches itself, so the runner pipes its output to wait for the real exit; heredocs lose backslashes, so
  write script and C++ files with an editor tool, not `cat <<EOF`.

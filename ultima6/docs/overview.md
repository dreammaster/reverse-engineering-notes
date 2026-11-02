# Ultima 6 engine — disassembly and re-implementation notes

Goal (in order): disassemble and document the DOS executables of Ultima VI,
Martian Dreams and The Savage Empire; write a stand-alone C++ implementation
in [`../src`](../src) that matches the originals; add the two spin-offs to it;
move the result into ScummVM's Ultima engine; and finally replace the old Nuvie
engine (reference copy in [`../nuvie`](../nuvie)) with it.

[roadmap.md](roadmap.md) lists what is done and what is next.

## Layout

| Path | Contents |
|---|---|
| `u6/`, `md/`, `se/` | per game: `*.idb` (git-ignored), `*.asm`, `*.idc` (tracked, regenerated from the idb) |
| `u6/game/`, `md/game/`, `se/game/` | the original installed games (git-ignored, never modified; Martian Dreams and Savage Empire keep the real game files in the `MARTIAN/` and `SAVAGE/` sub-folder) |
| `ida_scripts/` | headless IDA 8.3 pipeline and the per-database rename/struct scripts |
| `tools/` | helper tools (`unexepack.py`) |
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
| `md/md_game` | `MARTIAN\GAME.EXE` (341 KB) | Borland C++ | Martian Dreams game proper (has a 70 KB Borland `FBOV` overlay tail, see roadmap) |
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

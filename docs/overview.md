# Fountain of Dreams — Disassembly Overview

Entry point for the analysis notes. See also [roadmap.md](roadmap.md).

## Executables

Both are 16-bit real-mode MS-DOS `MZ` executables built with Microsoft C
(MS Run-Time Library strings present), each with its own `.idb`.

| File | Role | Evidence |
|---|---|---|
| `FOD.EXE` (28 KB) | Main menu: add / edit / remove party member, create characters, "Play the game", then launches KEH.EXE | Strings: "Choose a Profession", "Attribute Pts", "Really remove this member"; procs `addPartyMember`, `editPartyMember`, `createNewMember`, `loadDisk1`, `loadClasses` |
| `KEH.EXE` (78 KB) | Main game: map movement, combat, inventory, scripts | Strings: "Choose a firing mode", "This weapon is jammed", "Load Which Ammo" |

Both share a graphics/window layer (`Window`, `setWindow`, `writeString`,
`writeChar`, `gfxInit`) and a CRT layer. The KEH segment layout is
`sg01a2` (code, 64 KB) + `sg11a2` + `dseg` + `seg003`.

## Headless IDA pipeline

IDA Pro 8.3, `idat.exe`; the GUI must be closed (it locks the `.idb`).

```powershell
powershell -ExecutionPolicy Bypass -File ida_scripts\run_ida_script.ps1 -Idb KEH -ScriptName identify.py -NoExport
powershell -ExecutionPolicy Bypass -File ida_scripts\run_ida_script.ps1 -Idb KEH -ScriptName apply_renames_keh.py
```

- `-Idb` aliases: `FOD`, `KEH`, `WL`, `WLLAUNCH`, or any path.
- Without `-NoExport`, the `.asm` and `.idc` are regenerated and the `.idb`
  saved. Read-only scripts must pass `-NoExport`.
- Trace goes to `ida_scripts/batch_run_and_export.log` (append-only).
- Scripts: `identify.py` (progress report), `rank_unnamed_functions.py`
  (unnamed functions by caller count). Convention for edits: one accumulating
  `apply_renames_<exe>.py` per executable with `RENAMES`/`GLOBAL_RENAMES`
  tables carrying a justification note per entry.
- `.idb` files are git-ignored; the `.asm`/`.idc` exports and scripts are the
  committed record.

## Findings log

*(none yet — pipeline set up 2026-10-21)*

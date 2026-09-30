# IDA workflow (all games)

Everything is done headlessly with `idat.exe` (IDA Pro 8.3, `C:\Program Files\IDA Pro 8.3`),
following the process used in `C:\dev\ultima1`.

## Running a script against a database

```powershell
# from C:\dev\might_and_magic
powershell -NoProfile -ExecutionPolicy Bypass -File ida_scripts\run_ida_script.ps1 `
    -Idb mm2\mm2.idb -ScriptName mm2\ida_scripts\some_script.py [-NoExport]
```

* `run_ida_script.ps1` refuses to run if the `.idb` is locked (IDA GUI still open — close it).
* It launches `idat.exe -A -S"batch_run_and_export.py <script> [noexport]" <idb>`.
* `batch_run_and_export.py` waits for auto-analysis, `exec`s the script (so `__file__` is set and
  `stdout` is captured to the log), waits again, then writes `<idb stem>.asm` and `<idb stem>.idc`
  next to the database and saves it.  `-NoExport` (read-only discovery runs) skips export + save.
* A script may set a global `EXPORT_RANGE = (start_ea, end_ea)` to export only part of the
  database (used for the MM2 overlay databases).
* The trace is in `ida_scripts\batch_run_and_export.log` (git-ignored).  `idat`'s own console
  output is unreliable; trust the log.
* If PowerShell refuses the script ("running scripts is disabled"), keep `-ExecutionPolicy Bypass`
  as above.

## Round = script → export → commit → push

1. Write/adjust an idempotent script in `<game>/ida_scripts/` (renames, struct application,
   comments, segment fixes…).  Prefer scripts over hand edits in the GUI so the work is
   reproducible; if edits are made in the GUI, save + export from the GUI and commit those too.
2. Run it with `run_ida_script.ps1` — the exports regenerate.
3. Commit the scripts, docs and `.asm`/`.idc` together and push.

## Conventions

* Scripts must be safe to re-run.
* Facts recorded in docs cite the function/address that proves them; anything not traced is
  flagged as such rather than guessed.
* Game data is read from the installed copy (`D:\GOG Games\Might and Magic 2`); nothing from the
  game is committed.

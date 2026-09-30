# Might and Magic II — status & database guide

Game: *Might and Magic II: Gates to Another World* (DOS, v1.01, GOG copy at
`D:\GOG Games\Might and Magic 2`).  MS-C + Plink86 overlays — read
[exe-layout.md](exe-layout.md) first.

## Files

| File | What |
|---|---|
| `mm2.idb/.asm/.idc` | resident image + DGROUP (+ Plink86 runtime, thunks) |
| `ovl/<NAME>.idb/.asm/.idc` | one database per overlay; `.asm/.idc` cover only that overlay |
| `ida_scripts/fix_main_layout.py` | idempotent: loads DGROUP + relocs, creates `OVL_A/B`, names segments, sets `ds`, defines thunks, names the Plink86 runtime |
| `ida_scripts/define_strings.py` | defines the C strings in initialised DGROUP |
| `ida_scripts/export_names.py` | main db → `build/shared_names.json` (user names + comments; git-ignored) |
| `ida_scripts/load_overlay.py` | turns a copy of `mm2.idb` into an overlay database |
| `ida_scripts/build_overlays.ps1` | export names, copy `mm2.idb`, run `load_overlay.py` for each overlay |
| `tools/plink_info.py`, `tools/mm2_layout.py` | dump / parse the EXE + OVL layout |

## Working rules

* Resident code and data: edit `mm2.idb`.  Overlay code: edit `ovl/<NAME>.idb`.
* Overlay databases hold a snapshot of the resident names taken when they were built.  An
  `import_names.py` (apply `build/shared_names.json` to an existing overlay database) is still to be
  written — until then only fresh builds pick up new resident names.
* `build_overlays.ps1 -Force` throws an overlay database away (and any edits in it) and rebuilds
  it; without `-Force` existing databases are left alone.
* Names given inside an overlay database (functions the resident code reaches through a thunk)
  are not yet fed back to `mm2.idb`; the thunks there are `thk_<overlay>_<offset>`.

## State (2026-09-30)

* Layout fully understood; all 14 overlays load and analyse (85 functions in 2PLAY, 5–59 in the
  others, a few hundred bytes of jump-table/unreached code left as `db`).
* Main db: 900 strings defined, DGROUP referenced from code, 219 thunks defined.
* Almost nothing named yet beyond the Plink86 runtime and thunks.

## Next steps

1. Identify the C runtime (`__FF_MSGBANNER`, `_nheapinit`, `printf`, `int86`, …) — compare with
   `../ultima1` (same MS-C startup shape) and MM1.
2. Name overlay entry points (thunk targets) and the resident functions the thunks reach; write
   `import_names.py`.
3. Decode the data files (`MAP.DAT`, `EVENTS*.DAT`, `MONSTERS.DAT`, `ITEMS.DAT`, `ROSTER.DAT`,
   `*.16`/`*.4` graphics, `*.DRV` video drivers) starting from the loader functions; record in
   `docs/file-formats.md`.
4. Compare with the MM1 disassembly for shared structures (character record, spell tables).

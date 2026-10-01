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

## State (2026-10-01)

* Layout fully understood; all 14 overlays load and analyse; every overlay's top-level functions are named.
* Documented: [exe-layout.md](exe-layout.md), [file-formats.md](file-formats.md), [events.md](events.md),
  [game-library.md](game-library.md), [drivers.md](drivers.md), [party-commands.md](party-commands.md),
  [classes.md](classes.md), [shops.md](shops.md) (temple, guilds, blacksmith with prices), [view.md](view.md)
  (indoor view verified with `tools/mm2_view.py`; outdoor outlined), [spells.md](spells.md),
  [combat.md](combat.md) (formulas re-read), [save-format.md](save-format.md).
* For an engine author: [scummvm-notes.md](scummvm-notes.md) maps all of it onto a suggested module layout.
* Tools (`tools/README.md`): LZW, data readers, image/monster/view renderers, layout dump.

## Open / next

1. Graphics: exact CGA/Tandy/Hercules palettes; the 3-4 unused monster animation entries; outdoor view
   needs a renderer to confirm the tile tables.
2. Remaining monster record fields' exact meaning; spell effect internals (2CAST1/2) beyond the summaries.
3. Driver code itself (`*.DRV`) is not disassembled in IDA (ABI is documented).
4. Remaining unnamed functions (about 60 resident, about 50 in overlays) are small helpers.
5. `import_names.py` to refresh names of *existing* overlay databases without a rebuild.
6. Where the party position is restored from after loading (only `g_inn_town` is saved).

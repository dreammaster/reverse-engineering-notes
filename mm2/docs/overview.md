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

* Layout fully understood; all 14 overlays load and analyse.  Resident library and 2PLAY (event VM,
  map loading, 3D view) named; every overlay's top-level functions named from their strings.
* Documented: [exe-layout.md](exe-layout.md), [file-formats.md](file-formats.md) (LZW, MAP/EVENTS/STR/
  MONSTERS, character record, image banks), [events.md](events.md) (50-opcode script VM),
  [game-library.md](game-library.md) (windows, video-driver ABI, state globals).
* [combat.md](combat.md): combat flow, party/monster turns, tables.  Monster pictures decoded (`mm2_monsters.py`).
* Tools: `mm2_lzw.py`, `mm2_data.py` (maps/events/strings/monsters), `mm2_gfx.py` (renders `*.16`
  image banks to PNG), `plink_info.py`.

## Open / next

1. CGA monster pictures (`MONSTERS.4`, CGA driver's own piece decoder) -- the EGA `MONSTERS.16` format is done.
2. `.4` (CGA) banks that don't decode with the 2 bpp rule; palette mapping per video mode.
3. Combat is documented ([combat.md](combat.md)); still to verify: to-hit formula details `(check)`, the
   remaining monster record fields' exact meaning, touch-effect implementation (`1AFE2`), spell effects (2CAST1/2).
4. `ITEMS.DAT` fields (20 bytes: 12-byte name, class/type flags, three words), `SPELLS.DAT`, `ATTRIB.DAT`.
5. Save format done ([save-format.md](save-format.md)); open: where the current map/position are stored.
6. Disassemble the `.DRV` modules (video, timer/sound) — jump table at offset `fn*3`.
7. Shops (2SMITH/2TEMPLE/2BRAIN), inn (1RETINN), caves specials (2CAVES): read and name internals.
8. `import_names.py` to refresh names of *existing* overlay databases without a rebuild.

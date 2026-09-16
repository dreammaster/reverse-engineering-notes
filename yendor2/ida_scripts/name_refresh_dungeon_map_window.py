"""
Names sub_209D2, called from 16 sites throughout `start` (always
right before RedrawDungeonScreen + BuildMinimapTileData + DrawMinimap,
confirmed at the first call site) -- resolves a long-standing open
question from file-formats.md's "In-memory dungeon map grid" section
("source file not yet identified -- plausibly loaded from WORLD.DAT").
This IS that loader: it (re)builds the loaded map-grid window
(8-byte cells, segment word_2E562, 78x78, row stride 0x270 -- all
confirmed constants from that section) around the party's current
position whenever it moves.

1. If the current region/level id (word_2E4A6) matches either of two
   tracked values (word_36CB1/word_36CB3), stops the music
   (StopMusicAndResetTimer) -- a region-transition signal.
2. Computes a clamped window origin (word_2E55C/word_2E564) 39 cells
   back from the party's position (word_36CF7/word_36CF9), clamped to
   the map bounds (word_32A00/32A02/32A08/32A0A, the confirmed map-
   bounds globals). Stores it as the grid's own origin fields
   (confirmed by file-formats.md).
3. Reads 78 WORLD.DAT rows into the grid segment, unpacking a
   packed-bit "explored" flag per cell from a bitmask row alongside
   each cell's two tile-type indices (the confirmed [+0]/[+2] fields)
   -- setting the confirmed [+6] bit 0x8000 "already explored" flag.
4. A second full pass calls TryInteractAtPosition per cell and bakes
   the result into each cell's [+2]/[+4]/[+6] fields (item/trigger/
   trap markers) directly into the loaded grid data.
5. Calls UpdateAmbientMusicForRegion (region-based music switch).
6. Walks the 80-slot g_levelMonsters array: for each occupied
   monster whose position falls inside the new window, marks its
   cell ([+6] bit 0x400, [+4]=monster id) -- placing monster markers
   into the grid; for one that has scrolled outside the window,
   calls ClearCellMonsterSpawnedFlag and zeroes its 156-byte
   g_levelMonsters record -- despawning it as the window moves away,
   confirming ClearCellMonsterSpawnedFlag's own documented caller
   context exactly.

-> RefreshDungeonMapWindow

Run via:
    .\run_ida_script.ps1 name_refresh_dungeon_map_window.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x209D2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RefreshDungeonMapWindow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RefreshDungeonMapWindow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loads/refreshes the in-memory 78x78 dungeon map-grid window "
    "(segment word_2E562) around the party's position from WORLD.DAT, "
    "bakes per-cell interaction markers via TryInteractAtPosition, "
    "updates region music, and places/despawns g_levelMonsters "
    "entries as they scroll into/out of the window. Called from 16 "
    "sites in `start`, always right before "
    "RedrawDungeonScreen+BuildMinimapTileData+DrawMinimap.",
    False,
)

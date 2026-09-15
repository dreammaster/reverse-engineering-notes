"""
Names sub_1F0CD, called from `start` at several sites: initializes/
enters a dungeon level.

- Copies a 48-word template block (0x9535 -> 0x46CA) -- fresh
  per-level metadata.
- Calls sub_209D2 (not traced), then the already-named
  RevealCellsAroundPlayer.
- Calls sub_22387/sub_222F8 (not traced, plausibly a level-transition
  fade/sound), RedrawDungeonScreen, BuildMinimapTileData, DrawMinimap,
  DrawMouseCursor, sub_2587E, sub_1FD03 (not traced).
- Zeroes the entire g_monsterSlots array (di=0x51C0, exactly its
  3 x 0x9C-byte size) and clears word_32A1E (the "active combat
  monster" global) -- resets combat state for the new level.
- Defaults word_36CE7 to 5 if it was 0.

-> InitializeDungeonLevel

Run via:
    .\run_ida_script.ps1 name_enter_dungeon_level.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F0CD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InitializeDungeonLevel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InitializeDungeonLevel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Initializes/enters a dungeon level: copies a per-level metadata "
    "template, calls RevealCellsAroundPlayer, redraws the dungeon "
    "screen and minimap, and resets combat state -- zeroes the entire "
    "g_monsterSlots array and clears word_32A1E (active combat "
    "monster). Called from `start` at several sites.",
    False,
)

"""
Names sub_26E11/sub_26EE8, a byte-for-byte-duplicate pair (the same
overlay-segment duplication pattern seen repeatedly this session)
called from two separate unresolved raw addresses (seg000:0AD2 /
seg000:0AEA) very early in the binary -- the same family as
EnforceDemoBoundary and DrawDebugPositionOverlay, both also reached
from raw un-labeled call sites, plausibly a debug/cheat hotkey table.

Each prompts for a 4-digit number via ReadTypedInteger at a fixed HUD
position, loops back on invalid input (errorCode==0), aborts on
cancel (errorCode==2), then range-checks the value (_val41/_val42)
before writing it directly into the current map cell
(GetMapCellPtr(word_36CF7, word_36CF9)) -- persists the change to
WORLD.DAT via FileEntry_Read/Write, updates the per-column minimap
cache array (word_368A7 + 4*word_36CF7), then
RestoreFullScreenFromEMS + RedrawDungeonScreen + BuildMinimapTileData
+ DrawMinimap + DrawMouseCursor.

sub_26E11 writes es:[bx] (the FLOOR field, per GetMapCellPtr's
confirmed layout from PaintCursorCellAndPersist) and range-checks
against _val41. -> DebugSetFloorTileByNumber

sub_26EE8 writes es:[bx+2] (the OVERLAY/wall field, per
PaintCursorOverlayCellAndPersist) and range-checks against _val42.
-> DebugSetOverlayTileByNumber

Reads as a developer cheat: type in a floor/overlay tile-type number
directly at the cursor's current dungeon position, bypassing the
map editor's palette-picker UI, while playing the normal dungeon
view (not the map editor screen).

Run via:
    .\run_ida_script.ps1 name_debug_set_tile_value_by_number.py
"""
import idc
import ida_name
import ida_bytes

for ea, name, field, bound in [
    (0x26E11, "DebugSetFloorTileByNumber", "floor (es:[bx])", "_val41"),
    (0x26EE8, "DebugSetOverlayTileByNumber", "overlay (es:[bx+2])", "_val42"),
]:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(
        ea,
        f"Debug cheat: prompts for a 4-digit number via "
        f"ReadTypedInteger, range-checks against {bound}, writes it "
        f"into the current map cell's {field} field, persists to "
        f"WORLD.DAT, updates the minimap cache, and redraws. Called "
        f"from an unresolved raw address, plausibly a debug hotkey "
        f"table. Byte-for-byte duplicate pair with its floor/overlay "
        f"sibling.",
        False,
    )

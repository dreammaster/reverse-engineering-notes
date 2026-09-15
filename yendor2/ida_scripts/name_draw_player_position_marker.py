"""
Names sub_22255, called from ToggleMapViewMode: if the party's current
position (word_36CF7/36CF9) falls within the local area map's visible
bounding box (0xA0-0x27F x, 0x30-0xEF y), computes the corresponding
grid cell on screen and draws a fixed marker icon (picture 0x11) there
-- a "you are here" player-position marker on the local area map.

-> DrawPlayerPositionMarker

Run via:
    .\run_ida_script.ps1 name_draw_player_position_marker.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22255
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawPlayerPositionMarker", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawPlayerPositionMarker': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a 'you are here' marker (picture 0x11) at the party's "
    "current position on the local area map, if within its visible "
    "bounding box. Called from ToggleMapViewMode.",
    False,
)

"""
Names sub_29297, called from RevealMapRegion: converts a mouse click
position into a map cell (bounds-checked against a grid size,
word_328FA/word_32900), reads that cell from WORLD.DAT, and checks its
"explored" bit (a packed array at word_3685F -- errorCode=2 if not yet
explored, i.e. you can't click-travel to an unexplored cell). If
explored, validates the target via sub_1119A/sub_11160 (errorCode=3/4
on failure), then calls the already-named TryInteractAtPosition at
that cell; on success (errorCode not 6/7), updates the party's actual
position (word_36CF7/36CF9) to the clicked cell.

Reads as "click a cell on an explored map to travel/teleport there
directly" -- a minimap click-to-travel feature.

-> TryTravelToClickedMapCell

Run via:
    .\run_ida_script.ps1 name_travel_to_clicked_cell.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x29297
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryTravelToClickedMapCell", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryTravelToClickedMapCell': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Click-to-travel: converts a mouse click into a map cell, checks "
    "its 'explored' bit (word_3685F, errorCode=2 if unexplored), "
    "validates via sub_1119A/sub_11160, then TryInteractAtPosition; on "
    "success moves the party (word_36CF7/36CF9) to that cell. Called "
    "from RevealMapRegion.",
    False,
)

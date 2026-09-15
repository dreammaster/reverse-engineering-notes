"""
Traced sub_21E71, HandleGameCommand's handler for word_32974==0x1E
(also called from RunMapEditorScreen). Renders a full-screen (24-row)
map view centered on the player: for each of 24 rows, reads a block
from both WORLD.DAT and CURGAME (FileEntry 0x9043/0x8FFB) and draws it
via sub_22140 (not traced) -- reads as the "full local area map" view,
distinct from the already-named ShowWorldMap (overworld). Falls back
to a smaller view (sub_222BD) when word_328C4 bit 1 is clear.

Named on its confirmed structure (full-screen, WORLD.DAT+CURGAME-
backed, row-by-row render) rather than a guessed manual key.

-> ShowLocalAreaMap

Run via:
    .\run_ida_script.ps1 name_full_map_view.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21E71
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowLocalAreaMap", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowLocalAreaMap': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "HandleGameCommand's handler for word_32974==0x1E (also called "
    "from RunMapEditorScreen). Renders a full-screen (24-row) map "
    "view centered on the player (word_36CF7/word_36CF9): for each "
    "row, reads a block from WORLD.DAT and CURGAME and draws it via "
    "sub_22140 (not traced). Falls back to a smaller view "
    "(sub_222BD) when word_328C4 bit 1 is clear. Distinct from the "
    "overworld ShowWorldMap -- reads as the 'full local area map'.",
    False,
)

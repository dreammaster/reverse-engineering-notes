"""
Traced sub_220DF, HandleGameCommand's handler for word_32974==0x1F.
Branches on word_36C7F bit 0x100 (a new bit in the minimap/view-mode
flags word already documented for light/darkness modes): if clear,
calls a normal small-view redraw (sub_222BD); if set, draws a
FULL-SCREEN picture (g_pictureDir entry 6, at x=0,y=0 -- filling the
whole 320x200 screen, not the small minimap position) instead.
Doesn't flip the bit itself in what was read, so this may be a
"show full view while held/toggled elsewhere" display step rather
than the toggle itself -- named on the confirmed branch behavior.

-> ToggleMapViewMode

Run via:
    .\run_ida_script.ps1 name_toggle_map_view.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x220DF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ToggleMapViewMode", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ToggleMapViewMode': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "HandleGameCommand's handler for word_32974==0x1F. If "
    "word_36C7F bit 0x100 is clear, does a normal small-view redraw "
    "(sub_222BD); if set, draws a full-screen picture (g_pictureDir "
    "entry 6, x=0,y=0 -- fills the whole screen rather than the "
    "small minimap position) instead. Exact trigger for the bit "
    "itself not traced.",
    False,
)

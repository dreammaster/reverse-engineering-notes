"""
Names sub_23B19, called from `start`'s main command loop with a
caller-supplied bx (message pointer) / cx (line count): sets up the
status panel position/colors (0xF0,0x60), clears it if dirty, restores
the cursor background if dirty, then calls DrawStringColumn(bx, cx),
DrawMouseCursor, and sub_238CD.

Confirmed via string dump at both traced call sites: bx=0x7D71,cx=1
draws "NOTHING HERE" (after a failed ProbeFacingTile search/examine),
and bx=0x86AD,cx=2 draws "YOU ARE NOT" / "YET READY!" (after a
3-flag quest-gate check fails). A generic "show an N-line message in
the status panel" utility. -> ShowStatusPanelMessage

Run via:
    .\run_ida_script.ps1 name_show_status_panel_message.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23B19
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowStatusPanelMessage", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowStatusPanelMessage': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generic status-panel message display: sets position (0xF0,0x60) "
    "and colors, clears the panel if dirty, restores cursor "
    "background if dirty, then DrawStringColumn(bx, cx) + "
    "DrawMouseCursor + sub_238CD. Callers pass bx=message pointer, "
    "cx=line count. Confirmed uses: 'NOTHING HERE' (1 line, after a "
    "failed search) and 'YOU ARE NOT'/'YET READY!' (2 lines, after a "
    "quest-flag gate). Called from `start`.",
    False,
)

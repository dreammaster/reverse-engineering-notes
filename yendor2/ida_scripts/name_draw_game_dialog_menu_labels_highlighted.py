"""
Names sub_1F53E, called from RunGameDialog right after
GameDialog_drawButtons, both at initial entry and again after each
SelectGameDialogOption cycle -- loops 6 times over a fixed table at
0x6CBE (stride 0x1B: byte flags at [si+1], text at [si+2]) paired
with a position table at 0x5CD0 (stride 0xA: x at [di], y at [di+4]).
For each of the 6 entries, sets fgColor to a highlighted color (0x7B)
if the entry's flags byte has bit 0x40 set, else the normal color
(0xF), then draws its text at the paired position. This is the
per-selection-change label redraw for the same 6 pause/system menu
entries that DrawGameDialogMenuLabels draws once at dialog setup --
here the bit 0x40 flag marks which entry is currently
highlighted/selected. -> DrawGameDialogMenuLabelsHighlighted

Run via:
    .\run_ida_script.ps1 name_draw_game_dialog_menu_labels_highlighted.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F53E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawGameDialogMenuLabelsHighlighted", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawGameDialogMenuLabelsHighlighted': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loops 6 times over table 0x6CBE (stride 0x1B: flags byte at "
    "[si+1], text at [si+2]) paired with position table 0x5CD0 "
    "(stride 0xA: x/[di], y/[di+4]), drawing each entry's text in a "
    "highlighted color if its flags bit 0x40 is set, else the normal "
    "color -- the selection-highlight redraw of the same 6 menu "
    "entries DrawGameDialogMenuLabels draws once at setup. Called "
    "from RunGameDialog after GameDialog_drawButtons and after each "
    "SelectGameDialogOption cycle.",
    False,
)

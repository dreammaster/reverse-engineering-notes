"""
Names sub_226FC, called twice from sub_22445 (an unnamed function
called directly from the main input loop sub_1869D): draws a 5-row
proportional stat bar (health/mana-gauge style) at a position from
[di+0x46]/[di+0x4A]. Computes a fill fraction (bx=current, cx=max,
scaled/clamped), then draws 5 horizontal rows, each split into a
filled segment (_font_fgColor, width proportional to the fraction)
and an empty segment (_font_bgColor, out of a fixed 0x26/38-pixel
total width), stepping down one screen row (+0x140) per bar row. If
bx<=0, draws all 5 rows empty.

-> DrawStatBar

Run via:
    .\run_ida_script.ps1 name_draw_stat_bar.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x226FC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawStatBar", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawStatBar': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a 5-row proportional stat bar (health/mana-gauge style): "
    "bx=current, cx=max, drawn as filled (_font_fgColor) vs empty "
    "(_font_bgColor) pixels across a 38-pixel width, 5 rows tall. "
    "Called from sub_22445.",
    False,
)

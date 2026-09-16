"""
Names sub_23442, called once from DrawMonsterInfoPanel with bx/cx
(current/max value) already set by the caller.

Computes the video offset from (x,y), clamps bx to [1,cx] (bumping
_font_fgColor by 2 -- a color-shift, plausibly an "overfull" tint --
if bx exceeded cx), then computes a proportional bar-fill width
(word_3293E, in pixels) from the bx/cx ratio, defaulting to 1 pixel
minimum. Draws an 8-row-tall, 0x2D(45)-pixel-wide horizontal bar:
word_3293E pixels of _font_fgColor followed by the remainder in
_font_bgColor, per row. A proportional-fill gauge bar, most likely
the monster info panel's HP bar. -> DrawMonsterHealthBar

Run via:
    .\run_ida_script.ps1 name_draw_monster_health_bar.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23442
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMonsterHealthBar", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMonsterHealthBar': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Proportional-fill gauge bar: caller sets bx=current, cx=max. "
    "Clamps bx to [1,cx] (shifting _font_fgColor by 2 if bx exceeded "
    "cx), computes a pixel fill width from the ratio, then draws an "
    "8-row, 45px-wide bar (fgColor fill + bgColor remainder per "
    "row). Called once from DrawMonsterInfoPanel, most likely the "
    "monster's HP bar.",
    False,
)

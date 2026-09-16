"""
Names sub_161D0 and sub_11E4A -- byte-for-byte identical functions
(likely duplicated across overlay segments), found investigating a
ranked candidate. Both: optionally play a sound (ax, gated on
g_driverStateFlags bit 3), then draw text/a string column twice --
once in the background color, then shifted 1 pixel up-left in the
actual foreground color -- a classic drop-shadow text effect. Single
line (writeString) when cx<=1, multi-line (DrawStringColumn) otherwise.

sub_161D0 (0x161D0, called from the still-untraced character-creation
step sub_1559A) -> DrawShadowedText
sub_11E4A (0x11E4A, called from the still-untraced sub_11A10, itself
called directly from `start`) -> DrawShadowedTextAlt

Run via:
    .\run_ida_script.ps1 name_shadowed_text.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x161D0, "DrawShadowedText"),
    (0x11E4A, "DrawShadowedTextAlt"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

comment = (
    "Draws text/a string column with a 1-pixel drop-shadow (background "
    "color pass, then foreground color pass shifted up-left). Single "
    "line via writeString when cx<=1, multi-line via DrawStringColumn "
    "otherwise. Optionally plays a sound first."
)
ida_bytes.set_cmt(0x161D0, comment + " Called from sub_1559A.", False)
ida_bytes.set_cmt(0x11E4A, comment + " Called from sub_11A10. Byte-for-byte identical to DrawShadowedText.", False)

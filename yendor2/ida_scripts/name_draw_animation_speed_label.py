"""
Names sub_1F884, called from CycleAnimationSetting (the pause menu's
animation-setting cycle handler) and from DrawGameDialogMenuLabels's
ANIMATION branch (in place of GameDialog_drawAnimation, when
word_328C4 bit 0x8 is set).

Draws one of 3 messages at the animation label position (0x61,0x77)
depending on word_36CE7's value: msg 8402h if ==1, msg 83FBh if ==5,
else msg 83F4h (the ==9 case). CycleAnimationSetting confirms
word_36CE7 cycles through exactly {1, 5, 9} on each activation
(1->9, 5->1, other->5), so this is the label for a 3-way animation
speed/mode setting. -> DrawAnimationSpeedLabel

Run via:
    .\run_ida_script.ps1 name_draw_animation_speed_label.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1F884
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAnimationSpeedLabel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAnimationSpeedLabel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one of 3 messages at (0x61,0x77) selected by word_36CE7 "
    "(1/5/9, per CycleAnimationSetting's cycle order 1->9, 5->1, "
    "other->5) -- the label for the pause menu's 3-way animation "
    "speed/mode setting. Called from CycleAnimationSetting and from "
    "DrawGameDialogMenuLabels's ANIMATION branch.",
    False,
)

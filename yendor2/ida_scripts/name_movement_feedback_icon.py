"""
Names sub_116CF, moderate confidence: called several times from
HandleMovementInput (including the "destination out of the dungeon
grid bounds" branch, which sets word_2E530 = word_32940 first).
Draws a picture (id = word_2E530, category 0x80) at a position read
from a struct pointed to by word_32904 ([+0]=x, [+4]=y -- word_32904
is a generic scratch pointer reused for unrelated things elsewhere in
the codebase, e.g. MulBCD4ByWord), then shows the mouse cursor.
Reads as a small movement-feedback icon draw (e.g. a "can't go that
way" indicator), but the exact icon/narrative isn't confirmed.
-> DrawMovementFeedbackIcon

Run via:
    .\run_ida_script.ps1 name_movement_feedback_icon.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x116CF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawMovementFeedbackIcon", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawMovementFeedbackIcon': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a picture (id=word_2E530, category 0x80) at a position from "
    "the struct word_32904 points at, then shows the mouse cursor. "
    "Called several times from HandleMovementInput, including the "
    "out-of-bounds destination branch. Exact icon/narrative not "
    "confirmed.",
    False,
)

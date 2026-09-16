"""
Names sub_2BA62, moderate confidence: called from the still-untraced
sub_2621C (a flagged, container-related open lead) and sub_271DC.

Plays a sound (ax=4) via the sound dispatch, restores the cursor
background, loads the currently-held item's catalog record
(word_31948), and reads a flag byte from it (word_2E548's [+0] field --
the "current target" pointer): if bit 0x80 is set, ORs 0 into
word_36C81 (a global not otherwise documented); if clear, ORs in the
high byte of that field. Then clears the held-item cursor
(UpdateCursorForHeldItem(0)) and calls the common redraw-completion
helper (sub_238CD). Reads as "finish placing/dropping the held item",
clearing the cursor and recording something about the placed item's
flags into word_36C81 -- exact meaning of that global not confirmed.
-> FinishPlacingHeldItem

Run via:
    .\run_ida_script.ps1 name_finish_placing_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2BA62
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FinishPlacingHeldItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FinishPlacingHeldItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Plays a sound, restores the cursor background, loads the held "
    "item's catalog record, ORs a value derived from its flag byte "
    "into word_36C81 (not otherwise documented), then clears the "
    "held-item cursor (UpdateCursorForHeldItem(0)). Called from "
    "sub_2621C (a still-untraced container-related handler) and "
    "sub_271DC.",
    False,
)

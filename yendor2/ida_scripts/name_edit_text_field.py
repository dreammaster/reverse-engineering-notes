"""
Names sub_1D1D4, high confidence: a generic single-line text input
editor (bx=buffer pointer via si, cx=max length), confirmed by one of
its 6 call sites being inside EditCharacterName's own address range
(character name entry). Draws a '-' cursor, polls keyboard
(PollKeyboardInput) for: Enter (0xD) -> confirm, null-terminate,
errorCode=0; Backspace (8) -> delete last char (or beep via the sound
dispatch, ax=3, if already empty); Escape (0x1B) -> cancel, null-
terminate, errorCode=2; printable chars (0x20-0x7F) -> append (or beep
if at max length). Reused across at least 6 different text-entry
screens (character name, and others not individually traced).
-> EditTextField

Run via:
    .\run_ida_script.ps1 name_edit_text_field.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D1D4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "EditTextField", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'EditTextField': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Generic single-line text input editor (bx=buffer, cx=max length): "
    "draws a '-' cursor, polls keyboard for Enter (confirm, "
    "errorCode=0), Backspace (delete/beep), Escape (cancel, "
    "errorCode=2), or printable chars (append/beep at limit). One of "
    "its 6 call sites is inside EditCharacterName.",
    False,
)

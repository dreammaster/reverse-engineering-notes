"""
Names sub_23A7C, called from sub_193BE and sub_23C18 (both not fully
traced): draws a null-terminated string (bx) character by character
via writeChar, using one color (word_2E412) for the first character
and a second color (word_2E414) for the rest -- a common
"highlighted-hotkey-letter" label style (e.g. drawing "Yes" with the
Y in a different color). Advances bx past the terminator.

-> WriteTwoToneString

Run via:
    .\run_ida_script.ps1 name_write_two_tone_string.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x23A7C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "WriteTwoToneString", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'WriteTwoToneString': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws a string (bx) with the first character in word_2E412's "
    "color and the rest in word_2E414's -- a highlighted-hotkey-letter "
    "label style. Called from sub_193BE and sub_23C18.",
    False,
)

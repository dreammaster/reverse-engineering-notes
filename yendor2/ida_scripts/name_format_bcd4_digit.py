"""
Names sub_19E15, called twice from FormatAndDrawBCD4's digit loop.

Per-digit output helper for BCD4 formatting: ah=the digit value
(0-9), dl=a "first significant digit already emitted" flag, di=output
buffer cursor. If the digit is a leading zero (ah==0) and no
significant digit has been emitted yet (dl!=1), blanks out any
pre-filled ',' thousands-separator placeholder in the buffer (turns
it into a space) rather than emitting the zero, or emits a literal
'0' if this is the ones place. Otherwise (a real digit, or once past
the leading-zero region), keeps/advances past any ',' separator and
writes the digit's ASCII character. Implements BCD4's leading-zero
suppression with comma-thousands-separator formatting.
-> FormatBCD4Digit

Run via:
    .\run_ida_script.ps1 name_format_bcd4_digit.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19E15
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FormatBCD4Digit", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FormatBCD4Digit': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Per-digit output helper for FormatAndDrawBCD4: ah=digit value, "
    "dl=first-significant-digit-emitted flag, di=output cursor. "
    "Implements leading-zero suppression (blanking pre-filled ',' "
    "separators to spaces) and comma-thousands-separator formatting. "
    "Called twice from FormatAndDrawBCD4.",
    False,
)

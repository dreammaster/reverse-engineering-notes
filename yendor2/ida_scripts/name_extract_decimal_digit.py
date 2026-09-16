"""
Names sub_18041, called repeatedly from FormatNumber's own
implementation -- the digit-extraction helper for its digit-by-digit
decimal conversion loop.

Divides the remaining value ([bp-2]) by a power-of-10 divisor
([bp-4]), writes the quotient as an ASCII digit at [bx] (+'0'),
subtracts the digit's contribution back out of [bp-2] (remainder),
and advances bx. Then, unless [bp-6]==1 (plausibly "this is the final/
ones digit, always show it"), blanks a leading zero: if the digit just
written is '0', replaces it with a space instead -- the classic
leading-zero-suppression trick for fixed-width number formatting.
-> ExtractDecimalDigit

Run via:
    .\run_ida_script.ps1 name_extract_decimal_digit.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x18041
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ExtractDecimalDigit", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ExtractDecimalDigit': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Divides [bp-2] by divisor [bp-4], writes the quotient digit at "
    "[bx] (+'0'), subtracts it back out of [bp-2], advances bx. "
    "Blanks a leading zero to a space unless [bp-6]==1. Called "
    "repeatedly from FormatNumber's digit-conversion loop.",
    False,
)

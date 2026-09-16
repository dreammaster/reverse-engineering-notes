"""
Names sub_16262, called from DrawLabeledNumberIfNonzero and
FormatAndDrawAlchemyFraction -- inserts a decimal point into an
in-place number string, word_2E4AC digits from the end.

No-op if the string at [bx] is empty or word_2E4AC <= 0. Otherwise
finds the string's NUL terminator, then shifts everything from
word_2E4AC+1 characters before the end rightward by one position, and
writes '.' into the vacated slot -- turning e.g. "1234" (with
word_2E4AC=2) into "12.34". A fixed-point decimal formatting helper.
-> InsertDecimalPointFromEnd

Run via:
    .\run_ida_script.ps1 name_insert_decimal_point.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x16262
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "InsertDecimalPointFromEnd", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'InsertDecimalPointFromEnd': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Inserts '.' into the in-place number string at [bx], "
    "word_2E4AC digits from the end (e.g. '1234' -> '12.34' for "
    "word_2E4AC=2) by shifting the trailing digits right. No-op if "
    "the string is empty or word_2E4AC<=0. Called from "
    "DrawLabeledNumberIfNonzero and FormatAndDrawAlchemyFraction.",
    False,
)

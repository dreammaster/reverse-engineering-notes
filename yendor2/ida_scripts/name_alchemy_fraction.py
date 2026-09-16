"""
Names sub_1E2E5, called only from DrawAlchemyStatusPanel: a near-
duplicate of the already-named FormatAndDrawFraction (same
"<num1>/<num2>" formatting shape, separator string at 0x7960 reused
via 0xAFDA), but unconditionally calling sub_2570C + sub_16262 for
both numbers instead of FormatAndDrawFraction's conditional
StripSpaces step. -> FormatAndDrawAlchemyFraction

Run via:
    .\run_ida_script.ps1 name_alchemy_fraction.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E2E5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FormatAndDrawAlchemyFraction", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FormatAndDrawAlchemyFraction': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Near-duplicate of FormatAndDrawFraction ('<num1>/<num2>' display), "
    "but unconditionally applying sub_2570C + sub_16262 to both "
    "numbers. Called only from DrawAlchemyStatusPanel.",
    False,
)

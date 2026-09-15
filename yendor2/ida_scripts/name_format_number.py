"""
Names sub_17FB8: formats ax as a decimal string into the buffer at bx.
ax==0 is special-cased (writes "  0", space-padded). Otherwise loops
extracting digits via successive divisors (0x2710=10000, 0x3E8=1000,
presumably continuing 100/10/1) through a helper (sub_18041) tracking
whether a nonzero digit has been seen yet (leading-zero suppression).
Used by CastSpell (formatting a spell's damage/effect amount) and
sub_13957. -> FormatNumber

Run via:
    .\run_ida_script.ps1 name_format_number.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x17FB8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "FormatNumber", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'FormatNumber': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Formats ax as a decimal string into the buffer at bx (space-"
    "padded '0' for ax==0). Extracts digits via successive divisors "
    "with leading-zero suppression (sub_18041).",
    False,
)

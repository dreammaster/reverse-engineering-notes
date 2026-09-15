"""
Traced sub_21DE2 (called from sub_12ECD and sub_13FCF, both unexplored
-- likely save/load related) and dumped its two fixed strings: a
version/compatibility check, not a text-record parse.

Reads a small record from WORLD.DAT (block 3) plus a second field via
sub_28000, trims trailing spaces. Two specific character positions in
the result are checked against '0' (no-mismatch sentinel) and ' '
(unset sentinel): if the first is set, builds " LEVEL X" (X = that
character) and returns ax=2; else if the second is set, builds
" MAP X" and returns ax=1; else returns ax=0 (compatible). Reads as
validating that the loaded WORLD.DAT/CURGAME matches an expected
level/map number -- a save-compatibility check.

-> CheckWorldDatCompatibility

Run via:
    .\run_ida_script.ps1 name_check_worlddat_compat.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x21DE2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckWorldDatCompatibility", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckWorldDatCompatibility': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Reads a small WORLD.DAT record (block 3) plus a second field "
    "(sub_28000), trims trailing spaces. Checks two character "
    "positions against sentinels: if set, builds ' LEVEL X' (ax=2) "
    "or ' MAP X' (ax=1) with the mismatched value; else ax=0 (ok). "
    "Reads as a save/WORLD.DAT version-compatibility check.",
    False,
)

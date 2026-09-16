"""
Names sub_1D146, called 7 times incl. from EditWallLegendTypeNumber and
EditFloorLegendTypeNumber (map editor cluster): calls EditTextField to
read a typed string, then parses it digit-by-digit into a 16-bit
integer at [si] (standard *10+digit accumulation), setting errorCode=4
if a non-digit character is found. A generic "prompt for and parse a
typed integer" utility built on EditTextField. -> ReadTypedInteger

Run via:
    .\run_ida_script.ps1 name_read_typed_integer.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1D146
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ReadTypedInteger", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ReadTypedInteger': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Calls EditTextField to read a typed string, then parses it "
    "digit-by-digit into a 16-bit integer at [si] (errorCode=4 on a "
    "non-digit character). A generic typed-integer prompt, used by the "
    "map editor's legend-number editors among others.",
    False,
)

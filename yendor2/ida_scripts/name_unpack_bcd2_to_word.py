"""
Names sub_19B3E, called once from ComputeAlchemyRefinementYield.

Unpacks 4 BCD digits from 2 bytes ([si+2] high/low nibbles,
[si+3] high/low nibbles) into their decimal value: high([si+2])*1000
+ low([si+2])*100 + high([si+3])*10 + low([si+3]). A small,
self-contained "2-byte packed-BCD to word" converter -- distinct from
the general 4-byte BCD4 bignum library (ConvertWordToBCD4 etc) used
elsewhere in the game. -> UnpackBCD2ToWord

Run via:
    .\run_ida_script.ps1 name_unpack_bcd2_to_word.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19B3E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UnpackBCD2ToWord", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UnpackBCD2ToWord': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Unpacks 4 packed-BCD digits from [si+2]/[si+3] (2 bytes) into "
    "their decimal value (high([si+2])*1000 + low([si+2])*100 + "
    "high([si+3])*10 + low([si+3])). Distinct from the 4-byte BCD4 "
    "bignum library used elsewhere. Called once from "
    "ComputeAlchemyRefinementYield.",
    False,
)

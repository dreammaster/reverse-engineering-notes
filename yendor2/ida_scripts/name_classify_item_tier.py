"""
Names sub_1AE9D, called 6 times from sub_1AC80 and sub_1ACD7 (both
unnamed): loads an item catalog record and classifies it into one of
3 error/tier codes based on its flags. If neither of two "type" flags
([+0xC] bits 0xC000 or 0x800 -- similar shape to the flags
ListCompatibleClueBookItems checks) is set, errorCode=3 (wrong item
type entirely). Otherwise checks a secondary flag pair ([+2] bits
0x100/0x200 for the 0xC000 branch, 0x40/0x80 for the 0x800 branch) to
set errorCode=2 or 1. Returns the loaded record pointer in bx. Reads
as a 3-tier item-compatibility classifier, but the specific meaning of
each tier and the two callers' purpose aren't confirmed.
-> ClassifyItemServiceTier

Run via:
    .\run_ida_script.ps1 name_classify_item_tier.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AE9D
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClassifyItemServiceTier", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClassifyItemServiceTier': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Loads an item and classifies it via errorCode: 3 if neither "
    "[+0xC] bit 0xC000 nor 0x800 is set (wrong item type), else 2 or 1 "
    "based on a secondary [+2] flag pair. A 3-tier item-compatibility "
    "classifier; exact tier meanings and the callers' (sub_1AC80/"
    "sub_1ACD7) purpose aren't confirmed.",
    False,
)

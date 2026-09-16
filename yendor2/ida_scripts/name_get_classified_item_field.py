"""
Names sub_1AE23, called from sub_16BF6: uses ClassifyItemServiceTier
to classify an item; if classification fails (errorCode != 0), returns
bx=0. Otherwise selects one of two fields from word_2E548 (the
"current target" struct) based on the item's category flag ([+0xC]
bit 0xC000): [+4] for one category, [+8] for the other. Reads as
"pick the target field matching this item's classified category," but
the specific meaning of word_2E548+4/+8 isn't confirmed.
-> GetClassifiedItemStatField

Run via:
    .\run_ida_script.ps1 name_get_classified_item_field.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AE23
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "GetClassifiedItemStatField", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'GetClassifiedItemStatField': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Uses ClassifyItemServiceTier; if classification fails, returns "
    "bx=0. Otherwise picks word_2E548+4 or +8 based on the item's "
    "[+0xC] bit 0xC000 category flag. Called from sub_16BF6.",
    False,
)

"""
Names sub_1BBED, gated on UseItem's word_2E410 bit 0x800 -- the
direct structural sibling of the already-named UseItemType_400 (bit
0x400), sub-dispatching on the identical SelectItemUseRecord
es:[si+0x10] bit pattern (1/4/2/0x400/default).

Its bit-2 branch (the type-2 branch file-formats.md already
references informally as "sub_1BBED's type-2 branch"): pays a BCD
gold cost (CompareBCD4 against the same 0x512A threshold
UseItemType_400 uses), then checks a per-character stat field
([+0x14] offset, compared against a [+0x16] cap) -- if already at
cap, shows a cost-confirmation dialog (DrawItemUseConfirmDialog +
DrawConfirmPromptGoldLine) before proceeding, else applies silently.
-> UseItemType_800

Run via:
    .\run_ida_script.ps1 name_use_item_type_800.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1BBED
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseItemType_800", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseItemType_800': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem's item-type handler for word_2E410 bit 0x800 -- "
    "structural sibling of UseItemType_400, same "
    "SelectItemUseRecord [si+0x10] bit dispatch (1/4/2/0x400/"
    "default). Bit-2 branch pays a BCD gold cost against the shared "
    "0x512A threshold, confirming first if a per-character stat is "
    "already at its cap.",
    False,
)

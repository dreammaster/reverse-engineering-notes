"""
Names sub_276C5, called 3 times from sub_274B4: checks the current
item's category flags ([+0xC] bits 0xC000/0x800, the same pair
ClassifyItemServiceTier/GetClassifiedItemStatField check) and a
matching word_2E548 sub-flag ([+2] bits 0x200/0x80); if both match,
removes the current item's (word_32974) multi-stat effect via
RemoveMultiStatEffect, swaps in a new item id from word_2E548's `+4`
or `+8` field (the exact fields GetClassifiedItemStatField selects
between), and applies the new item's effect via
ApplyMultiStatEffectForItem. Reads as "replace this equipped item's
stat effect with a different item's, per category" -- ties directly
into the same category/field-selection pattern found across several
functions this session. -> SwapItemMultiStatEffect

Run via:
    .\run_ida_script.ps1 name_swap_item_multi_stat_effect.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x276C5
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SwapItemMultiStatEffect", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SwapItemMultiStatEffect': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "If the current item's category ([+0xC] 0xC000/0x800) and a "
    "matching word_2E548 sub-flag ([+2] 0x200/0x80) both hold, removes "
    "the current item's effect (RemoveMultiStatEffect), swaps in a new "
    "item id from word_2E548+4/+8 (the same fields "
    "GetClassifiedItemStatField selects), and applies its effect "
    "(ApplyMultiStatEffectForItem). Called from sub_274B4.",
    False,
)

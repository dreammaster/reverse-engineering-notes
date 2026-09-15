"""
Names the two remaining entry points completing the shop-screen
cluster, siblings of RunSellItemScreen (sub_1B245, word_328C6 bit
0x10, from UseItem+0xD3):

sub_1B0CF (-> RunEnhanceItemScreen): called from UseItem+0x1C1. Sets
word_328C6 bit 8 -- TryEnhanceItemForGold's gating bit -- then runs
sub_1869D (main input loop) so Space triggers TryEnhanceItemForGold,
identical shape to RunSellItemScreen otherwise (resource overlay,
material/gold HUD, minimap rebuild on exit).

sub_1B194 (-> RunRepairItemScreen): called from UseItem+0x1D0. Same
shape, but sets word_328C6 bit 4 -- TryRepairItemForGold's gating bit.

Completes the shop cluster's UseItem-reachable entry points: sell
(bit 0x10), enhance (bit 8), repair (bit 4) -- each its own item
flag branch in UseItem, all three running the same main input loop
with a different action bit set.

Run via:
    .\run_ida_script.ps1 name_run_enhance_repair_screens.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1B0CF: "RunEnhanceItemScreen",
    0x1B194: "RunRepairItemScreen",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1B0CF,
    "Entry point for the interactive enhance-item screen, reached from "
    "UseItem+0x1C1. Sets word_328C6 bit 8 (gates TryEnhanceItemForGold "
    "in the main input loop sub_1869D), shows the resource-depleted "
    "overlay and material/gold HUD, runs sub_1869D, then rebuilds/"
    "redraws the minimap on exit. Sibling of RunSellItemScreen "
    "(bit 0x10) and RunRepairItemScreen (bit 4).",
    False,
)
ida_bytes.set_cmt(
    0x1B194,
    "Entry point for the interactive repair-item screen, reached from "
    "UseItem+0x1D0. Sets word_328C6 bit 4 (gates TryRepairItemForGold "
    "in the main input loop sub_1869D). Otherwise identical to "
    "RunSellItemScreen (bit 0x10) and RunEnhanceItemScreen (bit 8).",
    False,
)

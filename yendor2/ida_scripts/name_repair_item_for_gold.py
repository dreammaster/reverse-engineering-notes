"""
Names sub_19140, the third sibling Space-bar action in sub_1869D's
main input loop (word_328C6 bit 4, alongside TrySellItemForGold's
bit 0x10 and TryEnhanceItemForGold's bit 8). Structurally identical to
TryEnhanceItemForGold: eligibility check (sub_1B20C) gating a 2-line
rejection message, dumped as "I CAN NOT REPAIR THAT" (msg 0x81E6) --
confirming this is the shop's gold-based item repair (distinct from
RepairItemCommand's skill-based repair minigame, already documented in
file-formats.md, which can critically fail and destroy the item).

Mechanism: CompareBCD4(g_partyGold, [table 0x5082]) -- on insufficient
gold, "YOU DON'T HAVE ENOUGH GOLD!" (msg 0x8376, via sub_190AF, shared
with TryEnhanceItemForGold); else SubBCD4(g_partyGold -= [0x5082]),
then restores the item id from word_3194C into word_31948 (the
"repaired" result -- unlike TryEnhanceItemForGold, which advances to
word_32974+1, this restores a previously-known id, consistent with
"fixing" the same item rather than upgrading to a new one) and reloads
it via LoadItemCatalogRecord.

-> TryRepairItemForGold

Run via:
    .\run_ida_script.ps1 name_repair_item_for_gold.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19140
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TryRepairItemForGold", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TryRepairItemForGold': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Space-bar 'repair item' action (sub_1869D, word_328C6 bit 4), "
    "sibling of TrySellItemForGold/TryEnhanceItemForGold. Eligibility "
    "via sub_1B20C; on failure 'I CAN NOT REPAIR THAT' (msg 0x81E6). "
    "Else CompareBCD4/SubBCD4(g_partyGold, [table 0x5082]) -- "
    "'YOU DON'T HAVE ENOUGH GOLD!' on failure (msg 0x8376, shared with "
    "TryEnhanceItemForGold) -- then restores the item from "
    "word_3194C into word_31948 (fixing the same item, not upgrading "
    "to a new catalog entry) and reloads it. Distinct from the "
    "skill-based RepairItemCommand minigame, which can critically fail "
    "and destroy the item.",
    False,
)

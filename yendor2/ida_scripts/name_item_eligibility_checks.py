"""
Names the two small eligibility-check helpers gating
TryEnhanceItemForGold and TryRepairItemForGold (each has other
callers too -- sub_1CCBC among them -- not traced, so these are named
for their confirmed primary behavior only).

sub_1B147 (-> IsItemEligibleForEnhance): given the held item
(word_2E548) and the standing location (word_2E546), checks whether
the location's [+0xC] flags (0xA00 exact-match special case, else
0xC000/broad) select one of two held-item fields ([+8] or [+6]) and
whether that value falls within a range table at DS:0xBCE
([+0x14]..[+0x16]) -- returns "eligible" (ax=0) only when in range (or
immediately via the 0xA00 special-case reads). Gates
TryEnhanceItemForGold's "I CAN NOT ENHANCE THAT" rejection.

sub_1B20C (-> IsItemEligibleForRepair): simpler bit-pair match: for
each of 2 location-flag bits (0xC000, 0x800) on word_2E546's [+0xC],
if set AND the held item's corresponding flag (word_2E548's [+2] bit
0x100 or 0x40) is also set, returns "eligible" (ax=0); otherwise not
eligible. Gates TryRepairItemForGold's "I CAN NOT REPAIR THAT"
rejection.

Both read as "does this station/location accept this category of
item" checks -- the same shape of problem TrySellItemForGold solves
with a plain bitmask AND test, just with different flag-selection
logic per action.

Run via:
    .\run_ida_script.ps1 name_item_eligibility_checks.py
"""
import idc
import ida_name
import ida_bytes

RENAMES = {
    0x1B147: "IsItemEligibleForEnhance",
    0x1B20C: "IsItemEligibleForRepair",
}

for ea, name in RENAMES.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x1B147,
    "Eligibility check for TryEnhanceItemForGold (also called from "
    "sub_1CCBC and others, not traced). Selects a held-item field "
    "([+8] or [+6], word_2E548) based on the location's ([+0xC], "
    "word_2E546) flag bits, and checks it against a range table at "
    "DS:0xBCE ([+0x14]..[+0x16]). Returns eligible (ax=0) if in range.",
    False,
)
ida_bytes.set_cmt(
    0x1B20C,
    "Eligibility check for TryRepairItemForGold (also called "
    "elsewhere, not traced). For each of 2 location-flag bits "
    "(word_2E546's [+0xC] 0xC000/0x800), if set and the held item's "
    "matching flag (word_2E548's [+2] 0x100/0x40) is also set, "
    "returns eligible (ax=0).",
    False,
)

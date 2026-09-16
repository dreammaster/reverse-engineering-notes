"""
Names sub_1AA9B, called from HandleIconBarItemExpiry and
ApplyIconBarStatDelta -- recomputes carry capacity and two
threshold-gated attribute bonuses, then triggers a full equipment
bonus recompute.

For the current party member (si=word_328D4):
1. Recomputes carry capacity: [si+0x56] = [si+0x3C]*10 and
   [si+0x96] = [si+0x7C]*10 -- confirms and re-derives the documented
   Strength-derived carry-capacity fields (base/derived pair
   +0x3C/+0x7C).
2. For each of 4 fields -- [si+0x3C] (Strength base) -> [si+0x38],
   [si+0x3E] -> [si+0x3A], [si+0x7C] (Strength derived) -> [si+0x78],
   [si+0x7E] -> [si+0x7A] -- zeroes the target field, then if the
   source exceeds 0x48 (72), scales 20% of the excess
   (ScaleByPercentRounded) into the target. This is a new data point
   for +0x3E/+0x7E (previously "no secondary use found yet"): values
   above 72 grant a scaled bonus into +0x3A/+0x7A.
3. Calls RecomputeEquipmentStatBonuses to refresh equipment-derived
   totals afterward.

In short: a runtime refresh of carry capacity and threshold-gated
attribute bonuses, run whenever an icon-bar effect changes a
relevant stat. -> RefreshCarryCapacityAndAttributeBonuses

Run via:
    .\run_ida_script.ps1 name_refresh_carry_capacity_bonuses.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1AA9B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RefreshCarryCapacityAndAttributeBonuses", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RefreshCarryCapacityAndAttributeBonuses': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Recomputes carry capacity ([si+0x56]/[si+0x96] = "
    "[si+0x3C]/[si+0x7C] * 10), then for [si+0x3C]->[si+0x38], "
    "[si+0x3E]->[si+0x3A], [si+0x7C]->[si+0x78], [si+0x7E]->[si+0x7A]: "
    "if the source exceeds 0x48 (72), scales 20% of the excess into "
    "the target (else zeroes it). Finishes with "
    "RecomputeEquipmentStatBonuses. Called from "
    "HandleIconBarItemExpiry and ApplyIconBarStatDelta.",
    False,
)

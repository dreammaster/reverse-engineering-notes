"""
Names sub_1ACD7, called from HandleDungeonInput and sub_16881 with
ax = an equipment-slot field offset (0x13A main weapon, 0x142 second
slot, 0x146 array entry -- the confirmed equipment-slot layout from
RecomputeEquipmentStatBonuses/RefreshCarryCapacityAndAttributeBonuses).

For the given slot, if occupied: classifies the item
(ClassifyItemServiceTier), increments a per-slot wear counter
([+0xBE]/[+0xC0]/[+0xC2] for weapon/second-slot/array respectively),
and once it exceeds a slot-specific threshold (0x78/0x50/0x14),
re-classifies again and rolls a percentage breakage chance: tests the
item's [+0xC] flag bits 0x8000/0x4000 to pick which of word_2E548's
break-chance fields ([+0xA]/[+0x6], paired with replacement-item ids
at [+0x8]/[+0x4]) applies, then RandomInRange(1000) against it. On a
break, calls ApplyItemEffectIconSlot (a "item broke" status effect)
and resets the wear counter to 0 (errorCode=0); otherwise leaves it
worn (errorCode=1). Implements the game's equipped-item durability
and random breakage system. -> TickEquippedItemDurability

Run via:
    .\run_ida_script.ps1 name_tick_equipped_item_durability.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1ACD7
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickEquippedItemDurability", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickEquippedItemDurability': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Equipped-item durability/breakage tracker for the slot given by "
    "ax (0x13A/0x142/0x146). Increments a per-slot wear counter "
    "([+0xBE]/[+0xC0]/[+0xC2]); once it crosses a slot-specific "
    "threshold, rolls a percentage breakage chance from word_2E548's "
    "fields and, on a break, applies an 'item broke' effect "
    "(ApplyItemEffectIconSlot) and resets the counter. Called from "
    "HandleDungeonInput and sub_16881.",
    False,
)

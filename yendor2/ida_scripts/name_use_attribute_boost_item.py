"""
Names sub_1B4C2, reached from UseItem's item [+0xE] flag 0x200 path
(the sibling of UseExperienceBoostItem's 0x400 path, same structure).

Gated on the same kind of one-time-use global flag mechanism
(TestGlobalFlag/SetGlobalFlag on a value at a fixed table 0xBCE+0x12).
Computes a label via ((0xBCE+0x14 value - 0x3C) / 2) * 13 + 0x7DC7 --
a 13-byte-stride string table indexed by a party-record field offset
(0xBCE+0x14 IS that offset, e.g. 0x3C/0x3E/... one of the "core
stat" fields) -- draws that stat's name, then "+<amount>" (0xBCE+0x16).
Loops all 4 party slots and, for each eligible member (not flagged
+0x1C bit 0x40), adds the amount to the stat field at that offset AND
again at offset+0x40 (the paired field 0x40 bytes over, matching the
current/max stat-pair stride already established for +0x3E/+0x7E),
clamping each via ClampValueAtSlotToTypeCap, then
RefreshCarryCapacityAndAttributeBonuses. A one-time-use "tome of
attribute" item: permanently raises one core stat (both its current
and max/paired field) for every living party member, item data
selecting which stat by literal field offset.
-> UseAttributeBoostItem

Run via:
    .\run_ida_script.ps1 name_use_attribute_boost_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B4C2
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseAttributeBoostItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseAttributeBoostItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One-time-use 'tome of attribute' item effect (UseItem's item "
    "[+0xE] bit 0x200 path, sibling of UseExperienceBoostItem). "
    "0xBCE+0x14 holds the target stat's party-record field offset "
    "directly (also used to look up its display name in a 13-byte "
    "string table at 0x7DC7); 0xBCE+0x16 is the boost amount. Adds "
    "the amount to that field and to field+0x40 (the paired "
    "current/max stat 0x40 bytes over) for every eligible living "
    "party member, clamped via ClampValueAtSlotToTypeCap.",
    False,
)

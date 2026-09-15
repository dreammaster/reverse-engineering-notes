"""
Names sub_2738B, called from `start` and HandleDungeonInput: the click
handler for the 6-slot equipment icon bar (gated on word_36C7F bit
0x1000, the "icon bar visible" flag from ApplyEffectAndDrawIconBar's
context).

Hit-tests region table 0x636C for one of 6 slots (values 4-9), mapping
each to an entry in the fixed 6-entry table at 0x9519 -- the *same*
table `IsItemRangeAvailable` checks first before falling back to a
full inventory search. If not already carrying an item (word_31946==0)
and the slot has an item, loads its catalog record; unless its
[+0xC] bit 0x100 is set, validates via sub_2D5E0 (not traced) first.
On success, stages the item into the "carrying" state (word_32974=
item id, word_32970/3297A and friends) and returns errorCode=2 --
picking up/removing an equipped icon-bar item.

-> HandleEquipmentIconClick

Run via:
    .\run_ida_script.ps1 name_equipment_icon_click.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2738B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleEquipmentIconClick", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleEquipmentIconClick': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Click handler for the 6-slot equipment icon bar (word_36C7F bit "
    "0x1000). Hit-tests region table 0x636C, maps to the fixed 6-entry "
    "table 0x9519 (same table IsItemRangeAvailable checks), validates "
    "via sub_2D5E0 if needed, then stages the item into the "
    "'carrying' state (errorCode=2 on success). Called from `start` "
    "and HandleDungeonInput.",
    False,
)

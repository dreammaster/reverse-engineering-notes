"""
Names sub_2772C, called from sub_274B4 (itself called from
CheckAndPaySpecialItemCost and RestCharacter's cleanup): writes each of
the 3 "alternate bag" inventory groups (party record +0x17E/+0x180,
+0x1A4/+0x1A6, +0x1CA/+0x1CC -- the same group-base fields
GetInventorySlotPtr and WriteContainerSubBlock already established,
count field followed by its slot data) back to CURGAME via
WriteContainerSubBlock, only when each group's count is nonzero (i.e.
that alternate bag is actually populated). -> SyncAlternateBagsToSave

Run via:
    .\run_ida_script.ps1 name_sync_alternate_bags.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2772C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SyncAlternateBagsToSave", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SyncAlternateBagsToSave': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Writes each of the 3 alternate-bag inventory groups (+0x17E/0x180, "
    "+0x1A4/0x1A6, +0x1CA/0x1CC -- the same fields GetInventorySlotPtr/"
    "WriteContainerSubBlock established) back to CURGAME via "
    "WriteContainerSubBlock, only when populated. Called from "
    "sub_274B4.",
    False,
)

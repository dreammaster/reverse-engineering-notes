"""
Names sub_22A35, called from sub_22989: repeatedly picks a random
party slot (RandomInRange(3), 0-3) until it finds one that's occupied
(g_partySlotAssignment[slot] != 0) and not incapacitated (+0x1C bits
0x1C40 -- the confirmed DEAD/STONED/FROZEN/PARALYZED mask), leaving
its record selected via SelectPartyRecordById. Also computes
di=0xBC28+slot*0x18 along the way (a second, smaller per-slot table,
purpose not confirmed) for the caller. -> PickRandomActivePartyMember

Run via:
    .\run_ida_script.ps1 name_pick_random_member.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22A35
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "PickRandomActivePartyMember", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'PickRandomActivePartyMember': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Retries RandomInRange(3) until it lands on an occupied, "
    "non-incapacitated (+0x1C bits 0x1C40) party slot, leaving it "
    "selected via SelectPartyRecordById. Also computes "
    "di=0xBC28+slot*0x18 (a smaller per-slot table, not confirmed) for "
    "the caller. Called from sub_22989.",
    False,
)

"""
Names sub_2D7EA, called from InteractWithContainer: the exact inverse
of MarkIneligiblePartyMembers -- clears +0x15E bit 0x8000 for all 4
g_partySlotAssignment members (unconditionally, not gated on the
eligibility check). Reads as resetting the "needs attention" flag
before/after a container interaction sequence. -> ClearIneligibleFlagForAllMembers

Run via:
    .\run_ida_script.ps1 name_clear_ineligible_flags.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D7EA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ClearIneligibleFlagForAllMembers", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ClearIneligibleFlagForAllMembers': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Clears +0x15E bit 0x8000 (the 'needs attention' flag "
    "MarkIneligiblePartyMembers sets) for all 4 party slots "
    "unconditionally. Called from InteractWithContainer.",
    False,
)

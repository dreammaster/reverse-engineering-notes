"""
Names sub_1B7DD, called from UseItem and UseTrainingItem: dereferences
word_32924 (a pointer into g_partySlotAssignment, already documented --
e.g. set to 0x95EB/0x95ED/0x95EF elsewhere) to get the active slot's
character id, resolves it via SelectPartyRecordById, calls
CheckForLevelUp, and if the resulting party record's +0x1E ("pending
level-up", already confirmed) is nonzero, calls ShowLevelUpMessage.
The standard "did that item use trigger a level-up" check shared by
both callers. -> CheckAndAnnounceLevelUp

Run via:
    .\run_ida_script.ps1 name_check_announce_levelup.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B7DD
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CheckAndAnnounceLevelUp", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CheckAndAnnounceLevelUp': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Resolves the active party slot (word_32924 -> g_partySlotAssignment "
    "entry -> character id -> SelectPartyRecordById), calls "
    "CheckForLevelUp, and shows ShowLevelUpMessage if +0x1E (pending "
    "level-up) is now nonzero. Shared post-item-use level-up check, "
    "called from UseItem and UseTrainingItem.",
    False,
)

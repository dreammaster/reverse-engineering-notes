"""
Names 4 more small helpers.

sub_1FD03 (called very widely from `start`, 30 refs): always sets
word_3295A bit 0x4000, plus bit 0x2000 if word_36DF5 is nonzero -- a
"mark screen state dirty" trigger. -> MarkScreenRedrawFlags

sub_2313D (called once from ProcessCombatRound, right after granting
a dead monster's rewards): zeroes 0x4E words (156 bytes) at [si] --
exactly one g_monsterSlots record's size. Clears the dead monster's
slot record. -> ClearMonsterSlotRecord

sub_1DCC6 (called from HandleRangedOrCombatAction): scans a 3-entry
table (0x5078) for the first nonzero entry and clears it to 0.
-> ClearFirstOccupiedCombatSlot

sub_1F58D (called from RunGameDialog and SaveCurrentGameToSlot):
clears the highlight flag bit 0x40 across all 6 entries of the pause
menu's label table (0x6CBE, the same table
DrawGameDialogMenuLabelsHighlighted uses), then sets it on one
specific entry (word_32906) -- moves the pause menu's highlight to a
new selection. -> HighlightGameDialogMenuEntry

Run via:
    .\run_ida_script.ps1 name_last_small_batch.py
"""
import idc
import ida_name
import ida_bytes

entries = [
    (0x1FD03, "MarkScreenRedrawFlags",
     "Sets word_3295A bit 0x4000 always, plus bit 0x2000 if "
     "word_36DF5 is nonzero. Called very widely from `start`."),
    (0x2313D, "ClearMonsterSlotRecord",
     "Zeroes one g_monsterSlots record (156 bytes) at [si]. Called "
     "once from ProcessCombatRound after granting a dead monster's "
     "rewards."),
    (0x1DCC6, "ClearFirstOccupiedCombatSlot",
     "Scans a 3-entry table (0x5078) for the first nonzero entry and "
     "clears it. Called from HandleRangedOrCombatAction."),
    (0x1F58D, "HighlightGameDialogMenuEntry",
     "Clears highlight bit 0x40 across all 6 pause-menu label "
     "entries (0x6CBE), then sets it on one entry (word_32906) -- "
     "moves the highlight to a new selection. Called from "
     "RunGameDialog and SaveCurrentGameToSlot."),
]

for ea, name, desc in entries:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")
    ida_bytes.set_cmt(ea, desc, False)

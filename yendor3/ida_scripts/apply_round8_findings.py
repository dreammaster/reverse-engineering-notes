"""
Round 8: found the real CastSpell by structural analysis rather than
call-list diffing. Read yendor2's HandleGameCommand to find CastSpell's
exact dispatch gate shape (a 3-comparison pattern: range-low, range-
high-or-call, then a variable-equality exception -- "g_currentActionId
in [0x12,0x1D] or == _val46"), then found the identical shape in
yendor3's already-renamed HandleGameCommand: "ds:0x5426 in [0x34,0x3F]
or == ds:0x5464" -> call sub_2AE87. That's the address BinDiff had
separately (and wrongly, 0.17 similarity) labeled
"PartyMassHealAndOverheal" -- its actual call list (RedrawItemDescriptionAndMaterials/
ConfirmAndSelectPartyTarget/ConsumeItemChargeResource/
RedrawAllPartyStatusPanels/ApplyMapTriggerEffect) independently matches
CastSpell's self-heal/target-effect shape too. Two independent
confirmations. See docs23/engine-diffs.md.

Also confirms (from the same HandleGameCommand read) that
DispatchItemAbilityCommand's old command slot was not just mismatched
by BinDiff but genuinely replaced: the exact same tail dispatch
position that called DispatchItemAbilityCommand in yendor2 now calls
HandleSpecialQuestCommand in yendor3 (already renamed, round 6) -- no
separate "real" DispatchItemAbilityCommand exists to find.

Run via:
    .\run_ida_script.ps1 apply_round8_findings.py
"""
import idc
import ida_name

ea = 0x2AE87
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CastSpell", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CastSpell': {'ok' if ok else 'FAILED'}")

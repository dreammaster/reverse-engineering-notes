"""
Precision fix (not a naming error, just an overstated comment): all 6
traced call sites of ShowHealingCostPrompt (5 in UseHealingItem/
UseItemType_400, 1 in UseTrainingItem) retf immediately after the
call -- none of them poll Y/N or deduct gold themselves. The earlier
comment said "the Y/N poll and gold deduction happen in the caller,
not traced", which overstates what's confirmed: no traced call site
does this at all, so the actual confirm+pay step (if any) must happen
on a separate, later re-entry (e.g. after a keypress), not found yet.

Run via:
    .\run_ida_script.ps1 fix_healing_cost_prompt_comment.py
"""
import ida_bytes

ea = 0x1B96F
ida_bytes.set_cmt(
    ea,
    "Computes and displays a temple/healer paid-service cost: total = "
    "sum over [word_328D4+0x16] iterations of (ax * [0xBCE+0x18]), "
    "shown as 'IT WILL COST <total> GOLD <bx-selected reason text>. "
    "IS THAT PRICE AGREEABLE?' (msgs 0x805F/0x806D/0x8073). Reason "
    "text/bx varies per caller (UseHealingItem x4, UseItemType_400, "
    "UseTrainingItem). All 6 traced call sites retf immediately after "
    "calling this -- none poll Y/N or deduct gold here. The actual "
    "confirm+pay step, if any, isn't found yet.",
    False,
)
print("comment updated")

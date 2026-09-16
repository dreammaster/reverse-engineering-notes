"""
Names sub_2D7A7, moderate confidence: called from InteractWithContainer.
Iterates the 4 g_partySlotAssignment members; for each, checks
eligibility via sub_27A66(ax=word_3330A, not traced) and, unless
eligible AND either the member's +0x1C has a matching bit
(word_332FE & 0x3F) AND their level (+0x16) already meets a threshold
(word_332D0), sets the member's +0x15E bit 0x8000 (the same
not-yet-documented flag DrawPartyStatusIcon draws a warning overlay
for -- this call is the other half of that finding: it's the "flag
this member" side) and forces a redraw via DrawPartyMemberStatusPanel.
Reads as "mark party members who don't qualify to use/interact with
whatever's in this container" (a class- or level-restricted item?),
but sub_27A66 and the exact word_332FE/word_332D0/word_3330A semantics
aren't traced -- not asserting the specific restriction.
-> MarkIneligiblePartyMembers

Run via:
    .\run_ida_script.ps1 name_flag_ineligible_members.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D7A7
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "MarkIneligiblePartyMembers", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'MarkIneligiblePartyMembers': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "For each of the 4 party slots, unless an eligibility check "
    "(sub_27A66) plus a status-flag/level test passes, sets +0x15E bit "
    "0x8000 (the flag DrawPartyStatusIcon shows a warning overlay for) "
    "and redraws DrawPartyMemberStatusPanel. Called from "
    "InteractWithContainer; the exact restriction (class/level-gated "
    "item?) isn't confirmed.",
    False,
)

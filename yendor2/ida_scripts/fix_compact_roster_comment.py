"""
Corrects CompactPartyRosterSlots's comment: it's called from
ShowWorldMap's exit path specifically ('D' key, byte_2E400==0x44, or
an equivalent mouse-click hit, both leading to a retf out of
ShowWorldMap right after), not generically "after a dismiss/toggle" --
fixing before this becomes stale.

Run via:
    .\run_ida_script.ps1 fix_compact_roster_comment.py
"""
import ida_bytes

ida_bytes.set_cmt(
    0x2BF3C,
    "Cascades non-empty roster entries down to fill gaps across the 4 "
    "active slots (g_partySlotAssignment=0x95EB, plus 0x95ED/0x95EF/"
    "0x95F1) and 3 reserve slots (word_36E4D/36E4F/36E51, not otherwise "
    "documented), setting word_328C4 bit 0x200 if anything changed. "
    "Called from ShowWorldMap's exit path ('D' key or equivalent mouse "
    "click), right before it returns -- a cleanup-on-exit step.",
    False,
)
print("done")

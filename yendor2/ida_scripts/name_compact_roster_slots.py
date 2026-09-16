"""
Names sub_2BF3C, called from ShowWorldMap right after a roster
dismiss/toggle action: a cascade-compact for the party roster slots.

If g_partySlotAssignment (the first of the 4 confirmed active slots,
0x95EB) and all 3 "reserve" globals (word_36E4D/36E4F/36E51 -- a roster
beyond the 4 active slots, not otherwise documented) are all empty,
does nothing. Otherwise sets word_328C4 bit 0x200 (a "roster changed"
flag) and cascades non-empty entries down to fill gaps: slot 1 from
slots 2-4 (0x95ED/0x95EF/0x95F1) if empty, slot 2 from slots 3-4 if
empty, and reserve slot 1 (word_36E4D) from reserve slot 3
(word_36E51) if reserve slot 2 is empty. Confirms the roster extends
beyond the 4 active slots into at least 3 more reserve slots.
-> CompactPartyRosterSlots

Run via:
    .\run_ida_script.ps1 name_compact_roster_slots.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2BF3C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "CompactPartyRosterSlots", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'CompactPartyRosterSlots': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Cascades non-empty roster entries down to fill gaps across the 4 "
    "active slots (g_partySlotAssignment=0x95EB, plus 0x95ED/0x95EF/"
    "0x95F1) and 3 reserve slots (word_36E4D/36E4F/36E51, not otherwise "
    "documented), setting word_328C4 bit 0x200 if anything changed. "
    "Called from ShowWorldMap after a roster dismiss/toggle.",
    False,
)

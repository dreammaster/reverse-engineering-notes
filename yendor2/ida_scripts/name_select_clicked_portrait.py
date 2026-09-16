"""
Names sub_1930E, high confidence: called (twice) from the still-unnamed
sub_1869D. Hit-tests region table 0x6304 for one of 4 portrait slots
(ax=1-4), each gated on visibility flags in word_328C6 (bits 0x4000/
0x2000/0x1000/0x800), sets the portrait draw position (word_328BC/
word_328C0 -- x=8/0x40/0x78/0xB0, same y=8, four evenly-spaced
portraits) and word_32924 to the matching g_partySlotAssignment entry
(0x95EB/0x95ED/0x95EF/0x95F1 -- confirms that table's exact base
address and 4-slot layout, previously only described generically).
Resolves the slot's character id via SelectPartyRecordById and returns
errorCode=0 on success, 1 if the click missed or the slot is
empty/hidden. -> SelectClickedRosterPortrait

Run via:
    .\run_ida_script.ps1 name_select_clicked_portrait.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1930E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectClickedRosterPortrait", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectClickedRosterPortrait': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Hit-tests region table 0x6304 for one of 4 portrait slots, each "
    "gated on a word_328C6 visibility bit; sets the draw position and "
    "word_32924 to the matching g_partySlotAssignment entry (0x95EB/"
    "0x95ED/0x95EF/0x95F1), then resolves it via SelectPartyRecordById. "
    "errorCode=0 on success, 1 on a miss/empty/hidden slot. Called from "
    "sub_1869D.",
    False,
)

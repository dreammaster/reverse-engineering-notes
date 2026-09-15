"""
Names sub_19957, called from sub_193BE (itself from UseItemType_400)
and sub_19553 (the F1-F4 / clicked-portrait party-member selection
handler, called from `start`'s main loop and HandleDungeonInput).

Given a party record pointer (word_32924), matches it against
g_partySlotAssignment's 4 entries (0x95EB/0x95ED/0x95EF/0x95F1) to
pick a slot number (1-4), temporarily fakes that digit into
byte_2E400 (the "pressed key" byte) and calls sub_25B34 -- the same
routine the main input loop's '1'-'4' keys reach, i.e. this performs
that party member's panel *selection* as a side effect -- then draws
a status row: portrait icon, name (sub_19768/writeString), then two
formatted fields read from the character record at +0x16 and +0x18 --
already confirmed elsewhere (CheckForLevelUp) as **level** and
**packed-BCD XP** respectively. Restores byte_2E400 before returning.

Reads as "select and draw this party member's status-bar row (icon,
name, level, XP)". -> SelectAndDrawPartyStatusRow

Run via:
    .\run_ida_script.ps1 name_select_and_draw_party_row.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19957
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "SelectAndDrawPartyStatusRow", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'SelectAndDrawPartyStatusRow': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Given a party record (word_32924), matches it to a slot in "
    "g_partySlotAssignment, fakes that digit ('1'-'4') into byte_2E400 "
    "and calls sub_25B34 (the same panel-select path the main loop's "
    "1-4 keys use), then draws the status row: portrait icon, name, "
    "level (+0x16), and packed-BCD XP (+0x18). Called from sub_193BE "
    "(via UseItemType_400) and sub_19553 (the F1-F4/click party-member "
    "selection handler).",
    False,
)

"""
Names sub_26C9E and sub_26CFB, called from HandleDungeonInput: draws
the 4-icon party status row shown during dungeon exploration.

sub_26C9E (0x26C9E): iterates the 4 g_partySlotAssignment entries
(0x95EB/0x95ED/0x95EF/0x95F1 -- the same base confirmed by
SelectClickedRosterPortrait) at 4 fixed x positions (8/0x42/0x7C/0xB6,
same y), picture category 0x70 (the same directory ShowWorldMap's
DrawPartyRosterEntry uses), calling sub_26CFB once per slot.
-> DrawPartyStatusIconRow

sub_26CFB (0x26CFB): per-slot worker. Draws the character's icon
([+0x12], the same field DrawPartyRosterEntry uses) via DrawPicture.
If the record is incapacitated (+0x1C bits 0x1C40 -- the same bits
CheckPartyWipeAndReinitLevel checks) OR a new flag (+0x15E bit 0x8000,
not otherwise documented -- plausibly a second "needs attention"
condition, e.g. low HP, distinct from the ailment bits), draws an
overlay icon (_val38) on top with a transparent background. If this
slot is the currently-selected one (word_32924), draws a further
overlay (picture id 0) -- a selection highlight. -> DrawPartyStatusIcon

Run via:
    .\run_ida_script.ps1 name_party_status_icon_row.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x26C9E, "DrawPartyStatusIconRow"),
    (0x26CFB, "DrawPartyStatusIcon"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x26C9E,
    "Draws the 4-icon party status row (dungeon screen) by calling "
    "DrawPartyStatusIcon once per g_partySlotAssignment slot "
    "(0x95EB/0x95ED/0x95EF/0x95F1) at 4 fixed x positions. Called from "
    "HandleDungeonInput.",
    False,
)
ida_bytes.set_cmt(
    0x26CFB,
    "Draws one party-status icon: the character's icon ([+0x12]), an "
    "overlay (_val38) if incapacitated (+0x1C bits 0x1C40, matching "
    "CheckPartyWipeAndReinitLevel) or a new flag (+0x15E bit 0x8000, "
    "not otherwise documented), and a selection-highlight overlay if "
    "this is the currently-selected slot (word_32924). Called from "
    "DrawPartyStatusIconRow.",
    False,
)

"""
Names sub_22445, called from the main input loop sub_1869D (multiple
sites): draws one party member's full status panel.

Given a slot pointer (bx, one of g_partySlotAssignment's 4 entries),
bails if empty. Otherwise looks up a per-slot UI-state record (table
0x95F3, 0x1F4-byte stride, indexed by the record id) and a layout
table (0x61C2, indexed by slot number) for screen positions. Draws:
the character's portrait icon ([+0x12]), an unconscious/dead overlay
when status flags indicate it, three DrawStatBar gauges -- HP
([+0x52]/[+0x92], zeroed if a status flag is set), MP
([+0x54]/[+0x94]), and a third stat ([+0x118]/[+0x56], not otherwise
identified) -- an ability-readiness icon (variant selected by [+0xB4],
the "learned abilities" bitmask), and level-up/training text (keyed
off [+0x1C] bit 0x40 and [+0x1E], the same pending-level-up fields
RestCharacter/ShowLevelUpMessage use).

-> DrawPartyMemberStatusPanel

Run via:
    .\run_ida_script.ps1 name_draw_status_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x22445
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawPartyMemberStatusPanel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawPartyMemberStatusPanel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws one party member's full status panel: portrait, unconscious/"
    "dead overlay, three DrawStatBar gauges (HP [+0x52]/[+0x92], MP "
    "[+0x54]/[+0x94], a third stat [+0x118]/[+0x56] not identified), "
    "an ability-readiness icon ([+0xB4]), and level-up/training text "
    "([+0x1C] bit 0x40, [+0x1E]). Called from the main input loop "
    "sub_1869D.",
    False,
)

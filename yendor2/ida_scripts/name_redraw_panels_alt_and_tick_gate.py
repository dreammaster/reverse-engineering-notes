"""
Names two more small helpers.

sub_222F8, called from HandleMovementInput and InitGame: unconditionally
calls DrawPartyMemberStatusPanel for all 4 g_partySlotAssignment slots
(0x95EB/0x95ED/0x95EF/0x95F1) via far calls -- the same conceptual
role as the already-named RedrawAllPartyStatusPanels (which instead
loops with an occupied-slot check and stops at the first empty slot,
via near calls), but a different, non-identical implementation in a
different overlay segment. -> RedrawAllPartyStatusPanelsAlt

sub_1FC3F, called from ApplyMapTriggerEffect and
RestPartyAndAdvanceClock: gated on word_3295A bit 0x800; if set,
resets cs:word_1F984 to 0x270F (9999) and calls the already-named
TickWorldAilments. Otherwise a no-op. -> MaybeForceTickWorldAilments

Run via:
    .\run_ida_script.ps1 name_redraw_panels_alt_and_tick_gate.py
"""
import idc
import ida_name
import ida_bytes

names = {
    0x222F8: "RedrawAllPartyStatusPanelsAlt",
    0x1FC3F: "MaybeForceTickWorldAilments",
}

for ea, name in names.items():
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x222F8,
    "Unconditionally redraws all 4 g_partySlotAssignment status "
    "panels via far calls to DrawPartyMemberStatusPanel -- a "
    "different-segment, non-identical counterpart to "
    "RedrawAllPartyStatusPanels. Called from HandleMovementInput and "
    "InitGame.",
    False,
)
ida_bytes.set_cmt(
    0x1FC3F,
    "If word_3295A bit 0x800 is set, resets cs:word_1F984 to 0x270F "
    "(9999) and calls TickWorldAilments; no-op otherwise. Called from "
    "ApplyMapTriggerEffect and RestPartyAndAdvanceClock.",
    False,
)

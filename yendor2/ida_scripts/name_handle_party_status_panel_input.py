"""
Names sub_25B34, called from `start` and HandleDungeonInput -- the
"party-member panel-select routine" already referenced informally in
file-formats.md (RunAlchemyScreen "reuses the party-member panel-
select routine", SelectAndDrawPartyStatusRow "fakes that digit as a
keypress to reuse the main loop's existing panel-select routine").

Two input modes:
1. Mouse click (byte_2E400==0): hit-tests the cursor against table
   0x61C2 covering all 4 party members' status-panel sub-regions
   (portrait/stat-bar/affliction-icon areas etc, 7 sub-regions per
   member). Maps the hit index to a party slot (g_partySlotAssignment/
   word_36E4D/word_36E4F/word_36E51) and a normalized sub-region
   index (0-6), then draws the matching detail overlay:
   DrawAfflictionsList (0/default), ShowLevelUpMessage (3),
   DrawCharacterProtectionsList (4), DrawAbilityReadinessList (5),
   DrawThreeStatBars (6).
2. Direct '1'-'4' keypress: selects that party slot directly and
   unconditionally draws DrawThreeStatBars for it -- confirming the
   SelectAndDrawPartyStatusRow doc's description exactly.
-> HandlePartyStatusPanelInput

Run via:
    .\run_ida_script.ps1 name_handle_party_status_panel_input.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x25B34
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandlePartyStatusPanelInput", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandlePartyStatusPanelInput': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "The party-member panel-select routine: on a mouse click, "
    "hit-tests against table 0x61C2 to pick a party slot + sub-"
    "region and draws the matching detail overlay "
    "(DrawAfflictionsList/ShowLevelUpMessage/"
    "DrawCharacterProtectionsList/DrawAbilityReadinessList/"
    "DrawThreeStatBars); on a direct '1'-'4' keypress, selects that "
    "party slot and always draws DrawThreeStatBars. Called from "
    "`start` and HandleDungeonInput.",
    False,
)

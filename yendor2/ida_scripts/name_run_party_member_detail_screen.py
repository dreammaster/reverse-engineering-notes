"""
Names sub_19553, called from `start` (as the fallback when a click
isn't on the status icon bar / compass HUD) and HandleDungeonInput --
an interactive party-member detail screen, entered by an F1-F4
keypress or a portrait click (hit-test table 0x61C2, the same table
HandlePartyStatusPanelInput/HandleItemDropOnPartyPortrait use).

Loads the selected party slot (table 0x95EB, the confirmed
g_partySlotAssignment) and enters a polling loop drawing
SelectAndDrawPartyStatusRow + DrawTrainingScreenStatSheet +
DrawPartyStatusIconRow (the same trio RunItemServiceRecipientLoop
uses). Within the loop:
- F1-F4 (errorCode 2) switches which of the 4 active party members
  is shown.
- A mouse click (errorCode 3) hit-tests a second region table
  (0x632E): index 1 exits; indices 2-6 each map to one of 5
  two-byte slots at 0x94A3/94A5/94A7/94A9/94AB and toggle the
  currently-viewed character's id into/out of that slot (rejecting
  an incapacitated character). A miss instead re-hit-tests the
  portrait table (0x61C2) to let the player switch which party
  member is shown by clicking instead of pressing F1-F4.
- ESC or 'C' (errorCode 1) exits.
Exit redraws the status icon bar and material HUD.

The exact narrative purpose of the 5-slot toggle (0x94A3) isn't
confirmed -- a pre-existing comment on a separate, still-untraced
roster screen (drawing all 9 `g_partyRecords` slots, digit keys 1-9)
notes that dismissing a character from the active party "removes the
slot's index from two small lookup tables (0x95EB/0x94A3)", so
0x94A3 is plausibly some kind of secondary/reserve roster tracking,
but that link isn't independently confirmed here.
-> RunPartyMemberDetailScreen

Run via:
    .\run_ida_script.ps1 name_run_party_member_detail_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x19553
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunPartyMemberDetailScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunPartyMemberDetailScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Interactive party-member detail screen (F1-F4 or portrait click "
    "to enter): draws the training-style stat sheet, lets the player "
    "switch which of the 4 active members is shown, and toggles the "
    "viewed character into/out of one of 5 slots at "
    "0x94A3/94A5/94A7/94A9/94AB (exact purpose unconfirmed) via a "
    "second hit-test region. Called from `start` and "
    "HandleDungeonInput.",
    False,
)

"""
Names sub_1DCE0, called directly from `start`'s top-level dispatch --
the Alchemy screen's own interactive loop (534 lines, not fully
traced in detail). Repeatedly redraws via DrawAlchemyStatusPanel
(4 call sites) after various sub-actions, polls keyboard input,
hit-tests region tables (multiple) so the player can click on-screen
elements, calls sub_25B34 (the same party-member panel-select routine
used elsewhere) twice, shows a ShowConfirmPrompt at one point
(plausibly confirming an ore conversion), and calls
ApplyMapTriggerEffect near an exit path (returning to the dungeon).

Given it's reached directly from `start` and is entirely built around
DrawAlchemyStatusPanel (name/MP/ore-counter panel) plus confirm/select
flows, this is confidently the Alchemy screen's driver loop, even
though its many internal helper calls (sub_1E473, sub_1E1A7, sub_1E522,
sub_1E3AF, sub_1E356, sub_1E4D6, sub_1E4AA, sub_1E4FA, sub_1E61B,
sub_1D198, etc.) aren't individually traced yet.

-> RunAlchemyScreen

Run via:
    .\run_ida_script.ps1 name_run_alchemy_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1DCE0
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunAlchemyScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunAlchemyScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Alchemy screen driver loop, reached directly from `start`. "
    "Redraws via DrawAlchemyStatusPanel after various sub-actions, "
    "polls input, hit-tests region tables for clickable elements, uses "
    "sub_25B34 for party-member selection, shows ShowConfirmPrompt "
    "(plausibly for an ore conversion), and calls "
    "ApplyMapTriggerEffect on an exit path back to the dungeon. Many "
    "internal helper calls not individually traced yet.",
    False,
)

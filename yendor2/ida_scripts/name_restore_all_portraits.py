"""
Names sub_18F6C, called from the still-untraced sub_1869D: restores
the cursor background, then all 4 portrait slots (x=8/0x40/0x78/0xB0,
g_partySlotAssignment 0x95EB/0x95ED/0x95EF/0x95F1 -- the same 4
positions SelectClickedRosterPortrait/ClearPortraitPanelAreas use) via
RestorePortraitAreaAtPosition, clears the matching 4 dirty-flag bits in
word_328C6 (0x800/0x1000/0x2000/0x4000, the same bits
SelectClickedRosterPortrait tests for visibility), and finishes with
ClearStatusPanelIfDirty. A simpler sibling of RefreshPartyPortraits
(no shop-hint text) -- a batch "restore all 4 portraits" step.
-> RestoreAllPortraitsFromEMS

Run via:
    .\run_ida_script.ps1 name_restore_all_portraits.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x18F6C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RestoreAllPortraitsFromEMS", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RestoreAllPortraitsFromEMS': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Restores all 4 portrait slots via RestorePortraitAreaAtPosition, "
    "clears their word_328C6 dirty bits, and calls "
    "ClearStatusPanelIfDirty. A simpler sibling of RefreshPartyPortraits "
    "(no shop-hint text). Called from sub_1869D.",
    False,
)

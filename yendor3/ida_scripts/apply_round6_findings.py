"""
Round 6: full read of the address BinDiff matched to
DispatchItemAbilityCommand (0.60 similarity, deferred in round 5).
Confirms it is NOT that function -- it's an entirely different,
genuinely new dispatcher. See docs23/engine-diffs.md for the full
writeup and a correction to round 5's NUORE-removal hypothesis (one of
its three data points depended on this being the same function, which
it isn't).

sub_1B490 -> HandleSpecialQuestCommand: dispatches on a 3-way key
(compared against 0x176/0x17E/0x277) to:
  - 0x176: pass straight through to UseAbilityOnTarget (an existing,
    already-named function).
  - 0x17E: wait for a target click, then check the clicked target's
    identity (id 0x160, subtype 'C', a flag bit) -- on a match, sets
    global flag 0x91 and shows an ability-description text.
  - 0x277 (the interesting one): wait for a target click and check its
    identity (id 0xF6, subtype 0x1E, a flag bit); if it doesn't match,
    or global flag 0xDF isn't set, show a rejection message. Otherwise
    checks IsItemRangeAvailable for 5 specific item ids (0xD6/0x12C/
    0x20B/0x224/0x276) -- if all 5 are available, consumes all 5 via
    ConsumeItemChargeResource, calls UseItem(ax=0x8C), and teleports
    the party to a new fixed location (0x294, 0x84) via the standard
    RefreshDungeonMapWindow/RevealCellsAroundPlayer/RedrawDungeonScreen
    sequence. Reads as a "collect 5 quest artifacts, then unlock a new
    area" mechanic -- a genuinely new endgame/quest feature, not a
    restructured DispatchItemAbilityCommand.

Run via:
    .\run_ida_script.ps1 apply_round6_findings.py
"""
import idc
import ida_name

ea = 0x1B490
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleSpecialQuestCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleSpecialQuestCommand': {'ok' if ok else 'FAILED'}")

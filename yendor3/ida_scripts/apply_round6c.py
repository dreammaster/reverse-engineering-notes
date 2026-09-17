"""
Round 6 continued: full read of the address BinDiff matched to
CastSpell (0.36 similarity, deferred in round 5). Confirms it is NOT
CastSpell -- a third bad-match correction this round (alongside
DispatchItemAbilityCommand). See docs23/engine-diffs.md.

Strong evidence it's not CastSpell: yendor2's CastSpell is called from
HandleGameCommand's action-id dispatch; this address is called
directly from `start` (start+42C), an entirely different call site
shape. Reading it confirms: it dispatches on a record field ([si+4],
1, 2, ...) -- each branch gates on a distinct one-time global flag
(5, 0x1B, ...), and on first trigger: sets the flag, plays a sound,
teleports the party to a new fixed location via the standard
RefreshDungeonMapWindow/RevealCellsAroundPlayer/RedrawDungeonScreen
sequence, and shows a message. Reads as a shared "resolve one
scripted one-time story/world-unlock event" handler, structurally a
sibling to the already-known ApplyMapTriggerEffect but for one-time
story beats rather than repeatable map triggers.

The real yendor2-equivalent CastSpell in yendor3 is still unidentified
-- presumably hiding among the remaining low-confidence tier or
unmatched entirely.

Run via:
    .\run_ida_script.ps1 apply_round6c.py
"""
import idc
import ida_name

ea = 0x1B7A8
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "HandleScriptedStoryEventTrigger", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'HandleScriptedStoryEventTrigger': {'ok' if ok else 'FAILED'}")

"""
Traced sub_1C123, UseItem's handler for word_2E410 bit 0x4000. Its
bit-2 branch (material-gated, same 0x94B3-vs-threshold pattern as the
other handlers) is a clear level-up/training mechanic: increments the
current party member's own [+0x16] field (capped at 0x5A/90 -- the
same field FailsSavingThrow reads as a level/skill stat, 2 rounds
ago), then recalculates derived stats from it -- max HP
([+0x92], via sub_25A66(bx=0x1E, ax=[+0x80])) with a full heal to
match, and max MP ([+0x86]-derived, via sub_1AA53/sub_25A66(bx=0xD),
capped at 15) -- before branching further on [+0xE] (a mod-20-reduced
value compared against several small constants: 4/5/6/8) to select a
class/race-specific growth path, not fully traced.

That [+0xE] usage is worth flagging: it doesn't look like the earlier
"time-of-day-like value" guess from RestCharacter fits here (this is a
per-character constant selecting a permanent stat-growth table, not
something that changes with in-game time) -- plausibly a class or race
id instead. Not corrected in file-formats.md yet since RestCharacter's
own use of it wasn't re-examined this round; flagged as a lead.

-> UseTrainingItem

Run via:
    .\run_ida_script.ps1 name_use_training_item.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1C123
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "UseTrainingItem", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'UseTrainingItem': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "UseItem's handler for word_2E410 bit 0x4000. Bit-2 branch: pays "
    "a BCD material cost (0x94B3 vs threshold 0x512A), then "
    "increments word_328D4's [+0x16] (level/skill stat, capped at "
    "0x5A) and recalculates max HP/MP from it -- a level-up/training "
    "mechanic. Branches further on [+0xE] (compared against small "
    "constants after mod-20 reduction) to select a class/race-"
    "specific growth path -- [+0xE] plausibly a class/race id rather "
    "than the earlier 'time-of-day-like' guess from RestCharacter, "
    "not confirmed either way.",
    False,
)

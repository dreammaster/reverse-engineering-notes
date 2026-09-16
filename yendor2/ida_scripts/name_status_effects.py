"""
Names sub_208CA, moderate-high confidence: manages 3 timed-effect
duration counters (word_36C85/36C89/36C8B, selected by word_32974 ==
9/0xF/0xC respectively) -- decrements the relevant one each call, and
when it hits 0, clears the corresponding active-flag bit in word_36C79.
Fits the manual's affliction list (Diseased/Poisoned/Stoned/Frozen/
Paralyzed/Cursed/Hexed/Jinxed) -- plausibly the subset that are timed
rather than permanent-until-cured. Also plays a sound cue
(PlaySoundEffect-style call, ax=9) and decrements an outer per-tick
counter ([si]) each time. Called both directly from HandleGameCommand
and from sub_1A5A6. -> TickStatusEffects

Run via:
    .\run_ida_script.ps1 name_status_effects.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x208CA
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "TickStatusEffects", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'TickStatusEffects': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Manages 3 timed-effect duration counters (word_36C85/36C89/36C8B, "
    "selected by word_32974==9/0xF/0xC), decrementing the relevant one "
    "and clearing its active flag (word_36C79) when it hits 0. "
    "Plausibly a subset of the manual's afflictions (Diseased/Poisoned/"
    "Stoned/Frozen/Paralyzed/Cursed/Hexed/Jinxed) that are timed rather "
    "than permanent-until-cured.",
    False,
)

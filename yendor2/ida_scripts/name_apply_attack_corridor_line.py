"""
Names sub_2D470, called 3 times in a row from unnamed sub_2C0FE (each
call preceded by `pop word_3292C`, loading one of a 3-entry table of
starting viewport-row values set up just before) -- a sibling of the
already-named `ApplyDamageAlongCorridorLine`, but driving the *full*
resistance-aware attack pipeline instead of straight damage.

Loops 3 times over consecutive viewport rows starting at word_3292C
(incrementing it after each iteration, so one call covers rows N,
N+1, N+2): calls `GetMonsterAtViewportRow` (which itself indexes by
word_3292C) to find a monster at that row; if found, calls
`ApplyAttackToTarget` against it. If the attack left any damage or
status pending (`word_2E49C`/`word_2E49A`, which `ApplyAttackToTarget`
does not zero after committing), also calls a still-unnamed sibling,
`sub_2D428`, which has a similar-shaped but not identical commit
sequence (a proper bit-by-bit compounding resistance halving loop,
vs. `ApplyTargetResistancesToAttack`'s single first-match halving) --
its exact relationship to `ApplyAttackToTarget`'s own commit (i.e.
whether it's re-applying to the same target, a stacking/splash
follow-up, or something else) isn't resolved, so it's deliberately
left unnamed this round.

-> ApplyAttackAlongCorridorLine

Run via:
    .\run_ida_script.ps1 name_apply_attack_corridor_line.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D470
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ApplyAttackAlongCorridorLine", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ApplyAttackAlongCorridorLine': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Sibling of ApplyDamageAlongCorridorLine using the full "
    "resistance-aware pipeline: for 3 consecutive viewport rows "
    "starting at word_3292C (incrementing it each iteration), finds "
    "a monster via GetMonsterAtViewportRow and calls "
    "ApplyAttackToTarget against it, then conditionally calls "
    "still-unnamed sub_2D428 if any damage/status is pending. Called "
    "3x in a row from sub_2C0FE, once per starting row of a 3-row "
    "band.",
    False,
)

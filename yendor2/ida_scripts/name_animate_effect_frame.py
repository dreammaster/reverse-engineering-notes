"""
Names sub_2D3DC, called from sub_2C0FE (a large unnamed dispatcher
also behind ApplyDamageToMapMonster, GetMonsterAtViewportRow, and
ScrollCorridorBackgroundFromEMS): structurally the same shape as
AnimateProjectileStep -- draws the current sprite via
DrawViewportSprite (word_328C6 bit 1, a different layer flag than
AnimateProjectileStep's), redraws the cursor, restores the background
via RestoreCorridorBackgroundFromEMS, waits 5 ticks (vs.
AnimateProjectileStep's 2). One animation frame for some other
in-viewport effect/sprite sequence driven by sub_2C0FE.

-> AnimateEffectFrame

Run via:
    .\run_ida_script.ps1 name_animate_effect_frame.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2D3DC
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "AnimateEffectFrame", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'AnimateEffectFrame': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "One animation frame (same shape as AnimateProjectileStep, but "
    "word_328C6 bit 1 and a 5-tick wait): draws via DrawViewportSprite, "
    "redraws cursor, restores background via "
    "RestoreCorridorBackgroundFromEMS. Called from sub_2C0FE (a large "
    "unnamed dispatcher).",
    False,
)

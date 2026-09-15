"""
Named sub_175F4: a small HUD element, gated on word_328C6 bit 0x80,
that draws a fixed label plus the current value of material counter
0x94B3 (the same counter consumed by CastSpell's alchemy branch,
UseHealingItem, UseTrainingItem, and UseAbilityScroll) via
FormatAndDrawBCD4. Only traced caller is sub_1732B (itself called from
sub_178A6, part of the UseAbilityOnTarget/0xDFBB discovery-mechanic
area from early in the session) -- not enough context yet to say
definitively which screen(s) show this HUD element, so named on its
own confirmed behavior only.

-> ShowMaterialCounterHud

Run via:
    .\run_ida_script.ps1 name_material_hud.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x175F4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowMaterialCounterHud", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowMaterialCounterHud': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Gated on word_328C6 bit 0x80: draws a fixed label (0x7FC4) plus "
    "the current value of material counter 0x94B3 via "
    "FormatAndDrawBCD4 -- a small HUD element. Only traced caller is "
    "sub_1732B.",
    False,
)

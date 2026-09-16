"""
Names sub_2714A, called from `start`'s main status-panel redraw path
(right after ShowResourceDepletedOverlay, in the branch taken when
word_36C7F bit 0x1000 is clear -- the alternative to calling
sub_271DC) and from sub_271DC itself.

Draws 3 labeled rows at the status panel position (0xF0,0x60,
matching RedrawItemDescriptionAndMaterials's position for the same
panel area): "GOLD COINS:" + FormatAndDrawBCD4(0x94B3, confirmed
g_partyGold), "MAGIC ORE: " + FormatAndDrawBCD4(0x94B7), "NUORE: " +
FormatAndDrawBCD4(0x94BB) -- the same 3 confirmed global BCD4
material counters documented in file-formats.md's "Global material
counters" section. This is the detailed, fully-labeled resource
panel shown in the status panel area, distinct from the compact
icon-based ShowMaterialCounterHud (which only shows gold, in shop
screens). -> DrawResourceCounterPanel

Run via:
    .\run_ida_script.ps1 name_draw_resource_counter_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2714A
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawResourceCounterPanel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawResourceCounterPanel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws the status panel's fully-labeled resource readout at "
    "(0xF0,0x60): 'GOLD COINS:' + BCD4 0x94B3 (g_partyGold), "
    "'MAGIC ORE: ' + BCD4 0x94B7, 'NUORE: ' + BCD4 0x94BB. Called "
    "from `start`'s status-panel redraw path (after "
    "ShowResourceDepletedOverlay) and from sub_271DC.",
    False,
)

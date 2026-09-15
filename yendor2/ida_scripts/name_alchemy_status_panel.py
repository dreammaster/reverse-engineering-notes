"""
Names sub_1E546, called from sub_1DCE0 (twice, not yet traced/named).
Draws a status panel: character name (word_328D4+0), a "MAGIC:"
current/max bar (dumped msg 0x7B15 confirms the label; reads
[+0x54]/[+0x94] via sub_1E2E5) -- this CONFIRMS +0x54/+0x94 as the
party record's MP current/max pair (previously "one of two stat
pairs, order not confirmed" between HP/MP in RestCharacter's
documentation; +0x52/+0x92 is therefore the other, HP) -- then the two
alchemy ore counters with labels dumped as "MAGIC ORE: " (0x94B7) and
"NUORE: " (0x94BB). This is the Alchemy screen's character/resource
panel, pairing with CastSpell's 0x1C ability that converts between
these same two counters.

-> DrawAlchemyStatusPanel

Run via:
    .\run_ida_script.ps1 name_alchemy_status_panel.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1E546
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawAlchemyStatusPanel", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawAlchemyStatusPanel': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Alchemy screen status panel (called from sub_1DCE0, unnamed): "
    "character name, a 'MAGIC:' current/max bar ([+0x54]/[+0x94] -- "
    "confirms these are MP current/max, so +0x52/+0x92 is HP), then "
    "'MAGIC ORE: ' (0x94B7) and 'NUORE: ' (0x94BB) counter readouts. "
    "Pairs with CastSpell's 0x1C ability, which converts between these "
    "two ore counters.",
    False,
)

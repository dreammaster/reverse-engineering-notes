"""
Names sub_1B96F, called from UseHealingItem (5 sites) and
UseItemType_400: computes and displays a temple/healer-style paid
service cost. Confirmed via message dump:

  bx (per call site) = "TO REPLENISH YOUR HEALTH POINTS." /
    "TO REMOVE YOUR CONDITIONS." / "TO RETURN YOU TO LIFE." /
    "TO COMPLETELY RESTORE YOU." -- the specific service being paid for
  msg 0x805F = "IT WILL COST ", msg 0x806D = " GOLD",
  msg 0x8073 = "IS THAT PRICE AGREEABLE?"

Mechanism: multiplies a base per-unit cost (ax, set per call site --
e.g. 0x64=100 or 0x14=20, sometimes built up via word_2E40C condition
bits) by an item-catalog quantity field ([0xBCE+0x18]), then adds that
product into a BCD4 total (0x512A) in a loop running
[word_328D4+0x16] times (a party-record field -- plausibly the number
of afflicted/eligible party members), formats and draws the total via
FormatAndDrawBCD4, then draws the 3-line prompt: "IT WILL COST <total>
GOLD <bx text>" / "IS THAT PRICE AGREEABLE?". Doesn't itself poll for
Y/N -- that's handled by the caller after this returns. Applies
ApplyItemEffectFlags first (the item's own effect), so the priced
service appears to be layered on top of / alongside the item's direct
effect rather than gating it.

-> ShowHealingCostPrompt

Run via:
    .\run_ida_script.ps1 name_show_healing_cost_prompt.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1B96F
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowHealingCostPrompt", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowHealingCostPrompt': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Computes and displays a temple/healer paid-service cost: total = "
    "sum over [word_328D4+0x16] iterations of (ax * [0xBCE+0x18]), "
    "shown as 'IT WILL COST <total> GOLD <bx-selected reason text>. "
    "IS THAT PRICE AGREEABLE?' (msgs 0x805F/0x806D/0x8073). Reason "
    "text per caller: 'TO REPLENISH YOUR HEALTH POINTS.' / 'TO REMOVE "
    "YOUR CONDITIONS.' / 'TO RETURN YOU TO LIFE.' / 'TO COMPLETELY "
    "RESTORE YOU.'. Doesn't poll Y/N itself -- caller handles that.",
    False,
)

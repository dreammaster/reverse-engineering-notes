"""
Names sub_19BE6 and sub_1AF49, found via the "ORE COSTS 10 GOLD PER
UNIT" / "ENTER QUANTITY TO BUY" / "GOLD COINS:" message cluster.

sub_19BE6 (0x19BE6, called from sub_1AF49): uses EditTextField (max 11
chars) to read a digit string, then parses it backward into a
packed-BCD4 value (2 digits/byte) at si -- a "prompt for a BCD4
quantity" helper. errorCode=1 if the parsed result is zero (empty/
invalid entry). -> PromptForBCD4Quantity

sub_1AF49 (0x1AF49, called from UseItem): shows "PRESS ESCAPE TO EXIT",
"ORE COSTS 10 GOLD PER UNIT.", the current gold balance ("GOLD COINS:",
via g_partyGold), and "ENTER QUANTITY TO BUY", then calls
PromptForBCD4Quantity and (via CompareBCD4 against the party's gold)
validates the entered quantity can be afforded -- the buy-quantity
prompt for an Ore-type item used from the inventory. -> PromptBuyOreQuantity

Run via:
    .\run_ida_script.ps1 name_prompt_buy_ore.py
"""
import idc
import ida_name
import ida_bytes

renames = [
    (0x19BE6, "PromptForBCD4Quantity"),
    (0x1AF49, "PromptBuyOreQuantity"),
]
for ea, name in renames:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    0x19BE6,
    "Reads a digit string via EditTextField (max 11 chars) and parses "
    "it backward into a packed-BCD4 value (2 digits/byte). errorCode=1 "
    "if the result is zero (empty/invalid entry). Called from "
    "PromptBuyOreQuantity.",
    False,
)
ida_bytes.set_cmt(
    0x1AF49,
    "Shows 'ORE COSTS 10 GOLD PER UNIT.', the party's gold balance, and "
    "'ENTER QUANTITY TO BUY', then reads a quantity via "
    "PromptForBCD4Quantity and validates affordability via CompareBCD4 "
    "against g_partyGold. Called from UseItem for an Ore-type item.",
    False,
)

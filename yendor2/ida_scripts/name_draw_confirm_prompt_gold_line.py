"""
Names sub_1CBC4, called from ShowHealingCostPrompt and unnamed
sub_1BBED -- draws "GOLD COINS:" (msg 0x7C55, the same label
DrawResourceCounterPanel uses) followed by the current g_partyGold
value (BCD4 0x94B3) at a fixed position (0x16,0x73) matching a
cost-confirmation dialog layout. Shows the player's current gold
balance inline in a gold-cost confirm prompt. -> DrawConfirmPromptGoldLine

Run via:
    .\run_ida_script.ps1 name_draw_confirm_prompt_gold_line.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CBC4
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DrawConfirmPromptGoldLine", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DrawConfirmPromptGoldLine': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Draws 'GOLD COINS:' + the current g_partyGold BCD4 value at "
    "(0x16,0x73) -- shows the player's gold balance inline in a "
    "cost-confirmation prompt. Called from ShowHealingCostPrompt and "
    "sub_1BBED.",
    False,
)

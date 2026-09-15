"""
Names sub_1CC2E: draws a blank-space label (msg 0x7952, clearing the
area) then the current g_partyGold value via FormatAndDrawBCD4, and
conditionally shows ShowResourceDepletedOverlay (word_36C7F bit
0x1000). Called twice from sub_1BBED (a large, not-yet-fully-traced
UseItem handler for a shop/vendor item that spends gold via
CompareBCD4/SubBCD4 against a price table at 0x512A -- left unnamed
this round; its multiple branches (single-item purchase vs a
quantity-loop branch that repeatedly adds the unit price to both
g_partyGold and a second counter 0xB30) aren't disentangled with
enough confidence yet to name safely).

sub_1CC2E itself is a small, clearly-understood "redraw the gold
readout after it changed" helper -- the same role ShowMaterialCounterHud
plays for its own screen, but at a different fixed position/color.

-> RedrawPartyGoldDisplay

Run via:
    .\run_ida_script.ps1 name_redraw_gold_display.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1CC2E
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RedrawPartyGoldDisplay", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RedrawPartyGoldDisplay': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Redraws the gold readout: blanks the area (msg 0x7952) then "
    "FormatAndDrawBCD4(g_partyGold), then conditionally "
    "ShowResourceDepletedOverlay (word_36C7F bit 0x1000). Called from "
    "sub_1BBED (an unnamed, not-yet-fully-traced UseItem shop/vendor "
    "handler) after it spends gold.",
    False,
)

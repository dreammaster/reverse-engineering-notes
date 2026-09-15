"""
Names sub_1732B, called from UseAbilityCommand (already named): a
327-line screen driver -- the umbrella shop screen. Calls sub_1869D
(the main input loop, which hosts TrySellItemForGold/
TryEnhanceItemForGold/TryRepairItemForGold via the Space-bar cluster)
directly, twice; calls sub_17032 (the mouse-click "buy" catalog
handler that reaches PayGoldAndAcquireItem); repeatedly redraws
ShowMaterialCounterHud; hit-tests several region tables for other
clickable UI; and near one exit path writes state back via
FileEntry_Write/ErrorCheck (persisting shop/game state on leaving).

Ties together the whole shop/vendor cluster identified this session
(sell, enhance, repair, buy) under one entry point reached via
UseAbilityCommand rather than UseItem. Internal structure not fully
traced -- many helper calls (sub_1B47A, sub_179AE, sub_1728A,
sub_17270, sub_1A37E, sub_18C79, sub_2738B, sub_17A21, sub_18504,
etc.) left for a future round.

-> RunShopScreen

Run via:
    .\run_ida_script.ps1 name_run_shop_screen.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x1732B
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "RunShopScreen", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'RunShopScreen': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Umbrella shop screen, reached from UseAbilityCommand. Calls "
    "sub_1869D (main input loop -- hosts the sell/enhance/repair "
    "Space-bar cluster) directly, twice; calls sub_17032 (the "
    "mouse-click 'buy' handler reaching PayGoldAndAcquireItem); "
    "redraws ShowMaterialCounterHud repeatedly; hit-tests several "
    "region tables; writes state via FileEntry_Write near an exit "
    "path. Ties the whole shop cluster together. Many internal helper "
    "calls not individually traced yet.",
    False,
)

"""
Names sub_190AF: the shared "insufficient gold" message helper called
by both TryEnhanceItemForGold and TryRepairItemForGold when
CompareBCD4(g_partyGold, cost) fails. Draws msg 0x8376, cx=3
("YOU DON'T HAVE ENOUGH GOLD!", confirmed by raw byte dump last
round). -> ShowInsufficientGoldMessage

Run via:
    .\run_ida_script.ps1 name_insufficient_gold_msg.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x190AF
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "ShowInsufficientGoldMessage", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'ShowInsufficientGoldMessage': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Shared rejection message for TryEnhanceItemForGold/"
    "TryRepairItemForGold when g_partyGold is below the action's cost: "
    "'YOU DON'T HAVE ENOUGH GOLD!' (msg 0x8376, cx=3).",
    False,
)

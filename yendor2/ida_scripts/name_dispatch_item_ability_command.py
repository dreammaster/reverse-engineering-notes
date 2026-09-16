"""
Names sub_2AE3C, called once from HandleGameCommand -- the top-level
command dispatcher on word_32974 (event/command code, 0x242-0x2C8)
tying together the already-documented "quest/relic items" cluster
(see the function's own pre-existing header comment and
file-formats.md's "Global quest/world-state flags" section):
UseAbilityOnTarget (0x242-0x245), the 4 TestGlobalFlag(0xB1)-gated
relic effects CollectNuoreCache/CollectMagicOreCache/
PartyMassHealAndOverheal/InstantKillActiveMonster (0x246-0x249),
ShowVisionAtLocation/UseLocationBoundPotion/CheckQuestItemsCompleted
(0x253/0x258/0x2C8), and a one-off forced-music-track case (0x26D).
When a 0x246-0x249 relic is used before its recharge flag (global
flag 0xB1) is set, falls through to a shared "PATIENCE IS A VIRTUE."
message (confirmed via string dump) instead of firing the effect.
-> DispatchItemAbilityCommand

Run via:
    .\run_ida_script.ps1 name_dispatch_item_ability_command.py
"""
import idc
import ida_name
import ida_bytes

ea = 0x2AE3C
old = idc.get_name(ea)
ok = ida_name.set_name(ea, "DispatchItemAbilityCommand", ida_name.SN_NOWARN | ida_name.SN_FORCE)
print(f"{ea:#x}  {old!r} -> 'DispatchItemAbilityCommand': {'ok' if ok else 'FAILED'}")

ida_bytes.set_cmt(
    ea,
    "Top-level dispatcher on word_32974 (0x242-0x2C8) for the "
    "quest/relic item cluster: UseAbilityOnTarget, the 4 recharge-"
    "gated relic effects (CollectNuoreCache/CollectMagicOreCache/"
    "PartyMassHealAndOverheal/InstantKillActiveMonster), "
    "ShowVisionAtLocation/UseLocationBoundPotion/"
    "CheckQuestItemsCompleted, and a forced-music-track one-off. "
    "Shows 'PATIENCE IS A VIRTUE.' when a relic is used before its "
    "recharge flag (global flag 0xB1) is set. Called once from "
    "HandleGameCommand.",
    False,
)

"""
Refreshes sub_2AE3C's own header comment -- the old hedge ("plausibly
the manual's 4 single-key inventory item icons... specific code<->item
mapping isn't confirmed") is now resolved for the 0x246-0x249 group.

Run via:
    .\run_ida_script.ps1 update_item_icon_dispatch_comment.py
"""
import idc
import ida_bytes

ea = idc.get_name_ea_simple("sub_2AE3C")
print(f"sub_2AE3C @ {ea:#x}")

new_cmt = (
    "Command dispatcher on word_32974 (event/command code), covering "
    "0x242-0x2C8. 0x242-0x245 (4 codes) share one handler, "
    "UseAbilityOnTarget -- a discovery mechanic (try the current "
    "command against whatever object the player is facing; the right "
    "one permanently unlocks it). 0x246-0x249 are a themed cluster of "
    "powerful, TestGlobalFlag(0xB1)-gated relic effects, all confirmed "
    "by their own message strings: CollectNuoreCache (+5,000 NUORE), "
    "CollectMagicOreCache (+5,000 MAGIC ORE), "
    "PartyMassHealAndOverheal (2x HP/MP for the whole party), "
    "InstantKillActiveMonster. 0x253/0x258/0x254-0x257/0x2C8 are a "
    "related cluster (ShowVisionAtLocation, UseLocationBoundPotion, "
    "CheckQuestItemsCompleted) -- together these look like a set of "
    "quest/relic items central to the main story, exact narrative "
    "still unidentified. 0x26D is a separate one-off (plays a forced "
    "music track). See ida_scripts/document_item_icon_dispatch.py for "
    "the original trace."
)
ida_bytes.set_cmt(ea, new_cmt, False)
print("comment updated")

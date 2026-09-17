"""
Applies yendor2 function names to yendor3.idb for the 68 BinDiff matches
with similarity >= 0.95 (the 100%-confidence exact matches were already
applied directly by Paul before this script was written). At this
similarity level the match is almost certainly the same function with
only compiler/register-allocation-level noise -- not individually
verified function-by-function, but worth a later spot-check of a
sample since "very likely" isn't "certain".

Source: BinDiff comparison of yendor3.idb against yendor2.idb,
2026-09-16, primary=yendor3 (unnamed sub_XXXXX) / secondary=yendor2
(already-named reference).

Run via:
    .\run_ida_script.ps1 apply_bindiff_high_confidence.py
"""
import idc
import ida_name

RENAMES = [
    (0x23F36, "ShowCharacterSkills"),  # sim=0.99
    (0x121EB, "RunShopScreen"),  # sim=0.99
    (0x199B4, "UseTrainingItem"),  # sim=0.99
    (0x10F9E, "ProcessMonsterAttackTurn"),  # sim=0.99
    (0x175E2, "ShowClueBookSpellDetail"),  # sim=0.99
    (0x20F9A, "ShowItemPurchaseConfirmPrompt"),  # sim=0.99
    (0x12654, "ShowLockStatus"),  # sim=0.99
    (0x25B1A, "HandlePartyStatusPanelInput"),  # sim=0.99
    (0x1EE2C, "InitGame"),  # sim=0.99
    (0x109FA, "RunDungeonGameLoop"),  # sim=0.99
    (0x1EC05, "AdvanceGameClock"),  # sim=0.99
    (0x1BB84, "PollKeyboardInput"),  # sim=0.99
    (0x25688, "StepPaletteFadeRange"),  # sim=0.99
    (0x2606B, "DrawPartyMemberPortrait"),  # sim=0.99
    (0x2DB48, "InteractWithContainer"),  # sim=0.99
    (0x25124, "InitializeNewGameWorldState"),  # sim=0.99
    (0x13763, "ApplyEffectCost"),  # sim=0.99
    (0x12FFE, "HandleMovementInput"),  # sim=0.99
    (0x1C5C8, "HighlightSelectedAbilityIcon"),  # sim=0.99
    (0x1F44C, "LoadItemCatalogRecord"),  # sim=0.99
    (0x225E6, "SpawnMonsterInFacingDirection"),  # sim=0.99
    (0x16C6B, "RunClueBookWeaponCategory"),  # sim=0.99
    (0x2A4BF, "DrawMouseCursor"),  # sim=0.99
    (0x16B74, "RunClueBookItemCategory"),  # sim=0.99
    (0x10B10, "HandleDungeonInput"),  # sim=0.99
    (0x13BA3, "RunPartyInventoryScreen"),  # sim=0.99
    (0x2DCA1, "MarkIneligiblePartyMembers"),  # sim=0.99
    (0x1AE45, "IsPositionInTriggerList"),  # sim=0.98
    (0x15DB5, "RemoveMultiStatEffect"),  # sim=0.98
    (0x28648, "TriggerSoundEvent"),  # sim=0.98
    (0x206FE, "RenderDungeonVanishingPoint"),  # sim=0.98
    (0x16DC4, "ShowPagedEntryScreen"),  # sim=0.98
    (0x15B88, "ApplyMultiStatEffectForItem"),  # sim=0.98
    (0x20AF4, "BuildMinimapTileData"),  # sim=0.98
    (0x20506, "RenderDungeonViewRow"),  # sim=0.98
    (0x12AAC, "UseItem"),  # sim=0.98
    (0x12A1C, "SellClickedCatalogItem"),  # sim=0.98
    (0x14188, "HandleItemDropOnPartyPortrait"),  # sim=0.98
    (0x13447, "HandleSpecialCellEntry"),  # sim=0.98
    (0x27112, "HandleStatusIconBarClick"),  # sim=0.98
    (0x2C4E4, "ApplyEncodedItemEffect"),  # sim=0.98
    (0x1FE74, "RefreshDungeonMapWindow"),  # sim=0.97
    (0x1D6A6, "ApplyRestEffectsToCharacter"),  # sim=0.97
    (0x26939, "GetInventorySlotPtr"),  # sim=0.97
    (0x228C2, "ProcessLevelMonsters"),  # sim=0.97
    (0x1276B, "UseAbilityCommand"),  # sim=0.97
    (0x16CEE, "RunClueBookSpellCategory"),  # sim=0.97
    (0x13737, "DeductHPClamped"),  # sim=0.97
    (0x2020B, "DrawDungeonFloorAndCeiling"),  # sim=0.97
    (0x278B0, "ComputeAmbientLightingTable"),  # sim=0.97
    (0x1731F, "ShowWeaponDetailRow"),  # sim=0.97
    (0x20605, "DrawDungeonCellWallTexture"),  # sim=0.97
    (0x294F5, "DrawRevealedCellIcon"),  # sim=0.97
    (0x1DE85, "InitializeDungeonLevel"),  # sim=0.97
    (0x14AB5, "RunPartyMemberDetailScreen"),  # sim=0.96
    (0x2C3E6, "RepairItemCommand"),  # sim=0.96
    (0x20BC8, "TryInteractAtPosition"),  # sim=0.96
    (0x17429, "ShowItemAbilityEffectInfo"),  # sim=0.96
    (0x20337, "DrawMonsterAndUpdateAttackState"),  # sim=0.96
    (0x24F87, "ApplySecondaryClassTierFlags"),  # sim=0.96
    (0x1F8B6, "DrawWallTypeLegendRow"),  # sim=0.95
    (0x1F956, "DrawFloorTypeLegendRow"),  # sim=0.95
    (0x29DC9, "DrawViewportSprite"),  # sim=0.95
    (0x2B066, "RunConversation"),  # sim=0.95
    (0x1C976, "RunAlchemyScreen"),  # sim=0.95
    (0x2ABC8, "HandleSearchCommand"),  # sim=0.95
    (0x274F4, "ConsumeItemChargeResource"),  # sim=0.95
    (0x21388, "BuildClueLocationSuffix"),  # sim=0.95
]

ok_count = 0
fail_count = 0
for ea, name in RENAMES:
    old = idc.get_name(ea)
    ok = ida_name.set_name(ea, name, ida_name.SN_NOWARN | ida_name.SN_FORCE)
    if ok:
        ok_count += 1
    else:
        fail_count += 1
    print(f"{ea:#x}  {old!r} -> {name!r}: {'ok' if ok else 'FAILED'}")

print(f"\n{ok_count} renamed, {fail_count} failed, {len(RENAMES)} total")

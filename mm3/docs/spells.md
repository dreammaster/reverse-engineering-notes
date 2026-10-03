# MM3 spells (id -> effect routine)

Ids are the index into the name table at DGROUP `56E6h` and the case number of the switch in `spellsDialog` (`4F74F`, `jpt_4F954`);
the routine is the overlay function (`ovl11`, `4C2AD`-`4DD99`) the case calls through its thunk.

| id | Spell | Routine |
|---|---|---|
| 0 | Light | `4C2FF` |
| 1 | Awaken | `4C326` |
| 2 | First Aid | `4C358` |
| 3 | Flying Fist | `4C3B4` |
| 4 | Detect Magic | `4C3DF` |
| 5 | Elemental Arrow | `4C42E` |
| 6 | Revitalize | `4C45A` |
| 7 | Cure Wounds | `4C4A2` |
| 8 | Sparks | `4C4FE` |
| 9 | Energy Blast | `4C534` |
| 10 | Sleep | `4C57D` |
| 11 | Pain | `4C5A8` |
| 12 | Create Rope | `4C5D3` |
| 13 | Toxic Cloud | `4C5EC` |
| 14 | Suppress Poison | `4C617` |
| 15 | Prot. from Elements | `4C678` |
| 16 | Turn Undead | `4C6E6` |
| 17 | Jump | `4C2AD` |
| 18 | Acid Stream | `4C7DE` |
| 19 | Suppress Disease | `4C809` |
| 20 | Silence | `4C86A` |
| 21 | Blessed | `4C895` |
| 22 | Levitate | `4C8EA` |
| 23 | Wizard Eye | `4C2AD` |
| 24 | Identify Monster | `4C917` |
| 25 | Holy Bonus | `4CA6E` |
| 26 | Power Cure | `4CAC3` |
| 27 | Nature's Cure | `4CB3D` |
| 28 | Lightning Bolt | `4CB99` |
| 29 | Immobilize | `4CBE2` |
| 30 | Heroism | `4CC0D` |
| 31 | Walk on Water | `4CC62` |
| 32 | Frost Bite | `4CC76` |
| 33 | Lloyd's Beacon | `4C2AD` |
| 34 | Power Shield | `4CE39` |
| 35 | Cure Poison | `4CE8E` |
| 36 | Fireball | `4CED6` |
| 37 | Detect Monster | `4CF1E` |
| 38 | Acid Spray | `4D128` |
| 39 | Cold Ray | `4D153` |
| 40 | Cure Disease | `4D19C` |
| 41 | Nature's Gate | `4C2AD` |
| 42 | Time Distortion | `4D269` |
| 43 | Feeble Mind | `4D2A2` |
| 44 | Deadly Swarm | `4D2CD` |
| 45 | Teleport | `4C2AD` |
| 46 | Finger of Death | `4D514` |
| 47 | Cure Paralysis | `4D53F` |
| 48 | Paralyze | `4D587` |
| 49 | Dragon Breath | `4D5B2` |
| 50 | Super Shelter | `4C2AD` |
| 51 | Fiery Flail | `4D64E` |
| 52 | Create Food | `4D679` |
| 53 | Town Portal | `4C2AD` |
| 54 | Stone to Flesh | `4D763` |
| 55 | Recharge Item | `4D7AB` |
| 56 | Fantastic Freeze | `4D7F0` |
| 57 | Duplication | `4D81B` |
| 58 | Disintegrate | `4D860` |
| 59 | Raise Dead | `4D88B` |
| 60 | Half for Me | `4D901` |
| 61 | Etherealize | `4C2AD` |
| 62 | Dancing Sword | `4DA44` |
| 63 | Prismatic Light | `4DA8D` |
| 64 | Moon Ray | `4DAC3` |
| 65 | Mass Distortion | `4DB2E` |
| 66 | Enchant Item | `4DB59` |
| 67 | Incinerate | `4DB9E` |
| 68 | Elemental Storm | `4DBC8` |
| 69 | Holy Word | `4DC10` |
| 70 | Resurrect | `4DC3B` |
| 71 | Mega Volts | `4DCC3` |
| 72 | Inferno | `4DCEE` |
| 73 | Sun Ray | `4DD18` |
| 74 | Implosion | `4DD43` |
| 75 | Star Burst | `4DD6E` |
| 76 | Divine Intervention | `4DD99` |

The spell point cost is `Spells_subSpellCost` (`4C237`); `Spell_sharedHandler` (`4C2AD`) serves the spells that need a target/direction choice outside combat; Lloyd's Beacon (`4CCA1`, `spellLloydsBeacon`) and Town Portal etc. are reached through it.

## Item spells (`castItemSpell`, `4FF60`)

The item's spell id (character record `+0EFh`+slot, 1-77) selects the same effect routines in a different order:

| item spell id | routine |
|---|---|
| 1 | `Spell_00_Light` |
| 2 | `Spell_01_Awaken` |
| 3 | `Spell_04_DetectMagic` |
| 4 | `Spell_05_ElementalArrow` |
| 5 | `Spell_02_FirstAid` |
| 6 | `Spell_03_FlyingFist` |
| 7 | `Spell_09_EnergyBlast` |
| 8 | `Spell_10_Sleep` |
| 9 | `Spell_06_Revitalize` |
| 10 | `Spell_07_CureWounds` |
| 11 | `Spell_08_Sparks` |
| 12 | `Spell_12_CreateRope` |
| 13 | `Spell_13_ToxicCloud` |
| 14 | `Spell_15_ProtFromElements` |
| 15 | `Spell_11_Pain` |
| 16 | `sub_4C711` |
| 17 | `Spell_18_AcidStream` |
| 18 | `Spell_16_TurnUndead` |
| 19 | `Spell_22_Levitate` |
| 20 | `sub_4C8FE` |
| 21 | `Spell_20_Silence` |
| 22 | `Spell_21_Blessed` |
| 23 | `Spell_24_IdentifyMonster` |
| 24 | `Spell_28_LightningBolt` |
| 25 | `Spell_25_HolyBonus` |
| 26 | `Spell_26_PowerCure` |
| 27 | `Spell_27_NatureSCure` |
| 28 | `spellLloydsBeacon` |
| 29 | `Spell_34_PowerShield` |
| 30 | `Spell_30_Heroism` |
| 31 | `Spell_29_Immobilize` |
| 32 | `Spell_31_WalkOnWater` |
| 33 | `Spell_32_FrostBite` |
| 34 | `Spell_37_DetectMonster` |
| 35 | `Spell_36_Fireball` |
| 36 | `Spell_39_ColdRay` |
| 37 | `Spell_35_CurePoison` |
| 38 | `Spell_38_AcidSpray` |
| 39 | `Spell_42_TimeDistortion` |
| 40 | `Spell_43_FeebleMind` |
| 41 | `Spell_40_CureDisease` |
| 42 | `sub_4D1E4` |
| 43 | `sub_4D2F8` |
| 44 | `Spell_46_FingerOfDeath` |
| 45 | `Spell_47_CureParalysis` |
| 46 | `Spell_48_Paralyze` |
| 47 | `Spell_44_DeadlySwarm` |
| 48 | `sub_4D608` |
| 49 | `Spell_49_DragonBreath` |
| 50 | `Spell_52_CreateFood` |
| 51 | `Spell_51_FieryFlail` |
| 52 | `Spell_55_RechargeItem` |
| 53 | `Spell_56_FantasticFreeze` |
| 54 | `sub_4D691` |
| 55 | `Spell_54_StoneToFlesh` |
| 56 | `Spell_57_Duplication` |
| 57 | `Spell_58_Disintegrate` |
| 58 | `Spell_60_HalfForMe` |
| 59 | `Spell_59_RaiseDead` |
| 60 | `sub_4C106` |
| 61 | `Spell_62_DancingSword` |
| 62 | `Spell_64_MoonRay` |
| 63 | `Spell_65_MassDistortion` |
| 64 | `Spell_63_PrismaticLight` |
| 65 | `Spell_66_EnchantItem` |
| 66 | `Spell_67_Incinerate` |
| 67 | `Spell_69_HolyWord` |
| 68 | `Spell_70_Resurrect` |
| 69 | `Spell_68_ElementalStorm` |
| 70 | `Spell_71_MegaVolts` |
| 71 | `Spell_72_Inferno` |
| 72 | `Spell_73_SunRay` |
| 73 | `Spell_74_Implosion` |
| 74 | `Spell_75_StarBurst` |
| 75 | `Spell_76_DivineIntervention` |
| 76 | `sub_3E08F` |
| 77 | `-` |

## Costs (`Spells_subSpellCost`, `4C237`)

Arguments (character, spell id). Spell point cost = word table at DGROUP `1B7Ah[id]`; a value below 1 means `|value| x character level` (e.g. Sparks costs level SP). Gem cost = word table `1C16h[id]`, taken from `Party_gems`. Returns 0 on success, 1 if the character lacks spell points, 2 if the party lacks gems (the SP are only deducted when both checks pass).

| id | spell | SP | gems | id | spell | SP | gems |
|---|---|---|---|---|---|---|---|
| 0 | Light | 1 | 0 | 1 | Awaken | 1 | 0 |
| 2 | First Aid | 1 | 0 | 3 | Flying Fist | 2 | 0 |
| 4 | Detect Magic | 1 | 0 | 5 | Elemental Arrow | 2 | 0 |
| 6 | Revitalize | 2 | 0 | 7 | Cure Wounds | 3 | 1 |
| 8 | Sparks | 1/lvl | 1 | 9 | Energy Blast | 1/lvl | 1 |
| 10 | Sleep | 3 | 1 | 11 | Pain | 4 | 0 |
| 12 | Create Rope | 3 | 0 | 13 | Toxic Cloud | 4 | 1 |
| 14 | Suppress Poison | 4 | 0 | 15 | Prot. from Elements | 1/lvl | 2 |
| 16 | Turn Undead | 5 | 2 | 17 | Jump | 4 | 0 |
| 18 | Acid Stream | 5 | 0 | 19 | Suppress Disease | 5 | 0 |
| 20 | Silence | 6 | 0 | 21 | Blessed | 2/lvl | 0 |
| 22 | Levitate | 5 | 0 | 23 | Wizard Eye | 5 | 2 |
| 24 | Identify Monster | 5 | 0 | 25 | Holy Bonus | 2/lvl | 0 |
| 26 | Power Cure | 2/lvl | 3 | 27 | Nature's Cure | 6 | 0 |
| 28 | Lightning Bolt | 2/lvl | 2 | 29 | Immobilize | 6 | 3 |
| 30 | Heroism | 2/lvl | 3 | 31 | Walk on Water | 7 | 0 |
| 32 | Frost Bite | 7 | 0 | 33 | Lloyd's Beacon | 6 | 2 |
| 34 | Power Shield | 2/lvl | 2 | 35 | Cure Poison | 8 | 0 |
| 36 | Fireball | 2/lvl | 2 | 37 | Detect Monster | 6 | 0 |
| 38 | Acid Spray | 8 | 0 | 39 | Cold Ray | 2/lvl | 4 |

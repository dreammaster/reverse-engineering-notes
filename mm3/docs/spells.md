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

| id | spell | SP | gems |
|---|---|---|---|
| 0 | Light | 1 | 0 |
| 1 | Awaken | 1 | 0 |
| 2 | First Aid | 1 | 0 |
| 3 | Flying Fist | 2 | 0 |
| 4 | Detect Magic | 1 | 0 |
| 5 | Elemental Arrow | 2 | 0 |
| 6 | Revitalize | 2 | 0 |
| 7 | Cure Wounds | 3 | 1 |
| 8 | Sparks | 1/lvl | 1 |
| 9 | Energy Blast | 1/lvl | 1 |
| 10 | Sleep | 3 | 1 |
| 11 | Pain | 4 | 0 |
| 12 | Create Rope | 3 | 0 |
| 13 | Toxic Cloud | 4 | 1 |
| 14 | Suppress Poison | 4 | 0 |
| 15 | Prot. from Elements | 1/lvl | 2 |
| 16 | Turn Undead | 5 | 2 |
| 17 | Jump | 4 | 0 |
| 18 | Acid Stream | 5 | 0 |
| 19 | Suppress Disease | 5 | 0 |
| 20 | Silence | 6 | 0 |
| 21 | Blessed | 2/lvl | 0 |
| 22 | Levitate | 5 | 0 |
| 23 | Wizard Eye | 5 | 2 |
| 24 | Identify Monster | 5 | 0 |
| 25 | Holy Bonus | 2/lvl | 0 |
| 26 | Power Cure | 2/lvl | 3 |
| 27 | Nature's Cure | 6 | 0 |
| 28 | Lightning Bolt | 2/lvl | 2 |
| 29 | Immobilize | 6 | 3 |
| 30 | Heroism | 2/lvl | 3 |
| 31 | Walk on Water | 7 | 0 |
| 32 | Frost Bite | 7 | 0 |
| 33 | Lloyd's Beacon | 6 | 2 |
| 34 | Power Shield | 2/lvl | 2 |
| 35 | Cure Poison | 8 | 0 |
| 36 | Fireball | 2/lvl | 2 |
| 37 | Detect Monster | 6 | 0 |
| 38 | Acid Spray | 8 | 0 |
| 39 | Cold Ray | 2/lvl | 4 |
| 40 | Cure Disease | 10 | 0 |
| 41 | Nature's Gate | 10 | 0 |
| 42 | Time Distortion | 8 | 3 |
| 43 | Feeble Mind | 8 | 0 |
| 44 | Deadly Swarm | 12 | 0 |
| 45 | Teleport | 10 | 0 |
| 46 | Finger of Death | 10 | 4 |
| 47 | Cure Paralysis | 12 | 0 |
| 48 | Paralyze | 15 | 4 |
| 49 | Dragon Breath | 3/lvl | 5 |
| 50 | Super Shelter | 15 | 5 |
| 51 | Fiery Flail | 25 | 5 |
| 52 | Create Food | 20 | 5 |
| 53 | Town Portal | 30 | 5 |
| 54 | Stone to Flesh | 35 | 5 |
| 55 | Recharge Item | 15 | 10 |
| 56 | Fantastic Freeze | 15 | 5 |
| 57 | Duplication | 20 | 50 |
| 58 | Disintegrate | 25 | 8 |
| 59 | Raise Dead | 50 | 10 |
| 60 | Half for Me | 40 | 10 |
| 61 | Etherealize | 30 | 8 |
| 62 | Dancing Sword | 3/lvl | 10 |
| 63 | Prismatic Light | 60 | 10 |
| 64 | Moon Ray | 60 | 10 |
| 65 | Mass Distortion | 75 | 10 |
| 66 | Enchant Item | 30 | 20 |
| 67 | Incinerate | 35 | 10 |
| 68 | Elemental Storm | 100 | 10 |
| 69 | Holy Word | 100 | 20 |
| 70 | Resurrect | 125 | 20 |
| 71 | Mega Volts | 40 | 10 |
| 72 | Inferno | 75 | 10 |
| 73 | Sun Ray | 150 | 10 |
| 74 | Implosion | 100 | 20 |
| 75 | Star Burst | 200 | 20 |
| 76 | Divine Intervention | 200 | 20 |

## Who can learn what (`trainCharacter`, `45C21`; guild purchase list `sub_45F29`)

There are 77 spells (ids 0-76), in three schools (`SCHOOL_LISTS`, byte table at DGROUP `3C36h`, 36 entries per school, with the number of spells per spell level in `SCHOOL_COUNTS`, `3C03h`, 17 per school). `char.spells[i]` (+53h) is indexed by the **position in the character's school list**, not by spell id. Class (+13h) -> school:

| class | school | full/half caster |
|---|---|---|
| Cleric (3) | cleric | full |
| Paladin (1) | cleric | half (gold price x2) |
| Sorcerer (4) | sorcerer | full |
| Archer (2) | sorcerer | half |
| Druid (8) | druid | full, spell level capped at 15 |
| Ranger (9) | druid | half |
| Knight, Robber, Ninja, Barbarian | none | - |

A character can buy the spells of spell levels 1..min(town cap, character level); the town cap (`3CA1h[Party_map]`) is 0, 3, 6, 9, 13, 17 for towns 0-5. Price of a spell in gold: `SP * 100` (or `|SP| * 500` for level-scaled spells, from the same SP table) shifted left once for half casters (`sub_45BF3`). Messages: "Come back when you're more experienced" (nothing offered yet) / "You have learned all we can teach".

**Cleric school list, by spell level:**

* level 1: Light, Awaken, First Aid, Flying Fist
* level 2: Revitalize, Cure Wounds, Sparks
* level 3: Prot. from Elements, Pain, Suppress Poison
* level 4: Suppress Disease, Turn Undead
* level 5: Silence, Blessed
* level 6: Holy Bonus, Power Cure
* level 7: Heroism, Immobilize
* level 8: Cold Ray, Cure Poison
* level 9: Acid Spray, Cure Disease
* level 10: Cure Paralysis, Paralyze
* level 11: Create Food, Fiery Flail
* level 12: Town Portal, Stone to Flesh
* level 13: Half for Me, Raise Dead
* level 14: Moon Ray, Mass Distortion
* level 15: Holy Word, Resurrect
* level 16: Sun Ray
* level 17: Divine Intervention

**Sorcerer school list, by spell level:**

* level 1: Light, Awaken, Detect Magic, Elemental Arrow
* level 2: Energy Blast, Sleep
* level 3: Create Rope, Toxic Cloud
* level 4: Jump, Acid Stream
* level 5: Levitate, Wizard Eye
* level 6: Identify Monster, Lightning Bolt
* level 7: Lloyd's Beacon, Power Shield
* level 8: Detect Monster, Fireball
* level 9: Time Distortion, Feeble Mind
* level 10: Teleport, Finger of Death
* level 11: Super Shelter, Dragon Breath
* level 12: Recharge Item, Fantastic Freeze
* level 13: Duplication, Disintegrate
* level 14: Etherealize, Dancing Sword
* level 15: Enchant Item, Incinerate
* level 16: Mega Volts, Inferno
* level 17: Implosion, Star Burst

**Druid school list, by spell level:**

* level 1: Light, Awaken, First Aid, Detect Magic
* level 2: Elemental Arrow, Revitalize
* level 3: Create Rope, Sleep
* level 4: Prot. from Elements, Suppress Poison
* level 5: Suppress Disease, Identify Monster
* level 6: Nature's Cure, Immobilize
* level 7: Walk on Water, Frost Bite
* level 8: Lightning Bolt, Acid Spray
* level 9: Cold Ray, Nature's Gate
* level 10: Fireball, Deadly Swarm
* level 11: Cure Paralysis, Paralyze
* level 12: Create Food, Stone to Flesh
* level 13: Raise Dead
* level 14: Prismatic Light
* level 15: Elemental Storm

## Attack spell parameters (by code reading)

Each attack spell routine sets three globals and calls `spellAttackAhead(animation)`: `word_340B4` = damage, `word_36CFE` = effect/damage type (numbering of `MONDMGT` for 0-6: 0 physical, 2 fire, 3 electricity, 4 cold, 5 poison, 6 energy; 7-16 are status effects), `word_36FAC` = reach (0 one monster, 1 one group, 2 all monsters; reach names inferred from the spell descriptions).

| id | spell | damage | type | reach |
|---|---|---|---|---|
| 3 | Flying Fist | 6 | 0 physical | single |
| 5 | Elemental Arrow | 8 | chosen element (dialog `element.icn`) | single |
| 8 | Sparks | 2 x level | 3 electricity | group |
| 9 | Energy Blast | rnd(2,6) x level | 6 energy | single |
| 10 | Sleep | - | 7 sleep | group |
| 11 | Pain | 8 | 0 physical | group |
| 13 | Toxic Cloud | 10 | 5 poison | group |
| 16 | Turn Undead | - | 14 (0Eh) turn undead | group |
| 18 | Acid Stream | 25 | 5 poison | single |
| 20 | Silence | - | 16 (10h) silence | group |
| 28 | Lightning Bolt | rnd(4,6) x level | 3 electricity | group |
| 29 | Immobilize | - | 8 immobilize | group |
| 32 | Frost Bite | 35 | 4 cold | single |
| 36 | Fireball | rnd(3,7) x level | 2 fire | group |
| 38 | Acid Spray | 15 | 5 poison | all |
| 39 | Cold Ray | rnd(2,4) x level | 4 cold | all |
| 43 | Feeble Mind | - | 9 feeble mind | group |
| 44 | Deadly Swarm | 40 | 0 physical | group |
| 46 | Finger of Death | - | 11 (0Bh) finger of death | group |
| 48 | Paralyze | - | 10 (0Ah) paralyse | group |
| 49 | Dragon Breath | 5 x level | chosen element | all |
| 51 | Fiery Flail | 100 | 2 fire | single |
| 56 | Fantastic Freeze | 40 | 4 cold | group |
| 58 | Disintegrate | - | 15 (0Fh) disintegrate | group |
| 62 | Dancing Sword | rnd(6,14) x level | 0 physical | group |
| 63 | Prismatic Light | 80 | random 0-6 | all |
| 64 | Moon Ray | 30 | 6 energy (also heals the party) | all |
| 65 | Mass Distortion | - | 13 (0Dh) mass distortion | group |
| 67 | Incinerate | 250 | 2 fire | single |
| 68 | Elemental Storm | 150 | chosen element | all |
| 69 | Holy Word | - | 12 (0Ch) holy word (undead) | all |
| 71 | Mega Volts | 150 | 3 electricity | group |
| 72 | Inferno | 250 | 2 fire | group |
| 73 | Sun Ray | 200 | 6 energy | all |
| 74 | Implosion | 1000 | 6 energy | single |
| 75 | Star Burst | 500 | 0 physical | all |

## Buff spells (by code reading)

All character-targeted buffs call `sub_4C027(spell id)` first (target selection with point/gem payment; `FFFFh` = cancelled), play the heal-style
effect via `healCharacterEffect`, and then store the **caster's level** (`getCurrentLevel` of the active caster) in a character byte:
Blessed -> `+102h` (`blessed`, AC bonus), Power Shield -> `+103h`, Holy Bonus -> `+104h` (damage), Heroism -> `+105h` (to-hit).
Light increments `Party_light` (one unit is used up each step on a light-burning cell, `updateLight`); Levitate / Walk on Water just set
`Party_levitate` / `Party_walkOnWater` to 1.

Healing: First Aid heals 6 hit points, Cure Wounds 15, Power Cure `rnd(2,12) x level`, through `healCharacterEffect`; the target may not be dead,
stone or eradicated (worst condition 0Dh-0Fh -> `showErrorMessage`).

Town Portal (`4D69A`): refused ("showErrorMessage") unless page header byte 18h is non-zero; shows a town list (1-5, 0 = cancel), loads the chosen town's map (`sub_281B2`) and puts the party on that page's default start cell (`setPartyStartCell`, header byte 13h).

Other effects (by code reading): Awaken clears the asleep counter (+11Bh) of the whole party; Revitalize heals nothing (heal 0) but clears Weak (+115h) of the target;
Suppress Poison reduces the poison counter (+116h) by 3 (to 1 if it was below 4; it does not cure); Protection from Elements sets the chosen element's party resistance
(`Party_fireResist` / `elecResist` / `coldResist` / `poisonResist`) to `min(2 x level + 5, 200)`; Create Food adds one food unit per party member;
Create Rope only raises the "rope available" flags (`byte_2879D`, `byte_2886E`).

Condition spells: Suppress Disease acts on +117h like Suppress Poison; Nature's Cure (target not dead/stone/eradicated) calls `healCharacterEffect(25)`; Cure Poison, Cure Disease, Cure Paralysis, Stone to Flesh each clear their condition counter and show the heal effect.
Raise Dead turns a dead character (+120h) into an unconscious one (+11Fh set, hit points 0) and costs it 1 point of the first byte of the Endurance pair (+1Ah, minimum 1); Resurrect clears Eradicated (+122h), costs the same endurance point and adds 5 years of age (`+26h`, capped at 250).

Travel spells and the page header permission bytes (`data-files.md`): Time Distortion (16h) moves the party to the page's default start cell (`setPartyStartCell`) and refreshes the view; Super Shelter (17h) runs `rest`; Nature's Gate (19h) loads another map and places the party at a fixed cell chosen from a table (the disassembly shows the coordinates read through variables IDA labelled `Party_day`, so the exact table was not traced); Etherealize (1Ah) steps the party one cell ahead through the wall after checking that the target cell exists (`mazeGetWordRel` mask 7777h != 1111h). Each shows `showErrorMessage` when its permission byte is 0.

Teleport (permission byte 14h) asks for a direction with the standard direction prompt (`sub_2824E`), then moves the party along it with repeated `mazeGetWordRel` existence tests and refreshes the page (`mazeUpdateSlot`); the exact maximum distance was not traced.
Lloyd's Beacon (byte 15h, `lloyds.icn`) lets the caster set the beacon or return to it: the position is stored in the character record (`lloydMap`, `lloydX`, `lloydY` at +77h), recall loads the stored map (`sub_281B2`) and places the party there.
`Spell_sharedHandler` (`4C2AD`) just adds the caster level to `Party_gems` (purpose not established; it may be a gem refund path for cancelled spells, which is only a guess).

Item spells: Recharge Item, Duplication and Enchant Item pay via `sub_4C027`, then open the target's inventory page with `characterInfoInventory(char, mode)` using modes 4, 5 and 6 respectively (the same page the shops open with modes 1/2; the item action itself lives in `itemsDialog`, whose price routine returns the charge count for modes 3-6).
Half for Me: the target (not the caster, not dead/stone/eradicated) is healed to full (`healCharacterEffect(max HP)`) and the caster takes `(max HP - current HP) / 2` damage of the target's deficit (`subtractHitPoints`).

Identify Monster (`4C917`, about 180 lines of UI) opens a window with the monster ahead's armour class (`Mon_ac`), attacks per turn (`Mon_numa`) and special attack (`Mon_spec`), plus its other stats; Detect Monster (`4CF1E`, about 250 lines) is presumably the monster-radar display (inferred from its size and name, not read in detail). Identify Monster calls `showErrorMessage` on its failure path.

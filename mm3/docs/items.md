# Items (work in progress)

Item names are **built from parts**, exactly like Xeen: `<attribute prefix> <element/metal material> <base item>`
(e.g. "sharp ... long sword").  The DGROUP pointer tables (named in `names/mm3.tsv`):

| Table | Content |
|---|---|
| `ITEM_ELEMENT_NAMES` (`5300h`+) | elemental adjectives: burning/fiery/... (fire), sparking/... (electric), icy/... (cold), acidic/... (poison), glowing/... (energy), mystic/... (magic) |
| metals / gems (`5300h` + 2*...) | wooden, leather, brass, bronze, iron, silver, steel, gold, platinum, glass, coral, crystal, lapis, pearl, amber, ebony, quartz, ruby, emerald, sapphire, diamond, obsidian |
| `ITEM_ATTR_NAMES` (`5382h`) | 'might strength warrior ogre giant thunder force power dragon photon' (might), 'clever mind sage ...' (intellect), ... 'pirate' |
| `ITEM_WEAPON_NAMES` (`5414h`) | 33 weapons, ids 1-33 |
| `ITEM_ARMOR_NAMES` (`5456h`) | ids 33-41 ids 34-42: body armour and shield (`subtractHitPoints` breaks 22h-29h), then helm, crown, tiara, gauntlets, ring |
| `ITEM_ACCESSORY_NAMES` (`5472h`) | boots, cloak, robes, cape, belt, broach |
| `ITEM_MISC_NAMES` (`547Eh`) | medal ... wand, whistle, potion, scroll, Torch, Rope and Hooks |
| `QUEST_ITEM_NAMES` (`54B8h`) | the story items |

A character's inventory slot is described by six parallel arrays (see `character.h`): id (`slotId`, selects the base item and
`ARMOR_STRENGTHS`; 1-based), element material (`slotElement` -> `ELEMENTAL_RESISTANCES`), metal (`slotMetal` -> `METAL_LAC`),
attribute material (`slotAttribute` -> `ATTRIBUTE_BONUSES`) and the item's spell (`slotSpell`, used by `castItemSpell`).
Prices are decoded (`itemPrice`, see `rules.md` and the table below); the weapon damage tables are in `getWeaponDamage` (`4B283`).

## Base item table (decoded from DGROUP)

Slot item ids are **1-based** (0 = empty slot); the name tables start at id 1.  `subtractHitPoints` breaks ids 34-41 (body armour).

| id | name | dice x die |
|---|---|---|
| 1 | long sword | 3d3 |
| 2 | short sword | 2d3 |
| 3 | broad sword | 3d4 |
| 4 | scimitar | 2d5 |
| 5 | cutlass | 2d4 |
| 6 | sabre | 4d2 |
| 7 | club | 1d3 |
| 8 | hand axe | 2d3 |
| 9 | katana | 4d3 |
| 10 | nunchakas | 2d3 |
| 11 | wakazashi | 3d3 |
| 12 | dagger | 2d2 |
| 13 | mace | 2d4 |
| 14 | flail | 1d10 |
| 15 | cudgel | 1d6 |
| 16 | maul | 1d8 |
| 17 | spear | 1d9 |
| 18 | bardiche | 4d4 |
| 19 | glaive | 4d3 |
| 20 | halberd | 3d6 |
| 21 | pike | 2d8 |
| 22 | flamberge | 4d5 |
| 23 | trident | 2d6 |
| 24 | staff | 2d4 |
| 25 | hammer | 2d5 |
| 26 | naginata | 5d3 |
| 27 | battle axe | 3d5 |
| 28 | grand axe | 3d6 |
| 29 | great axe | 3d7 |
| 30 | short bow | 3d2 |
| 31 | long bow | 5d2 |
| 32 | crossbow | 4d2 |
| 33 | sling | 2d2 |

Armour: 34 padded armor (AC 2), 35 leather armor (AC 3), 36 scale armor (AC 4), 37 ring mail (AC 5), 38 chain mail (AC 6), 39 splint mail (AC 7), 40 plate mail (AC 8), 41 plate armor (AC 10), 42 shield (AC 4), 43 helm (AC 2), 44 crown (AC 0), 45 tiara (AC 0), 46 gauntlets (AC 1), 47 ring (AC 0)

Metal materials 0-21 (`wooden` ... `obsidian`; index into `METAL_DAMAGE_PERCENT`, `METAL_DAMAGE`, `METAL_LAC`): to-hit % [0, -3, -4, 3, 2, 1, 2, 3, 4, 6, 0, 1, 1, 2, 2, 3, 4, 5, 6, 7, 8, 9], damage [0, -3, -6, -4, -2, 2, 4, 6, 8, 10, 0, 1, 1, 2, 2, 3, 4, 5, 12, 15, 20, 30], armour class [0, -3, 0, -2, -1, 1, 2, 4, 6, 8, 0, 1, 1, 2, 2, 3, 4, 5, 10, 12, 14, 16]

## Random item generation (`generateItem`, 4432C)

`generateItem(level 1..6, buffer, slot)` (level 0 is treated as 1) fills slot `slot` of an inventory buffer; used for the
Blacksmith's stock (`resetBlacksmithWares`) and treasure. Two d100 rolls choose the item id (stored in the id array at +DCh):

| first roll | second roll | item id (uniform in range) |
|---|---|---|
| 1-35 | 1-30 / 31-60 / 61-85 / 86-100 | 1-6 / 7-17 / 18-29 / 30-33 |
| 36-60 | 1-70 / 71-100 | 34-41 / 42 |
| 61-100 | 1-10 / -20 / -35 / -45 / -55 / -65 / -75 / -80 / -100 | 43-45 / 46 / 47 / 48 / 49-51 / 52 / 53-57 / 58-60 / 61-69 |

Then a third d100 decides how many of the four enchantment fields (metal at +B6h, element +A3h, attribute +C9h, spell +EFh)
get a value: roll <= 95 -> one, <= 99 -> two, else three (the fields are chosen at random without repeats, each value is
drawn `rnd(0, level-1)`-style from the item level); at level 6 exactly one field is set. This is by code reading; the
exact per-field value ranges were not traced.

## Complete item id list with base prices (word table at DGROUP `0B16h`, indexed by id)

| id | item | base gold | id | item | base gold |
|---|---|---|---|---|---|
| 1 | long sword | 50 | 2 | short sword | 15 |
| 3 | broad sword | 100 | 4 | scimitar | 80 |
| 5 | cutlass | 40 | 6 | sabre | 60 |
| 7 | club | 1 | 8 | hand axe | 10 |
| 9 | katana | 150 | 10 | nunchakas | 30 |
| 11 | wakazashi | 60 | 12 | dagger | 8 |
| 13 | mace | 50 | 14 | flail | 100 |
| 15 | cudgel | 15 | 16 | maul | 30 |
| 17 | spear | 15 | 18 | bardiche | 200 |
| 19 | glaive | 80 | 20 | halberd | 250 |
| 21 | pike | 150 | 22 | flamberge | 400 |
| 23 | trident | 100 | 24 | staff | 40 |
| 25 | hammer | 120 | 26 | naginata | 300 |
| 27 | battle axe | 100 | 28 | grand axe | 200 |
| 29 | great axe | 300 | 30 | short bow | 25 |
| 31 | long bow | 100 | 32 | crossbow | 50 |
| 33 | sling | 15 | 34 | padded armor | 20 |
| 35 | leather armor | 40 | 36 | scale armor | 100 |
| 37 | ring mail | 200 | 38 | chain mail | 400 |
| 39 | splint mail | 600 | 40 | plate mail | 1000 |
| 41 | plate armor | 2000 | 42 | shield | 100 |
| 43 | helm | 60 | 44 | crown | 1000 |
| 45 | tiara | 200 | 46 | gauntlets | 100 |
| 47 | ring | 100 | 48 | boots | 40 |
| 49 | cloak | 250 | 50 | robes | 150 |
| 51 | cape | 200 | 52 | belt | 100 |
| 53 | broach | 250 | 54 | medal | 100 |
| 55 | charm | 50 | 56 | cameo | 300 |
| 57 | scarab | 200 | 58 | pendant | 500 |
| 59 | necklace | 1000 | 60 | amulet | 2000 |
| 61 | rod | 50 | 62 | jewel | 1000 |
| 63 | gem | 500 | 64 | box | 10 |
| 65 | orb | 100 | 66 | horn | 20 |
| 67 | coin | 10 | 68 | wand | 50 |
| 69 | whistle | 10 | 70 | potion | 10 |
| 71 | scroll | 100 | 72 | Torch | 5 |
| 73 | Rope and Hooks | 5 | | | |

Ids 34-41 are body armour (the ones `subtractHitPoints` can break), 42 shield, 43-47 head/hand/finger gear, 48-53 boots, cloak, robes, cape, belt, broach, 54+ the magical trinkets and consumables (73 = Rope and Hooks, the item the pit events test for). Prices shown are before the metal / enchantment multipliers and the merchant divisor.

## `itemScan(char, what)` (`16E6B`): the sum of equipment bonuses

Sums over the 18 inventory slots that are in use (+7Dh != 0) and neither cursed nor broken (flags & C0h == 0):

* `what` < 0Bh, not 3: the attribute enchantment (+C9h) is mapped through `getAttributeCategory` (category > 2 is shifted up by one so that value 3 is skipped); when that equals `what` the item adds `ATTRIBUTE_BONUSES[attr]` (DGROUP `0ACDh`);
* `what` > 0Ah: the element enchantment (+A3h) maps through `getElementalCategory` + 0Bh; on a match the item adds `ELEMENTAL_RESISTANCES[element]` (`0A27h`) -- so 0Bh..10h are the six resistances used by `charSavingThrow` and `getStat`;
* `what` == 9 (armour class): every item adds `ARMOR_STRENGTHS[id]` (`0C87h`) and, if it has a metal (+B6h) and is not a weapon (codes 1, 4, 0Dh; the shield is always counted), the metal's armour bonus `METAL_LAC[metal]` (`0A9Fh`).

Other callers use `what` 7 (hit point bonus), 8 (spell point bonus), 0Ah (thievery bonus) the same way.

## Item spell effects in `itemsDialog` (modes 4-6; by code reading)

* **Recharge** (mode 4): the item (id <= 71) must carry a spell (+EFh != 0); it gains `rnd(1,6)` charges in the low six bits of its flag byte (+90h), capped at 63.  Otherwise "Spell Failed!".
* **Duplication** (mode 5): needs the character's last backpack slot (+EDh) to be empty; the item (id <= 72, metal <= 17, spell <= 55, and an element/attribute combination not in a refusal list of 11 element values) is copied into that slot with its flags reduced to the cursed/broken bits (C0h) -- the copy has **no charges**.  Otherwise "Spell Failed!".
* **Enchant** (mode 6): only for plain items (id <= 71 and element, metal, attribute and spell all 0); it rolls `rnd(1, caster level / 10 + 1)` capped at 5, calls `generateItem(that level, character, same slot)` to fill the slot with a random item of that treasure level, and then **restores the original item id** -- the item keeps its identity but receives the random element/metal/attribute/spell enchantments that generation produced (so enchanting is a gamble on up to level-5 enchantments).

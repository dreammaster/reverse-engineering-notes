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
The weapon damage tables (`getWeaponDamage`, `4B283`) and prices (`calcItemCost`) are not decoded yet.

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

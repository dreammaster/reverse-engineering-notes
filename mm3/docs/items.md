# Items (work in progress)

Item names are **built from parts**, exactly like Xeen: `<attribute prefix> <element/metal material> <base item>`
(e.g. "sharp ... long sword").  The DGROUP pointer tables (named in `names/mm3.tsv`):

| Table | Content |
|---|---|
| `ITEM_ELEMENT_NAMES` (`5300h`+) | elemental adjectives: burning/fiery/... (fire), sparking/... (electric), icy/... (cold), acidic/... (poison), glowing/... (energy), mystic/... (magic) |
| metals / gems (`5300h` + 2*...) | wooden, leather, brass, bronze, iron, silver, steel, gold, platinum, glass, coral, crystal, lapis, pearl, amber, ebony, quartz, ruby, emerald, sapphire, diamond, obsidian |
| `ITEM_ATTR_NAMES` (`5382h`) | 'might strength warrior ogre giant thunder force power dragon photon' (might), 'clever mind sage ...' (intellect), ... 'pirate' |
| `ITEM_WEAPON_NAMES` (`5414h`) | 33 weapons, ids 0-32 |
| `ITEM_ARMOR_NAMES` (`5456h`) | ids 33-41 body armour and shield (the ones `subtractHitPoints` breaks: ids 21h-29h), then helm, crown, tiara, gauntlets, ring |
| `ITEM_ACCESSORY_NAMES` (`5472h`) | boots, cloak, robes, cape, belt, broach |
| `ITEM_MISC_NAMES` (`547Eh`) | medal ... wand, whistle, potion, scroll, Torch, Rope and Hooks |
| `QUEST_ITEM_NAMES` (`54B8h`) | the story items |

A character's inventory slot is described by six parallel arrays (see `character.h`): id (`slotId`, selects the base item and
`ARMOR_STRENGTHS`), element material (`slotElement` -> `ELEMENTAL_RESISTANCES`), metal (`slotMetal` -> `METAL_LAC`),
attribute material (`slotAttribute` -> `ATTRIBUTE_BONUSES`) and the item's spell (`slotSpell`, used by `castItemSpell`).
The weapon damage tables (`getWeaponDamage`, `4B283`) and prices (`calcItemCost`) are not decoded yet.

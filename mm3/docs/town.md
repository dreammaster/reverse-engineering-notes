# Town buildings (work in progress)

The town functions are `townBank` (`4816C`), `townGuild` (`48466`), `townInn` (`486AB`), `townTavern` (`487B4`), `townTemple` (`48D4B`),
`townTraining` (`49368`), `townSmithy` (`49730`); an `If`/`DoTownEvent n` script event (`n` 0-6 as in `MAZE02.EVT`) calls them.  Each
loads its music (`bank.m`, `guild.m`, `towninn.m`, `honky.m`, ..., `grounds.m`, `dungeon.m`) and, when closed, prints
"Sorry, the X's closed! Come back ...".

## Temple (`townTemple`)

Prices depend on the character's level `L` and the town (map id 1-5, `byte_34C1D`, the five towns of Terra), with five-entry tables
in DGROUP (`TEMPLE_*`; the tables are indexed 1..5):

| Service | Cost |
|---|---|
| Heal (HP below maximum) | `10 * L + TEMPLE_HEAL_COST[town]` where the table is 0, 10, 20, 100, 500 |
| each other condition 1-12 present | + `10 * L` |
| Dead (`+120h`) | `100 * L + 50 * dead + TEMPLE_DEAD_STONE_COST[town]` (0, 50, 100, 500, 2500) |
| Stoned (`+121h`) | same formula with the stoned counter |
| Eradicated (`+122h`) | `1000 * L + 500 * eradicated + TEMPLE_ERADICATE_COST[town]` (0, 500, 1000, 5000, 25000) |
| Uncurse (a cursed condition or an item with flag `40h`) | `20 * L + TEMPLE_UNCURSE_COST[town]` (0, 100, 200, 300, 500) |
| Donate | `TEMPLE_DONATE_COST[town]` (10, 25, 50, 100, 200) |

"Heal" clears the temporary stat bonuses (`+15h`, `+17h` ... and `+22h`, `+24h`), sets HP to `getMaxHP`, and clears conditions 0-15 except
Cursed.  "Donate" sets, for every party member, a bit for this temple (bit array at `+106h`), and gives the party blessings
(`blessed`, `powerShield`, `holyBonus`, `heroism` = `+102h`..`+105h`) whose strength depends on the number of donations; an
unaffordable action just returns.  (Derived by reading the code, not tested.)  Each action also advances the clock by 5A0h = 1440
minutes (a day).

## Smithy stock (`resetBlacksmithWares`, `385xx`)

The stock is rebuilt from tables (script clears `32Ah` bytes at DGROUP `E901h` first): 6 towns x 3 groups x 9 slots.
Group 0 = weapons, group 1 = armour, group 2 = jewellery/trinkets and consumables (ids as in `items.md`).

* `1CC6h` (162 bytes): the item id of each slot; **0 = empty slot, filled with `generateItem(level)`** where the level is
  `17A8h[town]` = 1, 2, 2, 3, 4, 1.  Example first town: weapons 2 short sword, 5 cutlass, 24 staff, 30 short bow, 10 nunchakas,
  12 dagger, 15 cudgel, 8 hand axe; armour 34-36 padded/leather/scale (two each), 42 shield x2; trinkets 43 helm, 46 gauntlets, 48 boots,
  49 cloak, 72 Torch, 73 Rope and Hooks x2, 70 potion.
* `1D7Ah` (162 bytes): the metal/material byte given to the slot (0, 2, 5, 6, 7: better metals in later towns);
  `1D4Dh`/`1E01h` (9 bytes per town): attribute and spell values for the third group.
Prices come from `itemPrice` (`rules.md`).

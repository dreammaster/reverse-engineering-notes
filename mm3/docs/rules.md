# Rules and tables (verified against the code where marked)

## Character creation (`checkClasses`, `46B2D`)

A class can be chosen when the rolled/assigned stats meet its minimums (might, intellect, personality, endurance, speed, accuracy, luck):

| Class | Requirement |
|---|---|
| Knight | might >= 15 |
| Paladin | might, personality, endurance >= 13 |
| Archer | intellect, accuracy >= 13 |
| Cleric | personality >= 13 |
| Sorcerer | intellect >= 13 |
| Robber | luck >= 13 |
| Ninja | speed, accuracy >= 13 |
| Barbarian | endurance >= 15 |
| Druid | intellect, personality >= 15 |
| Ranger | intellect, personality, endurance, speed >= 12 |

## Hit points and spell points (`getMaxHP` `513F4`, `getMaxSP` `51250`; same formulas as Xeen's `Character`)

`HP = max(1, BASE_HP_BY_CLASS[class] + statBonus(endurance) + RACE_HP_BONUSES[race] + (1 if Bodybuilder)) * level + itemScan(7)`.

BASE_HP_BY_CLASS: Knight 10, Paladin 8, Archer 7, Cleric 5, Sorcerer 4, Robber 8, Ninja 7, Barbarian 12, Druid 6, Ranger 9

RACE_HP_BONUSES (human, elf, dwarf, gnome, half-orc): [0, -2, -1, 1, 2]

`SP`: only characters with the `hasSpells` flag; casting stat = intellect for Archer/Sorcerer, personality otherwise (druid/ranger: average of both); `(statBonus(stat) + 3 + RACE_SP_BONUSES + 2 if Prestidigitation/Prayer Master/Astrologer skill) * level`, halved for classes other than Sorcerer/Cleric/Druid, plus `itemScan(8)`.  RACE_SP_BONUSES words [127, 0, 0, 2, 0, 1, 1, -1, -1, -2, -2, 0].

## Stat tables

`statBonus(v)` = `STAT_BONUSES[i]` for the first `i` with `STAT_VALUES[i] > v`.

STAT_VALUES: [3, 5, 7, 9, 11, 13, 15, 17, 19, 21, 25, 30, 35, 40, 50, 75, 100, 125, 150, 175, 200, 225, 250, 65535]

STAT_BONUSES: [-5, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 20]

Age adjustments (`getStat`): age thresholds `AGE_RANGES` [1, 6, 11, 18, 36, 51, 76, 101, 201, 65535]; per-age adjustment for might/endurance/speed/accuracy [6, -1, -50, -1, -20, -1, -10, -1, 0, 0] and for intellect/personality [6, -1, -50, -1, -20, -1, -10, -1, 0, 0] (luck is not age adjusted).

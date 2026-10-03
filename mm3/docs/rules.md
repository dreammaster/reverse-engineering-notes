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

## Training grounds (`townTraining` 492B1/49368)

* `experienceToNextLevel(char)` (0 when already eligible) is shown by `trainingNeedsExperience`:
  "%s needs %lu experience for level %u." / "%s is eligible for level %d." / "%s has learned all we can teach!".
* Each town teaches up to a maximum level held in the byte table at DGROUP `43A6h`, indexed by `Party_map`
  (town number): `0, 10, 15, 20, 25, 200`. A character whose level is at or above the cap gets the "learned all" message.
* Training a level costs `level * level * 10` gold (current level, 32-bit multiply in the code), taken with the party gold
  routine; it sets experience to exactly the threshold of the new level (`nextExperienceLevel` - `getCurrentExperience`
  difference is subtracted from the stored experience at char+12Bh), increments level (char+23h) and sets
  HP/SP current values (+125h/+127h) to the new `getMaxHP/getMaxSP`.
* The first character trained in a visit advances the clock by 5A0h (1440) minutes (one day) through `addTime`;
  a per-character flag array on the stack stops the day being charged twice.
* Choosing a character: command codes C9h.. select party slot; ESC leaves. `byte_32E68` set = grounds closed.

## Inn and tavern (`townInn` 486AB, `townTavern` 487B4)

* **Inn** (`towninn.m`): after the confirmation window the party is placed on the overworld at the town's entrance cell,
  table of (x,y) byte pairs at DGROUP `178Bh` indexed by `Party_map*2`: town 1 (2,5), 2 (3,2), 3 (7,13), 4 (3,9), 5 (14,6);
  facing is flipped (`^1`), `byte_36FEB` = the town number, each character's word at +123h is set to the town number
  (this is the "where I last slept" field, called `unknown123` in `character.h`), `Party_map` is cleared, time advances by
  5A0h minutes (one day), and the save dialog (`loadSaveDialog(1)`) is opened - the inn is how the game is saved.
* **Tavern** (`honky.m`, `tavern.bin`): open only when `Party_minutes` >= 1080 (18:00) or <= 300 (05:00); otherwise
  "Sorry, the Tavern's closed! Come back later". Options: food, drink, tip (rumours), ESC.
  * Food: fills `Party_food` up to `food_per_char[Party_map] * Party_count * 3`, gold cost from a word table; message
    "Your food packs are already full!" when the party already holds that much. Per-town byte table at `435Fh`:
    0,5,10,15,20,40; adjacent word table 10,50,250,1000,5000 are the matching prices (by code reading, unverified pairing).
  * Drink: costs 1 gold; a character that already has the drunk counter (+11Ah) non-zero gets "You're Drunk"; otherwise
    `rnd(1,100) < 26` increments +11Ah ("Good Stuff" / "Have a Drink").
  * Rumours: each town has a block of rumour strings in the maze text (start index table at `436Eh`: 13h,32h,51h,78h,95h,ACh,C0h;
    counts at `436Fh`), a pointer cycles through them (`Maze_curSlot` slot byte +? at -379Ch).

## Guild (`townGuild` 48466)

* Closed when `byte_32E68` is set. Window text comes from `guild.m`; spell descriptions from `spldesc.bin`.
* Shopping needs guild membership: the helper at 483FB tests `char[+38h + byte_34C1D]` (`byte_34C1D` = the guild's
  town number 1..5), i.e. `awards[17 + town]` (awards start at +27h, so bytes 39h..3Dh). Without it: "You have to be a
  member to shop here". Membership is therefore stored as one award byte per town.
* Visiting the guild costs 60 (3Ch) minutes of game time on exit.
* Shared town helpers (all reached through stubs at 281xx): `sub_281CB` -> `41A2F` (pay gold/gems; also used by
  `giveTake`), `sub_281D5` -> `41633` (character-can-act check, returns non-zero when the character is disabled),
  `sub_28149` -> `4033B` (format number into buffer, used for the "Gold" lines), `sub_47A33` = town menu input
  (mouse/keyboard command dispatcher returning command code; C9h+n selects party slot n, 1Bh = ESC).

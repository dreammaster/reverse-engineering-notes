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

RACE_HP_BONUSES (human, elf, gnome, dwarf, half-orc): [0, -2, -1, 1, 2]

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
  town number 1..5), i.e. `awards[town - 1]`: the award bytes start at +39h, and awards 0-4 are the five guild memberships
  ("Raven's Guild Member", "Albatross Guild Member", "Falcon's", "Buzzard's", "Eagle's", see `text-files.md`). Without it:
  "You have to be a member to shop here".
* Visiting the guild costs 60 (3Ch) minutes of game time on exit.
* Shared town helpers (all reached through stubs at 281xx): `sub_281CB` -> `41A2F` (pay gold/gems; also used by
  `giveTake`), `sub_281D5` -> `41633` (character-can-act check, returns non-zero when the character is disabled),
  `sub_28149` -> `4033B` (format number into buffer, used for the "Gold" lines), `sub_47A33` = town menu input
  (mouse/keyboard command dispatcher returning command code; C9h+n selects party slot n, 1Bh = ESC).

## Item prices (`sub_51E25`, reached through stub `sub_28690`; `itemsDialog` shows "Buy/Sell %s for %lu gold?")

Arguments: (character, slot, mode, flags). Only items with id <= 49h, or the special ids 4Bh/52h, have a price (others 0).
Base price: id 4Bh = 2000, 52h = 1000, else word table at DGROUP `0B16h` indexed by item id (1-based id; the first ~75
entries are gold values: 1=50, 2=15, 3=100, 4=80 ...). Then, in order:

1. metal (slotMetal at +B6h, 1-based): 1 -> /10, 2 -> /4, 3 -> /2, 4 -> x0.75, otherwise multiplied by the signed byte
   table `0AB6h[metal]` (2, 3, 5, 8, 12 ... 100 ...).
2. extras: element `0A4Ch[slotElement] * 100`, attribute `0ACDh[slotAttribute] * 100`, spell word table
   `0DABh[slotSpell]` (100, 200 or 300 by spell tier).
3. Mode 1/2 (buy/sell): `(base + extras) / D[flags & 7Fh]` with D at `5B15h`; if the item is cursed/broken (flags C0h) and
   the 80h flag asks for a sale it is worth 0. The caller passes flags `80h | (merchant skill == 0)` for selling where
   skills[8] (char +2Fh) is the Merchant skill, so a character without Merchant gets half price (divisor 2) and one with it
   the full price. Mode 3-6 return the item's charge/bonus count (`flags & 3Fh`) rather than a price; mode 0 returns 0.

In `itemsDialog`, an item may be sold only if (id < 46h and not cursed (flags & 40h)) or id is 4Bh or 52h
(the same classes that have a price).

Shops (`townSmithy`, `dungeon.m`) are `characterInfoInventory` pages opened in buy/sell mode (modes 1/2 of `itemsDialog`);
the smithy itself only handles opening hours (`byte_32E68`), the character selector and 60 minutes of game time.

## Monster damage and saving throws (`doCharDamage` 4A779, `charSavingThrow` 4A62C)

`doCharDamage(char, slot, monster)`:

1. The character wakes up (sleep counter at +11Bh cleared).
2. damage = sum of `MONDMGN` rolls of d`MONDMGS`.
3. If the monster's damage type (`MONDMGT`) is not 0 (physical): a general saving throw (type 0) halves the damage; then, for
   types 2-5 (fire, electricity, cold, poison) the party resistance word (`Party_fireResist`, `elecResist`, `coldResist`,
   `poisonResist`) is subtracted, and the damage is repeatedly halved for as long as saving throws of that type succeed (so a well
   resisted character takes very little). Type 1 = magic, 6 = energy get only the saving-throw halving loop.
4. `damage -= char.powerShield` (+103h), minimum 0.
5. If damage remains and `MONSPEC != 0`, a general saving throw is rolled; on failure the special attack of `monsters.md` applies.
   Then `subtractHitPoints(char, damage)`.

`charSavingThrow(char, type)` returns 1 = saved: roll `rnd(1, T + 20 or 40)` and succeed when the roll <= T, where
* type 0: `T = level + statBonus(2 * luck)` (range limit 20),
* types 1-6: `T = itemScan(res) + byte pair of the character's resistance (temporary + permanent)`, range limit 40.
  Pairs at char `+111h` (type 1, magic, itemScan 10h), `+107h` (2, fire, 0Bh), `+10Bh` (3, electricity, 0Ch), `+109h` (4, cold, 0Dh),
  `+10Dh` (5, poison, 0Eh), `+10Fh` (6, energy, 0Fh); `unknown106` in `character.h` is therefore six resistance pairs plus one byte.

## Experience (`getCurrentExperience` 415A8-ish, `nextExperienceLevel` 414F5-ish, `experienceToNextLevel`)

The stored experience at char `+12Bh` (dword) is the experience **above the base of the character's current level**; total experience
is `base(level) + stored`.  With the class multiplier `M` (word table `XP_CLASS_MULT` at DGROUP `1CB2h`, indexed by class:
knight 1500, paladin 2000, archer 2000, cleric 1500, sorcerer 2000, robber 1000, ninja 1500, barbarian 1500, druid 1500, ranger 2000):

* `base(1) = 0`; `base(n) = M * 2^(n-2)` for n = 2..12;  `base(n) = M * 1024 + (n-12) * 1,024,000` for n >= 13 (flat 1,024,000 per level).
* `nextExperienceLevel(char)` = `base(level + 1)`; `experienceToNextLevel` = `max(0, base(level+1) - (base(level) + stored))`.
* Training in the town raises the level and subtracts `base(level+1) - base(level)` from the stored value, so total experience is preserved.

## Skills (`getThievery` 4EC3E-ish, `checkSkill` 153EA)

* `getThievery(char)` = `2 * level` + class bonus (robber +30, ninja +15) + race bonus (race byte +11h: Elf (1) and Dwarf (3) give +10, Gnome (2) +5, Half-Orc (4) -10)
  + `itemScan(10)` (item bonuses); 0 if the character lacks the Thievery skill (+27h == 0); never negative.  Locks/traps compare
  `getThievery + d20` with the page's lock/trap difficulty bytes (`data-files.md`, bytes 11h/12h).
* `checkSkill(n)` = "does the party have skill n": the skill byte (+27h + n) must be non-zero in the character; skill ids 0-4, 6-8, 12, 13, 15-17 need **one** member
  with it, ids 9-11 need **two** members, ids 5 and 14 need **every** member of the party.  Used with ids 4 (Cartographer: auto-mapping) and 6 (Direction Sense: compass).

Skill ids (names in the data segment next to `Experience`/`Gold`/`Gems`/`Condition`): 0 Thievery, 1 Arms Master, 2 Astrologer, 3 Body Builder,
4 Cartographer, 5 Crusader, 6 Direction Sense, 7 Linguist, 8 Merchant, 9 Mountaineer, 10 Navigator, 11 Path Finder, 12 Prayer Master,
13 Prestidigitator, 14 Swimmer, 15 Tracker, 16 Spot Secret Doors, 17 Danger Sense -- so Mountaineer, Navigator and Path Finder need two party
members, Crusader and Swimmer every member, the rest one.  (Merchant = 8, the one `itemPrice` looks at.)

Names in the data segment confirm the id orders used in this document: races 0 Human, 1 Elf, 2 Gnome, 3 Dwarf, 4 Half-Orc (name pointer table at DGROUP `581Eh`; "H-Orc"), alignments 1 Good, 2 Neutral, 3 Evil; sexes Male, Female; the 16 condition
names in order Cursed, Heart Broken, Weak, Poisoned, Diseased, Insane, In Love, Drunk, Asleep, Depressed, Confused, Paralyzed, Unconscious, (Dead is drawn separately), Stone,
Eradicated -- the Xeen order, which settles the mapping assumed in `monsters.md`.

## Resting (`rest`, `41610`-ish)

Refused with "Too dangerous to rest here!" on cells with flag bit 04h or a page whose header byte 0Eh is 0. If the party cannot all be fed the game asks "Some Chars may die. Rest anyway?"
(`confirmDialog`). Resting plays out ten `chargeStep` ticks (during which monsters may arrive and interrupt), then `changeTime` for 8 hours ("0008 hours pass. Rest complete."), and for each
member that is not dead/stone/eradicated and is fed (`Party_food` > 0, one unit eaten per member) sets hit points and spell points to `getMaxHP`/`getMaxSP` ("Hit Pts and Spell Pts restored.");
`checkPartyDead` then runs. Super Shelter calls the same routine.

## Time (`changeTime` 16973, `chargeStep` 16DC0, `addTime` 1531F)

* `chargeStep` (every movement command) advances the clock by 1 minute, or 10 minutes when `Maze_wrapMode` is set (outdoor maps), then calls `moveMonsters`.
* Each time the clock crosses a multiple of 480 minutes (`1E0h`, 8 hours) `changeTime` runs the condition tick for every party member: a character with any stat reduced to 0 dies,
  several timed conditions (asleep, confused, paralysed, weak ...) are cleared or aged, poison/disease counters grow (doubling, with a 1 in 10 chance each tick to roll a saving throw that cures them),
  heart-broken counts up to 10 then turns into depression, and so on (the full per-condition table of this 550-line routine is not decoded).
* At the end of `changeTime`, on maps that have a day/night cycle (`mapHasDayNight`: maps below 6, 24-28, 41-104 and above 106), `Town_closed` (`byte_32E68`) is recomputed:
  night is `Party_minutes < 300 or >= 1260` (before 05:00 or from 21:00); when the state changed the town is reloaded (`sub_28194` -> `sub_43034`) in its day or night form. Guild, smithy and training grounds refuse entry while it is night (checked in those routines; the temple and bank were not checked); the tavern has its own hours (open 18:00-05:00).

## Days, years, bank interest, smithy restock (`addTime` 1531F)

* The day counter runs 0-99; passing 100 wraps it and increments `Party_year` and **clears game flags 6Fh-75h (111-117)**, the seven once-a-year event flags.
* On a day where `day mod 10 == 1` (or when more than 1440 minutes were added at once) and the day has changed, the Blacksmith's stock is rebuilt (`resetBlacksmithWares`) and the bank pays interest:
  `GiveBankInterest` adds 1 % (value / 100, integer) of the banked gold and of the banked gems -- so every 10 days, not every day.

Age (`getAge`, `16E06`-ish): `min(254, Party_year - birthYear) + tempAge`, where `birthYear` is the word at char +129h and `tempAge` the byte at +26h (raised by monsters and Resurrect); with the "permanent" argument set the temporary part is left out.

## Which classes may equip what (`equipItem` 3956E, `canEquip` = `sub_51C4A`)

`equipItem` first refuses a second identical item (same element, metal, attribute, id and spell) with "You cannot equip two of the same items!".  Then, for the item's id range, `canEquip(class, item)` is checked: Knights and Paladins may equip anything; for the other eight classes the item id indexes a byte table (`EQUIP_FORBID`, DGROUP `0D62h`) of **forbidden-class bits**, the class bit being `1 << (class - 2)` in the order Archer, Cleric, Sorcerer, Robber, Ninja, Barbarian, Druid, Ranger (word table `5AB5h`); a set bit gives "%ss are not proficient with %s!".  Ids 43 and above (trinkets, consumables) are unrestricted (0). Equipment slots are tracked in the per-slot array at char +7Dh: 0 = carried in the backpack, otherwise the code of the body location the item occupies, assigned by item id (`equipItem`): ids 1-17 = code 1 (one-handed weapon), 18-29 = 0Dh (two-handed weapon), 30-33 = 4 (missile weapon), 34-41 = 3 (body armour), 42 = 2 (shield), 43-45 = 5 (helm, crown, tiara), 46 = 6 (gauntlets), 47 = 8 (ring), 48 = 9 (boots), 49-51 = 0Ah (cloak, robes, cape), 52 = 0Ch (belt), 53-57 = 7 (broach, medal, charm, cameo, scarab), 58-60 = 0Bh (pendant, necklace, amulet); a second item of the same code is refused (`getEquipSlotName` names the occupied location), plus: a weapon (code 1) conflicts with an equipped two-hander, a two-hander (0Dh) with code 1, 2 (shield) or another 0Dh, and a shield with a two-hander. Ids 61 and above (rods, gems, potions...) are not wearable.

| id | item | forbid mask | classes that may use it |
|---|---|---|---|
| 1 | long sword | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 2 | short sword | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 3 | broad sword | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 4 | scimitar | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 5 | cutlass | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 6 | sabre | 118 | Knight, Paladin, Archer, Robber, Ranger |
| 7 | club | 0 | everyone |
| 8 | hand axe | 6 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Druid, Ranger |
| 9 | katana | 239 | Knight, Paladin, Ninja |
| 10 | nunchakas | 239 | Knight, Paladin, Ninja |
| 11 | wakazashi | 239 | Knight, Paladin, Ninja |
| 12 | dagger | 2 | Knight, Paladin, Archer, Sorcerer, Robber, Ninja, Barbarian, Druid, Ranger |
| 13 | mace | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 14 | flail | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 15 | cudgel | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 16 | maul | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 17 | spear | 6 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Druid, Ranger |
| 18 | bardiche | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 19 | glaive | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 20 | halberd | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 21 | pike | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 22 | flamberge | 126 | Knight, Paladin, Archer, Ranger |
| 23 | trident | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 24 | staff | 0 | everyone |
| 25 | hammer | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 26 | naginata | 239 | Knight, Paladin, Ninja |
| 27 | battle axe | 86 | Knight, Paladin, Archer, Robber, Barbarian, Ranger |
| 28 | grand axe | 86 | Knight, Paladin, Archer, Robber, Barbarian, Ranger |
| 29 | great axe | 86 | Knight, Paladin, Archer, Robber, Barbarian, Ranger |
| 30 | short bow | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 31 | long bow | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 32 | crossbow | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 33 | sling | 70 | Knight, Paladin, Archer, Robber, Ninja, Barbarian, Ranger |
| 34 | padded armor | 0 | everyone |
| 35 | leather armor | 4 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Druid, Ranger |
| 36 | scale armor | 68 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Barbarian, Ranger |
| 37 | ring mail | 100 | Knight, Paladin, Archer, Cleric, Robber, Ninja, Ranger |
| 38 | chain mail | 116 | Knight, Paladin, Archer, Cleric, Robber, Ranger |
| 39 | splint mail | 125 | Knight, Paladin, Cleric, Ranger |
| 40 | plate mail | 255 | Knight, Paladin |
| 41 | plate armor | 255 | Knight, Paladin |
| 42 | shield | 85 | Knight, Paladin, Cleric, Robber, Barbarian, Ranger |

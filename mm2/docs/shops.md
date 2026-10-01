# Temples, Mage Guilds and other town buildings

Town numbers 0-4: Middlegate, Atlantium, Tundara, Vulcania, Sansobar (`DGROUP:0000` strings).
`evt_op14_enter_location` selects the building: 1 inn, 2 training hall, 3 tavern, 4/5 temple/guild, 6
blacksmith (see events.md).  All shop texts come from `STR.DAT` (loaded into pointer tables at run time).

## Temple (2TEMPLE `temple_menu` `1CA88`)

Menu: **A** Restore Condition, **B** Restore Alignment, **C** Donations, **D/E/F** buy a cleric spell,
**G** gather gold (pool the party's gold on one character), 1-8 choose a character.

* Town price multiplier: `DGROUP:46A8` = {1, 5, 2, 3, 2} for towns 0-4.
* Restore condition: cost = (`10` if the character has any condition or HP < max, `100` if dead
  (>= 80h), `1000` if eradicated (FFh)) x character level x multiplier; cures the condition and heals to full.
* Restore alignment: `100 x level x multiplier` if the current alignment (`+6A`) differs from the original (`+0D`).
* Donation: `100 x multiplier` gold.  90 % of the time the party is blessed ("Today you are blessed!"):
  Light 200, Magic 60, Forces 60, Levitate/Walk on Water/Guard Dog 1, other effect bytes set; the donation
  counter `word_23130` grows and bit *town* of `byte_1DC32` is set (`DGROUP:470C` = 1,2,4,8,16); when all
  five towns' temples were donated to (`1Fh`) a special event triggers.
* Spell prices use a compressed code `c`: `(c & 1Fh)`, x10 if bit 5, x100 if bit 6, x1000 if bit 7.

## Spell sales

Cleric-list casters (Paladin, Cleric) buy from the temple (3 spells per town, `DGROUP:46B2` = spell index
(absolute, 30h-based), `46C6` = price codes); Sorcerer-list casters (Archer, Sorcerer) buy from the Mage
Guild (4 spells per town, `46DA` = index, `46EE` = price codes).  A character can only buy a spell whose
level <= its spell level and that it does not know yet (`362C`).

| Town | Temple (cleric spells, gold) | Mage Guild (sorcerer spells, gold) |
|---|---|---|
| Middlegate | Apparition 10, Awaken 10, Power Cure 1000 | Awaken 10, Energy Blast 1000, Sleep 50, Identify Monster 100 |
| Atlantium | Mass Distortion 20000, Resurrection 50000, Uncurse Item 100000 | Mega Volts 50000, Meteor Shower 50000, Implosion 100000, Inferno 100000 |
| Tundara | Cold Ray 400, Lasting Light 100, Restore Alignment 500 | Feeble Mind 600, Fire Ball 2000, Disrupt 3000, Sand Storm 3000 |
| Vulcania | Holy Bonus 2000, Remove Condition 3000, Fiery Flail 10000 | Disintegration 5000, Fantastic Freeze 5000, Super Shock 5000, Duplication 25000 |
| Sansobar | Heroism 250, Protection From Elements 300, Weaken 200 | Protection from Magic 400, Acid Stream 200, Lightning Bolt 1000, Cold Beam 500 |

(Spell names from the manual, see spells.md; prices decoded from the tables and rounded as stored.)

## Blacksmith (2SMITH `blacksmith_menu` `1CCBA`)

Menu keys **A-D** = four stock categories (`word_2307A` = 1-4), **E** = Sell (5), **F** = Identify (6),
**G** gathers the party's gold on the current character, **1-8** switch character, Esc leaves.  The labels
for A-D come from the town's own text (`DGROUP:57DA`, 4 pointers per map).  Every action first checks the
character is not incapacitated (condition `+26` != 0 gives message 8) and the slot is not empty (message 6).

### Stock (`smith_load_stock` `1C8E0`)

Six slots per category, taken from `DGROUP` tables indexed by town map id (6 bytes per town; all five towns
have a row).  Item ids, with the matching **bonus bytes** (the `+n` enchantment shown after the name; low 6 bits
= amount, top 2 bits unused here):

| Category | Ids at | Bonus at | Content | Notes |
|---|---|---|---|---|
| A (1) | `43C8` | `43E6` | cheap weapons (daggers, swords, flails ...) | fixed bonus per slot |
| B (2) | `447C` | same table reused | other weapons (bows, polearms, hammers) | bonus is **daily**: day of year `d` (`DGROUP:3A2[era]`): `d mod 30 = 29` uses `449A[d/30]` (5-12), else `44A0[d mod 30]` (0-4) |
| C (3) | `4404` | `4422` | shields and armour | fixed bonus |
| D (4) | `4440` | (charges) | potions, tools, tickets, wands | the 'bonus' table is actually the number of **charges** (`+5840`), bonus is 0 |

On map 1 in category D the party flags `byte_23060 = 5`, `byte_23062 = 2` (shop-specific side effect, not decoded).

### Prices (`smith_item_price` `1C7FC`)

Item record price `P` = word at `+12h`, bonus `b`:

* **Buy / sell base**: if `b = 0`, `P`; else `2P + 1000*(b-1)`.
* **Merchant skill** (skill id 10, tested with `res_3664(char, 10)`): **buy** price is halved when the character
  has it; **sell** price is `base/2` with it and `base/4` without it.
* **Identify**: `10` gold if `b = 0`, else `100*b`.  Shows class letters (`KPACSRNB`), alignment
  (evil/good/neutral only), attribute bonus, charges, "Use item" effect (spell number `S`/`C`), damage `1-n`
  (item id < 6Fh) or armour bonus (id 73h-9Fh).
* Buying needs the gold (`smith_pay_gold`: message 4 otherwise) and a free backpack slot (message 2 otherwise).
  The item's bonus byte and charges are copied into the character's slot arrays (`+3A` id, `+40` charges, `+46` bonus).
  A `-` in front of a row means the item's class mask excludes the character's class.

### Theft

A failed theft (`smith_robbery_fight` `1CEC8`, reached from the event `FDh` robbery trap) starts a fight with the
guards (monsters `FFh, E1h, C2h, C1h, E0h`, surprise flag `83h`).

## Lord Hoardall and Lord Slayer (2CAVES `caves_event_c9` / `caves_event_ca` -> `1D3C4`)

Two special locations (event 14 arguments `C9h` = Hoardall, `CAh` = Slayer; `byte_22E14` = 0 / 1) give the party
repeatable **quests** for experience.  Per character the state is in the record: `+78` = target (item id for
Hoardall, monster id for Slayer), `+7C` bit 0 = quest kind, bit 2 = quest active, bit 3 / bit 4 = Hoardall /
Slayer reward already taken, top 3 bits = number of completed rewards.

1. The lord asks "Will you gather more items / trophies (y/n)?"; "At what level of difficulty do you wish to aid Lord
   X?" -- **A) Page's, B) Squire's, C) Knight's, D) Lord's quest** (`DGROUP:3E00`).
2. A, B, C pick a random target for every living character: Hoardall draws an item from weighted bands
   (`DGROUP:3E0C` start ids per level, `3E1E` weights; level A = clubs/staffs/blowpipes/shields/armour/helms of the plain
   kind, B = the magical kind, C = the best kind); Slayer picks monster id `rand(1, 3E36[level]) + 3E3A[level]`, i.e. ids
   32-79, 80-143, 144-191.  **D** is the final quest: the three swords (items E2h-E4h: Valor, Honor and Noble Sword) or the
   three beasts `caves_lords_quest` just marks every character who has not yet taken that lord's final reward as on the quest (`+7C` bit 2 set, no random target); the party must then bring the three swords (`caves_three_swords_check` removes all three) / kill the three beasts.
3. **Rewards**: handing in a Hoardall item (`1CBCA`, the item is removed from the backpack) gives **8 x the item's price** in experience.
   A Slayer target kill (`1CB4A`) gives experience by monster id band: ids below 48/64/80/96/112/128/144/160/176/192 pay
   2000 / 4000 / 5000 / 7000 / 10000 / 15000 / 25000 / 50000 / 100000 / 250000 (`DGROUP:3E3E`, `3E48`).  Finishing the final
   **Lord's quest (D)** for the whole party pays **100 000 (Hoardall) or 1 000 000 (Slayer)** experience per character
   (`caves_quest_rewards`).  "Begone until you have completed your quest!" is shown when nothing is done yet; Esc gives "Then begone,
   knave!" and the party is moved to a fixed cell of the map (9,10 / 3,5).

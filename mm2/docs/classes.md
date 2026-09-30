# Classes, races, levels

Class index (character `+0F`): 0 Knight, 1 Paladin, 2 Archer, 3 Cleric, 4 Sorcerer, 5 Robber, 6 Ninja,
7 Barbarian.  Race (`+0E`): 0 Human, 1 Elf, 2 Dwarf, 3 Gnome, 4 Half-Orc.  Alignment (`+6A`): 0 Good,
1 Neutral, 2 Evil.  Stat order in creation: Might, Intellect, Personality, Endurance, Speed, Accuracy, Luck.
Spell lists: Archer and Sorcerer use the sorcerer list, Paladin and Cleric the cleric list (spells.md).

## Creation tables (1MENU2 `create_character_record` `18624`)

| Table (DGROUP) | Meaning | Values |
|---|---|---|
| `093C` (5 x 7 bytes) | racial stat adjustments (255 = -1) | Human 0; Elf Mgt-1 Int+1 End-1 Acy+1; Dwarf Int-1 End+1 Spd-1 Lck+1; Gnome Spd-1 Acy-1 Lck+2; Half-Orc Mgt+1 Int-1 Per-1 End+1 Lck-1 |
| `06AE + 6*k + race` (k = 0..7) | 8 resistance values copied to char `+16..+1D` | k=0: 0,0,0,35(Gnome),0; k=1..3: 5,0,10,0,5; k=4: 0; k=5: 60,30,0,0,30; k=6: 60,5,60,0,30; k=7: 0 |
| `06DE` (8) | starting Thievery per class | Robber 30, Ninja 10, others 0 |
| `06E6` (8 words) | starting hit points per class | 9,7,7,5,3,5,5,12 |
| `06F2` (words, index = Endurance) | bonus starting HP | see file |
| `071E` (words) | starting spell points for casters (index = Personality (Cleric) or Intellect (Sorcerer)) | see file |
| `074D` | starting AC from Speed | 1-2 at high Speed |
| `2F18` (8 words) | **hit points gained per level** (base) | 12,10,10,8,6,8,8,15 |

## Level up (2MISC2 training hall, `training_hall` `1CE30`, `char_train` `1C86A`)

* Requirements: character not disabled (condition 0), gold >= cost, experience >= `exp_for_level`.
* Cost = `50 x (current level + 1) x town multiplier` gold, multiplier `DGROUP:2F04` = {1,5,2,3,2} for towns 0-4
  (same multiplier as the temple).
* Experience to *reach* level L (`loc_1CC8C`, `tools/mm2_rules.py:exp_for_level`): table `DGROUP:2E5C`,
  index min(L,10): group 0 (Knight, Cleric, Robber, Barbarian) 1500, 3000, 6000, ... doubling up to
  384000 at L>=10; group 1 (Paladin, Archer, Sorcerer, Ninja) 2000, 4000, ... 512000; plus fixed additions
  for higher levels: L>=11: +192000 each for L=11,12,13; L>=14: +384000 each for 14,15; then
  +768000 x min(L-15,5), +1536000 x min(L-20,10), +3072000 x min(L-30,20), +1638400 x min(L-50,25),
  +6144000 x (L-75).
* Effects: base level (`+20`) and current level (`+71`) +1; Robber/Ninja with Thievery > 0 gain +1 Thievery;
  hit points gained = `HPperLevel[class] x townMult / town divisor` (`DGROUP:2F0E` = {2,5,3,4,3}), rounded
  up when a remainder exists (except Cleric, Robber, Ninja) `+` an Endurance bracket bonus
  (`lookup_bracket` on base endurance, table `DGROUP:4D84`); added to max HP (`+60`, `+74`) and current HP
  (`+5E`); spell casters (classes 1-4) also gain spell points / spells (`1C6CC`).
* "Free" training (cost 0) instead gives gold: `gold += gold/2` (cap 50000) -- a special case at the
  first town (check).

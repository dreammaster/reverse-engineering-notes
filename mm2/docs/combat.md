# Combat (2COMBAT.OVL)

Overlay entry point (through a thunk): `combat_encounter` (`1A2A6`), called by the resident
`start_combat` (`13EB2`).  Everything else is internal.  Names are in `names/2COMBAT.tsv`.  This
was read from the code; numeric details marked *(check)* were only skimmed.

## Flow

1. `start_combat` -> `combat_encounter` (`1A2A6`): resets state, loads `MONSTERS.DAT`
   (`dword_1DD54`), counts the monster ids supplied by the map event (`DGROUP:9680`, 10 ids, 0 = none),
   or invents an encounter: `combat_generate_encounter` (`197E6`) keeps rolling until the enemy strength
   is comparable to the party's (`combat_party_strength` `1974C`, `combat_encounter_ok` `198FE`).
   Monster id = `tier*16 + index` (tier 0-13 grows with party strength, index 0-15).
2. Surprise: `byte_1DC65` = 0 normal, 2 = party surprised the monsters, 3 = monsters surprised the
   party ("You surprised the monsters!" / "The monsters surprised you!").  Rolled here: party surprise
   when d100 <= 40 and <= `byte_22CEE` (stealth), monster surprise when d100 >= 90 and no protection flag.
3. Pre-fight menu (skipped if the monsters surprised the party): **A**ttack, **B**ribe (1 food / 2 gold /
   3 gems, amount read with `read_number`; each bribe kind must be accepted by the monster -- record byte
   `10h` bits -- and a roll must pass), **H**ide (roll vs `byte_22CEE`), **R**un (roll vs `byte_231E3`
   unless the party surprised them).  "Success!" ends the encounter.
4. `combat_battle_loop` (`1A0D4`): every iteration is one action.  It picks the **fastest ready monster**
   (speed `DGROUP:9F92[i]`, "already acted" flag `5480[i]`) and the **fastest ready party member** (Speed at
   character `+6E`, flag `548C[i]`); if the character's speed >= the monster's, the character acts
   (`combat_party_turn` `193B2`), else the monster (`combat_monster_turn` `184FE`); when nobody is ready the
   flags reset.  It ends with `combat_victory` (`19BF8`) when all monsters are dead, or
   `combat_party_flees` (`190EC`: back to the last safe cell `byte_231E4`; characters with status >= `10h`
   become dead `81h` unless `byte_27818`).

## Party turn (`combat_party_turn` `193B2`, keys)

| Key | Action |
|---|---|
| A / Enter | melee attack (front rank only: the first `byte_22CED` characters) -> `combat_party_attack` (`18DAA`, `byte_22CF4` = 0) |
| F | fight (same, when offered) |
| S | shoot (`byte_22CF4` = 1, ranged attack count) |
| C | cast: `cast_spell_menu`; then `2CAST1:cast_noncombat_spell` if the spell may be used outside combat, else `2CAST2:cast_spell_dispatch` |
| U | use item |
| B | block (skip the turn) |
| D | delay setting (`2MISC2:game_controls`) |
| E | exchange party order (`2MISC2:C370`) |
| P | show active protection spells (`1A882`) |
| V / Q / 1-8 | view a character |
| R | run: the character may leave the battle line (`combat_char_runs` `1914A`, chance `byte_231E3` %) |

## Party attack (`combat_party_attack` `18DAA`)

* Attacks per round = `level(+71) / D + 1`, `D` from the class table `DGROUP:1012` = {1,1,1,3,4,2,2,1}
  (Knight, Paladin, Archer, Cleric, Sorcerer, Robber, Ninja, Barbarian); for shooting the divisor table is
  `DGROUP:101A` = {4,4,5,7,10,5,5,4}.
* Each swing: d100 roll; below 6 is a hit *(check)*; otherwise `rand(1, min(250, 25 + bonus)) + accuracy
  bonus` is compared with the monster's AC (`byte_2767C`); a hit does `rand(1, weapon dice) + damage
  bonus` (Might bracket from `lookup_bracket` `1354A`); each hit also adds `byte_1DC37`.
* Robber (class 5): back-stab chance -> damage x2; Ninja (class 6): critical -> damage x4 (`byte_22CE6`
  1/2 -> " back stabs" / " criticals").  Immune monsters print " is not affected!".
* `combat_damage_monster` (`18B3E`) subtracts from the monster's HP (word `DGROUP:9FAA[i]`); at 0 ->
  `combat_kill_monster` (`18AF4`): rewards (`combat_monster_rewards` `188FC`: experience from the record,
  gold/gems by monster flags), removal and array shift (`18A22`), " goes down!".

## Monster turn (`combat_monster_turn` `184FE`)

Per monster slot *i* (11 slots; monsters beyond 10 are counted in `word_1DD58` and refill slots as the
front ones die): id `DGROUP:9680[i]`, status bits `9F86[i]`, ability uses left `9F9E[i]`, speed `9F92[i]`,
HP word `9FAA[i]`.  Status bits (`DGROUP:1022`, names at `0FEA`): 1 hurt/awake, 2 silenced, 4 weakened,
8 frightened, 10h asleep, 20h held, 40h mindless, 80h encased.

1. A sleeping/held/etc. monster (`status & B0h`) does not attack; otherwise:
2. chance to **summon friends** (" adds friends!") from `DGROUP:1036` by tier vs `byte_1E812`.
3. `combat_monster_spell_roll` (`1847E`): not silenced, uses left, d100 <= the record's cast chance ->
   cast (`combat_monster_casts` `18056`), else
4. front-rank monsters (`i < byte_27815`) melee (`18398`): `record[14].low + 1` blows; a blow hits with
   probability about `max(5, ToHit[tier] - character AC)` % where `ToHit` = `DGROUP:103A` =
   {40,45,50,55,60,65,70,75,80,90,100,120,150,250,100,200}; damage `rand(1, record damage dice)`;
   halved for weakened monsters / protection spells.  Back-rank monsters with the ranged flag shoot (80 %),
   the others advance (`combat_monster_advances` `1814A`, " advances!").
5. Hit effects (`combat_after_hit` `17E52`): HP <= 0 -> unconscious (`40h`), more negative -> dead
   (`81h`); " goes down!"; then possibly the **touch effect** (`record[12].low5`, table `DGROUP:106C`)
   through `combat_apply_touch_effect` (`1AFE2`), gated by a percentage test *(check)*.

## Spell hits (`combat_party_spell_hits` `18696`, `1B226`, `1B410`)

`" casts a spell:"`, then for each target monster (damage `word_27816`): resistance tests against the
monster record bytes, `" is not affected!"`, `" resisted!"`, `" takes N point(s)"`, or a status change
`" is silenced/weakened/frightened/slept/held/mindless/encased!"`.  Damage spells set the hurt bit.

## Victory (`19BF8`)

"Victory! Your party has won its Nth battle." (ordinal suffix logic `199C8`); every survivor gets
`word_1E80E/1E810` experience ("Each survivor receives N experience points"); treasure:
`combat_drop_treasure_item` (`19A3C`): a d100 against `DGROUP:10EA` = {25,40,50,55,70,75,100} selects a
row of the item tables at `DGROUP:10F6` (base item id + rand(1, range for the quality level)).

## Other tables

| Table | Meaning |
|---|---|
| `DGROUP:0FD8` | menu strings `A-Attack F-Fight S-Shoot C-Cast U-Use` |
| `DGROUP:0FC8` | status abbreviations in the monster list: Enca Mdls Held Aslp Afrd Weak Siln Hurt |
| `DGROUP:1058` | melee verbs: attacks, fights, charges, battles, thrusts at, slashes at, strikes at |
| `DGROUP:10AA` / `106C` | monster spell / touch-effect names (see file-formats.md) |

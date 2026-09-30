# Party commands outside combat

Key list (resident `DGROUP:4F68`): arrows move/turn, **B** Bash Door, **C** Controls, **D** Dismiss,
**E** Exchange, **Q** Quick Ref, **R** Rest, **S** Search, **U** Unlock, **1-8** view a character;
**O**/**P** switch the bottom panel between options and protection views.  Dispatch is in
`game_main_loop` (`2PLAY:17E10`).

## Rest (2MISC `party_rest` `1CF84`)

* Refused with "Too dangerous!" if the cell flag byte has bit 3 set (`byte_23218 & 8`); otherwise asks Y/N.
* `party_pay_hireling_upkeep`: the hirelings' gold fields (`+66`) are their daily fees; the party pays the
  total (else "Not enough gold - Dismiss hirelings").
* `party_do_rest` (`1CD8A`): clears every timed effect (Light, Magic, Forces, Levitate, Walk on Water, Guard
  Dog and the related bytes).  For each character that is not disabled: condition `&= 0Dh` (only cursed,
  diseased and poisoned survive); if age >= 80, 50 % chance to die in the night (`81h`); HP >= 1; maximum HP
  restored (`+74 = +60`, halved if poisoned); if the character has food, food -1 and, unless diseased, HP
  fully restored; spell points restored to `level x (bracket(stat) + 3)` (stat = Intellect for Archer/Sorcerer,
  else Personality) for characters with spell level > 0; current stats reset from base.  Then
  `advance_time(85)`; unless already in era 9, `rand(1,60) < 10` sends the party to era 9 (`g_era = 9`).
* `party_rest_ambush` (`1CEEE`): without Guard Dog, 1 in 50: everyone falls asleep and monsters surprise the
  party (`byte_1DC65` = 3).

## Search / treasure

`evt_op42_place_treasure` puts gold, gems and up to three items on the current spot and flags
`byte_1DC84`.  The **S** command opens `party_search` (`2MISC:1CA52`): it shows the chest picture
(`monster_gfx_load`), "Search... The Party Has found a: <container>" and offers options 1-4 (the container
name comes from a table at `DGROUP:28A2`, the difficulty from the number of treasure items/gold/gems and
quality bytes: `>= 4` needs a d100 roll >= 30 to find anything).  After opening: "Treasure!",
`treasure_share` (`1C64A`: "Each share = N Gold / N Gems", split among conscious members) and
`treasure_give_item` (`1C538`: " found <item>", "Backpacks full!").

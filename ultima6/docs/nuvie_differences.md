# Where Nuvie and the original disagree

**Rule:** when Nuvie (`ultima6/nuvie`) and the original executables disagree, this project follows the original. Every difference found is listed here so that we can later decide
whether any Nuvie variation is worth keeping as an option (the ScummVM/enhancement phase). Add an entry whenever a difference is found; give the evidence (routine address or
data) and say which behaviour the stand-alone engine currently implements.

Entries marked *pending* are places where the engine currently uses Nuvie's rule only because the original has not been traced yet (see `approximations.md`); if the original
turns out to differ, the engine is changed and the entry moves up to a real difference.

## Confirmed differences

| # | Topic | Original (followed) | Nuvie | Evidence |
|---|---|---|---|---|
| 1 | Gypsy quiz shuffle | `shuffle_words`: `rand % (n*n) + n` swaps | runs one extra swap | `docs/u6_intro.md`, `U.EXE` |
| 2 | Gypsy vial numbering | virtues 0-7 | numbered from 1 | `docs/u6_intro.md` |
| 3 | Humility karma | answering Humility adds 3 karma | omitted | `gypsy_apply_answer` in `U.EXE` |
| 4 | Objlist 0x12F1 / 0x13F1 | 0x13F1 = magic points, 0x12F1 = combat mode | reversed in its loader | `write_new_game_objlist` (14066) sets `+0x13F2` to 2 x INT |
| 5 | Conversation opcodes | jump table `jpt_20207` also has 0xC8, 0xD0 and 0xDF; 0xA8 (assign) is handled outside it; value operators BC, BD, C0-C3 exist | no names for them | `docs/u6_conversation.md` |
| 6 | Portrait numbers | no remapping | remaps a few (Exodus, shrines) | `load_portrait` (394A9) |
| 7 | Stackable object types | the table at `ds:0765` (21 types) plus gold (0x58, word quantity); quantity printed with `%d ` | decides by "the description has a plural form" | `sub_1B687`, `Describe::quantityKind` |
| 8 | Weights | `sub_1D8B1`: block 3 weight x quantity, divided by 10 for gold and the reagents (the nine types at ds:081F), 255 = unmovable (10000); the conversation operators work in ten times those units: 0x9B = qty x w (reduced types) or x10, 0x9A = strength x 200 - 10 x carried; carried = bare weight of readied things + weight and contents of the rest (`sub_1D998`) | Nuvie also divides reagents/gold by 10 for inventory weight (agrees), but scales conversation values differently | `conv_read_value` cases 0x9A/0x9B (1F553, 1F4A7), `sub_1D8B1`, `sub_1D957`, `sub_1D998` |
| 9 | Conversation opening text | `"\n\nYou see "` | `"\nYou see "` | string in `GAME.EXE` near `aYouSay`; engine uses the original |
| 10 | Skiff / raft / horse numbers | skiff 0x19E (414), raft 0x19F (415) are the water movers in the movement class code; horse with rider 0x1AF (431) | (same numbers, but my first reading of the original table was wrong) | `sub_25D7F` var_6 test, `seg066` |
| 11 | Creature statistics: the Guard's strength | 30 | 9 | table in `GAME.EXE` (code segment, `seg066`: columns at +0x82 str, +0xC2 dex, +0x102 int, +0x142 exp, +0x182 dmg, +0x1C2 hp); the other 63 rows agree |
| 12 | Where the creature flags live | alignment is `column(+0x202) >> 5` (0 neutral, 1 evil, 2 good, 3 chaotic) and the flags are bits of the movement class words (+0x242): 0x4000 can talk, 0x2000 bleeds, 0x1000 fades, 0x200 lives in water, 0x100 immune to tremors (fliers), 0x80 repels undead, 0x40 poisonous, 0x20 strength based, 0x10 double fire, 8 immune to magic, 4 immune to poison, 2 undead, 1 immune to sleep | separate flag arrays | same table; the engine reads the movement flags and these from the same data |
| 13 | Reagents of Mass Dispel, Poison Field and Sleep Field | masks 0x16, 0x54, 0x60 (the reagent table at `ds:0D44` indexed by spell number) | 0x60, 0x16, 0x54 (the manual's reagents; the three entries are rotated against the original's table) | `GAME.EXE` data; the other 77 spells' names, invocations and reagents agree exactly. The original's table looks like a bug but is what the game uses |
| 14 | Cast: message wording and order | `cmd_cast` (45593): name and quoted words of power are printed first; then "

Your level is not high enough.
" (level >= circle), "

Not enough magic points.
", "

No Reagents.
"; magic points and reagents are paid **before** the target is chosen (cancelling still costs them); "No spellbook is readied.
" has no leading newline | Nuvie checks everything first, uses "
No spellbook is readied.
" and single-newline wording, and takes payment after targeting | `check_spellbook` (45977), `cmd_cast` |
| 15 | Get | `cmd_get` (69942): "nothing", name, then "Not possible" (weight 0 and not a reduced type, weight >= 10000, actors, type 0x19B, damaging tiles), "The total is too heavy." when strength x 20 < weight + carried; a lit torch goes into a hand ("No free hand"); a few types change when picked up | (own invention in the first version of the engine) | `cmd_get`, `sub_1D88A`, `sub_1D8B1` |
| 16 | Prompt text | `"%s:
%>"` first, `"
%s:
%>"` after each later command | - | `command_loop` (13C64), 145DD |
| 17 | Hit check | `sub_2C8BF`: hit when `rnd(1..30) >= (defender's adjusted dex + 30 - attacker's adjusted str/dex) / 2` (str for clubs, maces, hammers, spiked helm and strength-based creatures; glass sword and non-actors always hit); adjusted = -3 (min 1) when the object status has 0x40 (cursed), dex 1 when asleep (actor status 4) | strict `<` (a roll equal to the threshold misses) | `sub_2C8BF` (60382), `sub_2B3D7`, `sub_2B401` |
| 18 | Combat messages | no text for a miss; "`%s grazed.
"; "`%s killed!
"; wound text "`%s " + "critical!
" or "heavily/lightly/barely wounded.
" with no leading newline (`sub_2C97F`) | leading newlines | seg067 strings, `sub_2C97F` (60508) |
| 19 | Where the party flag lives | actor status array (objlist +0x800): 0x80 in party, 0x40 attacks good, 0x20 attacks evil, 0x10 dead, 8 poisoned, 4 asleep, 2 paralysed, 1 protected; cursed is bit 0x40 of the object status | same bits (Nuvie's `status_flags`) | `sub_25D7F`, `update_actor_schedules`, `sub_2B401` (my first version wrongly read the object status) |
| 20 | Opening a chest or other container | `use_door` (chest, frame 1 closed -> 0 open, 2 locked, 3 magically locked) prints "opened!" and `search_here` puts the contents on the chest's square ("Searching here, you find a, b and c."); nothing goes into the inventory | Nuvie opens a container view | `use_door` 3224C, `search_here` 30031 (my first version took everything into the inventory) |
| 21 | Doors | `use_key_on_door`: frames 0-3 open, 4-7 closed, 8-11 locked ("locked"), 12-15 magic ("magically locked!"); plain Use of a locked door only says it is locked, a key (`use_key_or_lockpick`, matching quality; a lock pick works on quality 0 and may break: "Key broke.") locks or unlocks | the party's keys open locked doors automatically | `use_key_on_door` 320D4, `use_key_or_lockpick` 3241E, `sub_323C4` |
| 22 | Ladders | only type 0x131 (305) is usable (the 85-type table); type 106 prints "Not usable" | accepts both | `cmd_use_types` table at cs:6467 |

| 23 | Months in a year | `advance_time` (137A0) resets the month when it passes 0Ch: 12 months of 28 days | Nuvie uses 13 months | `advance_time` 13789-137B4 |

## Pending (engine uses Nuvie's rule until the original is traced)

| # | Topic | Nuvie rule in use | Original routine to check |
|---|---|---|---|
| P1 | Hit check, damage, armour, experience | `script/u6/actor.cpp` | combat code of `GAME.EXE` |
| P2 | ~~Creature / weapon / armour tables~~ | **done**: all of Nuvie's rows compared with `GAME.EXE` (weapon, armour, range tables at ds:1B0B/1B55, 1A87/1ABB, 1AD5/1AF9; creatures in seg066); one difference (#11). Still unread: the creatures' inventory columns (+0x302 on: spells, weapons, armour, treasure) | - |
| P3 | Spell table: **names, invocations, reagents verified** (#13); costs, level rules, effects still Nuvie's | `script/u6/magic.cpp`, `core/magic.cpp` | spell code in `GAME.EXE` (messages "Success", "Failed", "No effect", "On whom: ", "On what: ", "Location: " at ds:13F2-1459; spell words of power at ds:1005 An..Zu) |
| P4 | Egg hatching: **partly traced** (`sub_366B0`, `sub_36D4E`: hour windows, chance, counts, used eggs, side bits, eggs within 8 squares left alone, see approximations #19) | `core/egg_manager.cpp` | the scan window and placement |
| P5 | Per-minute hazards, torches, hourly magic points | `script/u6/actor.cpp` `advanceTime` | `sub_13305` |
| P6 | Conversation interpreter details, keyword matching | `core/converse_interpret.cpp` | `conv_execute_command` (201EB), `conv_read_value` (1F1BE) |
| P7 | ~~Door and ladder use~~ | **done for doors, chests, levers and switches** (#21, #22, `use_lever`, `use_switch` exact); ladders done as `climb_ladder` (the type 106 difference stays) | `use_lever`, `use_switch`, `climb_ladder` |
| P8 | Walking frames and facing | `actors/actor.cpp` `set_direction` | `sub_263D4` |
| P9 | Time-of-day words, party limit | `core/converse.cpp`, `core/party.cpp` | `sub_1EA97`, party join code |

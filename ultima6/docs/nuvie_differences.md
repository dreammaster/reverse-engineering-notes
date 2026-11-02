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
| 8 | Object weight | conversation operator 0x9B: `TILEFLAG block 3 weight x quantity` | divides gold and reagents again by 10 | `docs/u6_conversation.md` (the Nuvie rule is not used) |
| 9 | Conversation opening text | `"\n\nYou see "` | `"\nYou see "` | string in `GAME.EXE` near `aYouSay`; engine uses the original |
| 10 | Skiff / raft / horse numbers | skiff 0x19E (414), raft 0x19F (415) are the water movers in the movement class code; horse with rider 0x1AF (431) | (same numbers, but my first reading of the original table was wrong) | `sub_25D7F` var_6 test, `seg066` |

## Pending (engine uses Nuvie's rule until the original is traced)

| # | Topic | Nuvie rule in use | Original routine to check |
|---|---|---|---|
| P1 | Hit check, damage, armour, experience | `script/u6/actor.cpp` | combat code of `GAME.EXE` |
| P2 | Creature / weapon / armour tables | `script/u6/tables.cpp` | tables in `GAME.EXE` data |
| P3 | Spell table, costs, effects | `script/u6/magic.cpp`, `core/magic.cpp` | spell code in `GAME.EXE` |
| P4 | Egg hatching | `core/egg_manager.cpp` | egg code in `GAME.EXE` |
| P5 | Per-minute hazards, torches, hourly magic points | `script/u6/actor.cpp` `advanceTime` | `sub_13305` |
| P6 | Conversation interpreter details, keyword matching | `core/converse_interpret.cpp` | `conv_execute_command` (201EB), `conv_read_value` (1F1BE) |
| P7 | Door and ladder use | `usecode/u6_usecode.cpp` | the Use handlers behind `cmd_use_types` (0x35AF7) |
| P8 | Walking frames and facing | `actors/actor.cpp` `set_direction` | `sub_263D4` |
| P9 | Time-of-day words, party limit | `core/converse.cpp`, `core/party.cpp` | `sub_1EA97`, party join code |

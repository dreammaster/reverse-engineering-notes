# Savage Empire: the worktype table of `actor_worktype_act` (`sub_27211`)

From the IDA listing (seg011): 28 worktype numbers at `cs:4433`, 28 handler offsets at `cs:446B` (`jmp word ptr cs:[bx+38h]` at `loc_272FF`). A number that is not in the table is left alone (`loc_274FF`); the objects with
the bit 0x80 of their object status go to the berserk routine before the table (`loc_272CE`).

| worktype | handler | what it calls (guessed from the order of the stubs after `loc_272FF`) |
|---|---|---|
| 4 | `loc_27303` | `sub_2CBA9` |
| 6 | `loc_2730B` | `sub_2CE85` (the berserk fighter) |
| 8, 155 | `loc_2730D` | `sub_2C86D` (the fighter) |
| 14 | `loc_27315` | `sub_2D5A9` |
| 13 | `loc_2731D` | `sub_2D8F9` (the tangle vine) |
| 7, 9, 19, 154 | `loc_27325` | `sub_2D1CA` (the one that runs away) |
| 10 | `loc_2732D` | `sub_2D33E` |
| 11 | `loc_27335` | `sub_2D470` |
| 153 | `loc_2733D` | |
| 131-133 | `loc_27435` | the schedule walkers |
| 130 | `loc_27442` | |
| 12, 143 | `loc_27472` | wander |
| 144, 148 | `loc_2747A` | `sub_26A20` (work) |
| 15, 16, 139-142 | `loc_27488` | nothing |
| 150 | `loc_273E6` | |

# Martian Dreams: the worktype table of `actor_worktype_act` (`sub_29CE2`)

From the IDA listing (seg015): 45 worktype numbers at `cs:32C9`, 45 handler offsets at `cs:3323` (`jmp word ptr cs:[bx+5Ah]`). The worktypes 1, 2, 3 and 5 are not in it
(the party's own, the follower and the formation ones are handled before the table); a number that is not in it is left alone (`loc_2A375`).

| worktype | handler | what it calls |
|---|---|---|
| 4 | `loc_29D88` | `sub_1BB87` (ported; the ranged fighter: keeps three squares from the nearest member, "has no ranged weapons!") |
| 6 | `loc_29D90` | `sub_1BDB5` |
| 8, 0xA0 | `loc_29D92` | `sub_1B936` (the fighter: nearest member in view, best weapon, steps aside when there is no line) |
| 7, 9, 0x1B, 0x9F | `loc_29DA2` | `sub_1C062` |
| 0xA | `loc_29DAA` | `sub_1C1E8` |
| 0xB | `loc_29DB2` | `sub_1C322` |
| 0xC, 0x8F | `loc_29EF8` | `sub_298EF` (wander) |
| 0xD | `loc_2A336` | `sub_1D230` (the vine) |
| 0xE | `loc_29D9A` | `sub_1C4E8` (the stationary fighter) |
| 0xF, 0x10, 0x8B-0x8E | `loc_2A37E` | nothing |
| 0x11 | `loc_2A33F` | `sub_1D7E6` |
| 0x12 | `loc_2A324` | `sub_1C91A` (the thing under the sand) |
| 0x13 | `loc_2A312` | `sub_1C622` |
| 0x14 | `loc_2A32D` | `sub_1CDC2` (the oxium thief) |
| 0x15 | `loc_2A348` | `sub_1D819` (the child) |
| 0x16 | `loc_2A31B` | `sub_1C710` (the maze creature) |
| 0x17 | `loc_2A351` | `sub_1D89E` |
| 0x18 | `loc_2A35A` | `sub_3F650` (the cave worm) |
| 0x19 | `loc_2A363` | `sub_1CB44` (the burrowing worm) |
| 0x1A | `loc_2A36C` | `sub_2AF67` |
| 0x82 | `loc_29EC8` | |
| 0x83-0x85 | `loc_29EBB` | |
| 0x90, 0x94 | `loc_29F00` | `sub_29508` (walk to a place) |
| 0x95 | `loc_29E63` | `sub_2B00E` |
| 0x96 | `loc_29E6C` | |
| 0x98 | `loc_29DBA` | |
| 0x99 | `loc_29F0E` | the stoker: walks north, finds the rack 0x1BE |
| 0x9A | `loc_29F7B` | the stoker: coal on the belt |
| 0x9B | `loc_2A053` | the stoker: waits for the coal to go |
| 0x9C | `loc_2A0D6` | the stoker: walks south to the 0xBF |
| 0x9D | `loc_2A154` | the stoker: throws the coal away |
| 0x9E | `loc_2A1DA` | the stoker: walks north to the switch 0xE9 |

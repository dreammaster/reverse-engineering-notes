# MM3 code overview (work in progress)

Names come from three sources, all in `names/mm3.tsv`: BinDiff vs the Xeen database (>= 0.90 similarity, plus a
hand-picked second tier marked "unverified" in the comment), the Borland runtime (IDA FLIRT), and the game's own strings
("from strings" in the comment).  Nothing here is verified by reading the code unless the comment says so.

## Where things are

| Segment | Observed content |
|---|---|
| `seg000` | Borland C runtime (stdio, conio, time, dos wrappers) |
| `seg002`-`seg004` | game core: `getStat`/`getAge`/`conditionMod` (`16FB1`, `16E14`, `1918F`), a 6.7 KB event interpreter `runMazeEvent` (`19608`), maze/automap code |
| `seg005`, `seg006` | single 8 KB routines (`1E407`, `2045A`) |
| `seg007` | video/sound helpers + a 9 KB data blob (`22B30-24ECC`) |
| `seg008` | resource/file helpers (`fileExists`, `timeToString`, ...) |
| `ovl01` | monsters' combat code (`monstersAttack`, `stopAttack`), intro (`388A9`), `.CC` open (`37EB2`) |
| `ovl02` | one 8.8 KB routine (`3998B`) |
| `ovl03` | party time (`subPartyTime`), maze movement (`3C282`, 2.6 KB), `3D32E` (976 B) |
| `ovl04` | bank interest, arena (`3E113`), death (`3E982`) |
| `ovl05` | awards, roster (`sortParty`, `copyPartyToRoster`), roster menu (`40AAC`) |
| `ovl06` | experience tables (`nextExperienceLevel`, `experienceToNextLevel`, `getCurrentExperience`), rest, control panel |
| `ovl07` | `giveCharDamage`, map loading (`43698`), dismiss |
| `ovl08` | character creation (`46E49`), class checks, level-up teaching |
| `ovl09`-`ovl10` | town buildings (bank, guild, inn, tavern, training, smithy), combat helpers (`4A130 attack`, `4AB68 doMonsterTurn`, ...) |
| `ovl11`-`ovl13` | items dialog, spells (`Spells_*`), `getMaxSP`/`getMaxHP`/`getArmorClass` |

## Facts established by reading code

* The character record's stat block starts at `+14h`: seven (base, current) byte pairs at `+14h`..`+21h` (`getStat`
  `16FB1`, switch of 7); the stat index 5/6/... is adjusted by age (`getAge` tables at DGROUP `0FFAh`) and conditions.
  This matches the layout Xeen uses (to be compared in detail).
* The executable carries few strings: most text lives in `.CC` members (`*.m`, `*.bin`, `text%02u.maz`).

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

## Comparison with the Xeen source (`C:\dev\scummvm\engines\mm\xeen`)

The character code is the same engine as Xeen's `Character` class: `getStat`, `statBonus`, `conditionMod`, `getMaxHP`,
`getMaxSP`, `getArmorClass`, `getCurrentLevel`, `subtractHitPoints` and `itemScan` match the ScummVM C++ line for line
(class numbers too: 2 archer, 3 cleric, 4 sorcerer, 8 druid, 9 ranger).  The tables they use are in DGROUP and are
named in `names/mm3.tsv` (`BASE_HP_BY_CLASS`, `STAT_VALUES`, `AGE_RANGES`, ...).

The full character record (303 bytes) is in `character.h` and as a `Character` struct in the database
(`ida_scripts/apply_structs.py`).  Compared with Xeen it is the same layout up to the skills (`27h`), then: only 26 award
bytes (`39h`), 36 spells (`53h`), the inventory is 18 slots stored as six parallel byte arrays 19 bytes apart
(`7Dh` present, `90h` flags, `0A3h` elemental material, `0B6h` metal, `0C9h` attribute material, `0DCh` item id), conditions
at `113h`, hit points `125h`, spell points `127h`, birth year `129h`, experience `12Bh`.

The event condition code (`ifProc`, `3D32E`) is Xeen's `Scripts::ifProc` with the same action numbers (3 sex, 4 race,
5 class, 8 HP, 9 SP, 10 AC, 11 level bonus, 12 age, 13 skill, 15 award, 16 experience, 18 condition, 19 spell, 25 minutes,
34 gold, 35 gems ...), reading the offsets above.

`MM3.CUR` (and the `*.mm3` save games) is a `.CC` archive (240 members here, TOC cipher as in `mm3-re.md`), i.e. the
working copy of the maze data plus the party state.

## Spells

* The spell **names** are a 79-entry pointer table at DGROUP `56E6h` (ids 0-76 = display order: Light, Awaken, First Aid, Flying
  Fist, ...; `SPLDESC.BIN` descriptions use the same order). Per-class spell lists (ids 0-76 in learn order, 36 entries x 3
  categories: cleric/sorcerer/druid) are at DGROUP `3C36h`. Monster names: 90 pointers at `5632h`, monster picture names at
  `5590h`, locations at `5784h`.
* `castItemSpell` (`4FF60`) is the 77-way switch used for *item* spells; its numbering is Xeen's alphabetical order, not the
  spell ids above. The overlay `ovl11` holds ~88 spell effect routines behind thunks `28450`-`28608`.
* Caution: names that came from BinDiff for very small functions (a few instructions) can be coincidental matches
  (e.g. `Spells_moonRay` at `3E8AC`).

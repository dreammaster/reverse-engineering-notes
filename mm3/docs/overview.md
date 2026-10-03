# MM3 code overview (work in progress)

Names come from three sources, all in `names/mm3.tsv`: BinDiff vs the Xeen database (>= 0.90 similarity, plus a
hand-picked second tier marked "unverified" in the comment), the Borland runtime (IDA FLIRT), and the game's own strings
("from strings" in the comment).  Nothing here is verified by reading the code unless the comment says so.

## Where things are

| Segment | Content |
|---|---|
| `seg000` | Borland C runtime (stdio, conio, time, dos wrappers) |
| `seg001` | `_main` (`14BE3`, copy-protection junk in the prologue) |
| `seg002` | game core: `addTime`/`changeTime`, `statBonus`, `getAge`, `itemScan`, `getStat`, `getCurrentLevel`, `drawParty`, rendering helpers |
| `seg003` | `subtractHitPoints`, `conditionMod`, `runMazeEvent` (event interpreter, `19608`), `moveMonsters` |
| `seg004` | maze page access (`mazeGetWordRel`, `mazeSetBits`, ...), `drawView` (`1B669`), `prepareIndoorView` (`1C195`), `renderIndoorView` (`1E407`, 8 KB) |
| `seg005`/`seg006` | `renderIndoorView` / `renderOutdoorView` (one 8 KB routine each), `drawViewOutdoors` |
| `seg007` | wrappers into the video module (`vdrv_*`), `rnd`, bit helpers, a 9 KB data blob |
| `seg008` | file/resource layer (`ccOpen`, `loadResourceByName`, `loadSavedGame`, `readSaveHeader`, `loadMonsterData`), `getCommand` (input), `currentTime` |
| `seg009`-`seg011` | overlay manager |
| `stub01`-`stub13` | overlay thunks |
| `ovl01` | monsters' combat (`monstersAttack`, `stopAttack`), `introSequence`, `openMm3Cc`, equipment (`equipItem`) |
| `ovl02` | `endingCutscene` (8.8 KB) |
| `ovl03` | `subPartyTime`, `giveTake`, `setValue`, `ifProc` |
| `ovl04` | `exploreLoop` (`3E982`, main command loop), `resetTemps`, `GiveBankInterest`, `arenaEvent`, `showJoke`, `trapOrLockEvent` |
| `ovl05` | awards, roster (`sortParty`, `copyPartyToRoster`, `rosterMenu`), `showMessage`, `setButtons_*` |
| `ovl06` | experience tables, `rest`, `controlPanel`, `loadSaveDialog`, `confirmDialog` |
| `ovl07` | `giveCharDamage`, `Map_load`, `giveTreasure`, `dismissCharacter` |
| `ovl08` | `createCharacter`, `checkClasses`, `trainCharacter` |
| `ovl09` | town buildings (bank, guild, inn, tavern, temple, training, smithy) |
| `ovl10` | combat (`attack`, `doMonsterTurn`, `charSavingThrow`, `doCharDamage`, `hitMonster`, `getWeaponDamage`, `nextChar`, `quickFight`, `run`, `setSpeedTable`, `doCombat`) |
| `ovl11` | the ~70 spell effect routines (`Spell_NN_Name`) |
| `ovl12` | items dialog, `spellsDialog` (`4F74F`), `getThievery`, `bash` (`4EFB6`) |
| `ovl13` | `castItemSpell`, character stat screens, `getMaxSP`/`getMaxHP`/`getArmorClass`/`statColor` |
| `vdrv` | the video/draw module of `MM3.CC` (member 8F99h), added by `load_driver.py` |

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
bytes (`39h`), 36 learned-spell flags (`53h`, indexed by position in the class school list; the game has 77 spells, see spells.md), the inventory is 18 slots stored as six parallel byte arrays 19 bytes apart
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

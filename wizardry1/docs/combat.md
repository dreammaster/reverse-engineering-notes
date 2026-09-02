# Combat  (COMBAT / CINIT / CUTIL / MELEE / SWINGASW)

`P010401 .. P010901`, DOS segments 7-11.  A **core** is ported —
`engine/wiz/combat.{h,cpp}` (data + resolution) and `engine/wiz/combat_ui.cpp`
(the screen + round loop).  Spellcasting and monster spell AI are **not**
ported yet.

## Records

**`TENEMY`** (DOS `ZENEMY`, 94 B / 47 words — Apple's minus the 4 leading
`STRING[15]` names).  Confirmed from the maze/combat p-code + `REWARDS` proc 4:

| word | field | | word | field |
|--:|---|--:|--:|---|
| 0 | `PIC` | | 34 | `DRAINAMT` |
| 1-3 | `CALC1` — group-count dice | | 35 | `HEALPTS` |
| 4-6 | `HPREC` — per-monster HP dice | | 36 | `REWARD1` (0 here) |
| 7 | `CLASS` | | 37 | `REWARD2` → `ZREWARD` index |
| 8 | `AC` | | 38-39 | `ENMYTEAM` / `TEAMPERC` (provisional) |
| 9 | `RECSN` — attacks/round | | 40-41 | `MAGSPELS` / `PRISPELS` |
| 10-30 | `RECS[1..7]` — THPREC damage/attack | | 42 | `UNIQUE` (−1 = encounterable) |
| 31-33 | `EXPAMT` — **0 in this scenario** | | 43-44 | `BREATHE` / `UNAFFCT` |
| | | | 45-46 | `WEPVSTY3` / `SPPC` bits |

A dice roll (`CALCHP` / `ENEMYCNT`): `add + Σ_{dice} (rand mod fac + 1)`.

**`TCHAR` combat tail** (words 87-99, `deriveStats` = `UTILITIE` proc 25):
`HPCALCMD`(87) to-hit mod, `HEALPTS`(89), `CRITHITM`(90), `SWINGCNT`(91),
`HPDAMRC`(92-94) weapon dice, `WEPVSTYP`(98) slay flags, `POISNAMT`(99).
Base unarmed: `HPCALCMD = (fighty ? 2 + lvl/3 : lvl/5) + str-15 (if >15)`;
`HPDAMRC = 2d2 (+str-15)`; `SWINGCNT = 1 (+lvl/5 for fighters)`.

## Encounter build — `CINIT` (`INITGRUP`/`ENGROUPS`)

`buildEncounter(bt, sc, enemyInx, mazeLevel, rng)`: chase `ENMYTEAM` to the
`UNIQUE` record, then group count `= clamp(CALC1 roll, 1, min(9, 4+level))`,
each monster `HP = HPREC roll`.  Multi-group encounters via `ENMYTEAM`
chaining are held back until those offsets are confirmed — the maze's own
`ENCOUNTE`/`ENMYCALC` descriptors are the current source of an encounter.

## Resolution — `DAM2ENMY` / `DAM2ME`

* **Party → monster** (`DAM2ENMY`): to-hit `chance = clamp(21 − monAC −
  HPCALCMD + monAcMod − 3·group, 1, 19)`; for each of `SWINGCNT` swings a hit
  needs `rand%20 ≥ chance`, damage `CALCHP(HPDAMRC)`.  ×2 vs a sleeping
  monster or a slay-flag match.  A `CRITHITM` weapon: `rand%100 < min(50,
  2·lvl)` then `rand%35 > monHD+10` → instant kill.
* **Monster → party** (`DAM2ME`): `chance = clamp(20 − charAC − monHD + 2, 1,
  19)`; `RECSN` attacks, damage `CALCHP(RECS[i])`.  `CASEDAMG` applies `SPPC`
  bits (poison / paralyse / stone, resisted by `LUCKSKIL`), level drain, and
  the SPPC crit.  Monsters favour the front three.

Win → `distributeRewards` (XP from `EXPAMT`, or — since it is 0 here — a
`HD`/reward-tier heuristic; gold scaled by level), split among survivors.
Whole party down → `MazeExit::PartyWiped` (a real `XCEMETRY` scene is TODO).

## UI / flow

`runCombat(Ui&, Party&, Scenario&, StringPool*, Rng&, enemyInx, mazeLevel)`
→ `CombatResult {Won, Fled, PartyWiped, WindowClosed}`.  The round: each
conscious member picks `F)IGHT` (+ group letter if >1) / `P)ARRY` / `R)UN`
(≈75 % escape), then every living monster retaliates.  `wiz1 combat-test
<CHARSET> <SCENARIO.DATA> <monIdx> <keyscript> [ASCII.KRN]` (headless; test
`combat_fight`).  Wired into the maze: an `ENCOUNTE` square or the random
`ENCOUNTR` roll (`rand%99=35` / an unfought room / a kick into a fight
square) calls `runCombat`.

**Not ported:** spellcasting (`CASTASPE` — party and monster), `DOBREATH`,
allied-group summons / `YELLHELP`, item use, the `FRIENDLY` parley, the full
`ZREWARD` item-drop table, and the cemetery scene.

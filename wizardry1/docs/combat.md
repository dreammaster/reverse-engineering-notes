# Combat  (COMBAT / CINIT / CUTIL / MELEE / SWINGASW)

`P010401 .. P010901`, DOS segments 7-11.  Ported —
`engine/wiz/combat.{h,cpp}` (data + resolution + `CASTASPE` spell effects) and
`engine/wiz/combat_ui.cpp` (the screen + round loop + spell menus + monster
spell AI).

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

## Spellcasting — `CASTASPE` (`DOMAGE` proc 28 / `DOPRIEST` proc 27)

`castSpell(bt, sc, party, casterMon, casterLevel, no, tgGroup, tgInst, tgAlly,
rng, log)` applies spell `no` (Wizardry numbering 1-50; mage 1-21, priest
22-50 — matches `Character::spellKnown`).  A 50-entry table (`kTable` in
`combat.cpp`, mirrored by `spellDef()`) gives each spell a `level`, `priest`
flag, `SpTarg` (self / one-enemy / enemy-group / one-ally / party /
all-enemies) and an effect `kind`:

| kind | effect | source proc |
|---|---|---|
| `K_DMG1` | `a`d`b` to one monster, `DOHITS` `UNAFFCT` resist | `DOHITS` |
| `K_DMGG` | `a`d`b` to every monster in the group(s) | `HITGROUP` |
| `K_HEAL` / `K_HEALF` | restore `a`d`b` HP / full | `DOHEAL` |
| `K_SLEEP` / `K_HOLD` / `K_SILENCE` | status vs `ISISNOT` `rand%100` roll | `DOSLEPT`/`DOHOLD`/`DOSILENC` |
| `K_DEATH` | instant kill vs resist (`DI`/`BADI`/`MAKANITO`) | `DOSLAIN` |
| `K_ACSELF` / `K_ACPARTY` / `K_ACPEN` | AC buff (`bt.pAcMod`/`bt.acMod2`) / enemy AC penalty (`grp.acMod`) | `MODAC` |
| `K_CUREPARA` / `K_UNPOISON` / `K_SETHP` | cure paralysis / poison / `MABADI` set-HP | — |

Monster casters: `combat_ui.cpp monsterTurn` — a monster with `MAGSPELS` /
`PRISPELS` > 0 casts (`rand%3==0`) a level-appropriate offensive spell instead
of meleeing; effects land on the party and are resisted by `LUCKSKIL`.

Party casters: `doCast` lists the caster's known spells whose group pool
(`mageSpells` / `priestSpells`, refilled by `setSpells` at rest / combat
start) is > 0, prompts for a target, decrements the pool and calls
`castSpell`.  Buffs (`MOGREF`, `KALKI`, `MAPORFIC`, `DILTO`, …) persist for
the fight via the `Battle` AC-mod fields and feed back into the `DAM2ME`
to-hit.  Test `combat_spell`.

Win → `distributeRewards` (XP from `EXPAMT`, or — since it is 0 here — a
`HD`/reward-tier heuristic; gold scaled by level), split among survivors.
Whole party down → `MazeExit::PartyWiped` (a real `XCEMETRY` scene is TODO).

## UI / flow

`runCombat(Ui&, Party&, Scenario&, StringPool*, Rng&, enemyInx, mazeLevel)`
→ `CombatResult {Won, Fled, PartyWiped, WindowClosed}`.  The round: each
conscious member picks `F)IGHT` (+ group letter if >1) / `C)AST` / `P)ARRY` /
`R)UN` (≈75 % escape), then every living (awake, unparalysed) monster acts.
`wiz1 combat-test <CHARSET> <SCENARIO.DATA> <monIdx> <keyscript> [ASCII.KRN]`
(headless; prints the fight transcript + a `summary:` line; tests
`combat_fight` / `combat_spell`).  Wired into the maze: an `ENCOUNTE` square or the random
`ENCOUNTR` roll (`rand%99=35` / an unfought room / a kick into a fight
square) calls `runCombat`.

**Not ported:** `DOBREATH` (dragon breath), allied-group summons / `YELLHELP`,
item use in combat, the `FRIENDLY` parley, the full `ZREWARD` item-drop table,
and the cemetery scene.  Spell effects are modelled from `DOMAGE`/`DOPRIEST`
behaviour, not yet diffed opcode-for-opcode against `CASTASPE`; a few
utility spells (`DUMAPIC`, `MALOR`, `CALFO`, `LATUMAPI`, `KANDI`) are `K_NOP`.

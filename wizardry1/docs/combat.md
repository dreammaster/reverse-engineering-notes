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
| 8 | `AC` | | 38-39 | `ENMYTEAM` / `TEAMPERC` (allied group) |
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

`buildEncounter(bt, sc, enemyInx, mazeLevel, rng)` = `INITGRUP` + `ENGROUPS`:
for each group, chase `ENMYTEAM` to a record with `UNIQUE ≠ 0`, store it,
then — while the group index (1-based) is `< 4` **and** `≤ MAZELEV` **and**
`rand%100 < TEAMPERC` — recurse to add an **allied group** from that
record's `ENMYTEAM`.  So a level-2 delve can field up to 3 groups, level 3+
up to 4.  `TENEMY` `ENMYTEAM`/`TEAMPERC` = words 38/39 (Apple layout − 32,
confirmed; nearly every WIZ1 monster has an ally).  Per group the count is
`clamp(CALC1 roll, 1, min(9, 4+level))`, each monster `HP = HPREC roll`.
The maze's `ENCOUNTE`/`ENMYCALC` descriptors still pick the *lead* monster;
`combat-test`'s 9th arg sets the maze level for the `TEAMPERC` gate.  Test
`combat_groups`.

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

Monster turn: `combat_ui.cpp monsterTurn` follows `CUTIL`'s action priority
— **spell** (a `MAGSPELS`/`PRISPELS` monster, `rand%3==0`, a
level-appropriate offensive spell resisted by `LUCKSKIL`) → **breath**
(`BREATHE > 0`, `rand%100 < 60` → `DOBREATH`: `HPLEFT/2` to every party
member, halved on a `rand%20 ≥ LUCKSKIL[3]` save) → **yell for help**
(`SPPC[6]`, `<5` alive, `rand%100 < 75` → `YELLHELP`: add a fresh ally
unless the group is 9 or `rand%200 > 10·HD`) → **flee** (`SPPC[5]`, the
party outnumbers the group, `rand%100 < 65` → `DORUN`: the instance leaves,
`MonGroup::fled++` so it earns no XP) → **melee** (`DAM2ME`).  Tests
`combat_breath` / `combat_yell` / `combat_flee`.

`D)ISPEL` (`SWINGASW` `DODISPEL` P010913): offered to a `PRIEST` (any
level), a `LORD` over level 8 or a `BISHOP` over level 3.  Pick a group;
per conscious monster the chance is `50 + 5·casterLevel − 10·monsterLevel`
(`LORD −40`, `BISHOP −20`), and on a hit an **undead** monster (`CLASS 10`)
dissolves — `MonGroup::dispelled++`, removed with no XP (like a flee).  A
non-undead group just prints `TO NO AVAIL!`.  Test `combat_dispel`.

`U)SE` an item (`CUTIL` `USEITEM` P010604): `doUseItem` lists packed items
whose `SPELLPWR` names a spell and which are a `SPECIAL` or equipped, then
invokes that spell for free (targeted per the spell's own targeting — no
pool cost) and rolls `rand%100 < CHGCHANC` to transform the item to
`CHANGETO` (a spent scroll → object 0, `IDENTIF` cleared).  Test
`combat_useitem`.

Party casters: `doCast` lists the caster's known spells whose group pool
(`mageSpells` / `priestSpells`, refilled by `setSpells` at rest / combat
start) is > 0, prompts for a target, decrements the pool and calls
`castSpell`.  Buffs (`MOGREF`, `KALKI`, `MAPORFIC`, `DILTO`, …) persist for
the fight via the `Battle` AC-mod fields and feed back into the `DAM2ME`
to-hit.  Test `combat_spell`.

Win → `distributeRewards` = `giveExp` + `rollTreasure` (REWARDS, below).
Whole party down → `MazeExit::PartyWiped` (a real `XCEMETRY` scene is TODO).

## Rewards — `REWARDS` (`P010D01`, `wiz/rewards.{h,cpp}`)

**`GIVEEXP` / `CALCKILL`** — the record's `EXPAMT` is dead (the source
comments `KILLEXP := ENEMYREC.EXPAMT; LOL`).  XP per kill is built from the
monster's stats, and `mltAdd(n,a)` means `a·2^(n-1)` (a repeated-doubling
`ADDLONGS`):

```
e  = LEVEL·HPFAC · (BREATHE=0 ? 20 : 40)
   + mltAdd(MAGSPELS,35) + mltAdd(PRISPELS,35)
   + mltAdd(DRAINAMT,200) + mltAdd(HEALPTS,90)
   + 40·(11 − AC)
   + (RECSN>1  ? mltAdd(RECSN,30) : 0)
   + (UNAFFCT>0 ? mltAdd(UNAFFCT/10+1, 40) : 0)
   + mltAdd(#WEPVSTY3 bits 1..6, 35) + mltAdd(#SPPC bits 0..6, 40)
```

Each survivor gets `Σ_groups e·killed / aliveCount`.

**`TREWARD`** (ZREWARD, 24 × 168 B): `BCHEST`(w0), `BTRAPTYP`(w1, packed
bool[0..7]), `REWRDCNT`(w2), then `REWARDXX[1..9]` — 9 words each:
`REWDPERC`(0), `BITEM`(1), then a 7-word gold *or* item sub-record
(`TRIES/AVEAMT/MINADD/MULTX/TRIES2/AVEAMT2/MINADD2` vs
`MININDX/MFACTOR/MAXTIMES/RANGE/PERCBIGR`).

**`ENMYREWD`** picks the table from group 0's monster by `attk012`:
`0` → `REWARD1`; `1` → `REWARD1` with **×2 gold** (`ONEORTWO`); `2` →
`REWARD2`.  The maze sets it in `ENCOUNTR` — `0` wandering monster, `1`
set-piece room on first entry, `2` re-fought room / scripted `ENCOUNTE`.

**`GETREWRD`** per entry: skip if `REWDPERC < rand%100`; else
`GOLDREWD` (`gold = calculat(TRIES,AVEAMT,MINADD)·MULTX·calculat(TRIES2,…)·
ONEORTWO`, `calculat(t,a,m)=m+Σ_t(rand%a+1)`) accumulates a pot split by
`GIVEGOLD`; or `ITEMREWD` drops object
`MININDX + calculat(1,RANGE,1) + MFACTOR·bigrolls` into a random conscious
member's pack.

**`ACHEST`** (`combat_ui.cpp runChest`): `GTTRAPTY` rolls the real trap
(`type` 0 trapless / 1 poison / 2 gas / 3 `TRAP3TYP` sub-table / 4 teleport
/ 5 anti-mage / 6 anti-priest / 7 alarm), then `O)PEN` (level/1000 roll),
`I)NSPECT` (agility, ×6 thief / ×4 ninja), `D)ISARM` (type the trap name;
`rand%70 < level − mazeLevel + 50·rogue`), `C)ALFO` (spell 28, priest
group 2), `L)EAVE` (forfeits the treasure entirely).  `DOTRAPDM` =
`springTrap`: HP damage / poison / paralysis / `ANTIPM` stone-or-paralyse
by class; teleport and alarm bubble back to the caller.  Tests
`reward_drop` (+ `attk012` arg on `combat-test`).

## UI / flow

`runCombat(Ui&, Party&, Scenario&, StringPool*, Rng&, enemyInx, mazeLevel,
attk012=2, transcript=nullptr, parleyThresh=-1)` → `CombatResult {Won, Fled,
PartyWiped, WindowClosed, Friendly}`.  `INITATTK` order: `buildEncounter` →
the surprise roll (`rand%100 > 80` → **1** the party surprised the monsters,
a free party round; else a second roll → **2** the monsters surprised the
party, a free monster round; else **0**) → `FRIENDLY` (below).  The round: each
conscious member picks `F)IGHT` (+ group letter if >1) / `C)AST` / `P)ARRY`
/ `R)UN` (≈75 % escape) / `U)SE` an item / `D)ISPEL` (casters only, below),
then every living (awake, unparalysed) monster acts.
`wiz1 combat-test <CHARSET> <SCENARIO.DATA> <monIdx> <keyscript> [ASCII.KRN]
[attk012] [parleyThresh] [mazeLevel] [grants m:i,…]` (headless; prints the
fight transcript + a `summary:` line; tests `combat_fight` / `combat_spell`
/ `reward_drop` / `combat_dispel` / `combat_useitem` / `friendly_leave` /
`friendly_fight`).  Wired into the maze: an `ENCOUNTE`
square or the random `ENCOUNTR` roll (`rand%99=35` / an unfought room / a
kick into a fight square) calls `runCombat`.

## FRIENDLY — the parley  (`COMBAT` P010509)

Runs from `INITATTK`.  Only when the party has a `GOOD` member (`GOODLEAV`).
`z = rand%100`; the encounter is friendly iff `50 ≤ z ≤ thresh`, where
`thresh` is by the first group's monster `CLASS` — fighter 60, mage 55,
priest 65, thief 53, class-4 80, class-7 75, anything else 50 (never).
On a friendly encounter every group is identified and the party is offered
`F)IGHT` / `L)EAVE IN PEACE`.  `L` → `CombatResult::Friendly` (the maze
treats it like a clean exit — no XP, no loot).  `F` → the fight proceeds
and each `GOOD` member has a `rand%2000 == 565` chance to turn `EVIL`.
The weak PC RNG rarely lands `z` in the window on its own, so
`combat-test`'s `parleyThresh` arg overrides `thresh` for the tests.

**Not ported:** spell
effects are modelled from `DOMAGE`/`DOPRIEST` behaviour, not yet diffed
opcode-for-opcode against `CASTASPE`; a few utility spells (`DUMAPIC`,
`MALOR`, `CALFO`, `LATUMAPI`, `KANDI`) are `K_NOP`.  `ENMYREWD`'s `UNIQUE`
decrement/write-back (a killed unique monster becoming its non-unique form)
is skipped — the engine holds `SCENARIO.DATA` read-only.  The chest `ALARM`
re-fight recurses into `runCombat` once rather than looping via `CHSTALRM`.

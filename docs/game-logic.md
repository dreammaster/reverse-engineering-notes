# Game logic — mechanics reference

Reconstructed from the disassemblies (`../*.asm`), verified against DOSBox
traces where marked. This is the consolidated reference for a
reimplementation; the function-by-function pseudo-BASIC lives in
[`../recovered/`](../recovered/) and every formula below cross-references
its `.bas` file and the `*.asm` line range.

**Confidence:** *verified* = matched a DOSBox trace to the bit; *derived* =
falls out of the disassembly + constant pool with no ambiguity; *partial* =
structure known, a runtime-loaded constant or operand order still open.

---

## 1. The arithmetic model (how to read the formulas)

Compiled BASIC 6.0 evaluates every expression on a **software value
stack** (LEGLIB, base `ds:0FAC`, pointer `ds:111C`, 12-byte nodes; a
node's value is at `[node.ptr]`, and the top value is the single at
`[ds:111C]`). All game arithmetic is **single-precision float**; results
assigned to integer variables are truncated (`INT()` in the formulas).

Binary operators dispatch through a table at `ds:0F7C` (read out of
`LEGLIB.EXE`, file offset `5632 + 0x0F7C`):

| thunk(s) | operator |
|---|---|
| `rtm_FF44` / `rtm_FF42` | `+` |
| `sub_21A02` | `-` (a−b) |
| `rtm_FF53` | `-` reversed (b−a) |
| `rtm_FF4E` / `rtm_FF4C` | `*` |
| `rtm_FF47` | `/` (deeper ÷ top) |
| `rtm_FF49` | `/` reversed (top ÷ deeper; as immediate: `TOS / imm`) |
| `rtm_FF2B` | `^` (own thunk `seg004:0x3954`; `TOS ^ TOS1`) |
| `rtm_FF1F` | compare (sets flags for `jb`/`jnb`) |

`\`, `MOD` and the 32-bit ops are separate integer thunks.

`RND(1)` = `push ds:<seg> : push ds:<off> : call rtm_B8` where the arg is
each module's SINGLE constant `1.0` (OUT `ds:24E6`, DUN `ds:2274`, CASDR
`ds:25B0`). Result in `[0, 1)`.

**Named constants** (`ds:xxxx` floats/ints) are rendered `db 0` in the
`.asm` but ARE in the unpacked EXE — `decoders/dgroup_consts.py <EXE>`
reads them. DGROUP:0 is at OUT `0x8C80`, DUN `0x5F00`, CASDR `0x84C0`,
TWNDR `0xAE60`.

**Runtime-only constants** (`0` in the EXE, loaded from map/creature data
at startup) still need a DOSBox dump before a faithful port — collected in
§7.

---

## 2. Character stats

Five attributes, all start at **15**, stored in the CHAR.DAT scalar block
(see [file-formats.md](file-formats.md#chardat)):

| stat | `ds:` | raised by |
|---|---|---|
| Dexterity | `1AC0` | quest rewards; **SDEFENDR** training (DEXTERITY discipline) |
| Endurance | `1ACC` | **SDEFENDR** training (ENDURANCE discipline); potion wizard +5 (cap `0x24`=36) |
| Charm | `1ADE` | Tulip quest +10, Casandra +15 |
| Intelligence | `1AF0` | **STDRV** "Stones of Wisdom" (see below) |
| Strength | `1B08` | pirates cave +10, Armaz +15, third challenge +10; `psychoStrengthSpell` +50 % melee buff (`ds:1AE8`, DUN only) |

**STDRV — Stones of Wisdom** ([`stdrv_dice.bas`](../recovered/stdrv_dice.bas),
constants read from `STDRV.EXE`): a Perudo match; INT changes **once per
match**, keyed on current INT:
- **win** → `+3` (INT < 15) / `+2` (< 30) / `+1` (< 60) / `0` (≥ 60, cap)
- **loss** → `−3` (INT > 49) / `−2` (> 39) / `−1` (> 9) / `0` (≤ 9, floor)

**SDEFENDR — training school** ([`sdefendr_training.bas`](../recovered/sdefendr_training.bas)):
`attribute += (thisSessionScore − yourPreviousBest)` for the chosen
discipline — a worse-than-best run subtracts the difference.

**Character level** `ds:1AE0` (1..10) — raised **only by the museum
caretaker**, never in combat. There is **no experience stat** (the
CHAR.DAT `+0x10` dword long thought to be XP is the **bank balance**).
**Fully solved** — [`mus_caretaker.bas`](../recovered/mus_caretaker.bas):
`useCommand`'s `sub_10B59` re-checks the exhibit-coin-group rank ladder
(§7's `S4(10)`) each visit; the moment your level is *below* the rank
you now qualify for, the caretaker's dialogue (`caretakerOffer`) sets
`ds:1AE0` to that rank directly (`sub_12CAC`, the only place in the game
that writes it) — ranks 1..7 via `caretakerPraise`, rank 8 (all 7 coin
groups cleared) via a final-offer branch that also consumes the
Compendium, sets current HP to 3000, and caps gold at 50000, finalizing
level at **10**.

**Max HP IS a stored field** — `S4(19)` (`ds:1B96` elem `0x13`) —
recomputed by the same `sub_12CAC` every time level changes, **exact
formula**: `maxHP = 200 + 50·L·(L−1) − (100 if L > 5)`. `ds:1ADA` =
current HP; `outInit` copies `S4(19)` into it as the cap `buyFood`
healing respects. Matches every value below exactly:

| level | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 10 |
|---|---|---|---|---|---|---|---|---|
| max HP | 200 | 300 | 500 | 800 | 1200 | 1600 | 2200 | 4600 |

(An earlier note had "L10 3000" — that was the *ascension heal*
`ds:1ADA = 3000` granted at the moment of ascending, not the true max-HP
baseline, which the formula puts at 4600.)

**Equipped gear** — `ds:1AFC` weapon slot, `ds:1AFE` weapon id (0..8,
indexes `Weapon$()`); `ds:1AEA` armour slot, `ds:1AEC` armour id (9..13).
Slots index `S0()` (id) / `S1()` (condition 0..4 = Shoddy/Fair/Good/
Great/Superb).

### ComputeEquippedPower — [`out_combat.bas`](../recovered/out_combat.bas) — *derived* — `out.asm:4337`

```
weaponPower   = INT( weaponId + S1(weaponSlot) / 2.8 )        ' ds:2974
playerDefense = INT( S1(armorSlot) / 3.5 + (armorId - 9) )    ' ds:2970
```

`armorId - 9` = armour tier 0..4. `playerDefense` (`ds:2266`) feeds the
OUT and CASDR incoming-damage formulas. Runs once per encounter.

---

## 3. Combat — THREE different models

Each play module has its own combat math. Do not share code between them.

### 3a. OVERWORLD (OUT) — [`out_combat.bas`](../recovered/out_combat.bas)

Per-encounter setup (`beginEncounterView`, `out.asm` `beginEncounterView`),
from `OUTDAT.DAT` (`decoders/outdat_dat.py`):
- `creatureIndex` = `INT( creatureTypeBase + RND(1) * groupSize )`. A 3-way
  cascade on the encounter gates (§4) picks `(groupSize, creatureTypeBase)`:
  `RND(1) < encGate1` → per-tile band via `combatBeat_1..7`
  (`(4,3) (6,11) (4,17) (4,21) (3,25) (4,28) (4,28) (4,7)` for raw tile
  0..7); else `(4,3)` mid; `RND(1) < encGate2` and `RND(1) < 0.5` →
  `(3,0)` weak band (pixie/strider/farmer).
- `creatureDefense` = `A1(creatureIndex) \ 256` (`ds:21FC`) — the HIGH
  byte of the A1 word (the value `outdat_dat.py` prints as `atk`,
  range ~20–55). Feeds the player's to-hit divisor. — `out.asm:4369`
- `creatureAtk` = `A1(creatureIndex) AND 0xFF` (`ds:2264`) — the LOW byte
  (`outdat_dat.py`'s `HP` column, ~15–200). The monster's damage stat.
  — `out.asm:4340`
- `creatureWeak` = `A2(creatureIndex) \ 256` (`ds:22A6`), **99 = no weak
  weapon** — `out.asm:4389`
- **enemy hit points** (the depletable cell, `viewObjectArray(slot)`) is a
  ROLL from `creatureAtk`, per creature:
  `INT( creatureAtk * (RND(1)/4 + 0.35) * (S4(12) + 2) )` — `out.asm:4442`.
  `S4(12)` is a persistent character word (0 in the current save, so the
  `+2` term = 2; confirm whether anything ever raises it).
- `creaturesToFight` = `INT( r^(3.2·r + 0.83) · groupSize + 1 )`, skewed
  low.

*(Note: `outdat_dat.py`'s `HP` / `atk` column headers are the two A1 bytes;
the game uses the LOW byte as the attack stat and the HIGH byte as the
to-hit defense, i.e. swapped from the header names. The real depletable HP
is the roll above.)*

**Player to-hit** — *verified* (two DOSBox traces) — `out.asm:6687`:
```
hitScratch = Dexterity^0.8 * (weaponPower + 18) / (creatureDefense * 11)
HIT  when  RND(1) < hitScratch
```
The `^0.8` exponent is the constant `ds:2E3A`. A weapon that matches
`creatureWeak` forces `hitScratch = 1.0` (guaranteed hit).
Traces: Dex 16/wp 2/`creatureDefense` 50 → 0.334167 (`0x3EAB17EA`);
Dex 20/wp 8/`creatureDefense` 35 → 0.741885 (`0x3F3DEC2F`).

**Player damage** — *derived* — `out.asm:6830`:
```
base      = INT( Strength * (weaponPower/6 + 0.5) / (2*RND(1) + 1) )
```
Then two branches keyed on the creature's weak weapon:
```
' creatureWeak < 99 AND creatureWeak <> weaponId  (wrong weapon vs a weakness)
chip      = INT( 4*RND(1) + weaponPower/1.3 + 1 )              ' ~1..6

' weaponId = creatureWeak  (the "one blow")
oneShot   = INT( Strength + 20*RND(1) )                        ' Str..Str+19
```
Damage is subtracted straight from the enemy's `viewObjectArray` HP cell.

**Spell attack** — *derived* — `out.asm:6365` (`doAttackOrCast`):
```
fizzle  when  RND(1)*45 > (Intelligence + 20)     ' success = min(1,(INT+20)/45)
spellDmg = INT( (selectedSpell - 22.5) * 15 * (RND(1) + 1) )
```
`selectedSpell` in 23..28 (Seek = 29, handled separately). Casting
decrements the spell's `S2()` charge.

**Monster attack** (`CreatureAttack`) — *derived* (magnitude matched a
trace) — `out.asm:3474` (un-folded from `creatureAttack`):
```
toHitChance = MIN( 0.75, creatureDefense / (Dexterity*2 + 20) )
per creature:  IF RND(1) <= toHitChance THEN it hits
  blow     = creatureAtk * (RND(1) + 0.4) * 1.7
  totalDmg = (totalDmg + 0.5 + blow) * (S4(12) + 2) / (Endurance * (playerDefense + 2))
hitPoints -= INT(totalDmg)
```
Monster hit chance rises with `creatureDefense`, falls with your Dex.
With `S4(12) = 0` the mitigation scale is `2 / (Endurance*(playerDefense+2))`
(heavily damped — a single blow ≈ `creatureAtk*0.7*1.7 * 0.03..0.07`).
Verified magnitude: neural cloud (`creatureAtk` 60) vs PAULA (End 17,
playerDefense 1) → "Damage 3" (formula gives 3.6 → INT 3).

**Death is not a game over** — `out.asm:3698`:
```
"YOU FALL UNCONSCIOUS."
hitPoints = INT( RND(1)*50 + 60 )      ' revive at 60..110
' + a food-penalty roll, + teleport toward the nearest town
```

**Encounter modifier** (`RollEncounterMod`, `rollCreatureStats`) — *derived*
— `out.asm:3723`: `−1000` sentinel most of the time; otherwise
`INT( RND(1)*18 + 12.6 )` (12..30). Consumed by CreatureAttack.

**Creature defeated** — `out_combat.bas` — `out.asm:7023`:
```
rewardTier = A3(creatureIndex) \ 256
' "flesh for food?" offered ~when food < 200 and RND(1) < 0.4:
foodGain = 1 + SUM over creatureCount of  (RND(1)*0.6 + 0.7) * (rewardTier AND 63)
' gold (when a gate passes):
goldFound = INT( (RND(1)*0.6 + 0.7) * creatureCount * (rewardTier AND 63) )
partyGold += goldFound
```
**No experience is awarded** — overworld kills give food / gold / items only.

### 3b. DUNGEON (DUN) — [`dun_combat.bas`](../recovered/dun_combat.bas)

Simpler linear model — no weapon-weakness system, no per-encounter
`creatureHP` scalar.

**Player to-hit** — *derived* — `dun.asm:4017`:
```
HIT  when  RND(1) * 70 < Dexterity + 30      ' hit chance = min(1,(Dex+30)/70)
```

**Player damage** — *derived* — `dun.asm:4081`:
```
dmg = INT( (RND(1) + 0.5) * (Strength + 30) * (weaponPower + 40) / 450 )
' +50% while a spell buff (ds:1AE8) is active
```
`weaponPower` (`ds:21CE`, `updateLevelState` `dun.asm:7638`):
```
weaponPower = weaponId*10 + 10 + (S1(weaponSlot) * 100 \ 28)   ' 99 slot -> no bonus
```
(Great knife id 1 / Great cond 3 → `20 + 10 = 30`.)

**Monster to-hit** — *derived* — `dun.asm:2867`:
```
MISS  when  RND(1) * 70 <= Dexterity         ' hit chance = 1 - Dex/70
```

**Monster damage** — *derived* — `dun.asm:2912`:
```
dmg = INT( (RND(1) + 0.5) * monsterAtk )     ' no *further* mitigation here
```
`monsterAtk` (`ds:2192`, `updateLevelState` `dun.asm:7670`) already bakes in
the player's armour:
```
monsterAtk = (dungeonLevel + 7) * (dungeonNumber - k) * 100 \ (armorDefenseTerm + 30)
armorDefenseTerm = (S1(armorSlot) * 100 \ 35) + armorId*10 - 70
```
so a well-armoured party makes the monsters' rolls smaller at level load.

**Monster special attacks** — [`dun_combat.bas`](../recovered/dun_combat.bas)
— fire ~3% of the time (`ds:24B4 = 0.97`):
- **KNUCKLES** — destroys the equipped weapon (`S0(weaponSlot)=0`)
- **armour-eater** — "THE `<monster>` ATE YOUR `<armour>`" — destroys equipped armour
- **DANGLER** — `Endurance -= INT( RND(1)*3 + 1 )`

**Dungeon spells** — [`dun_spells.bas`](../recovered/dun_spells.bas) —
*derived*. The 6 LEGACY spells map to `S2()` charge slots 24–29; every
cast does `S2(slot) -= 1`. Seek (29) is not implemented in the dungeon
(overworld-only).

The **"M"** command (`useMagicMenu`) shows one 3-row picker
(`rt_FE57`, `firstRow = 9`): row 9 → Magic flame, row 10 → Firebolt
(`selectedSpell = row + 15`), row 11 → **OTHER**. Row 11 is a real
sub-menu only when a Befuddle / Psycho / Kill flash charge is held —
`castSpell` then temporarily zeroes the flame / Firebolt / Seek charges
so the second picker (`selectAbove` mode 4) offers only those three, and
dispatches with `ON (selectedSpell − 25) GOTO` {1 Befuddle, 2 Psycho,
3 Kill flash}. Otherwise row 11 is a "SELECT NO MAGIC" cancel.

| spell | slot | effect |
|---|---|---|
| Magic flame | 24 | attack, `INT( (45/(range+1) + 18)·(RND(1)+1) )` |
| Firebolt | 25 | as Magic flame **×2** + a longer stun |
| Befuddle | 26 | confuse a monster `INT( old\2 + RND(1)·10 + 25 )` turns; at full HP a `0.93` roll **backfires** → *you* are befuddled `~5` turns |
| Psycho strength | 27 | `+50 %` melee for `INT( RND(1)·10 + 20 )` turns (`ds:1AE8`); refunds the charge if already active; cleared on every level change |
| Kill flash | 28 | `clearViewObjects` — wipes **all** visible monsters, no roll |

Attack-spell fizzle (both must pass): `RND(1) ≤ (Int+15)/45` and
`RND(1) ≥ 0.05` — so success rises with Intelligence, ~67 % at Int 15,
~95 % (capped) at Int 30+. `confuseTimer` (`ds:1AE6`) is signed: `> 0`
the monster skips turns, `< 0` "YOU ARE BEFUDDLED."; it steps toward 0
one per turn.

### 3c. CASTLE (CASDR) — [`casdr_castle.bas`](../recovered/casdr_castle.bas)

**Incoming melee** — *derived* — `casdr.asm:4166` (`attackHit`):
```
raw = enemyAtk^1.8 * (RND(1)*600 + 300) * difficulty
dmg = INT( raw / (armorVal * Endurance^0.9) + 2 )
```
Endurance and armour mitigate as a **denominator** term (defensive
scaling, not subtraction). `armorVal` = `armorId - 6` with armour, `2`
without. `difficulty` (`ds:226E`, set by `loadCastleLevel` `casdr.asm:10033`):
**`3.5` inside the castle, `1.0` inside the fort** (constants `ds:31A8` /
`ds:25B0`).

**Player attack** — [`casdr_castle.bas`](../recovered/casdr_castle.bas)
`DoFight` — *derived*. "F"ight then "ENTER DIRECTION:"; a sub-menu can
switch to a spell (`attackMode` `ds:1AEE`). `traceCombatLine` projects
along the direction to find the target.
```
weapon HIT   when  RND(1) < (11·weaponId + 99)·(Dex + 13) / (7500·K)
                   K = Dexterity/26 (castle)  or  1.0 (fort)   [ds:2214]
weapon dmg   INT( base · (1 + 2·RND(1)) ),  base = (weaponId\2 + 1)·Str \ 7
spell cast   succeeds when  RND(1)·6 < Intelligence^0.53   (else FIZZLES)
spell dmg    INT( (selectedSpell − 22.5)·28·(RND(1)+1) ), then \5 in the
             castle, then \range
```
Damage subtracts from `viewObjectArray(tileHit)`; "`<n>` H.P. BLOW",
"`<enemy>` KILLED" at ≤ 0. *(The `K = Dex/26` castle term makes higher
Dex slightly **lower** the hit-rate — either an original quirk or an
`FF49` operand-order mis-read; flagged for a trace, as is the `FF1F`
compare polarity — the reversed reading that made the DUN spell rates
sane is the one used here.)*

**Non-Warlord enemy blow** — `EnemyAttack` (`casdr.asm:5683`) — *partial*:
"`<enemy>` ATTACK - BLOW `<n>`"; 32-bit math from a per-enemy stat
(`ds:20B8`), caller subtracts.

**Warlord blow** — *derived* — `casdr.asm:6034`: `INT( RND(1)*99 + 80 )`
(80..178). Applied by the caller.

**Gas room** — *partial* — `casdr.asm:4376`: `~INT( RND(1)*50 + base )`
per turn spent in a cloudy room.

**Potion wizard** — the CASDR room reward is +5 Endurance / +36 Dexterity
(and the guide's +5 END / +36 DEX quest).

---

## 4. Movement & terrain — [`out_movement.bas`](../recovered/out_movement.bas)

Per overworld step (`DoMovement`, `out.asm` `doMovement`), after committing
the move. The step direction is an `ON facing GOSUB` (`rt_FC`):
`facing` 1/2/3/4 → `trialY-1` / `trialX+1` / `trialY+1` / `trialX-1`;
out-of-range → "bad command".

**Terrain — two independent uses of the raw tile type (0..13):**

*(a) food cost* — `ClassifyLocationTile` (`out.asm` `classifyLocationTile`)
maps the raw tile to `enteredLocationId` ∈ {5,10,15}:

| raw tile | `enteredLocationId` | food / step |
|---|---|---|
| 1, 2, 7–13 | 5 | 0.25 |
| 0, 3, 4, 5 | 10 | 0.50 |
| 6 (forest / swamp) | 15 | 0.75 |

*(b) encounter preset* — `CreatureApproach` runs a separate
`ON (rawTile+1) GOSUB regionPreset_A..E` (`rt_FC`, arms at
`out.asm:0x1009C`) that loads a hardcoded `(encFreq, encGate1, encGate2)`
triple into `ds:208E` / `ds:2092` / `ds:2096`. **This is the whole of the
"region difficulty" system — a 5-row constant table, no per-map data, no
formula.** Table and field-sample confirmation in
[`out_encounter.bas`](../recovered/out_encounter.bas):

| raw tile | preset | `encFreq` (208E) | `encGate1` (2092) | `encGate2` (2096) |
|---|---|---|---|---|
| 0, 2 | B | 0.67 | 0.25 | 0.50 |
| 1 | A | 0.51 | 0.22 | 0.40 |
| 3, 5 | C | 0.90 | 0.35 | 0.55 |
| 4, 6 | D | 1.25 | 0.40 | 0.60 |
| 7 | E | 0.40 | −1.0 | *(kept)* |
| ≥ 8 | — | *(ON GOSUB out of range → all three unchanged)* |

**Per-step tick** — *derived* — `out.asm` `doMovement`:
```
stepCost     = enteredLocationId / 20
food        -= stepCost                       ' ds:1ACE
terrainWear += stepCost                       ' ds:1AF4  (S4(36), fed to bank interest)
stepCounter += 1                              ' ds:1AF8
```

**Food poisoning** — *partial*: gated on `stepCounter` passing 500 plus an
RND roll: `hitPoints -= INT( hitPoints / (3 * (RND(1) + 1)) )`, then
"YOU GROW SICK FROM SOMETHING YOU ATE!".

**Encounter trigger** (`CreatureApproach`) — *derived*:
```
no encounter this step when   encFreq  >  RND(1) * (characterLevel + 9)
```
i.e. per-step chance ≈ `encFreq / (characterLevel + 9)` — roads (encFreq
0.51) are safer than rough terrain (1.25), higher level slightly reduces
the rate. **Comparison polarity is one DOSBox jump-observation from
confirmed** (`creatureApproach` +0x83, the `ja`); the reverse reading
(`encFreq <= RND*(level+9)`) is possible but gives ~90 %/step. A rare
scripted ambush (`RND < 0.06`) needs `S2(15)` held, `stepCounter > 500`,
and level 2..7.

`encGate1` / `encGate2` are then consumed by `BeginEncounterView` as a
3-way cascade choosing the creature band — see §3a and
[`out_encounter.bas`](../recovered/out_encounter.bas).

`ResolveMoveTarget` / `ReadTileObject` are viewport clipping + the 13×13
map-window copy (`array[0x120 + Y*95 + X]` from `ds:1E2A`), not game logic.

### Overworld setup & map loading — [`out_overworld.bas`](../recovered/out_overworld.bas) — *derived*

`EnterOverworld` runs once from `outInit` and again each time a chained
module hands control back. It picks the **map layer** from `S4(12)` (a
"pending transition" slot `resolveMoveTarget` writes after every step):

| `MIN(S4(12), 2)` | file(s) loaded | notes |
|---|---|---|
| 0 | `OUTM0.BSV` | main overworld |
| 1 | `OUTM1.BSV` | secondary area (role unconfirmed) |
| 2 | `OUTM2.BSV` + `PEGASUS.BSV` | pegasus-flight view |

`S4(12) > 2` is a re-entry sentinel → teleport to `(7, 5)` and run the
arrival handler. Layers 1/2 are **one-shot** — `initOverworldViewport`
resets `S4(12)` to 0 after drawing, so the next entry is the main map.

`LoadOverworldData` (`out.asm` `loadOverworldData`) BLOADs, via
`rt_FE63` (resolve drive per `DRCONFIG.DAT` + open) then `rt_FE07`
(BSAVE-header + payload):

```
OUTM<layer>.BSV -> overworldArray[0]          (ds:1E2A)
OUTDATA.BSV     -> overworldArray[0x2B22]
OUTOBJ.BSV      -> spriteBank[0]              (ds:1E58, seg 0x140D)
PEGASUS.BSV     -> spriteBank + spriteBank[8]  (layer 2 only)
```

The **OUTM header** carries the CGA video mode (array word 4) and
colour/palette (word 2), pushed straight to `rt_FE29`.

`EnterOverworld` then resets the per-turn state (`contextMode = 0`,
`locationType = 0x0B` "open overworld", `turnActionFlag <- S4(9)`,
`S4(34) = 0`), clamps the two per-step accumulators (`ds:1AF8`, `ds:1AF4`)
to a **25000** ceiling, and calls `initOverworldState` — which recomputes
`S4(19)` max HP = `200 + 50·L·(L−1) − (100 if L>5)` (same formula as the
museum caretaker, so a level-up takes effect on the next overworld step).

`sub_12823` derives two equipment ratings the overworld combat code reads:
`armourRating = INT( S1(cond)/3.5 + armourId − 9 )`,
`weaponRating = INT( weaponId + S1(cond)/2.8 )`.

**Anti-tamper.** `EnterOverworld` re-opens `OUT.EXE` as a random-access
file, checksums two byte ranges into `ds:2236`, and `sub_116E1` sets
`S4(19) = 20` (max HP collapses to 20) unless the sum is exactly
`0x9D1A`. A copy-protection trap, no gameplay role.

---

## 5. Economy

### Overworld shops — [`out_economy.bas`](../recovered/out_economy.bas) — *derived*

```
shop item price   = INT( RND(1) * characterLevel * 20 + 50 )     ' out.asm:5364
healer restores   = INT( RND(1) * 40 + 30 )  HP  (capped at S4(19))
heal cost         = INT( hpRestored * (RND(1) + 2) / 3 )  gold
' the potion is only offered when hitPoints < 100
```

Prices scale with **party level** — a soft anti-hoarding curve.

### Town bank — [`twndr_services.bas`](../recovered/twndr_services.bas) — *derived*

Balance in `ds:1AC2:1AC4` (the CHAR.DAT "experience" slot). Interest
accrues **per visit** — `twndr.asm:4923`:
```
interest = MIN( 1500, MIN(5000, balance) * daysElapsed \ 999 )
balance += interest
```
`\ 999` is `ds:2C3C`, an **8-byte DOUBLE constant = 999.0** (the bank math
is done in double precision). `daysElapsed` = `S4(36)`, fed from `ds:1AF4`
(the per-step terrain-wear accumulator — a distance-travelled proxy, not
literal days). Deposit / withdraw move gold 1:1.

**Moneylender** (a separate NPC): a flat 50 % loan — borrow 200 → owe 300,
due in ~120 "days". Computed at loan time.

`SpendGold(amount)` = `partyGold -= amount` is the shared vendor helper.

### DUN chests — [`dun_chest.bas`](../recovered/dun_chest.bas) — *derived*

```
chestBase = (10 * dungeonNumber + dungeonLevel) * 20 + 20
goldFound = INT( chestBase * RND(1) + 60 )
partyGold += goldFound
```
A per-level gold high-water mark blocks re-farming. The quest jewel is a
level-7 chest, granted once (`S2(20)` gate).

### Town shops / guard / mail — [`twndr_services.bas`](../recovered/twndr_services.bas) — *derived*

**Buy.** Shop stock is *data, not a formula*: each menu slot's item id /
condition / price is read straight from the town's `TOWN<n>.BSV` record
(`viewObjectArray` `ds:1C7C`, elem offsets `0x48`/`0x5A`/`0x6C`). A
`townType == 0x0C` shop also rolls `RND(1) < 0.3` "…IS NOT FOR SALE".
`partyGold -= price` (32-bit), item drops into the first free inventory slot.

**Sell** (`sub_11F51` → `sub_1200B`, item id `ds:1F04`, condition `ds:1F06`):
```
' weapons (id 0..8)
baseValue = INT( ((weaponId^1.05 + condition/2.8 + 2) ^ 2.1) * 4 - 10 )
' armour  (id 9..13) -- ends on the outer power, NO trailing *m - k
baseValue = INT( (armourId^1.02 + condition/3.5 - 6) ^ 3.2 )
'   (consts ds:2B66 3.2 / 2B6A 1.02 / 2B6E 3.5 / 2B72 -6 ;
'    e.g. Studded hide cond 2 -> ~82, Mythan plate cond 4 -> ~1060)

raw   = INT( baseValue * (Charm ^ 0.7) / 11 )     ' ds:2B76 0.7, ds:2B7A 11
offer = INT( MIN(raw, baseValue) * 0.8 )          ' ds:2878 ; never above base
```
Charm 15 → ~47 % of base, Charm 30 → ~77 % — high Charm haggles better.

**Guard → player** (`guardAttack`, twndr.asm:2507 — castle-style denominator):
```
' auto-arrest instead of fighting when you are strong and outnumbered
IF hitPoints >= 90 AND guardCount > 1 AND ds:2194 > 0 THEN ArrestedByGuards
miss  when RND(1) * 70 < Dexterity                        ' ds:285C
raw       = guardAtk * (RND(1) * 25 + 12)                 ' ds:21A2, ds:2870/2874
armorTerm = 10 * armorId - 50   (or 30 when bare)
dmg       = INT( raw \ (armorTerm ^ 0.8 * Endurance ^ 0.8) ) + 3   ' ds:2878
```

**Player → guard** (`fightGuard`, twndr.asm:1407):
```
base = (weaponId + 2) * Strength \ 8 + 4
dmg  = INT( base * (RND(1) + 0.5) )       ' "GUARD STRUCK n H.P. BLOW" / "GUARD KILLED"
```

**Mail job** (`mailDeliveryJob`, offered by the provisioner):
```
destTown = INT( INT(RND(1)*3) + (currentTown − 1) )     ' ds:2940 = 3.0
' re-rolled while < 0, > 10, or == currentTown
```
so the job **always routes to an adjacent town** (`currentTown ± 1`). It
sets `S4(7) = destTown` (the pending job; `−1` = none) and `S2(9) = 1`
(hold the letter). **Payment is credited on entering the destination
town's provisioner** (`foodShop`, `twndr.asm:2082`):
```
IF S4(7) = currentTown AND locationType ≠ 0x0C THEN
    payment   = INT( INT(RND(1)*3) * 15 + 95 )    ' = 95 / 110 / 125 gold
    partyGold += payment  ;  S4(7) = −1  ;  S2(9) = 0
```
`INT(RND·3)` is 0/1/2 → the three payout tiers (the guide's 95/110/125).
Since every route is ±1 town, the payout is *not* distance-scaled — it is
just the flat random roll.

**Rations** (same `foodShop`, when not delivering mail):
`pricePerDay = INT(13 − Charm/7)·0.1` (≈ 1 gold/day, cheaper with high
Charm); `maxDays = MIN(1000, partyGold / pricePerDay)`. Food itself is
runtime-only (never saved).

**Steal / bribe.** `StealGold` (rob a till): `partyGold += S4(0)` then
`S4(0) *= 0.8` (till refills slower each time), with a `spendGold` fine if
caught. `OfferGuardBribe`: pay `ds:216E` (amount set by the caller), guard
leaves. `ArrestedByGuards`: "THE GUARDS OVERWHELM YOU!" → jail (lose a turn
/ some gold, teleport to the jail tile — not a death).

### Casino — [`gmb_casino.bas`](../recovered/gmb_casino.bas) — *derived*

`GMB1.EXE` (BlackJack) and `GMB2.EXE` (Flip-Flop Parlour) are standalone
EXEs chained from a town casino. Both share only the party-gold dword
(`ds:1AD2:1AD4`) with the caller.

**Break-the-bank cap** (both games): each snapshots your gold at session
start and computes `winCap = 250·characterLevel + 750` (`gmb1.asm:1132`,
`gmb2.asm:148` — `imul ds:1AE0`). After any settled round, if
`gold − startGold > winCap` → *"You broke the bank! The house is closed."*
— you keep everything and it chains back to `TWNDR.EXE`. (Level 1 → cut
off at +1000 net; level 10 → +3250.)

**BlackJack** — bet is *not* escrowed; settled net at round end:

| outcome | Δ party gold |
|---|---|
| loss / bust | `− bet` |
| tie / push | `0` |
| ordinary win / 5-card Charlie | `+ bet` |
| natural BlackJack | `+ 2·bet` |

Broke mid-session → *"Rotten luck. Here's five"* (`+5` pity stake) if
you started with > 9 gold, else *"Come back when you have some gold."*

**Flip-Flop** — ball drops into bucket 1–8; `win = multTable(bucket)·bet`
with `multTable ∈ {1 (buckets 1–2), 2 (3–4), 5 (5–6), 0 (7–8)}`, plus a
colour bonus `INT(win · ds:2AA6)` if the ball's colour matched your call.
**The parlour is rigged** — `computePayout` (`gmb2.asm:4626`) runs before
the ball is scored:
```
ratio = S4(14) / S4(15)          ' realised payback = totalWon / totalWagered
IF ratio > 0.94        THEN reset ledger to 99/99, bucket += 1  ' toward a 7–8 LOSS
ELSEIF ratio < ds:2B40 THEN reset ledger to 99/99, bucket −= 1  ' toward a 1–2 small win
```
so your realised payback is dragged toward ~94 %.

---

## 6. Dungeon traps — [`dun_traps.bas`](../recovered/dun_traps.bas)

Tile feature codes: `0` floor, `1..7` a **hidden** trap
(POISON GAS VENT / FLOOR HOLE / SLIME SPLOTCH / TRIP WIRE / CEILING HOLE /
TREASURE CHEST / BOX), `9..15` = the same trap **revealed / sprung**
(`code + 8`). Search adds 8 to a `1..7` tile.

Walking onto a hidden trap (`dun.asm:1040`, `moveHazards`):
- **FLOOR HOLE (2)** — "YOU FALL THROUGH A HIDDEN HOLE" → fall damage +
  drop one dungeon level. Fall damage `~ INT( (RND(1)*50 + 10) *
  dungeonLevel^1.6 * 0.6 )` (*partial*).
- **every other hidden trap** — "YOU'RE AMBUSHED BY A `<monster>`" → spawns
  a monster ambush.

---

## 7. Quest flags & items — [`out_flags_items.bas`](../recovered/out_flags_items.bas), [`quest_flags.bas`](../recovered/quest_flags.bas)

**Quest flags** = one 16-bit story bitfield, **`questFlagWord` = `S4(11)`**
(`ds:1B96` elem `0x16`), OR-only (never cleared), shared verbatim OUT /
DUN / MUS (TWNDR and CASDR never touch it). **Per-bit meaning is now
solved** — full writeup in [`quest_flags.bas`](../recovered/quest_flags.bas).
Summary:

- The **7 gem coins** (`S2`/`Item$` index 17..23 = Jade/Topaz/Amethyst/
  Sapphire/Turquoise/Ruby/Diamond coin) each map to a fixed bit-mask
  (`0x03`/`0x38`/`0xC0`/`0x0300`/*none*/`0x0800`/`0x1000`); OUT re-fires
  the mask for every coin you currently hold, once per step
  (`setupLocationDisplay`). "Currently hold" only matters the first time
  — the bit stays set forever after, even once the coin is spent.
- **DUN adds two more sources** on dungeon exit (`climbDownOrExit`):
  bit 4 (dungeon 1, only if holding both the Crown **and** the Sapphire
  coin) and bit 8 (dungeon 2, unconditional); bit 11 doubles as "hold
  the Ruby coin" **or** "exit dungeon 3 holding > 3 Guard jewels". The
  same exit also sets a **Strength floor**: `10*dungeonNumber +
  (dungeonNumber > 1 ? 20 : 15)` (25/40/50), only when that dungeon's
  condition just fired.
- **MUS gates on it two ways**: `enterExhibit`'s `exhibitId` (0..13, 14
  named display cases) maps via a threshold staircase to `chainTargetIdx`
  17..23, which **is** the required coin's index directly (Jade covers
  exhibits 0-3, Topaz 4-6, Amethyst 7-8 *(Stones of Wisdom = 7)*, Sapphire
  9-10, Turquoise 11, Ruby 12, Diamond 13 — verified against the Amethyst/
  Stones-of-Wisdom save-diff). Separately, `testExhibitFlag` does an
  **ALL-BITS-SET** test (`flagWord AND mask = mask`, not "any bit") to gate
  a *rank ladder* (`S4(10)`, 8 arms) that progressively unlocks each coin
  group — several ranks require the *previous* rank's bits too (Topaz's
  gate is `0x2B` = Jade's bits plus part of Topaz's own), and two ranks
  (Turquoise, Diamond) gate on a plain counter/item check instead of the
  bitfield at all.
- Bit `0x2000` = "already took the caretaker's final offer" — **and it is
  never set** (a latent bug; no gameplay effect). Every one of MUS's 16
  exhibit slots is *meant* to get its own permanent bit via `sub_11C38`
  (`questFlagWord OR= 2^(exhibitId−1)` — verified exact against 5 known
  coin bits; `eatFruitCommand` inlines the literal `or ax,8` for exhibit
  4). Exhibit 14 ("INFORMATION" — the caretaker's desk, handler
  `checkFlag_2000`) would be `2^13 = 0x2000`, but `checkFlag_2000` omits
  the `sub_11D02` tail call that every other exhibit makes; its only
  path to the setter is a dead "bit already set" branch. So
  `checkFlag_2000` always routes to `caretakerOffer`, the "already done"
  message is unreachable, and since the level-up logic is monotonic that
  has no consequence. Exhaustively verified — the whole game has exactly
  4 writers of `questFlagWord`, and every function in the caretaker call
  graph was read end-to-end. Details: [`quest_flags.bas`](../recovered/quest_flags.bas) §3c.

**Found items** (`AwardFoundItem`, `out.asm:8315`): `S2(droppedItemId)
+= 1`, "YOU FIND A `<item>`". `droppedItemId` = `ds:1AEE`.

---

## 8. Constants — all resolved

The formulas above once had ~7 "runtime-loaded" constants (`0` in the EXE
at a naïve read). Every one turned out to be static — an `OUTDAT.DAT`
byte, a `*.EXE` constant read at the wrong width, or a formula. **No
DOSBox dump is needed to implement the mechanics.**

| earlier "runtime" symbol | resolution |
|---|---|
| `ds:21FC` (player to-hit divisor) | `A1(creatureIndex) \ 256` — the HIGH byte |
| `ds:2264` (monster attack stat) | `A1(creatureIndex) AND 0xFF` — the LOW byte |
| `S4(12)` (CreatureAttack mitigation) | a persistent character word — `0` in the current save |
| `ds:21CE` / `ds:2192` (DUN) | `updateLevelState` formulas (§3b) |
| `ds:226E` (CASDR difficulty) | `3.5` castle / `1.0` fort — constants `ds:31A8` / `ds:25B0` |
| `ds:2C3C` (bank divisor) | an 8-byte DOUBLE = `999.0` (a 32-bit read misses it) |

| `ds:2092` / `ds:2096` (OUT encounter gates) | a 5-row constant table (`regionPreset_A..E`) keyed on the raw map tile — see §4. Not per-map, not runtime. Field samples match the table exactly. |

The `ds:2092` / `ds:2096` hunt is closed: `fix_on_gosub_tables.py` decoded
the `ON (rawTile+1) GOSUB` in `creatureApproach`, and the five arms
(`out.asm:0x1009C`) store hardcoded triples pulled from `ds:246E..249A`.
The earlier field samples (museum 0/0, west 0.22/0.40, NW 0.35/0.55) are
presets *(none)* / A / C respectively.

Also open (not blocking): the encounter-trigger comparison polarity (one
DOSBox jump obs), `CastSpell` (DUN), the casino payout math, the per-bit
quest-flag semantics.

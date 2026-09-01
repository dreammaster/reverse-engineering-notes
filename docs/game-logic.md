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
| Dexterity | `1AC0` | quest rewards |
| Endurance | `1ACC` | SDEFENDR training (`13+4x` / `14+4x` / `15+4x` per wave of level *x*), potion wizard +5 (cap 28) |
| Charm | `1ADE` | Tulip quest +10, Casandra +15 |
| Intelligence | `1AF0` | Stones of Wisdom: if INT < 30 a win gives **+2** else **+1**, a loss **−1** (cap 28); potion wizard +5 |
| Strength | `1B08` | pirates cave +10, Armaz +15, third challenge +10 |

**Character level** `ds:1AE0` (1..~10) — raised **only by the museum
caretaker**, never in combat. There is **no experience stat** (the
CHAR.DAT `+0x10` dword long thought to be XP is the **bank balance**).

**Max HP is a level lookup**, not a stored field (from the Apple guide,
applies to DOS): L1 200, L2 300, L3 500, L4 800, L5 1200, L6 1600,
L7 2200, L10 3000. `ds:1ADA` = current HP.

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

Per-encounter setup:
- `creatureIndex` = `INT( RND(1) * range + base )`, `(base,range)` picked
  by RND gates: `(3,4)` mid-tier, `(0,3)` weak (pixie/strider/farmer),
  higher tiers late — `out.asm:3863` (`beginEncounterView`).
- `creatureHP` = `A1(creatureIndex) MOD 256` (`ds:21FC`) — `out.asm:3924`
- `creatureWeak` = `A2(creatureIndex) MOD 256` (`ds:22A6`), **99 = no
  weak weapon** — `out.asm:3944`
- `creaturesToFight` = `INT( r^(3.2·r + 0.83) · groupSize + 1 )`, skewed
  low — `out.asm:3095`

**Player to-hit** — *verified* (two DOSBox traces) — `out.asm:6687`:
```
hitScratch = Dexterity^0.8 * (weaponPower + 18) / (creatureHP * 11)
HIT  when  RND(1) < hitScratch
```
The `^0.8` exponent is the constant `ds:2E3A`. A weapon that matches
`creatureWeak` forces `hitScratch = 1.0` (guaranteed hit).
Traces: Dex 16/wp 2/cHP 50 → 0.334167 (`0x3EAB17EA`); Dex 20/wp 8/cHP 35
→ 0.741885 (`0x3F3DEC2F`).

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

**Monster attack** (`CreatureAttack`) — *partial* — `out.asm:3474`
(un-folded from `creatureAttack`):
```
toHitChance = MIN( 0.75, creatureHP / (Dexterity*2 + 20) )
per creature:  IF RND(1) <= toHitChance THEN it hits
  blow     = creatureAtk * (RND(1) + 0.4) * 1.7
  totalDmg = (totalDmg + 0.5 + blow) * (S4(12) + 2) / (Endurance * (playerDefense + 2))
hitPoints -= INT(totalDmg)
```
Monster hit chance rises with the **creature's** HP, falls with your Dex.
The running total is re-scaled by the mitigation term on every hit
(*partial*: `S4(12)` is a runtime region word; with it ≈ `Endurance*(pd+2)`
the scale is ≈ 1). `creatureAtk` = `ds:2264`, rolled at encounter start.

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

**Player damage** — *partial* — `dun.asm:4081`:
```
dmg = INT( (RND(1) + 0.5) * (Strength + 30) * (weaponPower + 40) / 450 )
' +50% while a spell buff (ds:1AE8) is active
```
(*partial*: `weaponPower` = `ds:21CE`, DUN's own equip calc; confirm the
`/450` and `+40` with a trace.)

**Monster to-hit** — *derived* — `dun.asm:2867`:
```
MISS  when  RND(1) * 70 <= Dexterity         ' hit chance = 1 - Dex/70
```

**Monster damage** — *derived* — `dun.asm:2912`:
```
dmg = INT( (RND(1) + 0.5) * monsterAtk )     ' NO armour / Endurance mitigation
```

**Monster special attacks** — [`dun_combat.bas`](../recovered/dun_combat.bas)
— fire ~3% of the time (`ds:24B4 = 0.97`):
- **KNUCKLES** — destroys the equipped weapon (`S0(weaponSlot)=0`)
- **armour-eater** — "THE `<monster>` ATE YOUR `<armour>`" — destroys equipped armour
- **DANGLER** — `Endurance -= INT( RND(1)*3 + 1 )`

### 3c. CASTLE (CASDR) — [`casdr_castle.bas`](../recovered/casdr_castle.bas)

**Incoming melee** — *partial* — `casdr.asm:4166` (`attackHit`):
```
raw = enemyAtk^1.8 * (RND(1)*600 + 300) * difficulty
dmg = INT( raw / (armorVal * Endurance^0.9) + 2 )
```
Endurance and armour mitigate as a **denominator** term (defensive
scaling, not subtraction). `armorVal` = `armorId - 6` with armour, `2`
without. (*partial*: `difficulty` = `ds:226E`, runtime — it dominates
the magnitude.)

**Warlord blow** — *derived* — `casdr.asm:6034`: `INT( RND(1)*99 + 80 )`
(80..178). Applied by the caller.

**Gas room** — *partial* — `casdr.asm:4376`: `~INT( RND(1)*50 + base )`
per turn spent in a cloudy room.

**Potion wizard** — the CASDR room reward is +5 Endurance / +36 Dexterity
(and the guide's +5 END / +36 DEX quest).

---

## 4. Movement & terrain — [`out_movement.bas`](../recovered/out_movement.bas)

Per overworld step (`DoMovement`, `out.asm:1427`), after committing the move:

**Terrain classification** (`ClassifyLocationTile`, `out.asm:11858`) — the
tile-object type (0..13) maps to `enteredLocationId`, which IS the
terrain cost:

| tile type | `enteredLocationId` | food / step | encounter |
|---|---|---|---|
| 1, 2, 7–13 | 5 | 0.25 | most likely |
| 0, 3, 4, 5 | 10 | 0.50 | medium |
| 6 (forest / swamp) | 15 | 0.75 | least likely |

**Per-step tick** — *derived* — `out.asm:1507`:
```
stepCost     = enteredLocationId / 20
food        -= stepCost                       ' ds:1ACE
terrainWear += stepCost                       ' ds:1AF4
stepCounter += 1                              ' ds:1AF8
```

**Food poisoning** — *partial* — `out.asm:1550`: gated on the step
counter passing 500 plus an RND roll ("eaten questionable flesh and
walked far"): `hitPoints -= INT( hitPoints / (3 * (RND(1) + 1)) )`, then
"YOU GROW SICK FROM SOMETHING YOU ATE!".

**Encounter trigger** (`CreatureApproach`, `out.asm:2943`) — *derived*:
```
an encounter fires this step when
    (enteredLocationId / 20)  <=  RND(1) * (characterLevel + 9)
```
so rougher terrain is safer and higher level is more dangerous. A rare
special encounter (`RND < 0.06`) needs `S2(15)` held, `stepCounter > 500`,
and level 2..7.

`ResolveMoveTarget` / `ReadTileObject` (`out.asm:9973` / `10468`) are
viewport clipping + the 13×13 map-window copy (`array[0x120 + Y*95 + X]`
from `ds:1E2A`), not game logic.

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

### Town bank — [`twndr_services.bas`](../recovered/twndr_services.bas) — *partial*

Balance in `ds:1AC2:1AC4` (the CHAR.DAT "experience" slot). Interest
accrues **per visit** — `twndr.asm:4923`:
```
interest = MIN( 1500, MIN(5000, balance) * daysElapsed / K )
balance += interest
```
Deposit / withdraw move gold 1:1. (*partial*: `K` = `ds:2C3C`, runtime —
~1000 per the guide's "1 gold per 1000 per day".) `SpendGold(amount)` =
`partyGold -= amount` is the shared vendor helper.

### DUN chests — [`dun_chest.bas`](../recovered/dun_chest.bas) — *derived*

```
chestBase = (10 * dungeonNumber + dungeonLevel) * 20 + 20
goldFound = INT( chestBase * RND(1) + 60 )
partyGold += goldFound
```
A per-level gold high-water mark blocks re-farming. The quest jewel is a
level-7 chest, granted once (`S2(20)` gate).

### Casino / mail — *not yet located*

GMB1 (BlackJack) / GMB2 (Flip-Flop Parlour) payouts scale by
`imul ds:1AE0` (character level). Mail routes cost 95 / 110 / 125 gold
per the guide; not yet found in `twndr.asm`.

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

## 7. Quest flags & items — [`out_flags_items.bas`](../recovered/out_flags_items.bas)

**Quest flags** = one 16-bit story bitfield, **`S4(11)`** (`ds:1B96` elem
`0x16`). The overworld `SetFlag_*` family ORs a fixed mask into it
(`0x03` / `0x38` / `0xC0` / `0x0300` / `0x0800` / `0x1000`); MUS's
`testExhibitFlag` masks the same word (`0x03` / `0x2B` / `0xD0` /
`0x0300` / `0x0800` / `0x1000` / `0x2000`) so exhibits react to overworld
progress. **Per-bit meaning is still an open cross-module trace.**

**Found items** (`AwardFoundItem`, `out.asm:8315`): `S2(droppedItemId)
+= 1`, "YOU FIND A `<item>`". `droppedItemId` = `ds:1AEE`.

---

## 8. What still needs a DOSBox dump

Every runtime-loaded constant below reads `0` in the EXE (set from
map / creature / region data at startup). A faithful port needs their
real values — one memory dump each while the relevant screen is active:

| symbol | where | what it gates |
|---|---|---|
| `ds:2092` / `ds:2096` (OUT) | `loadOverworldData` | encounter weak/tier gates, per map |
| `ds:2264` (OUT) | encounter start | `creatureAtk` in CreatureAttack |
| `S4(12)` (OUT) | region | the CreatureAttack mitigation scale |
| `ds:2192` (DUN) | per monster | `monsterAtk` |
| `ds:21CE` (DUN) | equip | DUN `weaponPower` |
| `ds:226E` (CASDR) | level load | castle `difficulty` (dominates incoming damage) |
| `ds:2C3C` (TWNDR) | bank | interest divisor `K` |
| operand order | any | `-` / `/` where `'?ord` is tagged in the `.bas` files |

Also open: `CastSpell` (DUN), `enterOverworld` / `loadOverworldData`
(OUT), the casino payout math, mail routes, and the per-bit quest-flag
semantics.

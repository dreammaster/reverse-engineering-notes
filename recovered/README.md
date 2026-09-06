# `recovered/` — pseudo-BASIC reconstruction of the compiled modules

Legacy of the Ancients was written in **Microsoft BASIC Compiler 6.0** and
linked against the shared `LEGLIB.EXE` runtime. These files reconstruct the
original source, function by function, from the disassembly under
`../*.asm`. They are a **reading and review aid** — they will not compile,
and they deliberately keep the game's structure (one file per `.EXE`
module, calling a shared `leglib` library) rather than pre-flattening it
for the C++/ScummVM port.

**For the reimplementation, read [`../docs/game-logic.md`](../docs/game-logic.md)
first** — it consolidates every verified/derived formula from these files
with confidence levels. Every constant is resolved; no DOSBox dumps are
needed to implement the mechanics. These `.bas` files are the
function-level source of truth behind it.

## Confidence markers

Every non-trivial line is tagged:

| tag | meaning |
|---|---|
| *(no tag)* | mechanical translation, high confidence |
| `'?ord` | operator *identity* is known, but the operand **order** for a non-commutative op (`- / \ MOD ^`) is not — `A op B` vs `B op A` |
| `'?op` | the operator itself is still inferred (rare now that the dispatch indices are known) |
| `'??` | the whole expression / branch is a best guess |
| `'CHECK` | needs a DOSBox memory-watch to confirm |

Each reconstructed block ends with `' asm: out.asm:NNNN` pointing at the
source lines.

## The `leglib` value-stack model

Arithmetic in compiled BASIC 6.0 goes through a runtime value stack
(`ds:111C` = stack pointer, 12 bytes per slot). The disassembly is a
stream of `int 3Fh` thunks; this table is how to read them back as infix:

| thunk | leglib | meaning |
|---|---|---|
| `rtm_FF20(ax)` | `B$PUSHI` | push `ax` as an INTEGER |
| `rtm_FF4B(addr)` | push | push the **SINGLE** at `[addr]` (also used right after `B$RND` with `addr` = the pointer `B$RND` returned) |
| `rtm_FF50(addr)` | pop | pop top → `[addr]` |
| `rtm_FF22` | pop→AX | pop top into `ax` (end of expression) |
| `rtm_FF2B` | — | INT → SINGLE coercion |
| `rtm_FF27` | — | SINGLE → INT/LONG coercion (truncates) |
| `rtm_B8(seg,off)` | `B$RND` | **`RND(x)`** — `push ds:24E8 : push ds:24E6 : call B$RND`; `ds:24E6` is the SINGLE `1.0`, so this is `RND(1)` = next random in `[0,1)`. Returns a pointer to the result single (→ fed to `rtm_FF4B`). 24-bit LCG, `seed = seed*214013 + 2531011` (constants at `ds:1AE`/`ds:1B2`). |
| `rtm_FC` | `B$` ON-GOSUB | **`ON <bx> GOSUB/GOTO ...`** — 1-based selector in `bx`; inline `db count` + `dw arm[]` follow the `call`; out-of-range = no-op. NOT the RNG. `ida_scripts/fix_on_gosub_tables.py` decodes the tables. |

See **[`leglib_runtime.c`](leglib_runtime.c)** for the hand-written runtime
(value stack, RNG, ON-GOSUB, text codes, BSAVE) written out as C — the port
should call the C equivalents, not reimplement any of it as BASIC.

**Arithmetic ops** — each thunk loads an index into `bx`, then
`call word ptr [bx+0F7Ch]` (leglib `rtm_FF44` body, `seg004:21A69`). The
`[ds:0F7C]` table is runtime-filled and shows as `db 0` in `leglib.asm`,
**but it is present in `LEGLIB.EXE`** at file offset `5632 + 0x0F7C`
(leglib DGROUP:0 == file 5632). Reading it back and disassembling the 8
handlers (`seg004:0x2CA2 / 0x2C9A / 0x2C98 / 0x2E04 / 0x2E8A / 0x2E88 /
0x2BF3 / —`) gives the **actual** table, which is *not* the canonical
BASIC order — there is no `\`, `MOD`, or `^` in it:

| index | op | how the handler works | immediate thunk | stack-stack thunk |
|---|---|---|---|---|
| `0x00` | **`+`** | add | `rtm_FF44` | `rtm_FF42` |
| `0x04` | **`-`** (a−b) | negates b, then add | `sub_21A02` | — |
| `0x08` | **`-` reversed** (b−a) | swap, negate, add | `rtm_FF53` | — |
| `0x0C` | **`*`** | XOR signs, **add exponents** | `rtm_FF4E` | `rtm_FF4C` |
| `0x10` | **`/`** (TOS1÷TOS) | XOR signs, **sub exponents** | `sub_21A4A` | `rtm_FF47` |
| `0x14` | **`/` reversed** (TOS÷TOS1) | swap, then `0x10` | `rtm_FF49` | — |
| `0x18` | **compare** | ordered float compare | — | — |
| `0x1C` | **compare** | via `[ds:0F78]`, munges flags for `jb`/`jnb` | — | `rtm_FF1F` |

So: `FF4C`/`FF4E` = **`*`** (not `/`), `FF47` = **`/`**, `FF49` = **`/`
with operands reversed** (as an immediate op: `TOS / imm`), `FF53` = **`-`
reversed**. Verified against two independent expressions: `rollCreatureStats`
= `INT(RND*18 + 12.6)` (a 12–30 roll — `/` there would be a constant), and
OUT combat damage = `INT(Str*(wp/6 + 0.5)/(2*RND + 1))` (matches Paul's
DOSBox "BLOW OF 6").

**`\`, `MOD`, `^`, and the relational ops** are separate thunks with their
own leglib handlers, *not* entries in this table. `rtm_FF2B`
(-> `seg004:0x3954`) = **`^` with operands reversed** (`TOS ^ TOS1`): it
**pops two operands** (binary, not the coercion the earlier notes assumed).
Confirmed by an exact float match — OUT to-hit `Dex ^ (Dex/(wp+18)) * (wp+18)
/ (creatureHP*11)` reproduces the observed `ds:208E` = `0x3EAB17EA` bit for
bit (`16^0.8 * 20 / 550 = 0.33416682`).

## Value-stack node layout

Each 12-byte node is `[8 bytes unused][2-byte pointer][2-byte type tag]`.
**The node's value is stored at `[pointer]`**, not inline — and `pointer`
is `nodeStart + 12`, i.e. it points into the next node's space. So:

- the **top-of-stack value is the 4-byte single at `[ds:111C]`** (the stack
  pointer itself), *not* at `[ds:111C]-12`.
- `rtm_FF4B` (push) writes the value to `[oldPtr+12]`, sets the new node's
  pointer field to `oldPtr+12`, tag to 3, advances `ds:111C` by 12.
- `rtm_FF50 addr` (pop→var) does `si = topNode.pointer ; movsw movsw` from
  `ds:si` to `ds:addr`, then `ds:111C -= 12`.

When dumping the value stack in DOSBox, read `[ds:111C]` for the top value
and `[ds:111C - 12*k]` for the k-th-from-top **node header** (its value is
at that header's pointer field).

**Operand order — resolved:** the verified OUT to-hit
(`FF47` = deeper ÷ top) and damage (`FF49` imm = `TOS ÷ imm`) traces pin
it. `FF49` vs `FF47` / `FF53` vs `sub_21A02` already encode the two
directions of `/` and `-`.

**String / misc thunks:** `basStrAssign(dst,src)` = `dst$ = src$`;
`basStrConcat(a,b)` = `a$ + b$`; `rtm_D2(fmt,val)` = `fmt$ + STR$(val)`;
`drawString` / `drawStringInner` render a position-coded string to CGA;
`rtm_FE27(addr)` = pause `[addr]` ticks / wait for key.

## The DGROUP constant pool

Every module shares the LEGLIB DGROUP layout: LEGLIB scratch in the low
`~0x1AC0` bytes (value stack `ds:111C`, op table `ds:0F7C`), then the
module's own SINGLE / INTEGER / STRING constants and variables. IDA's
`.asm` export writes the whole DGROUP as `db 0`, so the constant *values*
are missing from `../*.asm` — but they are in the unpacked EXE image.
`decoders/dgroup_consts.py <MODULE.EXE>` reads them back (DGROUP:0 sits at
file offset `0x8C80` in `OUT.EXE`, found via the `"ENEMY HIT BY BLOW OF "`
anchor). The combat pool it prints is quoted at the top of
`out_combat.bas`.

## Model — done

- [x] `decoders/dgroup_consts.py` — pull the SINGLE/INT/STRING constant
      pool out of any module EXE
- [x] leglib op-dispatch table read from `LEGLIB.EXE` (`+ - -rev * / /rev
      cmp`); `FF2B` = `^` reversed
- [x] value-stack node layout (value at `[node.ptr]` == `[ds:111C]` for top)

## MENU.EXE / SAVER.EXE

- [x] `menu_saver.bas` — launcher flow, roster / new-game, and the save.
      **Starting stats** (from the LEGACY.DAT new-character template, bytes
      `0xA03..`): STR/DEX/END/INT/CHARM all **15**, HP **200**, level 1,
      **0 gold**, Studded-hide armour, bare hands.  SAVER = a byte-image of
      resident DGROUP `ds:1AC0..1B08` + the 7 arrays → CHAR.DAT record
      `ds:1B0A`.

## STDRV.EXE — Stones of Wisdom

- [x] `stdrv_dice.bas` — a Liar's-Dice / Perudo match vs a dealer.  Winning
      a **match** raises **Intelligence** (`+3/+2/+1/0` under Int 15/30/60),
      losing lowers it (`-3/-2/-1/0` over Int 49/39/9) — self-balancing,
      cap 60, floor ~9.  Delta tables read from `STDRV.EXE`.  The dealer
      bid/challenge AI is summarised (odds constants still open).

## SDEFENDR.EXE — the Training School

- [x] `sdefendr_training.bas` — an arena shooter.  The ENDURANCE and
      DEXTERITY disciplines set `attribute += (sessionScore − yourPrevBest)`
      for that discipline (a worse run subtracts the margin).  ARMOR /
      WEAPON / BLOCK / SHOOT effects still open.

## GMB1.EXE / GMB2.EXE — casino

- [x] `gmb_casino.bas` (v2) — Blackjack (`GMB1`) net settlement
      (`−bet` loss, `+bet` win / 5-card, `+2·bet` natural, `0` tie; bet
      not escrowed; `+5` pity stake when broke) and Flip-Flop (`GMB2`, a
      rigged Plinko: buckets pay 1×/2×/5× the bet + a colour bonus, but
      `computePayout` nudges the landing bucket ±1 to drag realised
      payback `S4(14)/S4(15)` toward ~0.94).  **Both** end the session
      (*"You broke the bank!"* → `TWNDR`) once `gold − startGold >
      250·characterLevel + 750` (`imul ds:1AE0`).  Only move party gold
      (`ds:1AD2:1AD4`) + `GMB2`'s `S4(14)/S4(15)` ledger.  Open: the
      flip-flop lower band `ds:2B40` / colour const `ds:2AA6`.

## CELDRV.EXE / CONFIGUR.EXE

- [x] `misc_drivers.bas` — CELDRV = the "AGAINST ALL ODDS!" endgame
      cinematic (CEL0-3/DIS9 image banks + a 999-line victory crawl + the
      credits).  CONFIGUR = a small **C** utility that edits the drive
      letters in DRCONFIG.DAT.  No game mechanics in either.

## OUT.EXE

- [x] `out_combat.bas` — `RollEncounterMod`, `ComputeEquippedPower`,
      `SpellAttack`, `ResolvePlayerAttack`, `CreatureDefeated`,
      `CreatureAttack`. All melee/spell formulas verified or derived.
      to-hit = `Dex^0.8 * (weaponPower+18) / (creatureDefense*11)` —
      confirmed by two DOSBox traces (`^0.8` is the constant `ds:2E3A`).
      `creatureDefense` = `A1(creature) \ 256`; `creatureAtk` (monster
      damage) = `A1(creature) AND 0xFF`.
- [x] `out_encounter.bas` — `CreatureApproach`, `BeginEncounterView`.
      `ds:2092`/`ds:2096`/`ds:208E` = a 5-row per-tile constant table
      (`regionPreset_A..E`, `ON (rawTile+1) GOSUB`), **fully recovered** —
      no per-map data.
- [x] `out_economy.bas` — `ProvisionerShop`, `ShopConfirmBuy` (prices)
- [x] `out_movement.bas` — `DoMovement` (per-step tick + food-poisoning)
      and `ClassifyLocationTile` (the terrain-cost classifier: tile type
      -> enteredLocationId 5/10/15 -> food per step 0.25/0.5/0.75 AND the
      encounter gate).  `ResolveMoveTarget`/`ReadTileObject` are viewport
      clipping, not game logic.
- [x] `CreatureAttack` (un-folded; out_combat.bas v7)
- [x] `out_flags_items.bas` — `ApplyGameFlag` + `SetFlag_*` (the story
      bitfield `S4(11)`, shared with MUS), `AwardFoundItem`
      (`S2(droppedItemId) += 1`)
- [x] `quest_flags.bas` — **the full cross-module quest-flag semantics**
      (OUT sets, DUN sets + tests, MUS tests): the 7 gem coins each map to
      a fixed bit; DUN's dungeon-exit side quests add 2 more bits + a
      Strength floor (25/40/50); MUS's `exhibitId -> chainTargetIdx`
      staircase IS the required coin's index; `testExhibitFlag` is an
      ALL-BITS-SET test driving a progressive per-coin unlock ladder.
- [x] `out_overworld.bas` — `EnterOverworld` / `LoadOverworldData` (+
      `initOverworldViewport` / `initOverworldState` / `sub_12823` /
      the OUT.EXE self-checksum). Map layer = `MIN(S4(12), 2)` picks
      `OUTM0/1/2.BSV` (layer 2 also loads `PEGASUS.BSV`); `OUTDATA.BSV`
      → `overworldArray[0x2B22]`, `OUTOBJ.BSV` → `spriteBank[0]`. The
      OUTM header carries the CGA mode/palette (→ `rt_FE29`).
      `initOverworldState` recomputes `S4(19)` max HP =
      `200 + 50·L·(L−1) − (100 if L>5)` on every entry. A tamper check
      (`ds:2236 ≠ 0x9D1A` → `S4(19) = 20`) crashes max HP on a patched
      EXE. Open: `OUTM1`'s role; the checksum record layout.
- [x] mail routes — see the TWNDR `MailDeliveryJob` / `FoodShop` entry
      below (job always routes ±1 town, paid 95/110/125 on arrival).

## DUN.EXE

- [x] `dun_combat.bas` — `DoAttack`, `MonsterAttack`, `MonsterSpecialAttack`.
      DUN uses a simpler linear model than OUT:
      player to-hit `RND*70 < Dex+30`; player damage
      `INT((RND+0.5)*(Str+30)*(weaponPower+40)/450)`;
      **monster** to-hit `miss when RND*70 <= Dex`, monster damage
      `INT((RND+0.5)*monsterAtk)` (no mitigation).
      DANGLER drains `INT(RND*3+1)` Endurance; KNUCKLES / armour-eater
      destroy equipment.  (`monsterAttack` un-folded/coerced via
      `ida_scripts/fix_dun_monsterattack.py`.)
- [x] `dun_combat.bas` (v2) — **monster movement** (`moveMonsters` /
      `sub_139FC` / `stepMonsterToward`).  Greedy Manhattan chase every
      turn: one orthogonal step toward the player, dominant axis first,
      other axis as a single fallback — **no aggro range, no randomness,
      no path-finding**.  Blocked by any map byte `≥ 0x10` or the
      player's cell.  Map byte = `bit7 flag | class<<4 | wall/floor`.
      `checkMonsterAdjacent` → attack direction 0/1–4 → `ds:2188`.
      Confirmed the Befuddle gate: `confuseTimer (ds:1AE6) > 0` skips the
      whole monster phase, `< 0` skips the player's turn.
- [x] `dun_traps.bas` (v2) — `MoveHazards`, `FallThroughDamage`,
      `DoLookSearch`, **`Climb` / `DungeonExit`**.
      Trap tiles 1..7 hidden / +8 revealed; FLOOR HOLE drops a level; other
      traps spawn a monster ambush; fall damage ~ `dungeonLevel^1.6`.
      **Climb** works only on a staircase (`0x0A` down / `0x0D` up);
      `ds:1AE2 += ±0x100`, stair tiles toggle, level map + monster band
      reload.  **Dungeon exit** (climb up off level 0) awards a quest bit
      (D1 `0x10` if `S2(16)&S2(20)`, D2 `0x100` always, D3 `0x800` if
      `S2(14) > 3`) and — if a bit was awarded — raises Strength to a
      floor of **25 / 40 / 50**.  Chains D1→OUT, D2/D3→MUS.
- [x] `dun_chest.bas` — `OpenChest`, `RollChestContents`, `FindJewel`.
      Chest gold = `INT(chestBase*RND(1) + 60)`,
      `chestBase = (10*dungeonNumber + dungeonLevel)*20 + 20`; a per-level
      gold high-water mark stops re-farming; the quest jewel is a level-7
      chest granted once.
- [x] `dun_spells.bas` (v2) — **the full 6-spell table + both
      dispatchers.** `UseMagicMenu` (the "M" command): a 3-row
      `rt_FE57` picker, row 9 → Magic flame (S2 24), row 10 → Firebolt
      (S2 25) via `selectedSpell = row + 15`, row 11 → OTHER. Attack
      damage `INT((45/(range+1)+18)·(RND+1)·(Firebolt?2:1))`, fizzle
      `RND ≤ (Int+15)/45 AND RND ≥ 0.05`. `CastSpell` (row 11, shown
      only if a Befuddle/Psycho/Kill-flash charge is held): zeroes the
      flame/Firebolt/Seek charges around a second `selectAbove` picker,
      then `ON (selectedSpell−25) GOTO` for Befuddle 26 / Psycho
      strength 27 / Kill flash 28. Seek (29) is overworld-only,
      unimplemented here. Constants read from `DUN.EXE`; tables coerced +
      unfolded via `ida_scripts/fix_dun_spells.py`. Open: `selectAbove`
      mode-4 row→slot math (inside LEGLIB).

## TWNDR.EXE

- [x] `twndr_services.bas` — `SpendGold`, `Bank`.  **Correction:** the
      CHAR.DAT "experience" dword (`ds:1AC2:1AC4`) is the **bank balance**;
      LotA has no XP stat.  Interest =
      `MIN(1500, MIN(5000, balance) * daysElapsed \ 999)` (`ds:2C3C` is an
      8-byte double = 999.0).  Moneylender = flat 50% loan.
- [x] `twndr_services.bas` (v2) — `WeaponArmorShop` / `FoodShop`,
      `MailDeliveryJob`, `GuardAttack` / `FightGuard` / `InitGuardCombat`,
      `StealGold` / `OfferGuardBribe` / `ArrestedByGuards` / `JailRelease`
      / `RobberyEvent`.  Shop **buy** prices are
      per-slot `TOWN<n>.BSV` data (not a formula); **sell** base value
      weapons `INT(((wid^1.05 + cond/2.8 + 2)^2.1)*4 - 10)`, armour
      `INT((id^1.02 + cond/3.5 - 6)^3.2)` (no trailing scale);
      `offer = INT(MIN(baseValue, baseValue*Charm^0.7/11) * 0.8)`.  Guard
      combat mirrors the castle (armour + Endurance denominator).  **Mail**
      job always routes ±1 town (`S4(7)` = pending, `S2(9)` = letter);
      **paid on entering the destination provisioner**:
      `payment = INT(INT(RND*3)*15 + 95)` = **95 / 110 / 125 gold**
      (`partyGold += payment`, `S4(7) = -1`, `S2(9) = 0`).  Food:
      `pricePerDay = INT(13 - Charm/7)·0.1`, `maxDays = MIN(1000,
      partyGold/pricePerDay)`; food is runtime-only.
      **Crime:** `stealGold` — `partyGold += S4(0)` (shop till),
      `S4(0) = INT(S4(0)·0.8)` (till shrinks 20 %/theft). Guard HP ==
      bribe demand `ds:216E = INT((ds:1E22 − 7.5)·22·(RND+1))`. Jail bail:
      `>149 g` → lose half; `1–149` → lose all + one weapon confiscated;
      broke+itemless → forced 100-gold loan (`S4(5) += 100`).
      (`foodShop`/`stealGold`/`robberyEvent`/`initGuardCombat`/`jailRelease`
      coerced + dumped read-only via `ida_scripts/dump_twndr_*.py
      -NoExport`.)
- [ ] `stealGold`'s FF22-pop question; `robCommand`'s caught roll (still a
      `db` blob); `offerGuardBribe`'s S2 item marking.
      *(NOTE: `twndr.idb` has a local coerce of `townServiceDispatch`
      that reflows the whole `.asm` on export — `twndr.asm` is left
      un-updated; the math above was read from the coerced idb.)*

## CASDR.EXE

- [x] `casdr_castle.bas` — `DoFight` (**player attack**), `EnemyAttack`
      (*partial*), `AttackHit`, `GasDamage`, `WarlordAttack`,
      `DescribeRoom`.  Castle incoming melee mitigates via **Endurance and
      armour in the denominator**: `dmg = INT(enemyAtk^1.8 * (RND*600+300)
      * difficulty / (armorVal * Endurance^0.9) + 2)`.  Warlord blow =
      `INT(RND(1)*99 + 80)` (80..178).  Player attack:
      weapon HIT `RND(1) < (11·wid + 99)·(Dex+13) / (7500·K)`; weapon dmg
      `INT( ((wid\2+1)·Str\7) · (1 + 2·RND(1)) )`; spell cast succeeds
      `RND(1)·6 < Int^0.53`, spell dmg `INT((selSpell−22.5)·28·(RND+1))`
      then `\5` in the castle then `\range`.
- [x] `casdr_castle.bas` (v2) — `OpenCommand` / `UseKey` / `ResolveUseKey`
      / `TakeChestItem` / `FortressSelfDestruct`.  Castle **box**: `OPEN`
      (tile `0xC3`) reveals a 2×2 group, `TAKE` (tile `0xDF`) grants
      `Item$(15)` = the **Compendium**, gated once by `S2(15)`; **no
      gold** (castle has no economy).  **Locked doors**: fort tiles
      `0xC0..0xC2/0xCB/0xCC/0xDA` = "DOORS LOCKED"; `USE` key 4-7 →
      per-tile match table (`0xC0→4`, `0xC1/0xDA→7`, `0xCB→8`, `0xE6→5`,
      `0xE7→6`) → "UNLOCK DOOR."  `S5` (`ds:1BF2`) **is live** in CASDR
      (per-tile door data).  **Self-destruct**: Warlord death → the
      "SELF-DESTRUCTION IN 5 MINUTES!" cinematic, `ds:20BC = 0x5F00`
      escape budget ticked per turn.  Coerced + dumped read-only via
      `ida_scripts/dump_casdr_castle.py -NoExport`.
- [x] `casdr_castle.bas` — **regular guard blow** (`sub_127C8` →
      `enemyAttack`).  `sub_127C8` = the per-turn enemy update; spawns a
      guard (`enemyAtk = 140`) when none active.  Blow ≈
      `INT( enemyAtk·(1−RND(1))/2 )` → 0..70 for a fresh guard, **no
      armour/Endurance mitigation** (contrast the Warlord blow).  Exact
      `raw − INT(raw)\2 − half` shape pending the `FF23`/`FF28` trace.
- [ ] `WarlordConfrontation`; the per-turn `ds:20BC` self-destruct cost;
      gas-room `base`; `enemyAttack` FF23/FF28 operand order.

## MUS.EXE

- [x] `mus_exhibits.bas` — `EnterExhibit`, `TestExhibitFlag` (an ALL-BITS-SET
      test, corrected).  Each exhibit chains to a driver EXE and consumes
      its required gem coin; responses gated on quest-flag bits — full
      table in `quest_flags.bas`.  Stones-of-Wisdom INT maths are in STDRV.
- [x] `mus_caretaker.bas` — **the character-LEVEL mechanism, fully solved.**
      `ds:1AE0` is written in exactly one place (`sub_12CAC`), set to
      whichever exhibit-coin-group rank `useCommand` just found you
      qualify for (1..7 via `caretakerPraise`; rank 8 = the final offer,
      consumes the Compendium, HP=3000, gold capped 50000, level -> 10).
      Found by coercing 5 functions' un-decoded `db` runs
      (`ida_scripts/fix_mus_caretaker_gaps.py`, the same "IDA chops the
      block after every far call" issue as the DUN coerce-gap fixes).
      Also nails max HP exactly: `200 + 50*L*(L-1) - (100 if L>5)`.
- [x] bit `0x2000` — **RESOLVED: it is never set** (latent bug, no
      gameplay effect). Every exhibit's handler is meant to OR in its own
      `2^(exhibitId-1)` bit via `sub_11C38`; `checkFlag_2000` (exhibit 14
      = "INFORMATION" = the caretaker's desk) omits the tail call, so its
      only path to the setter is a dead "already set" branch. Verified
      the whole game has exactly 4 `questFlagWord` writers and read every
      caretaker-graph function end-to-end. `caretakerOffer`'s `S3(0)>=2`
      fast-path is dead the same way. `quest_flags.bas` §3c.

## leglib — the shared runtime / engine primitives

- [x] `leglib_runtime.c` — the hand-written runtime **as C** (the form a
      ScummVM port wants): the value stack is a compiler artifact that
      collapses to plain C expressions (with the real operand-order table),
      the `B$RND` 24-bit LCG with its constants, the `rt_FC` ON-GOSUB
      dispatcher, the `drawString` control codes, the 7-byte BSAVE header,
      the graphics + music far calls, the SUB frame.
- [x] `leglib.bas` — the same primitives sketched as BASIC, so the
      `*.bas` module files read on their own.  `leglib_runtime.c` supersedes
      it for anything a port would actually implement.

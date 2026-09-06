# Roadmap

Working list of next steps, roughly in priority order. Check items off /
add notes as they're resolved — this file is meant to stay current across
sessions, unlike a one-off todo list. See [overview.md](overview.md) for
the per-executable breakdown this tracks against.

## 2026-09-03 — ON-GOSUB decode, full module coverage, runtime-as-C

- [x] **`rt_FC` / `rt_FD` inline dispatch tables decoded**
      (`ida_scripts/fix_on_gosub_tables.py`). `rt_FC` = compiled
      `ON n GOSUB` (not "reseed the RNG" as previously labelled), `rt_FD` =
      `ON n GOTO`; both carry `db count` + `dw arm[]` inline after the
      `call far`, which IDA had mis-decoded everywhere. ~40 tables across
      `out/dun/casdr/twndr/mus` now readable (quest-flag dispatch, region
      presets, dungeon-exit routing, `castSpell` spell dispatch, …).
- [x] **`ds:2092` / `ds:2096` closed** — a 5-row per-tile constant table
      (`regionPreset_A..E`), not per-map data. See
      [game-logic.md §4](game-logic.md#4-movement--terrain).
- [x] **`recovered/leglib_runtime.c`** — the hand-written runtime as C
      (value stack → plain expressions, `B$RND` LCG `seed*214013+2531011`,
      `rt_FC`/`rt_FD`, `drawString` codes, BSAVE header).
- [x] **Every executable now has a `recovered/*` file** — added
      `menu_saver.bas` (starting stats: all attrs 15 / HP 200 / 0 gold /
      Studded hide / bare hands), `stdrv_dice.bas`, `sdefendr_training.bas`,
      `gmb_casino.bas`, `misc_drivers.bas`.
- [x] **Quest-flag semantics closed** (`recovered/quest_flags.bas`) — the
      7 gem coins each map to a bit in the shared `S4(11)` story bitfield;
      DUN's dungeon-exit side quests add 2 more bits + a Strength floor
      (25/40/50); MUS's `exhibitId -> chainTargetIdx` staircase IS the
      required coin's index (14 exhibits / 7 coins, table in the file);
      `testExhibitFlag` is an ALL-BITS-SET test driving a progressive
      per-coin unlock ladder (`S4(10)`, 8 ranks).
- [x] **Character-level / caretaker mechanism closed**
      (`recovered/mus_caretaker.bas`) — `ds:1AE0` is written in exactly
      one place (`sub_12CAC`), found by coercing 5 un-decoded `db` runs
      inside `caretakerOffer`'s callees
      (`ida_scripts/fix_mus_caretaker_gaps.py`). Level = the highest
      exhibit-coin-group rank you qualify for (§7's `S4(10)` ladder);
      rank 8 finalizes it at 10. Max HP formula pinned exactly:
      `200 + 50*L*(L-1) - (100 if L>5)`.
- [x] bit `0x2000` — **RESOLVED: never set** (latent bug, no gameplay
      effect; `recovered/quest_flags.bas` §3c). Every exhibit's handler
      is meant to OR in its own `2^(exhibitId-1)` bit via `sub_11C38`;
      `checkFlag_2000` (exhibit 14 = "INFORMATION" = the caretaker's
      desk) omits the tail call every other exhibit makes, leaving only
      a dead "already set" branch. Verified the whole game has exactly 4
      `questFlagWord` writers and read every caretaker-graph function
      end-to-end.
- [x] **DUN spell table** (`recovered/dun_spells.bas`) — all 6 spells:
      Magic flame / Firebolt (attack, `INT((45/(range+1)+18)·(RND+1)·
      (Firebolt?2:1))`, fizzle `RND ≤ (Int+15)/45 AND RND ≥ 0.05`),
      Befuddle (confuse `INT(old\2+RND·10+25)` turns, 0.93 backfire at
      full HP), Psycho strength (`+50%` melee `INT(RND·10+20)` turns),
      Kill flash (`clearViewObjects` — wipes all monsters). Seek unused
      in DUN. `ida_scripts/fix_dun_spells.py` coerced/unfolded the arms.
- [x] **DUN CastSpell dispatch** (`dun_spells.bas` v2) — the "M" command
      `useMagicMenu` = a 3-row `rt_FE57` picker (row 9 → S2 24, row 10 →
      S2 25, `selectedSpell = row + 15`); row 11 → OTHER falls through to
      `castSpell`, shown only if a Befuddle/Psycho/Kill-flash charge is
      held. `castSpell` zeroes the flame/Firebolt/Seek charges around a
      second `selectAbove` picker, then `ON (selectedSpell−25) GOTO`.
      Open: `selectAbove` mode-4 row→slot math (inside LEGLIB).
- [x] **CASDR player attack** (`DoFight`, `casdr_castle.bas`) — weapon
      HIT `RND(1) < (11·wid + 99)·(Dex+13) / (7500·K)` (K = Dex/26 castle
      / 1.0 fort); weapon dmg `INT( ((wid\2+1)·Str\7)·(1 + 2·RND) )`;
      spell cast `RND(1)·6 < Int^0.53`, dmg `INT((selSpell−22.5)·28·
      (RND+1))` then `\5` castle then `\range`.
- [x] **CASDR regular guard blow** (`sub_127C8` → `enemyAttack`,
      `casdr_castle.bas`). `sub_127C8` = the per-turn enemy update;
      spawns a guard (`enemyAtk = 140`) when none is active. Blow ≈
      `INT( enemyAtk·(1−RND(1))/2 )` → 0..70 for a fresh guard, **no
      armour/Endurance mitigation** (unlike the Warlord blow). The
      value-stack ops are now settled (below); `enemyAttack` is still
      entered mid-expression with the FP stack one operand short at the
      `FF28` call, so the exact `raw − INT(raw)\2 − half` shape needs a
      live FP-stack dump. Magnitude solid.
- [x] **TWNDR shops/guard/mail** (`twndr_services.bas` v1) — shop BUY
      prices are per-slot `TOWN<n>.BSV` data; SELL
      `baseValue = INT(((wid^1.05 + cond/2.8 + 2)^2.1)*4 - 10)`,
      `offer = INT(MIN(baseValue, baseValue·Charm^0.7/11)·0.8)`; guard
      combat mirrors the castle (armour+Endurance denominator).
      `ida_scripts/fix_twndr_guard.py` coerces the 4 small handlers (NOT
      `townServiceDispatch` — it reflows the whole `.asm`, so `twndr.asm`
      is intentionally left un-updated).
- [x] **TWNDR mail payout** (`twndr_services.bas` v2) — the job always
      routes `currentTown ± 1` (`S4(7)` pending, `S2(9)` letter); paid on
      entering the destination provisioner (`foodShop`, twndr.asm:2082):
      `payment = INT(INT(RND*3)*15 + 95)` = **95 / 110 / 125 gold**
      (`partyGold += payment`, `S4(7) = −1`, `S2(9) = 0`). Also got the
      food price: `pricePerDay = INT(13 − Charm/7)·0.1`.
      `ida_scripts/dump_twndr_foodshop.py` coerces + dumps `foodShop`
      read-only (`-NoExport`).
- [x] **TWNDR armour-sell polynomial** (`sub_11F51` armour branch,
      twndr.asm:3908) — `baseValue = INT( (armourId^1.02 + condition/3.5
      − 6) ^ 3.2 )`, consts `ds:2B66` 3.2 / `2B6A` 1.02 / `2B6E` 3.5 /
      `2B72` −6. Ends on the outer power — no trailing `·m − k` like the
      weapon branch. Confirmed `rtm_FF2B` = `TOS ^ TOS1` (top = base).
- [x] **TWNDR crime / jail** (`stealGold` / `initGuardCombat` /
      `jailRelease`, `twndr_services.bas` v2). ROB success:
      `partyGold += S4(0)` (shop till), `S4(0) = INT(S4(0)·0.8)`. Guard HP
      == bribe demand `ds:216E = INT((ds:1E22 − 7.5)·22·(RND+1))`
      (`ds:283C` −7.5 / `2840` 22). Jail bail ladder: `>149 g` half /
      `1–149` all + weapon confiscated / broke+itemless → forced 100-gold
      loan (`S4(5)+=100`, `S4(6)` deadline). `ida_scripts/dump_twndr_crime.py`
      coerces + dumps read-only (`-NoExport`).
- [x] **`offerGuardBribe`** (`twndr.asm:15a2d`, coerced read-only) — a
      corrupt guard *selling* you an item, not "pay to leave":
      *"`<ds:216E>` GOLD?"* → yes → *"YOU GOT A[N] `<Item$(ds:1AEE)>`!"*,
      `S2(ds:1AEE) += 1`, `partyGold −= ds:216E`. `ds:1AEE` defaults to
      1 (Gold armband). "YOU'RE SHORT ON GOLD." if you can't afford it.
- [x] **CASDR chest / doors / self-destruct** (`casdr_castle.bas` v2).
      Castle box: `OPEN` (tile 0xC3) reveals a 2×2 group, `TAKE` (tile
      0xDF) grants `Item$(15)` = the Compendium, gated once by `S2(15)`;
      no gold. Locked doors: fort tiles `0xC0..C2/CB/CC/DA` = "DOORS
      LOCKED"; `USE` key 4–7 → per-tile match (`0xC0→4`, `0xC1/DA→7`,
      `0xCB→8`, `0xE6→5`, `0xE7→6`) → "UNLOCK DOOR." `S5` (`ds:1BF2`) is
      a live per-tile door table in CASDR. Self-destruct: Warlord death →
      "SELF-DESTRUCTION IN 5 MINUTES!" cinematic; `ds:20BC` is a
      **cosmetic** countdown gauge (ticked −28/turn while > `0x898`,
      freezes there) — no fail condition. `ida_scripts/dump_casdr_castle.py`
      coerces + dumps read-only.
- [x] **CASDR gas room** (`gasDamage`, `casdr.asm:4390`). Applied per
      castle turn while facing a gas tile (`ds:1F02 ∈ 0x17..0x19`):
      `dmg = INT( maxHP\4 + RND(1)·50 )` (`ds:20AA = S4(19)\4`, set by
      `gasTrap`; `ds:28DA = 50`) — ~¼ max HP/turn.
- [x] **DUN monster movement** (`moveMonsters` / `sub_139FC` /
      `stepMonsterToward`, `dun_combat.bas` v2). Greedy Manhattan chase
      every turn — one orthogonal step toward the player, dominant axis
      first, other axis as a single fallback. No aggro range, no
      randomness, no path-finding. Blocked by map byte `≥ 0x10` or the
      player's cell (map byte = `bit7 | class<<4 | wall/floor`).
      Confirmed the Befuddle gate: `confuseTimer > 0` skips the whole
      monster phase, `< 0` skips the player's turn.
- [x] **DUN climb / dungeon exit** (`climbUp` → `climbDownOrExit`,
      `dun_traps.bas` v2). `CLIMB` only on a staircase (`0x0A` down /
      `0x0D` up); `ds:1AE2 += ±0x100`, stair tiles toggle, level reloads.
      Exit (climb up off level 0): quest bit per dungeon (D1 `0x10` if
      `S2(16)&S2(20)`, D2 `0x100` always, D3 `0x800` if `S2(14)>3`), and
      if a bit was awarded raises Strength to a floor of 25/40/50
      (`10·dn + 15/20/20`); chains D1→OUT, D2/D3→MUS.
- [x] **CASDR Warlord fight** (`casdr_castle.bas` v3). `warlordHP`
      (`ds:20BA`) = **800** (`0x320`), set by `takeChestItem` the moment
      you grab the Compendium — that's what spawns him; `DoFight` hits
      subtract from `ds:20BA`, `≤ 0` → "WARLORD KILLED" →
      `fortressSelfDestruct`. Every castle turn while `ds:20BA > 0`:
      `warlordAttack` → `hitPoints −= INT(RND·99 + 80)` = 80–178.
      `warlordConfrontation` (walk into the final wall) is a *mid-fight*
      cinematic: forces `hitPoints = 28`, `questMarkState = 0xFF`,
      `ds:20B6 = 1` — never touches `ds:20BA`.
- [x] **S4(37) / S4(18) rank-gate overrides** (`mus_caretaker.bas` v2).
      All 8 caretaker rank-check arms mapped: ranks 1/2/3/4/6 gate on
      cumulative exhibit-flag groups (`0x03`/`0x2B`/`0xD0`/`0x0300`/
      `0x0800`), rank 7 on the Compendium, rank 8 never auto-promotes.
      `S4(37)` (byte `0x4A`) = "visited a dungeon" (`dunMain:88` sets it
      to 1) → gates rank 2. `S4(18)` (byte `0x24`) = main-quest stage
      (`kingConfides` advances; `exitCastle` → 4; TWNDR reads `== 3`) →
      rank 5 needs `S4(18) ≥ 2`.
- [x] **Value-stack operand order** — resolved from the `seg004` dispatch
      (`recovered/leglib_runtime.c`), no trace needed: `rtm_FF1F` swaps
      operands (`loc_21BC0` `xchg si,di`) → a following `Jcc` tests
      `TOS <cmp> TOS1` (the "reversed" reading used everywhere is right);
      `rtm_FF22`/`FF23` **pop**; `rtm_FF49` const-form = `TOS / const`;
      `rtm_FF28` = `TOS − TOS1`. Knock-on: the **OUT encounter trigger**
      is `INT(RND·(level+9)) ≤ encFreq` (the `FF27` truncation makes
      `encFreq` a 1-or-2-bucket threshold → ~10–20 %/step at L1, tapering);
      `stealGold`'s trailing `spendGold` is a no-op repaint (keep the full
      till); CASDR `K = Dex/26` in the to-hit denominator is a real quirk.
- [x] **`robCommand`** (`twndr.asm:15b77`, coerced read-only). The "ROB"
      command is **deterministic** — no die roll. Mint (tile `0xD2`) →
      `stealGold`; loose gold at a merchant → `INT(RND·100 + 150)` =
      150–250 g; merchant refuses unless `contextMode > 0` or heat
      (`ds:20B0`) `> 0`. **Caught = a turn timer**: `doWalk` ticks
      `ds:20B0` each town-turn while a robbery is in progress; at 20 turns
      → "DISCOVERED!!" + alarm + `contextMode = 1` (guards attack).
- [x] **CASDR `enemyAttack`** — traced to the limit of static analysis.
      The `db`-blob fragment (no `basProcEnter`, entered mid-expression)
      reaches its `FF28` (`TOS − TOS1`) with only one operand on the FP
      stack → it reads the stack base node `ds:0FAC`, a stale value from
      a prior statement. **Looks like an original bug.** Port as
      `INT(enemyAtk·(1−RND)/2)`. A live `[ds:0FAC]` dump is the only way
      to observe the real game.
- [ ] Cosmetic / not blocking a port: the exact first-set of `ds:20B0`
      (a `db` blob; timer path traced); a live `[ds:0FAC]` dump for
      `enemyAttack`; `selectAbove` mode-4 LEGLIB internals; `OUTM1`'s
      role; gmb2 `ds:2B40` / `ds:2AA6`.

## Infra (done, 2026-08-30)

- [x] Set up headless IDA pipeline
      (`ida_scripts/run_ida_script.ps1` + `batch_run_and_export.py`),
      copied from the sibling `ultima1` project. Path-derivation driver
      (works for any `<stem>.idb`), pre-flight GUI-lock check.
- [x] `ida_scripts/identify.py` (+ raw-vs-code byte coverage) and
      `rank_unnamed_functions.py`. Confirmed working end-to-end against
      `menu.idb` and `leglib.idb` (report mode).
- [x] Identified the toolchain: **Microsoft BASIC Compiler 6.0**, with
      `LEGLIB.EXE` as a shared BRUN-style run-time module that every
      other module loads via an `int 3Fh` thunk table. Not C.
- [x] Confirmed `.BSV`/`.GLB`/`.GMP`/`.BS1`/`.BS2` are Microsoft BASIC
      `BSAVE` images (`0xFD` + seg + off + len header) — see
      [file-formats.md](file-formats.md).
- [x] `.gitignore` already correct (commits `.asm`/`.idc`, ignores
      `.idb` and unpacked-db files).

## Executable order

Decided 2026-08-30 (with Paul): **`LEGLIB.EXE` first**, or alongside
`MENU.EXE`. Rationale: `LEGLIB` is the shared payload — the BASIC runtime
plus the common engine (`bmXXXX` graphics, file I/O, input). Every
per-module `.EXE` is a thin `call far` stream into it, so nothing
downstream is readable until `LEGLIB`'s entry points are mapped. `MENU`
is small and makes a good pipeline shakedown / first consumer to
validate the thunk-table → `LEGLIB` cross-reference.

Rough size order of the rest (biggest engine payoff first): `OUT` (done,
2026-08-30) → `DUN` → `TWNDR` → `CASDR` → `MUS` → minigames (`SDEFENDR`,
`GMB1`, `GMB2`) → drivers (`CELDRV`, `STDRV`, `SAVER`) → `CONFIGUR`
(standalone, not BASIC).

**All four formerly-packed modules — `OUT` / `DUN` / `TWNDR` / `CASDR` —
are built** (2026-08-30/31), each via the same pipeline: `idat -B` from a
copy of the UNP-unpacked exe → `resolve_thunks` → `coerce_code` →
`resolve_thunks` → `dump_strings` → `apply_renames_<m>`. `DUN`/`TWNDR`/
`CASDR` each link a 2nd compiled-BASIC code segment ("bmDUNG",
"bmTNCALB") — re-run `coerce_code` with `$env:COERCE_SEG='seg001'` for
it. `TWNDR`/`CASDR` share the identical `bmTNCALB` `seg001` module.

Once `leglib.idb` gets more `B$…` names, re-run `resolve_rtm_leglib.py`
to refresh `rtm_map.py` — the `-> name` comments propagate to every
module on the next `resolve_thunks` pass.

`mus.idb` built 2026-08-31 — and **`MUS.EXE` turned out to be the MUSEUM
driver, not music** ("MUS" = Museum; the hub, exhibits = portals; chains
to `TWNDR`/`DUN`/`STDRV`/`CELDRV`).

`stdrv.idb` built 2026-08-31 — and **`STDRV.EXE` turned out to be the
"Stones of Wisdom" dice game** (a Liar's-Dice / Perudo variant vs. the
"DEALER"), not a story driver. It's the museum's *Stones of Wisdom*
exhibit minigame; the match result adjusts the character's INTELLIGENCE.
`STDRVSCR.DAT` = the rules text it narrates. Single code segment, 467
thunks, 100% coerced; 7/39 named.

`celdrv.idb` built 2026-08-31 — and **`CELDRV.EXE` is the endgame victory
cinematic**, not a general cel player: "AGAINST ALL ODDS!" + the
scrolling victory-story recap (hero-name substitution, over music) + end
credits. Loads `CEL0`–`CEL2`/`DIS9`/`CEL3.BSV`. Chained from `CASDR`.

`saver.idb` built 2026-08-31 — **`SAVER.EXE` is the save-game handler**:
"SAVE THE GAME NOW IN PROGRESS?", validates the character disk, writes
`CHAR.DAT`, then ESC → DOS / else re-exec `OUT` / `DUN`. Confirms
`CHAR.DAT` doubles as the in-progress save (no separate file).

`sdefendr.idb` built 2026-08-31 — **`SDEFENDR.EXE` is the combat-training
school** (ARMOR / WEAPONS training arena, a 360° fireball-defense shooter
reached from a town; raises ARMOR/WEAPON/ENDURANCE, 50 gold/session,
7 levels, chains back to `TWNDR`). First module with a hand-written
**asm** code segment (`seg001` = the real-time arena engine).

`gmb1.idb` built 2026-08-31 — **`GMB1.EXE` is the BlackJack table**
("GMB" = gamble; hit/stay vs. the dealer, natural pays double, five-card
win, 17+ dealer stop, bet 0 to quit, broke → 5-gold stake, break the
bank → house closes). Loads `BJCHR.GLB`, chains back to `TWNDR`.

`gmb2.idb` built 2026-08-31 — **`GMB2.EXE` is "Flip-Flop Parlour"**, a
Plinko / pachinko betting game (drop a ball, it bounces bumper-to-bumper
into one of 6 buckets; bet the bucket number and/or colour; outer
buckets pay even / double / 5×; the bumpers "flip-flop"). Uses
`BIGNUM.DAT`, chains back to `TWNDR`. NOT a card game — it doesn't use
`BJCHR.GLB`.

`configur.idb` built 2026-08-31 — **`CONFIGUR.EXE` is the floppy-drive /
disk-layout config utility** (edits `DRCONFIG.DAT`: which drive letters
hold the game floppies, "to reduce disk swaps"). Standalone MSC, not
packed; only `_main` + six BIOS screen/keyboard wrappers are app code.

**All 14 executables now have an IDB.** Remaining work is depth per
module, `leglib` `rtm_*` → `B$…`, on-disk formats, then the
reimplementation.

## DUN.EXE — open questions

- [x] Build `dun.idb` (2026-08-30). 6 segments, two compiled-BASIC code
      segs (`seg000` "bmDUN" main + `seg001` "bmDUNG" graphics), thunk
      table `seg002`. Both coerced ~100%, 0 bad insns.
- [x] Map the DUN engine state vars (2026-08-31, `apply_dsvars_dun.py`;
      DUN's DGROUP is **seg004**). ~16 named: `dungeonLevel` (1ACA),
      `hitPoints` (1ADA), `playerX`/`playerY` (20CE/20D0), `tileAhead`
      (20C4), `selectedSpell` (1E24), `dungeonArrayPtr` (2274 far ptr),
      `levelProgressFlags` (1AE2), `actionPhase` (20EC); `turnActionFlag`
      (212E) and `chainDestType` (1F16) are the **same DGROUP slots as
      OUT**. `featureUnderfoot` / `scanTile` / `moveDelta` tentative.
- [x] 2nd DUN DGROUP pass (2026-08-31): `ds:20EA` = `workInt` (DUN's
      general-purpose int local, the OUT-`ds:1F04` equivalent -- was
      mislabelled "hpDisplayScratch"). Also `menuChoice` (1E22),
      `monsterIndex` (2188 = the moveMonsters loop), `chestGold` /
      `chestItemKind` (1F0A / 1F08), `attackTargetTile` (1AFE),
      `attackMode` (2140), `screenLayout` (1E20). ~24 DUN DGROUP vars.
- [~] Name `seg000` functions (`apply_renames_dun.py`): **38 / 72**
      (2026-08-31 second pass). New: `processTileFeature` (was
      `sub_12536` — the per-turn feature handler with the trap-name
      table), `moveMonsters`, `drawDungeonHud`, `doLookSearch`,
      `clearTurnFlag`, `setActionPhase_1/2/3`, plus tentatives
      (`stepMonsterToward`, `rebuildLevelView`, `updateLevelState`,
      `rollChestContents`, `monsterSpecialAttack`, `redrawDungeonView`).
- [x] Name `seg001` "bmDUNG" (2026-08-31) — it's the first-person
      **dungeon-view renderer** (7 funcs): `renderDungeonView` loops the
      depth bands calling `drawViewWallBand{Near,Mid,Far}` +
      `drawViewSprite`, all via the `blitViewCell` primitive (rtm_FE2A).
      Which band is which is a guess. MUS's `bmMUSDUNG` (6 funcs) is the
      same renderer for the dungeon-exhibit rooms — named identically.
- [x] Map `bmDUNG` / `bmMUSDUNG` DGROUP state (2026-08-31) — minimal:
      `dgroupSeg` (0101), `viewDepthBand` (1F5A), and the array
      descriptors the renderer binds: `dungeonMapArray`/`exhibitMapArray`
      (1E2A = tile grid), `viewObjectArray` (1C7C), `spriteBank` (1E58),
      `viewProjTable` (1C4E). Rest is registers / stack / seg004
      playfield via those descriptors.
- [ ] Confirm the DUN tentatives; identify the ~28 remaining `sub_`
      (many are runtime-dispatched 6–26 byte stubs).

## TWNDR.EXE / CASDR.EXE — open questions

- [x] Build `twndr.idb` (2026-08-31, 41/98 named) and `casdr.idb`
      (2026-08-31, 34/102 named) from the unpacked exes. Each: `seg000`
      main + `seg001` "bmTNCALB" (**shared, identical** between the two),
      thunk table `seg002` (431 entries).
- [x] Map the TWNDR engine state vars (2026-08-31,
      `apply_dsvars_twndr.py`; DGROUP = **seg004**). `partyGold` (1AD2)
      and `hitPoints` (1ADA) are the **same DGROUP slots as OUT/DUN**.
      Town-specific: `townServiceId` (1F22 — `townServiceDispatch`'s
      SELECT CASE), `tileAhead` (1F02), `guardHitPoints` (216E),
      `townArrayPtr` (278C far ptr); `menuChoice` / `turnFlag` /
      `shopWorkQty` / `viewMode` tentative.
- [~] `twndr` names: **51 / 98** (2nd pass 2026-08-31). The shops
      (`foodShop`, `weapon`/`armorShopEntry`, `buyBackShop`,
      `sellShopIntro`), money (`borrowMoney`, `loanRepayment`,
      `fortuneTeller`), crime/jail (`robCommand`, `arrestedByGuards`,
      `jailScene`, `jailRelease`, `fightGuard`), `townServiceDispatch`
      (~6 KB), `mailDeliveryJob`, `spendGold`, `enterTownService` (the
      ENTER/USE command -> dispatch), `facePlayerDirection`,
      `checkLineOfSight`, `redrawTownView`; `offerGuardBribe` /
      `initGuardCombat` / `grabTileItem` / `computeSellValue` tentative.
- [x] Map the CASDR engine state vars (2026-08-31,
      `apply_dsvars_casdr.py`; DGROUP = **seg004**). `partyGold` (1AD2),
      `hitPoints` (1ADA), `tileAhead` (1F02), `targetSlot` (1F24),
      `turnFlag` (1F2A) are the **shared LEGLIB slots** (4th module to
      confirm). Castle-specific: `playerX`/`playerY` (1B00/1B04),
      `enemyHitPoints` (2222), `castleArrayPtr` (25B0 far ptr);
      `castleOrFort` (20C0, 1/2) / `viewLevel` (2084) / `mapStride`
      (1F26) tentative.
- [x] 2nd TWNDR / CASDR DGROUP pass (2026-08-31). Both have `workInt`
      (1F04) + `workInt2` (209E, a second int temp -- CASDR's most-
      written word) + `scratchAcc` (20D0 twndr / 20CC casdr, a
      register-spill slot). Meaningful state: `questGold` / `jailState`
      (twndr), `questMarkState` (22E6 casdr = the forearm-mark quest,
      written only by kingConfides / warlordConfrontation), `viewState`,
      `destructTimer`. The 1F04..1F12 range is a row of ~8 BC-6.0
      codegen expression temps.
- [~] `casdr` names (endgame content): **47 / 102** (2nd pass
      2026-08-31). `warlordConfrontation`, `kingConfides` (the
      guardians-of-the-scroll quest + forearm mark), `potionWizard`,
      `doFight`, `describeRoom`/`describeObjects` (incl. "THE COMPENDIUM
      IS THERE!"), `loadCastleLevel` (CASTLE.BS1/2, FORT.BS1/2),
      `exitCastle`, `gasRoomTrap`, `useKey`, spell handlers,
      `fortressSelfDestruct` ("SELF-DESTRUCTION IN 5 MINUTES!"), the
      `describeRoom` cases (`describeChest`, `describeLockedDoor`,
      `describeGasRoom`, `describePotionShop`), `facePlayerDirection`,
      `checkLineOfSight`; `castleTurnUpdate` / `redrawCastleView` /
      `resolveOpenDoor` / `resolveUseKey` / `jailPlayer` tentative.
- [x] Name the shared `bmTNCALB` `seg001` module (2026-08-31,
      `apply_renames_tncalb.py` — keyed by seg001 offset, byte-identical
      structure in both, applied to each). 13/26: the town/castle
      *interior* engine — `drawInteriorTiles`, `refreshView`, `tileAt`,
      `scanLineOfSight`, `findObjectTile`, `moveActor`, `stepByDirection`,
      `dirBetween`, `viewFaceDirection`, `setViewport`, `drawActor`,
      `traceCombatLine`.
- [~] `bmTNCALB`: **21/26** (2026-08-31). Added `refreshTileGraphic`
      (the shared rtm_FE19 single-tile blit), `stepLineOfSight` /
      `sightBlockedBy`, `traceCombatRay` / `combatRayResult`,
      `placeNpcSprite`, `drawViewFrame`, `tileAtOffset` (several
      TENTATIVE). 5 tiny stubs (<20 b, runtime-dispatched) left `sub_`.
- [x] Map `bmTNCALB`'s DGROUP state (2026-08-31). Shares
      `dgroupSeg`/`playerX`/`playerY`/`tileAhead`/`mapStride`/`mapHeight`
      with `seg000`. Its **private** ~0x20-byte block lands at a
      different DGROUP offset per link (TWNDR `0x26xx`, CASDR `0x24xx`,
      `+0x176`): `tileScanIndex`, `losScanResult`, `actorDrawMode`
      (moveActor erase/draw toggle), `mapArrayBase`/`mapRowBytes`. Added
      to `apply_dsvars_twndr.py` / `apply_dsvars_casdr.py`.
- [x] `twndr` `sub_11ED0` -> `enterTownService` (the ENTER/USE command
      that locates the adjacent service tile and jumps to
      `townServiceDispatch`).
- [x] Map the CASDR state vars (2026-08-31, `apply_dsvars_casdr.py`) --
      done; see the CASDR section above.
- [x] Map the MUS.EXE state vars (2026-08-31, `apply_dsvars_mus.py`;
      `seg004`). `exhibitId` (20FE), `chainExeName` (210C string),
      `flagTestMask`/`flagTestResult` (2136/2138); shared slots as
      usual. 45/109 named (added `testExhibitFlag`, `checkFlag_*`).
- [x] **State-var mapping complete for all play modules.** `ds:1AD2` =
      partyGold and `ds:1ADA` = hitPoints in every module; `ds:1F02`,
      `ds:1F24`, `ds:1F2A`, `ds:212E`, `ds:1F16` are the other
      LEGLIB-fixed slots. Remaining: use them to finish naming the
      per-module `sub_` helpers. (`seg001` graphics helpers -- `bmDUNG`,
      `bmMUSDUNG`, `bmTNCALB` -- now named, 2026-08-31.)

## STDRV.EXE — open questions

- [x] Build `stdrv.idb` (2026-08-31). Single code seg `seg000` "bmSTDRV"
      (39 funcs), thunk table `seg001` (467), DGROUP `seg003`. 100%
      coerced, 0 bad insns, 467 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_stdrv.py`): 7/39 —
      `stdrv_entry` (builds the "NO"/"ONE"…"NINE" number-word table),
      `stonesOfWisdomMain` (loads `STDRVSCR.DAT`, "INSTRUCTIONS?", the
      per-match / play-again-for-gold loop), `playerBidTurn` ("HOW MANY
      DICE?", "OF WHAT VALUE?", "CHALLENGE!!"), `resolveChallenge`
      (reveal dice, win/lose, "YOUR INTELLIGENCE / INCREASES BY"),
      `formatBidText`. `dealerTurn` / `evalDiceOdds` = the dealer AI
      (no player text — **tentative**, confirm from the call graph).
- [x] Map the STDRV state vars (2026-08-31, `apply_dsvars_stdrv.py`;
      DGROUP `seg003`). `partyGold` (1AD2) / `menuChoice` (1E22) shared
      slots; `intelligenceStat` (1AF0 -- what resolveChallenge adjusts),
      `stdrvArrayPtr` (29CA far ptr), `diceCount` (28FE), `playerBid`
      (2108) / `dealerBid` (2106), `gameScore` (1EF2 dword, role
      unclear). Most of the other 30-odd DGROUP words are one-function
      drawString layout params.
- [ ] The ~30 remaining `sub_` helpers are all value-stack number
      crunching for the dice math -- no distinguishing text. `diceCount`
      / `stdrvArrayPtr` split them into "tally" and "table lookup"
      groups; naming each precisely needs the dice ruleset traced.
- [x] `STDRVSCR.DAT` (2026-09-01) — **not text**: `0xAA`-filled CGA screen
      graphics for the Stones-of-Wisdom table (`word[0]`=640 payload
      start). STDRV narrates its rules from strings in the EXE.
- [x] The INTELLIGENCE delta (2026-09-03, **from the STDRV.EXE code +
      constants**, supersedes the earlier guide-based guess) — `ds:1AF0`
      changes **once per match** in `resolveChallenge`:
      win → `+3/+2/+1/0` for INT below 15/30/60; loss → `−3/−2/−1/0` for
      INT above 49/39/9. Cap 60, floor ~9. See
      [`stdrv_dice.bas`](../recovered/stdrv_dice.bas).

## CELDRV.EXE — open questions

- [x] Build `celdrv.idb` (2026-08-31). Single code seg `seg000`
      "bmCELDRV" (16 funcs, 2 KB), thunk table `seg001` (373 entries),
      DGROUP `seg003`. 99.5% coerced, 0 bad insns, 373 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_celdrv.py`): 13/16 —
      `celdrv_entry` (loads `CEL0`–`CEL2`/`DIS9`/`CEL3.BSV` via `rt_FE07`,
      relocates their offset tables, "AGAINST ALL ODDS!", the story
      crawl), `scrollStoryText`, `runCreditsCrawl` + `showCreditIbmVersion`
      / `showCreditMusic` / `showCreditArtwork` / `showCreditArtworkCont`,
      `serviceMusic`, `delayWithMusic` / `waitKeyWithMusic` (tentative),
      `celAnimStep` / `blitCelFrame` (tentative). `sub_10777` is a NOP
      sled + `jmp` (dead).
- [x] Map the CELDRV state vars (2026-08-31, `apply_dsvars_celdrv.py`;
      DGROUP `seg003`). `storyLine` (20B2, the 0..999 line counter --
      997 → credits), `celBank` (208A, 0..4), `celRelocBase` (208C),
      `celFrame` (20BE, 1..5), `displayDuration` (20F8). No shared
      LEGLIB slots -- CELDRV has no partyGold / hitPoints (pure
      cinematic). The rest of DGROUP is drawString layout scratch.
- [ ] Confirm this is *only* the ending — does `MUS`'s `chainToCel` ever
      invoke `CELDRV` for a mid-game exhibit animation, or is that path
      dead? (All 54 string records here are ending / credits content.)
- [ ] `DIS9.BSV` is loaded here (the `DIS*.BSV` set was "display screens?"
      in file-formats — this ties one of them to the ending).
- [x] The victory-story text (2026-09-01) — **~51 lines, embedded string
      constants** in CELDRV's data segment (from `celdrv.asm:12944`, the
      `~` = 0x7E position code onward). Each = `<posCode> 07 " <TEXT> "
      [,"P",scroll,page] ,0` (`drawStringInner` position-coded format;
      `"N"` = hero name, `%` = line break, `"P",n,p` = scroll/page). Loaded
      into the `ds:20D6` string array by `rtm_B3` at start-up, replayed by
      `scrollStoryText`. The `storyLine` "0..999" is the **pixel-scroll
      counter**, not a line count. Story: the wizard's Compendium is
      stolen, terror spreads, the scroll reaches a peasant (`<hero>`),
      who clears the museum quests + 4 guard jewels, flies the pegasus,
      beats the Warlord, and enshrines the Compendium.

## SAVER.EXE — open questions

- [x] Build `saver.idb` (2026-08-31). Single code seg `seg000` "bmSAVER"
      (5 funcs, 1.5 KB), thunk table `seg001` (373), DGROUP `seg003`.
      99.8% coerced, 0 bad insns, 373 thunks resolved.
- [~] Name `seg000` functions (`apply_renames_saver.py`): 3/5 —
      `saver_entry` (the "SAVE THE GAME NOW IN PROGRESS?" flow +
      character-disk validation + quit/continue prompt), `saveRosterToDisk`
      (writes `CHAR.DAT` — `rt_73`/basStrBuild record build, `rt_FE35` /
      `rt_FE39` file I/O), `chainBackOrQuit` (ESC → DOS, else re-exec
      `OUT.EXE` / `DUN.EXE`). `sub_10411` is a 1-byte artifact.
- [x] Map the SAVER state vars (2026-08-31, `apply_dsvars_saver.py`;
      DGROUP `seg003`). `rosterIndex` (1B0A, same slot as MENU),
      `menuChoice` (1E22), and `returnTarget` (`ds:1ACA`) -- the last one
      is how SAVER picks OUT vs DUN: `chainBackOrQuit` tests "OUT" then
      "DUN" via `rtm_FF08` and folds the result through `sub ax,
      ds:1ACAh` before `rtm_FE05` execs. Everything else in DGROUP is
      one-function drawString layout scratch.
- [ ] Still open: where `returnTarget` (`ds:1ACA`) gets its value in
      `saver_entry` -- and whether saving from a town / castle / museum
      is even possible (only `OUT.EXE` / `DUN.EXE` are chain targets).
      `rtm_FF08` = ? (looks like a program-name / env test).
- [~] `CHAR.DAT` (2026-08-31, fields mapped 2026-09-01) — container +
      write/read path + framing fully decoded. 6-byte header
      (`05 06 | 07 00` = 7 arrays | `7E 01` = reclen 382), 9 records ×
      382. Record = 14-byte name + 74-byte scalar block (image of LEGLIB
      DGROUP `ds:1AC0..1B08`) + 7 BASIC int arrays whose `DIM` bounds
      (7/7/29/16/37/41/3 → 8/8/30/17/38/42/4 words) come from the
      `rt_AF` calls in `MENU.EXE` init and sum to exactly 294 bytes.
      Scalars placed via a full `ds:1AC0..1B08` xref sweep + the
      LEGACY.DAT template defaults: gold `+0x20` (dword, 20),
      hitPoints `+0x28` (200), strength `+0x3E` (15, cap 28), experience
      `+0x10` (dword), inventory count `+0x38` (5), overworld X/Y
      `+0x50`/`+0x54`, character level `+0x2E`, game speed `+0x16`
      (4), + dungeon pos/facing/timers. Array **S5** (`ds:1BF2`, `+0x122`)
      = the shop price table. `decoders/char_dat.py` prints the template
      split + an equipment / item readout. **S2 = the 24-item possession
      bitmap** (`S2[k]` = holds LEGACY item `[k]`), **S0/S1 = the
      equipment slots** (id + Shoddy/Fair/Good/Great/Superb condition;
      weapons 0–8, armour 9–13 share the id space) — all pinned from 4
      PAULA save-diffs (kill + shopping spree). **Food / XP are NOT
      persisted** — runtime-only working vars (searched 4 saves). Still
      open: XP / character level, and the S4 map/state slots. (The 5 attributes
      Dex/End/Charm/Int/Str = scalars 1AC0/1ACC/1ADE/1AF0/1B08 -- all pinned.)
- [ ] The "character disk" checks ("is not on this / character disk",
      "empty") imply a multi-disk / per-character-slot save scheme —
      confirm.

## SDEFENDR.EXE — open questions

- [x] Build `sdefendr.idb` (2026-08-31). Two code segs: `seg000`
      "bmSDEFENDR" (compiled BASIC) + `seg001` (hand-written asm arena
      engine); thunk table straddles `seg001`/`seg002` (328 entries).
      `seg000` 99.8% coerced, 0 bad insns.
- [~] Name functions (`apply_renames_sdefendr.py`): ~15 —
      `trainingSchoolMain` (mode select + rating + gold + `TWNDR`
      hand-off), `showBriefing`, `runTrainingLevel`, `runPractice`,
      `showWaveIntro`, `showWaveScore`, `drawScorePanel`, `drawFramedBox`;
      `arenaGameLoop` + its 8 step routines (`arenaInitPlayfield`,
      `pollPlayerTurn`, `firePlayerArrow`, `moveFireballs`,
      `arenaStepEndCheck`, `drawArenaSprites` — all **tentative**, from
      the loop structure + `ds:` byte-var usage, not yet verified).
- [ ] The `seg001` asm engine: it works on `ds:` bytes `0Ch/0Eh` (seg004
      playfield ptr), `11h` (turn key), `15h` (fire/shift), `16h`/`22h`
      (cooldown timers). Map the full arena state block in `seg004`.
- [x] `SDMAP.GLB` / `.GMP` (2026-08-31) — decoded: it's the arena
      *screen frame* (ornate viewport border), standard `.GLB`/`.GMP`
      tile+cellmap format, 256 tiles, 41×25 map. `decoders/glb_image.py`.
- [~] `SDOBJ.GLB` sprite atlas (2026-08-31) — tiles decoded (384 × 8×8,
      same field-interleaved format as `TITLE`, no `.GMP`);
      `decoders/glb_image.py SDOBJ` dumps it. Visible: approaching
      fireballs at ~6 scale steps, explosion frames, cyan directional
      shot arrows, a horned enemy head. Still open: the per-sprite tile
      grouping / index table (needs the arena blit routine in `seg001`).
- [ ] Confirm the stat-change math: the "40, 31, 22, 19, 16, 14, 12"
      table (per-level hit thresholds?) vs. the " INCREASE: + / DECREASE:
      -" applied to `ARMOR,WEAPON,ENDUR`.
- [ ] Which town building launches it, and how the trained stat writes
      back to `CHAR.DAT`.

## GMB1.EXE / GMB2.EXE — open questions

- [x] Build `gmb1.idb` (2026-08-31). Single compiled-BASIC code seg
      `seg000` "bmGMB1" (21 funcs), thunk table `seg001` (431), card
      graphics `seg004`. 99.8% coerced, 0 bad insns.
- [~] Name `gmb1` functions (`apply_renames_gmb1.py`): 14/21 —
      `blackjackMain` (the whole game + outcomes + bankroll handling +
      `TWNDR` hand-off), `showInstructions`, `showWagerRules`,
      `pressKeyToContinue`, `showGoldLine`, `shuffleDeck`, `drawFromDeck`;
      `dealCardToHand` / `dealInitialHands` / `revealDealerCard` /
      `drawGoldAndBet` / `drawDealerArea` / `clearPromptLine` /
      `drawHandSprites` (all **tentative** — no distinctive text).
- [ ] `gmb1`: the hand-scoring logic — it leans hard on the `leglib`
      `rtm_FF4B`/`FF27`/`FF44`/`FF22`/`FF1F` value-stack cluster.
      Identifying those `B$…` primitives would clarify `dealCardToHand`.
- [ ] `gmb1` `ds:` vars: `ds:1F1Ah` (shoe pointer), `ds:1F04h`,
      `ds:21A4h`, the hand struct at `ds:1C7Ch`.
- [~] `BJCHR.GLB` (2026-08-31) — tiles decoded: not 52 pre-drawn cards,
      but a rank/suit glyph font (`A 2..10 J Q K` + ♠♣♥♦, upright +
      180°-rotated) the renderer composites onto card frames, plus
      card-back tiles. Tiles 0–127 used. `decoders/glb_image.py BJCHR`.
      Still open: which tile-run makes each card, and the frame layout.
- [x] Build `gmb2.idb` (2026-08-31) — it's **"Flip-Flop Parlour"**, a
      Plinko / pachinko game, *not* cards (doesn't touch `BJCHR.GLB`).
      Single code seg `seg000` "bmGMB2" (20 funcs), 467 thunks, 100%
      coerced, 0 bad insns.
- [~] Name `gmb2` functions (`apply_renames_gmb2.py`): 14/20 —
      `flipFlopMain`, `showInstructions`, `playRound`,
      `playPracticeRound`, `dropBallAndBounce`, `computePayout`,
      `drawBumpers`, `playTune`; `promptYesNo` / `playBounceSound` /
      `playWinChime` / `drawBigNumberPanel` / `drawBallAnim` /
      `stepBallPhysics` (tentative).
- [ ] `gmb2`: the ball-physics / bumper model in `dropBallAndBounce` +
      `stepBallPhysics` — how "flip-flop" bumper state biases the drop,
      and how `computePayout` (struct at `ds:1B96h`) maps bucket → odds.
- [ ] `gmb2`: does it read a data file for the bumper layout, or is the
      whole board the hard-coded DRAW-macro set in `drawBumpers`?
- [x] Both games: **payout accounting** — `recovered/gmb_casino.bas` v2.
      BlackJack settles net (`±bet`, `+2·bet` natural, `0` tie; bet not
      escrowed); Flip-Flop `win = multTable{1,2,5}(bucket)·bet` + colour
      bonus, `computePayout` rigs the bucket ±1 toward realised payback
      `S4(14)/S4(15)` ≈ 0.94 (ledger resets to 99/99). Both stop the
      session at `gold − startGold > 250·characterLevel + 750`
      (`imul ds:1AE0`, gmb1.asm:1132 / gmb2.asm:148). The gold dword
      (`ds:1AD2:1AD4`) is LEGLIB-resident and rides the chain back to
      `TWNDR` → the normal save path (no direct `CHAR.DAT` write here).
- [ ] `gmb2` flip-flop lower band `ds:2B40` + colour const `ds:2AA6`
      didn't decode as clean singles; the bucket-geometry DATA tables
      (`ds:2100/210C/211C`) and the ball physics remain open (not
      RPG-relevant).

## CONFIGUR.EXE — open questions

- [x] Build `configur.idb` (2026-08-31). Standalone Microsoft C — IDA's
      C loader + FLIRT recovered the whole MSC CRT; 3 segments; not
      packed. Named the only 6 app helpers + commented `_main`
      (`apply_renames_configur.py`).
- [x] It's a **disk-drive** config tool (drive letters for the game
      floppies / "reduce disk swaps"), NOT graphics/sound.
- [x] Map the `dseg` globals (2026-08-31, `apply_dsvars_configur.py`) --
      there is **no application state**: `_main` works entirely on stack
      locals, and every `dseg` global is stock MSC CRT (`_errno`,
      `_doserrno`, `_osversion`, `_savedDS`, `_STKHQQ`, `_nfile` = 20,
      the `_output`/printf format-state block, argc/argv, heap/stdio
      state). Named the clear CRT ones.
- [ ] `DRCONFIG.DAT` (1015 B) exact field layout — `_main` reads it into
      a stack buffer and checks bytes for `'0'`/`'1'`/`'2'` + drive
      letters near the start; the other ~1000 bytes are unexamined.
- [ ] Who else reads `DRCONFIG.DAT` at runtime (the LEGLIB file loader?
      each module's `BLOAD` path builder?).

## LEGLIB.EXE — open questions

- [x] Map the `int 3Fh` thunk-table entry format and build a resolver
      (2026-08-30). `resolve_rtm_leglib.py` + `rtm_map.py`;
      `resolve_thunks.py` on the client side. Mechanism written up in
      [overview.md](overview.md#int-3fh-run-time-dispatch-decoded-2026-08-30).
- [x] Extended the FE table to `FE70` (2026-08-30) — `out.exe` uses
      `FE6E`–`FE70`; `FE71`+ is past the table.
- [~] Attach real names to `rtm_*` (`apply_renames_leglib.py`). Done: 14
      hot ones — `basProcEnter`/`basProcLeave` (SUB frame enter/leave, cx
      = frame size), `basProcExit1`/`basProcExit2` (outer exit wrapper),
      `basStrAssign` (164x in out), `basStrConcat`, `basStrClear`,
      `basStrBuild`, `basArrayCopy`, `basPlayMusic`, `basScreenInit`
      (provisional), `drawString`/`drawStringInner`/`screenRefresh`
      (engine text). Continue with the FF-cluster `out` leans on
      (`FF4B` 154x, `FF20`, `FF1F`, `FF44`, `FF4E`, `FF50` — value/screen
      stack ops around `valueStackPtr` = `ds:111Ch`).
- [~] DGROUP map (2026-08-31, `apply_dsvars_leglib.py`, ~26 named).
      Control block: `dgroupSeg` (0101), `nestLevel` (0118), `stopFlag`
      (0136), `chainCmdPtr` (0874). Value stack: `valueStackPtr` (111C,
      init 0xFAC), `vsWorkA`/`B` (0C62/0C64). Screen: `videoSegment`
      (0876, init 0xB800), `screenFlags` (0EFA), `screenCols`/`Rows`
      (0E68/0E6B), `dirtyRectA`/`B` (1E86/1E88 = the FE42 refresh rect),
      `pagerLineCount` (1FEC). Interior tile engine: `viewOriginX`/`Y`
      (02C4/02C8), `interiorDrawX`/`Y` (0250/0252), `interiorViewBase`
      (15FE). ~740 DGROUP words total; the rest need the surrounding
      `rtm_*`/`sub_` clusters named first. `dsvars.py` output (traffic
      >= 4) is the working list.
- [ ] Verify the 65 `[mid-func: verify]` `rtm_*` entries — confirm
      they're genuine shared-tail / multi-entry routines, not resolution
      errors.
- [ ] Separate stock BASIC runtime from LotA-specific engine code in
      `seg007`/`seg008` (the `bm*` graphics — `bmCOMBLIB`, `bmTCASANIM`).
- [ ] Find and name the `bmXXXX` bitmap/graphics primitives (the
      `bmMENU`, `bmREADY`, … names are already visible as strings).
- [ ] Find the `BLOAD`/`BSAVE` implementation and the file-I/O layer —
      the key to decoding every `.BSV`/`.GLB` file.
- [ ] Input handling (keyboard poll used by the menu state machine).
- [ ] `seg003` (53 KB) vs `seg004` (18 KB): which is runtime, which is
      game engine?
- [ ] Confirm graphics mode support (CGA / EGA / Tandy). NB `CONFIGUR.EXE`
      turned out to be **disk-drive** config only, not graphics — and the
      `*DR.EXE` files (`TWNDR`, `CASDR`, `STDRV`, `CELDRV`) are game
      *drivers*, not hardware drivers. Where the video mode is actually
      chosen is still open (LEGLIB startup? a `DIS*.BSV`?).

## MENU.EXE — open questions

- [x] Name all 467 `seg001` thunks + cross-reference to `leglib`
      (2026-08-30, `resolve_thunks.py`).
- [x] Force `seg000` to code (2026-08-30, `coerce_code.py`):
      99.5%, 0 bad insns, 25 functions, full call graph.
- [x] Name the 25 `seg000` functions (2026-08-30, `apply_renames_menu.py`).
      6 CHAR.DAT-record helpers left `sub_` (hard to distinguish).
- [x] Cosmetic block-chopping: post-process fixed via fall-through crefs
      past each `call far` (`apply_renames_menu.py`, final step). 1914 → 141.
- [x] Map the MENU DGROUP vars + name the 6 helpers (2026-08-31,
      `apply_dsvars_menu.py` + `apply_renames_menu.py`): `menuHighlight`
      (1F5C), `charCount` (2138), `rosterIndex` (1B0A), `charRecordSize`
      (211C, the CHAR.DAT `imul` stride); `sub_11A15` -> `pressAnyKey`,
      `sub_12055` -> `readCharDat`, `sub_12778` -> `writeCharDat`,
      `sub_10150` -> `menuStartup`, `sub_128A9` -> `updateCharDatEntry`,
      `sub_1210E` -> `enumerateRoster`. Only `sub_11A1E` still `sub_`.
      2nd pass added `menuRunning` (1F02), `menuLevel` (1F0C, 1 = main /
      2 = submenu -- both passed by-ref to the key dispatcher),
      `dgroupSeg` (0101). MENU's DGROUP is now fully mapped; the rest is
      1-function scratch (menuStartup locals, the menu-item coord list).
- [x] Map `menu` `seg001`'s DGROUP use (2026-08-31) -- the title-screen
      helpers: `titleGlbBuf`/`titleGmpBuf` (`ds:3194`/`5194` load
      buffers), `titleTilePtr` (`ds:6194`), `titleScrollX` (`ds:6196`,
      step 40 / wrap 160), `titleColOfsTable` / `titleColTileTable`,
      `titleGlbName` / `titleGmpName` / sizes. Confirmed `.GLB` = tile
      bitmaps, `.GMP` = the cell map (see file-formats.md).
- [ ] Confirm `playMusicTick` / `showTitleScreen` naming (the LEGACY.DAT
      touch in `playMusicTick` is unexplained).
- [ ] `menu.idb` input path reads `C:\dev\lota\menu.exe` (a copy; the
      original `.idb` was lost and rebuilt via `idat -B`). Harmless, but
      re-point at `C:\games\lota\MENU.EXE` if a full rebuild is ever
      needed.
- [ ] Mark up the `seg003:21D0h`+ text block as strings.
- [ ] Walk the menu state machine (main menu → play / instructions /
      credits / sound toggle; character management screens).
- [ ] Confirm whether `MENU.EXE` was unpacked before import (oversized
      header, entry `038F:00DF`), and whether the EA "Installation
      Program" is separable from the Quest menu program.
- [ ] Trace the chain-out to `OUT.EXE` / `MUS.EXE` and how the selected
      character is handed over (`CHAR.DAT`? `LEGACY.DAT`?).

## OUT.EXE — open questions

- [x] Build `out.idb` (2026-08-30). First built from the still-packed
      `OUT.EXE` (unreliable — un-relocated far ptrs, BSS DGROUP), then
      **rebuilt from the UNP-unpacked `OUT.EXE`**: 5 clean segments like
      menu, `seg000` coerced to 100%, 0 bad insns, ~97 functions, 467
      thunks (== menu namespace), 1297 run-time calls resolved. The
      `apply_renames_out.py` EAs carried over (reloc-only packing =
      byte-stable code).
- [~] Name the `seg000` functions (`apply_renames_out.py`): **67 / 121**
      (2026-08-31, second pass off the state vars + call graph). Movement
      pipeline (`doMovement` → `resolveMoveTarget` → `classifyLocationTile`
      / `identifyLocationObject` / `readTileObject` →
      `enterLocationOrChain`), overworld load (`enterOverworld` →
      `loadOverworldData` → `drawOverworldViewport`), combat
      (`beginEncounterView`, `resolvePlayerAttack`, `creatureDefeated`,
      `awardFoundItem`), events (`pegasusFlightAnim` / `pegasusFlyStep` /
      `showPegasusLanding`, `banditAmbushEvent`), `redrawAfterAction`.
      3rd pass (2026-08-31, all state vars named): **104 / 121** --
      `combatBeat_1..7`, `stageSfx_*` (named by trigger: hit / move /
      bump / attack / item / talk / event), `stageShopItem_1..3`,
      `setScenePosY_1..5`, `useCompass`, `showIndexedRemark`,
      `handleOverworldArrival`, `setupPromptScreen`, the resolveMoveTarget
      sub-tree. ~17 obscure sub-20-byte helpers left `sub_`.
- [ ] Confirm the tentatives: `addFoodDays` / `spendFoodDays` /
      `drawFoodGauge` (1F04/231C usage), `identifyLocationObject` /
      `readTileObject` (the `resolveMoveTarget` sub-tree), `rollCreatureStats`.
- [x] `ds:1F04` = `workInt` (2026-08-31) — OUT's general-purpose
      integer working variable (the single most-reused local, 27
      functions): push-to-value-stack / array index / the amount for
      `add/sub ds:hitPoints`. `workIntHi` (`ds:1F06`) is its high word /
      `idiv` divisor. Checked the other modules: **same offset in TWNDR
      (11 fn) and CASDR (21 fn)** but NOT a LEGLIB-fixed slot -- DUN's
      equivalent is `ds:20EA`, MUS's `ds:20B6`, and STDRV/CELDRV/MENU/
      SAVER have none. Second OUT DGROUP pass also added `trialX`/`trialY`
      (208C/208A), `selectedSpell` (1E24), `menuChoice` (1E22),
      `dgroupSeg` (0101), `remarkIndex` (1ADC) -> 33 OUT DGROUP vars.
- [x] Fixed the call-far fragmentation merge (2026-08-30) — was
      orphaning code when a fragment's successor wasn't adjacent. Now
      merges only truly-adjacent fragments + re-sweeps.
- [x] Decode the screen-string pool format (2026-08-30) — see
      [file-formats.md](file-formats.md#screen-string-pool-in-the-exe-not-a-file--decoded-2026-08-30).
      `dump_strings.py` recovers + annotates it; drove ~25 `out`
      function names.
- [~] Map the OUT engine state variables (2026-08-31,
      `dsvars.py` + `apply_dsvars_out.py`). 227 DGROUP words are touched
      by the code; ~15 are real cross-function state and are now named:
      `partyGold` (1AD2, 32-bit), `hitPoints` (1ADA), `playerX`/`playerY`
      (1B02/1B06), `contextMode` (1F2A), `subMode` (2146), `combatPhase`
      (2192), `encounterActive` (21FE), `questFlags` (2234),
      `chainDestType` (1F16), `enteredLocationId` (1F02),
      `turnActionFlag` (212E), `overworldArrayPtr` (24E6, far ptr);
      `tileAhead` / `activeCreaturePtr` / `targetSlot` tentative. The
      remaining ~200 are per-call BASIC scratch temps. Next: use these to
      name the ~55 `sub_` helpers, and confirm the tentative three.
- [ ] Combat helper cluster: `sub_1232F` / `sub_13D98` / `sub_14054`
      (all poke `activeCreaturePtr` + `combatPhase`) — the encounter
      resolution / talk / trade routines.
- [ ] Pin down the trailing `! # $ &` control codes in `drawString`.
- [ ] The post-thunk RTM-loader stub (`seg000:16E8C`+) is left unswept
      (`$`-terminated DOS strings + boilerplate) — disassemble if needed.
- [x] `BLOAD` sites for `OUTDATA.BSV` / `OUTM*.BSV` — `loadOverworldData`
      (`out` ~`0x14625`); builds `OUTM0<combatPhase>.BSV` + `OUTDATA.BSV`
      via `rtm_FE07`. Both land in seg `0x86AE` (`OUTM*` @0, `OUTDATA`
      @`0x2B22`) = one array bound at `ds:1E2A`.

## Data formats

- [x] **`.GLB` + `.GMP` (tile sheet + cell map)** (2026-08-31) — fully
      decoded; `decoders/glb_image.py` renders `TITLE` (title screen) and
      `SDMAP` (SDEFENDR combat-arena screen frame). `.GLB` = a flat sheet
      of 16-byte 8×8 CGA cells, **field-interleaved** (scanlines
      0,2,4,6,1,3,5,7); `.GMP` = a **column-major** cell map (dims in
      header words 3/4), word `W` -> tile `W//8`. The `.GLB` header
      word[1] is *not* tile width — tiles are always 8-px. CGA 320×200
      mode 4, palette 1. See file-formats.md.
- [~] `CHAR.DAT` (3444 bytes, fields mapped 2026-09-01) — 9 × 382-byte
      records: 14-B name + 74-B scalar block (`ds:1AC0..1B08`) + 7 int
      arrays with known `DIM` bounds (8/8/30/17/38/42/4 w, sum 294).
      Gold / HP / strength / experience / inv-count / overworld X,Y /
      level / game-speed scalars + the S5 shop price table
      placed. **S2 = 24-item possession bitmap, S0/S1 = equipment slots**
      (2026-09-02, from 4 PAULA save-diffs). Food / XP are runtime-only
      (not in the record). S2 FULLY decoded (24-item bitmap + 6 spell counts). The 5 attributes
      (Dex/End/Charm/Int/Str) = scalars 1AC0/1ACC/1ADE/1AF0/1B08. Remaining: XP/level.
- [x] `LEGACY.DAT` (2945 bytes, 2026-09-01, `decoders/legacy_dat.py` +
      `decoders/legacy_font.py`) — the master font/string/data table
      `menuStartup` loads into resident LEGLIB DGROUP. 6-B header; then
      **`0x006`–`0x605` = the 8×8 CGA software font** (96 glyphs ASCII
      0x20–0x7F, 16 B each, 2 bpp field-interleaved — `drawStringInner`
      renders all in-game text from it via `rtm_FE34`); `0x604`–`0x627`
      = the game-speed timing table (`ds:1E8E`); `0x628`–`0x647` = the
      first-person turn/step table (`ds:1C4E`, DUN/MUS `doMovement`);
      `0x648`–`0xA02` = the **123-string length-prefixed pool** (A–Z
      commands, weapon/armor/item/spell/gem-coin names, directions,
      responses, digits, 12 town names); tail = the 382-B new-character
      template + shop price table. (The "icon bitmaps" guess was wrong —
      it's a font.)
- [x] Overworld map graphics (2026-08-31, `decoders/outdata.py`) —
      mirrors the dungeon. `OUTM0/1/2.BSV` = map layers picked by
      `combatPhase`, 8-B header + 1 byte/tile, base grid **95 wide**
      (`0x5F` — the `imul ds:2444h,0x5F` is this width, *not* a record
      size). `OUTDATA.BSV` @ array `0x2B22`: `0x000`-`0x3FF` = **256 ×
      4-byte terrain records** (2×2 sub-cell indices; transitions in
      groups of 4), `0x400`-`~0xD1F` = **the ~146-cell 8×8 sub-cell
      bank** (`sub_28156` points `drawTileRun` here: `srcBase` = array
      `0x2F22` = OUTDATA `0x400`), `0x1402`+/`0x1F5C`+ = **64 × 124-byte
      object/creature sprite records** = MS-BASIC `PUT` GET-arrays
      (`dw 0x28`=40 bits=20 px ; `dw 0x14`=20 rows ; 100 B + 20 B pad),
      **20×20 single-frame** sprites, 32 image+`AND`-mask pairs (walking
      man, warrior, sword-fighter, pegasus, centaur, wings, ~24
      monsters) — `chainExec` draws them for the travel events;
      `decoders/outdata.py` renders the sheet. (2026-09-01: the old
      "40×12 / 2 frames" was a wrong 10 B/row stride.)
      `refreshMapView` builds a 26-wide index buffer from the terrain
      records; `rtm_FE69` blits it 26×17 via `drawTileRun`. All rendered
      clean.  `OUTOBJ.BSV` (landmark-icon bank) **structure decoded**
      (2026-09-01): BLOADs to `spriteBank` byte 0; 9-word ptr table →
      6 CGA blocks + a `0x1560+2n` column-address list; word[4] (`0x830`)
      is the `PEGASUS.BSV` slot for `combatPhase==2`; drawn via
      `identifyLocationObject` → scratch slots → `refreshMapView` phase
      2 → `rtm_FE69` (`sub_282FD` punches `0xFFFF` holes in the tile
      buffer, `rtm_FE0E` remap-copies `OUTOBJ[0..0x1B8]` in via a
      256-entry table at `tileBuf[0x1D8]`, then the terrain
      `drawTileRun` paints the merge).  Open: how a block's bytes become
      cells — a tile-cell merge, not a bitmap blit; needs a live memory
      dump (tile buffer + `0x1D8` remap table with a landmark visible).
- [~] Dungeon data (2026-08-31) — **rendering model cracked**
      (`drawViewSprite` / `blitViewCell` + LEGLIB `drawTileRun`
      `rtm_FE2A` / `andSpriteMaskCell` `rtm_FE2E` / `basPutSprite`
      `rtm_61`; 8×8 cells are the `.GLB` field-interleave):
      - `DUNM1/2/3.BSV` **fully decoded** — 8 levels × 16×16 tiles/byte.
        Feature codes: `0x01`–`0x07` = the 7 named hidden traps (POISON
        GAS VENT / FLOOR HOLE / SLIME SPLOTCH / TRIP WIRE / CEILING HOLE
        / TREASURE CHEST / BOX; spring → `+8`), `0x0A`/`0x0D` = stairs
        down/up, `0x10`+ = walls, `0xFF` = rock. Monsters (4 per file)
        recovered too. `decoders/dun_map.py`. (`0x09`/`0x0B`/`0x0C`/
        `0x0E`/`0x0F` "revealed" features not individually confirmed.)
      - `DUNDATA.BSV` (2026-08-31, `decoders/dundata.py`) — the
        first-person wall/floor/ceiling graphics; loads into
        `dungeonMapArray` (`ds:1E2A`) at array byte `0x800`, contiguous
        with `DUNM*`. `word[0]` = the tile bank's array offset
        (`0x0E94` → payload `0x694`). `0x020`–`0x10F` = the projection
        record table = **15 records**, walked by a `0xA/7/7`-word cursor:
        per depth a 10-word wall-band record (ceiling / floor / front-
        wall triples) + a 7-word left and 7-word right record (`videoOff,
        packedDims, p0..p3, pad`; `p0..p3` = the side wall in the 4
        columns of the `0x1BC` strip table). `packedDims =
        (ncols<<8)|nbands`, shrinking `3×15` → `1×5`. `0x110`–`0x693` =
        the tile lists — **every one a flat `ncols×nbands` cell-index
        array**, `0xFF` = skip, no marker layer. `0x694`–end = 255 ×
        16-B CGA cells. **Fully decoded** — `drawTileRun`, `blitViewCell`,
        the 15-record table, and all 4 wall-band drawers (incl. the
        blocked-view `Mid`/`Far` fallback, which re-use the side records'
        `p2`/`p3` and the depth-4 front-wall triple).
      - `DUNOBJ.BSV` (and `MUSOBJ.BSV` — same container, larger museum
        set, BSAVE `0x1447:0x0DB6`): `decoders/dunobj.py`. `BLOAD`ed into
        `spriteBank` (`ds:1E58`) at **offset 0** (the BSAVE `0x0DB6` is
        ignored) so every file offset = its `spriteBank` offset, no
        relocation; `DUNMON*` loads right after at word `0x1240`.
        region A = **40 records** (`dw 0x0110 ; dw endWord ; dw count ;
        dw startWord ; dw K`) in **5 depth-groups of 8**; the 8th of each
        group (records 7/15/23/31/39, at words `0x23/0x4B/0x73/0x9B/
        0xC3`) is the live mask descriptor, the other 7 are `count = 0`
        per-object descriptors (`startWord` marker + frame count `K`).
        `0x190`–`0x1A3` = the 6-word DUNMON bank table. `~0x247`–`0x8F1`
        = region B `(videoDest, maskSrc)` pair lists. `0x8F2`–`0x1211` =
        ~146 field-interleaved 8×8 object/decoration bitmap cells —
        **DUN.EXE reads none of this object path** (`drawViewSprite` is
        the only `spriteBank` reader and stops at region C); `MUSOBJ`'s
        equivalent region is dormant too — the OBJ-family container
        carries more than any renderer uses. `0x1212`–`0x15A1` = **region C: 57
        contiguous 16-byte AND-mask cells**. Loader **decoded**
        (`loadDungeonData`,
        `fix_dun_loaddungeondata.py`): `rtm_11` picks the array,
        `resolveAndOpenGameFile` (`rtm_FE63`) opens, `basBload`
        (`rtm_FE07`) reads header+payload — no relocation.
        **Consumer chain traced** (`fix_dun_coerce_gaps.py` coerced +837
        insns across 13 dun functions): `loadDungeonMonsters` (`0x12F9F`)
        reads `spriteBank[0x190]` (region A's 1st bank pointer, `0x1240`
        words) and BLOADs `DUNMONA` (levels 0–3) / `DUNMONB` (4–7) there;
        the 6 pointers = the 6 `DUNMON` records. Objects live in
        `viewObjectArray` (`ds:1C7C`) as 8 slots; `rebuildLevelView`
        stamps them into the map as `(slot<<4)|wall|0x10`;
        `renderDungeonView` → `drawViewSprite` draws them.
        `clearViewObjects` / `removeViewObject` tear them down.
        **Region B/C fully decoded**: `maskSrc` (2nd word of each region-B
        pair) is a direct byte offset `0x1212 + 16·n` into region C's
        57-cell pool. Each cell = a field-interleaved 8×8 2 bpp `AND`
        stencil (pair `11` keeps, `00` blacks); cells are dither patterns,
        reused within a depth and shared across depths. Composited per
        depth: a rectangular aperture with a dithered top edge that
        shrinks P0→P4 — the lit niche the `DUNMON` sprite is `PUT` into.
        The `0x8F2` object bitmaps are **dormant** — no `DUN.EXE` code
        reads them; `MUSOBJ`'s equivalent region is dormant too
        (`renderExhibitView` never blits from `spriteBank`).
      - `DUNMONA/B.BSV` — **fully decoded** (`decoders/dunmon.py`): 6
        blocks × 2356 B (one per monster-type slot), A = levels 0–3, B =
        4–7. Each block = 6-word frame-offset table (`9/725/973/1083/
        1148/1178`) + 3 zero words + **5 back-to-back stock MS-BASIC
        `PUT` GET-arrays** for view-depths P0…P4 (82×68 / 48×41 / 32×27 /
        24×21 / 16×14; `dw xBits ; dw yRows ; linear 2 bpp rows`, colour
        0 transparent). `drawViewSprite` picks the block by
        `viewObjectArray[8+slotClass] mod 6`, the frame by depth `P`.
      - `spriteBank` index arithmetic for `DUNMON` is now **resolved
        statically** — `bankBase = 0x1240 + monType·0x49A`, `getArray =
        bankBase + spriteBank[bankBase + P]`. `MUSOBJ.BSV` structure
        decoded 2026-09-01 (same `DUNOBJ` family, dormant — `MUS.EXE`'s
        `renderExhibitView` never blits sprites; exhibit art is
        `DIS*.BSV` + per-exhibit `.BSV`). No open sprite items remain
        on the dungeon/museum side.
      - **Palette animation: there is none** (checked 2026-09-01). The 5
        frames/block = the 5 view depths (static redraws); the 6 blocks =
        6 distinct monsters. `drawViewSprite` draws once per player
        action. CGA colour is set once per view by `rtm_FE29` (`out
        0x3D8,0x0A` / `out 0x3D9,0x30` = fixed palette 1 in the dungeon;
        map-data-driven on the overworld) — no register cycling anywhere.
        `decoders/dunmon.py --sheet`.
- [x] `TOWN0..B.BSV` (2026-08-31) — the 12 town layouts. **80×40 tile
      map** (`0x000`–`0xC7F`, confirmed by `setViewport` mode 0:
      `mapStride 0x50`, `mapHeight 0x28`, size `0xC80`) + object records
      + a `0x1A` slot table + shop-name text (`0x1300`+).
      `decoders/town_map.py`.
- [~] `TCASOBJ.BSV` (2026-08-31) structure mapped: loads into
      `spriteBank` (`ds:1E58`, seg `0x8537`) via `loadCastleObjects`;
      object records (`(offA, offA+0x80)` pair groups) + a CGA
      sprite/animation bank (`0x300`–`0xEFF`, ~12×`0x100` frames) + 4
      word-index tables + a tile tail. `FORTANIM.BSV` overlays the last
      `0x100`. Open: sprite-cell dims.
- [x] `CASTLE.BS1/2` / `FORT.BS1/2` (2026-08-31, bank rendered 2026-09-01)
      — the castle/fort floor maps: `0x0000`–`0x1FFD` = tile map (castle
      **90×91**, fort **112×73**, 1 byte/tile) + per-floor table + a
      shared **~234-cell CGA tile-graphic bank at `0x2400`** (field-
      interleaved 8×8, low-byte-first — walls, floors, doorframes,
      windows, torches, decoration; renders clean via
      `town_map.py CASTLE.BS1 --bank`). Interiors draw like the overworld:
      `drawInteriorTiles` blits a 26×17 8×8-cell grid via `rtm_FE1B`,
      tile bytes = direct graphic indices into the bank. Towns carry no
      bank of their own — they use the shared `bmTNCALB` tile set;
      `TOWN*.BSV` after the map holds a tile-code list + 4-byte
      `(x,y,type,0)` shop/door records + the wide-char shop names.
- [~] The standalone sprite atlases — `SDOBJ.GLB` and `BJCHR.GLB` tiles
      rendered (`decoders/glb_image.py`). `BJCHR` = a card rank/suit
      glyph font (upright + 180°-rotated) + card-back tiles, tiles
      0–127. Still open for both: the per-sprite tile grouping / index
      table.
- [x] `DIS0-15.BSV` (+ `DIS0A`/`DIS1A`) and `CEL0-3.BSV` (2026-08-31,
      `decoders/cel_image.py`) — one shared container. `DIS*` = the ~18
      **museum exhibit illustration screens**; `CEL0-3` + `DIS9` = the 5
      endgame-cinematic frames. 8-word header
      `[id, 0x10, W, H, 0x20, 0xA, mode, stripBase=0x220]`, then a **cell
      table** at `0xE0` (`dw videoDest ; dw stripPtr`, ≤80 entries,
      ending exactly at `stripBase`; `videoDest` bands step `0x140`,
      columns `+2`), then **16-byte field-interleaved 8×8 CGA cells**
      (`stripPtr` runs of `k·0x10` = `k` cells). **No RLE** — a frame
      stores only its changed cells with dedup; `celdrv_entry` loads
      each into `spriteBank` slot `bank·2000+1000` and rebases the
      `stripPtr`s. Rendered clean.
- [x] `MUSDATA.BSV` (2026-08-31) — **not music**; it's the Tarmalon
      Museum data: 3 exhibit floor maps (16×16, `0xE0`–`0xEF` = the 16
      display-case portals) at `0x000`–`0x7FF`, then a near-copy of
      `DUNDATA.BSV`'s dungeon-view tile/graphic data from `0x800`
      (`bmMUSDUNG` ≡ `bmDUNG`). `decoders/dun_map.py MUSDATA`.
- [x] Music format (2026-09-01, `decoders/music_mml.py`) — **no music
      file**; GW-BASIC `PLAY` MML string constants in `MENU.EXE`
      (`0x6EE4`, `0x7DB6`+) and `CELDRV.EXE` (`0x3920`+, same tune),
      played one voice via `basPlayMusic`/`rtm_CE` on the PC speaker.
      6 phrases: a looping 3-phrase title theme + a 2-phrase menu theme
      + a phrase-1 variant. Standard `oN`/`<>`/`lN`/`tN`/`a-g(+#-)(len)(.)`
      /`p`/`nN`/`mn`/`ml` dialect; parser emits note/freq/ms events.
- [x] `DRCONFIG.DAT` (2026-09-01, `decoders/drconfig_dat.py`) — `"DD2"`
      magic + 84 `<filename><diskcode>CRLF` records. Disk codes 0x00–0x03
      = floppies 1–4, 0x0E = "any play disk". `CONFIGUR.EXE` edits only
      the drive letters.
- [x] `TWNMSG.TXT` / `MUSMSG.TXT` (2026-09-01, `decoders/msg_txt.py`) —
      message banks: `word[0]` = table size = first-msg offset, then
      `word[0]/2` 1-based word offsets (monotonic run only), `\r`-
      terminated messages with `*`/`=`/`^`/`%`/`#X`/`@X`/`]…]` markup.
      37 town rumours + 39 museum-exhibit narration blocks.
- [x] `OUTDAT.DAT` (2026-09-01, `decoders/outdat_dat.py`) — the overworld
      **name / stat table** (NOT `OUTDATA.BSV`). `outInit` opens it as
      file #1 and reads 7 BASIC arrays whose DIMs tile the file: 24
      place names + 24 gem names + A1 (32 creature stat words: lo = HP
      15–200, hi = atk/XP) + A2 (32 w, reward + `0x01–0x04`/`0x63` tag)
      + A3 (32 w, combat -- `idiv`'d in `resolvePlayerAttack`) + **A4 = the
      12 town overworld (X,Y) coords** (verified vs the PAULA save) + 32
      creature names (index-aligned to the `OUTDATA.BSV` sprite pairs).
      The 24 place / 24 gem names are per-game random flavour IDs
      (`World-<x>` / `Stone-<y>` on the attributes screen). A2 tag / A3
      element meaning still open.
- [~] `D.BSV` / `R.BSV` (2026-09-01) — each = 220 field-interleaved 8×8
      sub-cells, i.e. **alternate overworld terrain sub-cell banks**
      (rendered; same format as `OUTDATA.BSV` 0x400). Likely the phase-1
      / phase-2 counterparts. Loader not yet found.

## pseudo-BASIC reconstruction (`recovered/`) — breadth-first pass DONE

Decided 2026-09-01 (with Paul): before the C++/ScummVM port, reconstruct
the compiled modules as reviewable pseudo-BASIC, one file per subsystem,
keeping the shared-`leglib` structure. The consolidated mechanics
reference distilled from it is **[game-logic.md](game-logic.md)** — start
there for the port. See also [recovered/README.md](../recovered/README.md).

**Model, done:**
- [x] `recovered/README.md` + `leglib.bas` — value-stack model, the
      **corrected** `ds:0F7C` op table (read from `LEGLIB.EXE`, NOT
      canonical order): `+ - -rev * / /rev cmp`; `rtm_FF2B` = `^`.
- [x] value-stack node layout — value at `[node.ptr]` == `[ds:111C]` for
      the top slot.
- [x] `decoders/dgroup_consts.py` — pull the `ds:xxxx` SINGLE/INT/STRING
      constant pool out of any unpacked module EXE.
- [x] to-hit formula **verified bit-exact** by two DOSBox traces:
      `Dex^0.8 * (weaponPower+18) / (creatureHP*11)`.

**Reconstructions (12 `.bas` files):**
- [x] OUT: `out_combat` (complete — to-hit/damage/spell/CreatureAttack/
      CreatureDefeated), `out_movement`, `out_economy`, `out_encounter`,
      `out_flags_items`.
- [x] DUN: `dun_combat` (`monsterAttack` un-folded + coerced via
      `ida_scripts/fix_dun_monsterattack.py`), `dun_traps`, `dun_chest`.
- [x] TWNDR: `twndr_services` (bank). CASDR: `casdr_castle`. MUS:
      `mus_exhibits`.

**IDA scripts added:** `expand_folded_out.py` (un-fold `creatureAttack`),
`fix_dun_monsterattack.py` (coerce the raw monster-hit loop).

**Corrections this pass forced** (not yet folded into `apply_dsvars_*`):
- `ds:24E6` (OUT) / `ds:2274` (DUN) / `ds:25B0` (CASDR) = the SINGLE `1.0`
  passed to `RND()`, **not** a far pointer — the `overworldArrayPtr`
  dsvars label is wrong.
- `ds:1AC2:1AC4` (CHAR.DAT `+0x10`, "experience") = the **bank balance**;
  LotA has **no XP stat**.
- `ds:2192` (OUT) = the shared workInt / rolled damage inside
  `resolvePlayerAttack`, mislabelled `combatPhase`.
- CHAR.DAT `1AEA`/`1AEC` = equipped armour slot/id; `1AFC`/`1AFE` =
  equipped weapon slot/id (see file-formats.md).

**Constants — ALL resolved statically** (2026-09-02). The formulas' ~7
"runtime-loaded" constants each turned out to be an `OUTDAT.DAT` byte, a
`*.EXE` constant read at the wrong width (`ds:2C3C` is an 8-byte double =
999.0; `ds:226E` = 3.5 castle / 1.0 fort), or a formula. Operand order
confirmed by the verified to-hit + damage traces. No DOSBox dumps needed
to implement the mechanics. `ds:2092`/`ds:2096` (OUT region encounter
gates) still merit a per-region table for tuning — 3 samples so far.

**Still open** (not blocking a first implementation):
- [x] **OUT enterOverworld / loadOverworldData** (`out_overworld.bas`) —
      map layer = `MIN(S4(12), 2)` → `OUTM0/1/2.BSV` (+`PEGASUS.BSV` on
      layer 2); `OUTDATA` → `overworldArray[0x2B22]`, `OUTOBJ` →
      `spriteBank[0]`. OUTM header = CGA mode/palette. `initOverworldState`
      recomputes `S4(19)` max HP each entry. Found an OUT.EXE self-checksum
      (`ds:2236 ≠ 0x9D1A` → max HP crippled to 20).
- [x] **casino payouts** (`gmb_casino.bas` v2) — BlackJack net settle
      (`±bet`, `+2·bet` natural); Flip-Flop `multTable{1,2,5}·bet` +
      colour bonus, `computePayout` rigs the bucket ±1 toward realised
      payback ~0.94 (`S4(14)/S4(15)` ledger, resets to 99/99). Both games
      cut you off at `gold − startGold > 250·characterLevel + 750`
      (`imul ds:1AE0`).
- [ ] `selectAbove` mode-4 internals (LEGLIB), `OUTM1`'s role,
      flip-flop `ds:2B40` / `ds:2AA6`.

## ScummVM engine (future)

Not started. Same end goal as `ultima1`/`ultima2`: a clean C++
reimplementation, then a ScummVM engine module. Documentation is now in
shape for a fresh implementation thread to work from:
**[game-logic.md](game-logic.md)** (mechanics) +
**[file-formats.md](file-formats.md)** (on-disk data) +
**[overview.md](overview.md)** (architecture). The `recovered/*.bas`
files are the function-level source of truth.

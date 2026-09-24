# Roadmap

Current status and prioritized next steps for the shared Chapter 2/3
engine work (`yendor2.idb`/`yendor3.idb`, `docs23/`, `src23/`). See
[overview.md](overview.md) for the full narrative and
[engine-diffs.md](engine-diffs.md) for the Chapter 2 vs. Chapter 3
behavioral-difference reference.

## Status (last updated 2026-09-24, added: combat turn order)

**Disassembly-level analysis is essentially done.** This is the
important thing to know before starting the C reimplementation: you
should not need to go back to IDA for basic engine logic — the
groundwork below is already in place.

- `yendor2.idb`: all 769 functions named, all global variables named,
  and the three major on-disk formats decoded (see `file-formats.md`).
- `yendor3.idb`: the full 197-function BinDiff match review against
  `yendor2.idb` is complete (all three confidence tiers), the
  segmented-addressing global-rename blocker is fixed, and every
  confirmed Chapter 2 vs. Chapter 3 behavioral difference is written
  up in `engine-diffs.md`. A handful of very minor, non-blocking
  function identities remain unresolved (`engine-diffs.md`'s "Review
  status" section has the current list) — none of them are load-bearing
  for reimplementation; they're mostly debug hooks, a studio-credits
  easter egg, and one new item-registry helper whose exact semantics
  aren't pinned down.
- `src23/`: the C reimplementation is under way. One module is done:
  `bcd4.c`/`bcd4.h` (packed-BCD arithmetic, now including the digit
  shifts and `bcd4MulPercent`, i.e. `MulBCD4ByWord`, which is really a
  multiply-by-percent). Build/test with MinGW GCC:
  `gcc -Wall -Wextra -std=c99 -I .. -o test_bcd4 test_bcd4.c ../bcd4.c`
  from `src23/tests/`. Second module, `savegame.c`/`.h`
  (`CURGAME`/`SAVGAMEn` container: layouts for both games, section and
  record accessors, name/header-field helpers), also done; tests in
  `tests/test_savegame.c`. Third module, `party.c`/`.h` (500-byte party
  record: fields, the 27-stat table, classes, inventory/equipment/bags,
  flag banks; game-aware for Chapter 3's small differences), with shared
  `game.h`; tests in `tests/test_party.c`. Fourth module, `item.c`/`.h`
  (`WORLD.DAT` item catalog for both games: records, target and effect
  tables, name joining), tests in `tests/test_item.c`. Fifth module,
  `monster.c`/`.h` (catalog blocks + type lookup for both games, the 156-byte
  live record, spawn, death flags), tests in `tests/test_monster.c`. Sixth
  module, `effect.c`/`.h` (`g_trapEffectDefs` for both games: cost,
  resistance and magnitude decoding), plus a new `random.c`/`.h` (faithful
  `RandomInRange` port — its range is inclusive, see `file-formats.md`),
  tests in `tests/test_effect.c` and `tests/test_random.c`. Seventh module,
  `worldmap.c`/`.h` (the world map: one continuous 800-column tile grid for
  both games, plus Chapter 2's wall/floor tile-legend tables), tests in
  `tests/test_worldmap.c`. **Big finding**: there is no per-level map data
  — the whole game (towns, wilderness, dungeons) is one seamless coordinate
  space. Eighth module, `document.c`/`.h` (in-world readable text: found
  books/notes/plaques, *not* NPC dialogue despite the disassembly's
  "RunConversation" name — see `engine-diffs.md`), tests in
  `tests/test_document.c`. Both halves of the previous "`WORLD.DAT` map/text
  blocks" item are now done. Ninth module, `movement.c`/`.h` (the core
  dungeon loop's first slice: `HandleMovementInput`'s 6-action
  direction/turn/strafe math, per-game playable bounding boxes, and the
  full per-cell passability decision — door/lock, special-cell range,
  force-move override, `ClassifyFloorType`, `IsCellTypeImpassable`, all
  game-aware since the two games' numeric thresholds differ substantially
  and non-uniformly), tests in `tests/test_movement.c`. **Correctness
  note**: an earlier, uncommitted pass at this module got the threshold
  bands wrong by extrapolating instead of reading the real compare
  chains — see `engine-diffs.md`'s movement section for the corrected
  exact bands and the lesson. Tenth module, `dungeongrid.c`/`.h` (the
  in-memory 78x78 dungeon-grid window: `RefreshDungeonMapWindow`'s
  origin-clamp formula, reusing `movement.h`'s `MovementBounds`
  directly, and the base per-cell copy — wall/floor types from the
  world map plus the explored-map bit from `CURGAME`, MSB-first
  packed), tests in `tests/test_dungeongrid.c`. **Corrected the
  existing grid-cell field count**: there's a third word at `+4`,
  zeroed on build, not 2 bytes of padding after `+2` — see
  `file-formats.md`. **No Chapter 2 vs. Chapter 3 behavioral
  difference found in this module** (see `engine-diffs.md`), the first
  module in this project where that's been true. `HandleSpecialCellEntry`
  investigated and ruled out as a reimplementation candidate for now —
  it's pure rendering/animation, nothing left to extract once
  `movement.c`'s `MovementCellSpecial` outcome is accounted for.
  Investigating its sibling `TryInteractAtPosition` (the real source
  of the door/lock flag) surfaced a previously-undecoded `WORLD.DAT`
  block, since fully decoded as the eleventh module,
  `worldobjects.c`/`.h`: a sparse per-cell object index (720
  column-offset entries into sorted-by-row 6-byte records — doors/locks,
  curgame-record triggers, and scripted monster-spawn markers), found
  at `WORLD.DAT` offset `0x1A1141` (Chapter 2) / `0x41090D` (Chapter 3)
  via `extract_resource_stubs.py` (new for `yendor3/`), tests in
  `tests/test_worldobjects.c` with exact reachable-record and per-flag
  counts checked against both real `WORLD.DAT` files. See
  `file-formats.md`'s "World object index" section for the full flag-bit
  table. Still open: flag bits `0x2000`/`0x400`'s consumers (if any —
  `0x2000` is real and common but untested by any traced caller).
  **The EMS-paging blocker flagged for `LoadLockState`/`ShowLockStatus`
  turned out not to be one — decoded as the twelfth module,
  `lockcatalog.c`/`.h` (2026-09-23)**: per Paul's steer, the EMS
  paging itself never needed reimplementing, only the flat bytes it
  was populated from. Traced `LoadLockState`'s EMS mapping-array
  pointer back to `loadWorldDat2`/`loadWorldDat3` (the loaders that
  first populate that EMS handle from `WORLD.DAT`) and found their
  offsets were **already in this session's earlier
  `extract_resource_stubs.py` dump**, just not yet matched to a
  consumer — `WORLD.DAT` offset `0x7E5BA` (Chapter 2) / `0x8F00A`
  (Chapter 3), 26 bytes/record, 608/1008 records (matching
  `SaveSectionLockAndShopState`'s `recordCount`). Caught two real
  mistakes before they reached committed code — see `overview.md` for
  both: a wrong bit-to-key-name mapping from trusting `.asm` label
  declaration order instead of verifying real addresses (a new script,
  `check_lock_key_strings.py`, resolved it — `0x8000`=BRASS, not GOLD
  as it first looked), and a wrong "one-hot selector" assumption from
  only checking Chapter 2's real data (123/1008 Chapter 3 records
  actually have more than one key-type bit set). Tests in
  `tests/test_lockcatalog.c` with exact per-key-type and
  multi-bit-record counts checked against both real `WORLD.DAT` files.
  See `file-formats.md`'s "Lock/door definition catalog" section and
  `engine-diffs.md`'s comparison (a real, sharper-than-usual Ch2/Ch3
  content difference here: one-hot vs. genuine bitmask key
  requirements). Still open there: flag bits `0x1`/`0x2`/`0x80`'s exact
  meaning and the remaining 22 undecoded bytes/record.
  **`LoadCurgameRecord` investigated further, genuinely unresolved**:
  it reads from the exact same EMS-backed region as the lock catalog,
  at a base offset (`_val9`/`word_3320E`) that's `600` in Chapter 2 but
  **always 0 in Chapter 3** (that global is read but never written
  anywhere in the whole disassembly) — meaning Chapter 3's
  curgame-record table would start at offset 0, overlapping lock id
  1's own record entirely. Not determined whether that's a real quirk,
  dead code, or simply never exercised in practice; see
  `file-formats.md`'s "Lock/door definition catalog" section for the
  full note. **`g_levelMonsters` placement/despawn — done as the
  thirteenth module, `monsterpool.c`/`.h` (2026-09-23)**: the rest of
  `RefreshDungeonMapWindow` past `dungeongrid.c`'s base build — a
  79-wide (`[origin, origin+78]` inclusive) scroll check per pool slot
  that relinks a still-visible monster (new cell offset, a "monster
  here" overlay baked into its `DungeonGridCell`) or despawns one
  that's scrolled out (zero the record, clear its spawn flag).
  **Corrected a real misreading from the `worldobjects.c` writeup**
  along the way: the `0x800` marker's `value` field is a monster
  **type id** directly (confirmed via the despawn path, which clears
  the same `SaveSectionMonsterSpawnFlags` bitmap by type id), not an
  abstract spawn-point index as first documented — fixed in both
  `worldobjects.h` and `file-formats.md`. Also resolved
  `dungeongrid.c`'s open `+4` field question: it's a "monster here"
  marker written at two life-cycle stages — an already-spawned
  monster's own scroll-relink (this module), or
  `TryInteractAtPosition`'s not-yet-spawned marker (`errorCode=5` from
  a `worldobjects.c` `0x800` record, still not reimplemented) — not a
  fixed-role field, and **not** written by the `0x4000` curgame-record
  branch as an earlier pass this session mistakenly concluded (that
  branch only ever overwrites wall/floor type, `+0`/`+2`; caught by
  rereading the exact `errorCode`-to-branch mapping, which had 5 and 7
  swapped). Tests in `tests/test_monsterpool.c`; all 14 suites pass.
  **No Chapter 2 vs. Chapter 3 difference** (see `engine-diffs.md`) —
  instruction-identical, same as `dungeongrid.c`.
  **How a monster first gets spawned — done, same module,
  `SpawnMonsterInFacingDirection` (2026-09-23)**: turned out to be
  mostly composition of functions an earlier session already wrote
  (`monster.h`'s `monsterRecordSpawn`/`monsterRecordPlace`/
  `monsterRecordStartAnimation`), plus one new piece — a
  facing-dependent spawn-position offset table (4 tables x 51 entries,
  a perspective-cone shape, extracted directly and confirmed
  byte-for-byte identical between both games). `TryTriggerMonsterEncounterAtCell`
  (the rendering-driven caller) was read enough to understand the
  trigger but not reimplemented — needs the SDL2 layer to mean
  anything. **No Chapter 2 vs. Chapter 3 difference** here either.
  **Side-trap/ambush pipeline investigated, corrected a stale doc, new
  lead found (2026-09-23)**: `ProcessSideTrapsOnMovement`'s "80-entry
  wall/cell table" (an existing, pre-this-session doc claim) is
  actually `g_levelMonsters` itself — same base/stride, confirmed by
  cross-checking against every other walk of that pool. Its trap flag
  (`+0xE` bit `0x1000`) turns out to be settable two ways: a wall/door
  trap (creation path not traced) or a monster ambush, set by
  `ProcessLevelMonsters` (monster AI/turn processing, not traced at
  all yet) via a proximity roll against a *second* bit range of
  `MonsterFieldAwareness` (`0x200`-`0x1000`, distinct from the
  already-documented `0x20`-`0x100` range). The trap-avoidance roll
  itself reads pool-record fields (`+0x60`/`+0x62`/`+0x64`/`+0x66`/`+0x70`)
  that fall within the monster catalog block's own byte range but
  aren't among `monster.h`'s named fields — suggesting wall traps
  carry a full catalog block too, reused for trap data instead of
  monster stats. Genuinely comparable in scope to the monster-pool
  work just finished, but needs `ProcessLevelMonsters` traced first;
  full writeup in `file-formats.md`'s "side trap"/ambush section.
  Deliberately not reimplemented this session — a good candidate for
  its own dedicated pass.
  **`GrantMonsterRewards`/`RemoveMonsterFromMap` — done, same module
  (2026-09-23)**: `ProcessLevelMonsters`'s two "monster's presence
  ended" calls turned out much more tractable than the AI/movement
  logic around them — small, clean, composed almost entirely from
  functions already in `bcd4.c`/`monster.h`/`dungeongrid.h`. Needed one
  new mechanism: the "Global quest/world-state flags" system a prior
  session had already identified (`file-formats.md`) but not
  reimplemented — traced its exact 1-based, MSB-first bit-packing (a
  zero-remainder special case that steps back a word) directly rather
  than assuming it matched this session's other, 0-based conventions,
  and confirmed it doesn't. New module `src23/globalflags.c`/`.h`
  (pure bit arithmetic over a caller-supplied buffer, since
  `g_globalFlags`'s real size/persistence still isn't confirmed);
  `monsterGrantRewards`/`monsterPoolRemove` added to
  `src23/monsterpool.c`/`.h`. Tests in `tests/test_globalflags.c` and
  `tests/test_monsterpool.c`; all 15 suites pass. **No Chapter 2 vs.
  Chapter 3 difference**, spot-checked directly.
  **`ProcessLevelMonsters`'s approach/ambush check — done (2026-09-23)**:
  turned out not to be movement AI at all — a monster's world position
  is never touched; it only checks grid-alignment (same row/column) and
  a short obstacle-free path to the party, then arms an ambush in
  place. Found a **confirmed Chapter 2 bug fixed in Chapter 3**: the
  5-step scan bound is only explicitly set on one side of the
  alignment axis in Chapter 2, the other reusing an unrelated
  outer-loop counter (pool-slot-index-dependent, not deliberate) —
  Chapter 3 adds the missing initialization; see `engine-diffs.md`.
  Added `monsterClassifyObstacle`/`monsterApproachParty` to
  `src23/monsterpool.c`/`.h` and `monsterAmbushThreshold` to
  `monster.c`/`.h` (plus named constants for two previously-unlabeled
  bit ranges: `MonsterFieldWound`'s direction/ambush bits and
  `MonsterFieldAwareness`'s second bit range). Tests in
  `tests/test_monsterai.c`; all 16 suites pass.
  **`TickMonsterTimer` and the full per-slot flow — done (2026-09-23)**:
  the mechanism (a gated state machine that decrements
  `MonsterFieldHealth` by `MonsterFieldTickAmount`, once or twice, with
  an independent countdown-driven reset) is fully confirmed and
  reimplemented as `monsterTickTimer`; *why* it exists is genuinely
  unresolved — neither traced caller ever sets the bits/fields it
  reads, only consumes the result, so this is reimplemented faithfully
  without inventing a narrative for it. Composed with the
  already-done pieces into `monsterPoolProcessSlot`, matching
  `ProcessLevelMonsters`' exact per-slot order. **A real bug caught by
  the test suite itself**: `MonsterStateBusy` (`0x800`) turns out to
  also be one of `TickMonsterTimer`'s own gate bits, so an early test
  case triggered a real, unintended tick that reached the
  reward-granting step with no staging buffer — a segfault, diagnosed
  via unbuffered stdout and fixed. Tests split between
  `tests/test_monster.c` (state machine) and `tests/test_monsterai.c`
  (composed flow); all 17 suites pass. **No Chapter 2 vs. Chapter 3
  difference**.
  **`TryActivateMonsterByDistance` — done (2026-09-23)**: the actual
  `MonsterStateAware` setter (everything else this session only reads
  it). Instruction-identical between both games. Checked a hypothesis
  about the existing `MonsterAwarenessFar`/`Middle`/`Near` names
  reading backwards (preferred engagement range vs. detection range)
  against Chapter 2's real monster catalog — not supported by the
  data; recorded as still-unresolved rather than forced either way.
  Wired into `monsterPoolSpawn` at the same point the original calls
  it. Tests in `tests/test_monster.c`; all 17 suites pass. With this,
  every fully-confirmed piece of `ProcessLevelMonsters` and
  `SpawnMonsterInFacingDirection` is reimplemented.
  **`TryInteractAtPosition` — done as a new module, `interact.c`/`.h`
  (2026-09-24)**: the per-cell interaction dispatcher run on every
  movement step, the actual consumer of `worldobjects.c`'s records —
  fully traced and reimplemented, all 11 `errorCode` outcomes. Found a
  previously-undocumented CURGAME structure along the way: the
  "already unlocked"/"already triggered" check the door and
  curgame-record branches both make turns out to be the **same shared,
  bit-packed bitmap** (CURGAME section 4, `SaveSectionEventState` —
  previously only documented as generic "byte-addressed state"),
  confirmed end-to-end by reading `UnlockDoorCommand`'s write-back.
  Locks get bits `[0, lockCount)`; curgame records get bits offset by
  `_val10` (confirmed to be exactly Chapter 2's lock count, 608) so the
  two id spaces don't collide — **except in Chapter 3, where the
  equivalent global (`word_2ECF8`) is read but never written, always
  0**, the same always-zero-global quirk already found for
  `LoadCurgameRecord`'s unrelated EMS-record multiplier
  (`word_3320E`/`_val9`) — two independent always-zero offset globals
  in the same function, still not conclusively a bug vs. dead code.
  Added `LockFlagUnknown40` to `lockcatalog.h` (real bit, now known to
  select `errorCode=8`, meaning still unconfirmed). Full writeup in
  `file-formats.md`'s new "TryInteractAtPosition" section. Tests in
  `tests/test_interact.c`; all 18 suites pass. Still open: how a
  wall/door trap pool entry (as opposed to an ordinary monster)
  actually gets created, whether monsters ever reposition themselves at
  all and via what function (**resolved 2026-09-24: they don't — see
  the later status entry below**), `LoadCurgameRecord`'s own 4-byte EMS
  record format (a separate, still-unresolved question from the bitmap
  above), `ShowLockStatus`/`HandleSpecialCellEntry` (pure UI/rendering,
  deferred to the eventual SDL2 layer), map-trigger effects, and
  first-person viewport rendering (needs the SDL2 layer, not yet
  started).
  **The wall/door-trap-creation search (2026-09-24) didn't find a
  separate creation path, but strengthened the existing "traps are
  reused monster catalog entries" hypothesis**: `RollTrapAvoidanceMagnitude`'s
  own field offsets (`+0x64`/`+0x66`) turn out to be the *exact same
  bytes* as `monster.h`'s already-named `MonsterFieldRangedAccuracy`/
  `RangedDamage` — real fields on an ordinary monster catalog block,
  not a separate trap-record layout. Still not proven (no confirmed
  "this type id is scenery, not a monster" marker found), so still
  listed as open above rather than closed. While chasing it, confirmed
  `SpawnMonsterInFacingDirection`'s `0xE4E9`/`0xCE51` death-flag-override
  table lookup was already correctly reimplemented by a prior session
  (`monster.c`'s `monsterDeathFlags`) — re-discovered, not new.
  **`ShowLootAndAwardExperience`'s staging drain and character leveling
  — done as new pieces of `monsterpool.c`/`.h` and `party.c`/`.h`
  (2026-09-24)**: `GrantMonsterRewards`' own doc comment already
  pointed at this as the missing piece — draining the 4 staging BCD
  counters into permanent totals (confirmed the exact,
  not-obvious-from-names mapping: ore → `SaveHeaderOreCounter1`, nuore
  → `SaveHeaderOreCounter2`) and awarding experience to the party.
  Experience awarding triggers `CheckForLevelUp`, decoded as a new
  `party.c` function, `partyCheckForLevelUp` — walks a **89-entry
  packed-BCD XP-threshold table**, extracted directly from both EXEs
  via a new IDA script (`dump_xp_threshold_table.py`) rather than
  guessed, capping at level 90 exactly matching `PartyFieldLevel`'s
  existing "capped at 90 by training items" doc note. Computes a
  pending level into a new field, `PartyFieldPendingLevel`, but — like
  every traced caller — never applies it. Found a real, exactly-
  reproduced asymmetry in the threshold comparison (first step `>=`,
  cascade steps `>`) and a genuine Chapter 2 vs. Chapter 3 content
  difference in the XP curve itself, including different "unreachable"
  cap sentinels (90,000,000 vs. 99,999,999) — see `engine-diffs.md`.
  **What applies `PartyFieldPendingLevel` — resolved, same day: nothing
  does.** Traced every reader of it (`ShowLevelUpMessage`,
  `DrawPartyMemberStatusPanel`'s training-icon glyph, `CheckAndAnnounceLevelUp`)
  and found it's purely a UI eligibility hint. The actual leveling
  mechanic is `UseTrainingItem` (`yendor2.asm:21510`) — a wholly
  separate, **gold-gated** system: pay a fixed cost, `PartyFieldLevel`
  increments by exactly 1 (capped at 90) regardless of
  `PartyFieldPendingLevel`'s value, max HP grows from a percentage of
  max Stamina, and a class-dependent MP/skill spread and secondary-class
  promotion follow. This is precisely why `PartyFieldLevel`'s
  pre-existing doc says "capped at 90 by training items" — XP alone
  can't reach past its curve's real end (level 39 in both games); the
  remaining 51 levels are training-item-only.
  **`UseTrainingItem`'s core state-mutating branch — done, same day
  (2026-09-24)**: `partyApplyTraining`/`partyClassPromotionThresholds`
  in `src23/party.c`/`.h`. Cost gate, level increment/cap, HP growth
  (30% of max Stamina), the full 6-case per-class-base MP-growth
  weighting (cross-checked against an older, independent note in
  `file-formats.md` about `RestCharacter`'s matching class-id
  reduction), the two flat-`+2` attribute/skill growth loops, and
  secondary-class promotion are all reimplemented and instruction-
  identical between the games *except* the promotion thresholds — a
  **third** always-zero-global quirk found in Chapter 3
  (`word_331F8`/`word_331FA`), disabling secondary-class promotion via
  training entirely; see `engine-diffs.md`.
  **`UseTrainingItem`'s tail calls — done, same round**:
  `partySyncStagedStats` (`SyncPartyRecordStagedStats`) and
  `partyRefreshCarryCapacityAndAttributeBonuses`
  (`RefreshCarryCapacityAndAttributeBonuses`'s own body, not its tail
  call — see below), both wired into `partyApplyTraining`'s own tail in
  the original's exact call order. Found training does more than grow
  maximums: `SyncPartyRecordStagedStats` pulls every stat's *current*
  value up to its (possibly just-grown) max too — real, easy to miss
  from the growth formulas alone, confirmed by tracing the copy's exact
  byte-range math against the record's field offsets rather than
  trusting the name. `RefreshCarryCapacityAndAttributeBonuses`
  recomputes carry capacity (10× Strength) and two new fields,
  `PartyFieldStrengthBonus`/`DexterityBonus` (+ `Max` variants) — 20%
  of however far Strength/Dexterity sits past 72 — resolving an old
  `file-formats.md` note that had flagged those fields without knowing
  what computed them.
  **`RecomputeEquipmentStatBonuses` — done, same round**: turned out
  not to need any new item-catalog decoding after all —
  `partyRecomputeEquipmentStatBonuses` uses `item.c`'s already-existing
  `itemCatalogRecord`/`itemTargetEntry`/`itemTargetWord` directly, once
  reading `LoadItemCatalogRecord` itself showed its return value in
  this context *is* the target-entry pointer `itemTargetEntry` already
  computes. Named the gap's remaining three baseline fields
  (`PartyFieldEquipRatingBase1`/`2`/`3`, confirming a real swap: base
  *2*'s field is the gap's third slot, base *3*'s is its second) and
  fully traced `PartyStatEquipRating1-5`'s per-slot formula: main
  weapon → Projectile skill + weapon bonus; second/off-hand slot → a
  melee skill selected by the item's own type flag (Slashing/Bashing/
  Polearm) + its bonus, plus a `PartyFieldUiFlags` bit 0x20 side effect
  (meaning unconfirmed); every other equipped item accumulates into a
  fifth rating. Not called automatically by `partyApplyTraining` (needs
  an `ItemCatalog` it doesn't take) — deliberately kept as a separate,
  composable function.
  **The ability/spell-unlock table walk — done, same round**: dumping
  `DS:0xD22B` directly (new IDA scripts in both games) at first looked
  like up to 10 rows, but rows 6-9 turned out to be reads past the real
  table's end — confirmed architecturally, not just by eyeballing the
  data: the real table is exactly 6 rows (one per class base 4-9), and
  `tableBase + 6*0x50` lands *exactly* on `TravelToDestination`'s own
  destination-table base address in **both** games. This resolves class
  base 1-3 (FIGHTER/MERCHANT/ROGUE) at *any* tier as a genuine,
  reachable original-engine quirk: their row-index arithmetic always
  lands out of the real table's bounds, reading `TravelToDestination`'s
  unrelated data as if it were ability ids — not reproduced;
  `partyAbilityUnlocksAtLevel` returns 0 ids for these instead. Ability
  ids themselves genuinely differ between the games (same per-game
  id-space pattern as everywhere else); the table shape doesn't. Wired
  into `partyApplyTraining` automatically (it needs no extra
  dependency, unlike the equipment-bonus step), using the
  pre-promotion class id, matching the original's exact call order.
  Deliberately still deferred (own future pass, wider blast radius than
  training itself): `RunItemServiceRecipientLoop` (offering the item to
  other party members — resolved an old "feeds a separate growth
  calculation" note to "feeds UI flow control, not a stat"). `cost` is
  caller-supplied, since this project hasn't built the upstream
  item-use pipeline (`UseItem`) that would resolve an item's own price.
  Tests in `tests/test_party.c` (per-class MP-formula spot checks, both
  caps, untrained-slot skip, promotion at both Ch2 thresholds and its
  Ch3 absence, the current-value sync, the excess-over-72 bonus, the
  full equipment-bonus formula against a synthetic item catalog, and
  the ability-unlock table including the Ch2/Ch3 content difference and
  every class-base-1-3/any-tier "no valid row" case). Full writeup in
  `file-formats.md`. All 18 suites pass (rebuilt entirely across all
  three rounds — surfaced and fixed several pre-existing stale
  build-command comments, `test_effect.c`/`test_monsterpool.c`/
  `test_monsterai.c` all missing dependencies `party.c` picked up).
  **Whether monsters ever reposition themselves — resolved, same day:
  no.** Swept every reachable loop over `g_levelMonsters` in the
  disassembly (not just `ProcessLevelMonsters`), not only searching but
  actually reading each one, looking for any write to a live record's
  world position outside its one-time spawn placement. Found none —
  every consumer spawns, reads, or removes a record wholesale, never
  moves one. Surfaced two things along the way, neither reimplemented:
  a genuine separate combat subsystem (`BuildCombatTurnOrder`/
  `ProcessCombatRound`, a 3-slot `g_monsterSlots` staging array distinct
  from the 80-slot pool) and a previously-undocumented item-effect
  "banish the monster on the facing tile" mechanic
  (`ApplyEncodedItemEffect`, `yendor2.asm:51586`) that copies a
  record out to a UI scratch buffer before despawning it, no rewards
  granted. Full writeup in `file-formats.md`'s "Monster approach and
  ambush check" section.
  **Turn-based combat's turn order — started, same day (2026-09-24)**:
  the combat subsystem surfaced above turns out to be a genuinely
  separate, fixed-size 3-slot monster pool (`g_monsterSlots`, full
  156-byte record copies, not pointers into the 80-slot dungeon pool).
  New module `src23/combat.c`/`.h`: `combatBuildTurnOrder`
  (`BuildCombatTurnOrder`, `yendor2.asm:10952`) builds a single
  Dexterity-descending turn order over every living party member and
  occupied monster slot (a stable insertion sort matching the
  original's swap-only-on-strictly-greater exactly) and assigns each
  monster a random living party target; `combatSelectActiveMonster`
  (`SelectActiveMonster`, `yendor2.asm:11340`) picks the next
  not-yet-defeated monster in that order. Found and deliberately did
  not reproduce a dead branch: a pre-check that reads a turn-order
  slot's flags before that slot has ever been written on the same
  call, so it can never actually fire. Represents each monster's
  random target as a 1-based `SaveHeaderPartySlots` id (this project's
  existing convention) rather than the original's raw pointer.
  **Instruction-identical between the two games**, checked directly.
  Tests in `tests/test_combat.c`; all 19 suites pass. **Still open,
  the much larger remaining piece**: `ProcessCombatRound`'s actual
  turn advancement and attack resolution — this round only covers
  turn-order *construction*. Full writeup in `file-formats.md`'s
  "Turn-based combat: turn order" section.

## Next: continue the C reimplementation

This is the current priority. Recommended approach (established
early in this project and still the right one): scope one module at a
time rather than trying to plan the whole engine up front, doing any
remaining disassembly-level verification inline as each module is
written rather than as a separate upfront pass — the groundwork above
means that should rarely be necessary now.

**Naming conventions** (established, keep following them):
lowerCamelCase for C functions/variables (e.g. `bcd4Add`,
`bcd4FromU16`); `g_` prefix for actual global variables; PascalCase
for type names (e.g. `Bcd4`).

**Write for both games from the start.** `src23/` is shared between
Chapter 2 and Chapter 3 specifically so this doesn't need to be
revisited later — when a function differs between the two (check
`engine-diffs.md` first), reimplement the union of behavior with a
runtime or compile-time switch rather than picking one game's version.

**Platform backend: SDL2**, decided 2026-09-22. Graphics, sound, and
input in the C reimplementation are built on SDL2, not a bespoke
DOS-VGA/EMS-shaped layer — chosen specifically because the eventual
target is a ScummVM engine, and SDL is the closest available API shape
to what ScummVM's own backend expects, so the adaptation later should
be smaller. Practical implications for upcoming modules:
- Keep a clean split between game logic (the data-model modules
  written so far: `bcd4`, `savegame`, `party`, `item`, `monster`,
  `effect`, `random`, all platform-independent) and a thin platform
  layer that owns the SDL2 calls — window/surface, blitting,
  palette/color conversion, audio playback, keyboard/mouse polling.
  Don't let SDL types leak into the data-model headers.
- The original renders into a palettized VGA framebuffer
  (`PICTURES.VGA`, a master 256-color palette at `WORLD.DAT` offset
  `0x8270A` — see `file-formats.md`); the SDL platform layer should
  decode/composite into an indexed or converted-RGB surface and let
  SDL handle the actual blit/present, rather than reimplementing
  VGA-register tricks.
- Audio: the original drives Sound Blaster/FM synth through
  `SBFMDRV.COM` and an in-EXE sound-event table (`TriggerSoundEvent`,
  `word_368A7` family) — not yet decoded. When that's tackled, target
  SDL_mixer or raw SDL audio callbacks rather than emulating the
  original driver protocol.
- No SDL-specific game code has been written yet; this section exists
  so the decision is on record before the rendering-dependent modules
  (dungeon loop, `PICTURES.VGA`) start.

**SDL2 is installed on this machine** (2026-09-22): SDL2 2.32.10 mingw
devel package at `C:\sdk\SDL2-2.32.10` (headers under `include\SDL2`,
import libs under `lib`, `SDL2.dll` under `bin`) — not part of this
repo, a machine-local SDK matching `C:\mingw64`'s toolchain. Verified
with a smoke test (`SDL_Init`, driver enumeration, clean `SDL_Quit`).
Build/link a program against it:
```
gcc -I C:\sdk\SDL2-2.32.10\include\SDL2 -c yourfile.c -o yourfile.o
gcc -o yourprogram.exe yourfile.o -L C:\sdk\SDL2-2.32.10\lib -lmingw32 -lSDL2main -lSDL2 -mwindows
```
`SDL2.dll` must be next to the built `.exe` to run (copy it from
`C:\sdk\SDL2-2.32.10\bin`). `-lmingw32 -lSDL2main` before `-lSDL2`,
plus `-mwindows`, are required — `SDL2main` supplies the real `main`
that sets up console/argv redirection before calling the program's
`main`. A future SDL-consuming module's own test/build instructions
should follow this same pattern (mirroring how `src23/tests/*.c` state
their own `gcc` command in a header comment). If this repo is set up
on another machine, SDL2 needs reinstalling the same way (download the
mingw devel package matching the installed GCC).

**Picking the next module** — candidates, roughly in a sensible
dependency order (not a hard sequence; pick whatever's most useful
next):
1. ~~Round out `bcd4`'s own scope~~ — **done 2026-09-19**
   (`bcd4ShiftLeftNibble`/`bcd4ShiftRightNibble`/`bcd4MulPercent`; also
   fixed a half-carry bug in `bcd4Add`, see `overview.md`).
2. ~~Savegame I/O~~ — **done 2026-09-19** (`savegame.c`/`.h`). Still
   open there: the "new game" initializer (`InitializeNewGameWorldState`
   writes every section's defaults), and decoding sections 3-6's
   internals (item-instance fields, state-byte meanings) — do those
   with the modules that consume them. Section 2's fog-of-war bit order
   is now resolved (MSB-first per byte — see `dungeongrid.c` below and
   `file-formats.md`'s "In-memory dungeon map grid").
3. ~~Party/character record structures~~ — **done 2026-09-19**
   (`party.c`/`.h`). Still open there: `+0x12`, the five equipment
   ratings `+0x48..+0x50`, and the item catalog fields (in `WORLD.DAT`),
   which the inventory slots' item ids refer to.
4. **Core dungeon-crawling loop** (90°-turn rendering, monster
   processing on movement, side-trap processing, map-trigger effects,
   `TryInteractAtPosition`'s per-cell marker baking,
   `HandleSpecialCellEntry`/`ShowLockStatus`) — the gameplay Paul is
   most interested in eventually, per the Eye-of-the-Beholder-style
   description in `overview.md`. Movement itself (direction/turn math,
   playable bounds, cell passability) and the in-memory dungeon grid's
   base window (windowing/origin-clamp, per-cell wall/floor/explored
   data) are **done** — see `movement.c`/`dungeongrid.c`
   above; the remaining pieces are bigger and more rendering-dependent.
   **`HandleSpecialCellEntry` checked and ruled out as a data-model
   module (2026-09-23)**: it's pure rendering/animation (a door-swing
   frame buffer feeding `RefreshDungeonScreen`), no logic beyond what
   `movementClassifyCell`'s `MovementCellSpecial` outcome already
   captures — defer it to the eventual rendering layer, don't
   reimplement it standalone.

~~World object index~~ — **done 2026-09-23** (`worldobjects.c`/`.h`,
see item 4's status line above and `file-formats.md`'s "World object
index" section for the full decode). Still open there:
`LoadCurgameRecord`/`LoadLockState`'s own `CURGAME`-side record formats
(what a `0x4000`/`0x8000` record's `value` actually indexes into), and
flag bits `0x2000`/`0x400`'s consumers, if any.

~~5. `UseTrainingItem`, fully~~ — **done 2026-09-24**
   (`partyApplyTraining`/`partyClassPromotionThresholds`/
   `partySyncStagedStats`/`partyRefreshCarryCapacityAndAttributeBonuses`/
   `partyRecomputeEquipmentStatBonuses`/`partyAbilityUnlocksAtLevel`/
   `partyApplyAbilityUnlocks`, see the status entry above and
   `file-formats.md`'s "UseTrainingItem"/"ability/spell-unlock table"
   sections). Still open, a good candidate for its own pass:
   `RunItemServiceRecipientLoop` (the "offer to other party members" UI
   flow). Also still needs: the upstream `UseItem`/`SelectItemUseRecord`
   item-use pipeline this project hasn't built yet, which is what would
   actually resolve `partyApplyTraining`'s `cost` parameter from a real
   item record instead of a caller-supplied value.
6. **The side-trap/ambush pipeline's wall-trap half** (see
   `file-formats.md`'s "side trap"/ambush section) — still blocked on
   confirming how a wall/door trap pool entry gets created; the
   monster-ambush half is done (`monsterApproachParty`). The
   2026-09-24 investigation didn't find a separate creation path but
   strengthened the "traps reuse monster catalog fields" hypothesis
   (`RollTrapAvoidanceMagnitude`'s `+0x64`/`+0x66` are literally
   `MonsterFieldRangedAccuracy`/`RangedDamage`) without proving it.
~~7. **Turn-based combat's turn order**~~ — **done 2026-09-24**
   (`combatBuildTurnOrder`/`combatSelectActiveMonster` in
   `src23/combat.c`/`.h`, see the status entry above and
   `file-formats.md`'s "Turn-based combat: turn order" section).
   **Still open, a good candidate for its own pass**:
   `ProcessCombatRound`'s turn advancement and attack resolution —
   the much larger remaining part of the combat subsystem (damage
   formulas, spell/ability use in combat, victory/flee conditions),
   not investigated beyond turn-order construction so far.
8. **The item-effect "banish" mechanic** (`ApplyEncodedItemEffect`,
   `yendor2.asm:51586`, surfaced the same session) — finds whichever
   monster occupies the party's facing tile, copies its record to a UI
   scratch buffer, and despawns it (no rewards). Needs the broader
   `ApplyEncodedItemEffect` dispatch framework (this project hasn't
   started decoding item-effect codes at all) before it's worth
   reimplementing on its own.

`WORLD.DAT` and `PICTURES.VGA` (both decoded, see `file-formats.md`)
will be needed once map/graphics loading is in scope, but don't need
their own dedicated module ahead of that.

## Remaining minor open threads (not blocking)

From `engine-diffs.md`'s "Review status" section — only worth chasing
if reimplementing the specific function that touches them:
- `DrawShadowedTextAlt`/`PlayStudioCreditsIntro` (yendor3): a studio
  credits easter egg, real identity still not found.
- `sub_1B085` (yendor3): a new random party-ailment mechanic, traced
  structurally but its exact trap-effect id semantics aren't decoded.
- `sub_11778` (yendor3): a "limited item instance" registry helper,
  purpose not fully pinned down.
- `sub_2566C` (yendor3): a small, generic, widely-reused palette-fade
  stub with no confirmed yendor2 name.

## Open questions (yendor2, from earlier sessions, still unresolved)

- Is `NUORE` a currency, a resource, or both? Appears alongside "MAGIC
  ORE" in several UI strings — looks like a 3-resource economy. Note:
  `engine-diffs.md` documents that Chapter 3 appears to remove/replace
  the NUORE mechanic with a new 5-artifact quest system, which may
  make this moot for Chapter 3 but still matters for Chapter 2.
- Relationship between the "region" passwords (`YENDORIAN`,
  `BARIAGIAN`, `OBVERSIAN`, `MONTESERIAN`, `SLATORIAN`, `HEARDONIAN`)
  and the "town" passwords (`PORT HOPE`, `THIEF'S DEN`, etc.) — two
  separate systems, or one list split across two string clusters.
  **Sharper hypothesis as of the world-map decode (2026-09-22)**: since
  the whole game is one seamless coordinate space with no per-level
  files (`file-formats.md`'s "World map"), these are plausibly just
  named teleport coordinates into that single map rather than separate
  loadable areas — worth checking against whatever function actually
  consumes a matched password next time this is picked up. **New lead
  (2026-09-23)**: the in-world text decode found a Chapter 3 "Note"
  entry that is literally `THE PASSWORD IS` `` `RUSE~ `` (`file-formats.md`'s
  "In-world readable text"), signed by an NPC — real evidence that at
  least some passwords in this game are narrative/spoken, discovered and
  entered by the player, which may or may not be the same mechanism as
  the region/town password list; worth cross-checking directly.
- What `sg0977`/`sg0ffc`/`sg1486`/`sg195C`/`sg1ABC` (the 5 segments
  with IDA hex-address names instead of sequential `segNNN`) actually
  are — likely harmless IDA bookkeeping, not investigated further
  since it hasn't blocked anything.

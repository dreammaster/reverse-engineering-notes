# Roadmap
See [porting-guide.md](porting-guide.md) for how the modules fit together into an engine.

Current status and prioritized next steps for the shared Chapter 2/3
engine work (`yendor2.idb`/`yendor3.idb`, `docs23/`, `src23/`). See
[overview.md](overview.md) for the full narrative and
[engine-diffs.md](engine-diffs.md) for the Chapter 2 vs. Chapter 3
behavioral-difference reference.

## Snapshot, 2026-10-03 (read this first; the dated status below is older)

71 test suites (`python src23/tests/run_all.py` builds and runs them all) (`src23/tests/`, each file's header comment has its build line). Since the status below
was written: **`ApplyEncodedItemEffect` is complete** to the data model (`spellSelectBranch` names the whole
dispatch; LIFE FORCE, projectile hit/splash, screen-wide attack, held item, JUMP spells, light timers,
bookmark all implemented); combat has the **monster turn**, **player melee** and **Search**; and the
biggest find of the session: **`UseItem`/`LoadItemData` is the NPC dialogue + service engine** -- `dialog.c`
(105/140 NPCs, 929/1073 topics, 3218/4090 text lines, the topic-menu mask machinery, verified on real NPCs of
both games), `dialogservice.c` (healer, trainer, challenges, tomes, ore, riddles, mounts, sell/enhance/
repair rules), `shop.c`, with the lock catalog's unexplained bytes decoded as chest/shop **contents**.
Also `lightsource.c`, `spellcast.c` (castability, learning), `repair.c`, `consumable.c` (potions, alchemy).
What is left is mostly UI/orchestration (shop screen, text entry, animation shells, row-walking), container
recursion's consumers (`partyFindItemDeep` finds, nothing yet consumes in a bag), and the SDL layer. Method
notes: list the real data that reaches a code path before decoding it; subtract the data-segment base from
IDA `word_XXXXX` names; write helper scripts with the editor tool, not shell heredocs.

## Snapshot, 2026-10-04 (newest; supersedes the paragraphs above where they disagree)

71 test suites, all passing. The decoded-and-reimplemented set now also covers the whole visible game (see [porting-guide.md](porting-guide.md)):

* **Decision logic** added since the snapshot above: `mapview.c` (map tiers, local map), `travelExamineKey`, the relic dispatcher and potion
  brewing, `partyFindItemDeep` / `partyConsumeItemChargeMode` (containers), `chargen.c` + `newgame.c` (attribute roll, derived skills, classes,
  the new-game template with four ready-made heroes), `ailment.c`, `explore.c`, `mount.c`, `thrown.c`, document readability, `inventory.c`
  (slot eligibility, container fit, no-drop, auto-equip, dropped containers), `lighting.c`, `viewport.c`, `music.c`, the tile legends (Chapter 3's
  paged tables included), **monster walking** (`monsterStepBlocked`, `monsterWalkTowardParty`, `monsterPoolTakeTurn`) and `partyCompactSlots`.
* **Assets**: `pictures.c` (PICTURES.VGA is ten categories of same-sized pictures), `palette.c` (dawn/dusk fade, colour cycle, full fades),
  `font.c`, `audio.c` + `cmf.c` + `voc.c` (music and effects are CMF and VOC files inside WORLD.DAT; every track and effect of both games parses).
* **Drawing**: `viewrender.c` (the first-person view incl. monsters, splashes and hit colour effects), `minimap.c`, `statuspanel.c`, `monsterpanel.c`,
  `paperdoll.c`, `statsheet.c`, `roster.c`, `shopgrid.c`, `alchemylist.c`, with `uiregions.c` holding every screen's click regions.
  `src23/tools/render_view.c` renders a position (and, with `RENDER_*` environment variables, the party panels, a monster, the dolls, a sheet or the
  roster) to a PNG: the quickest way to check any of it.

* **Screens and input added later the same day**: `charcreate.c` (portrait, class, item pick, roll, summary menu, name prompt) + `textfield.c`,
  `gamedialog.c` (pause dialog), `titlemenu.c`, `clueitem.c` / `cluemonster.c` (clue book item and monster pages), the alchemy status panel,
  `maininput.c` (the main loop's and the combat turn's key and click maps), `partyUsePercentRestorative` (RestCharacter), and `exedata.c`: **the
  executables are not packed** -- Chapter 2's `SW.EXE` holds its data segment at file offset `0x21660`, Chapter 3's `REGISTER.EXE` at `0x21DB0` -- so the
  fixed UI text is read from them at run time instead of being copied into the source. `tools/walk.c` walks the party through a real map headlessly.

Still missing: the animated sequences (title / intro / creation / credits), the clue book's spell, transport and map pages and the run-time sub-icon row,
the shop's category tab list, the message strings used by the game logic (`exedata.c` can fetch any, none are catalogued), input/timing glue (the
original is driven by DOS interrupts), OPL/mixer playback of the parsed music and effects, Chapter 2's debug commands, and Chapter 1's engine. A name
census of the 769 functions against `src23` is the quick way to find what is still uncovered (what remains is overwhelmingly UI).

## Status (last updated 2026-10-01, resolved: the spell/ability catalog's full 80-byte field layout (spellrecord.c, new), the ApplyAttackToTarget/TryResolveAttackAgainstTarget attack-resolution family's composition (combatResolveSpellAttack/combatApplySpellAttack), ApplyEncodedItemEffect's full dispatch chain (all 19 branch addresses recorded), and two previously-untraced caller-context questions -- plus the prior round's "region/town password" mechanism, the wall/door trap creation mystery, and the "R rest" command's decision logic)

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
  moves one. Surfaced two things along the way, neither reimplemented
  at the time: a genuine separate combat subsystem (`BuildCombatTurnOrder`/
  `ProcessCombatRound`, a 3-slot `g_monsterSlots` staging array distinct
  from the 80-slot pool -- now fully reimplemented, see below) and an
  `ApplyEncodedItemEffect` mechanic first guessed here to "banish the
  monster on the facing tile" — **corrected 2026-09-25: it doesn't
  despawn the monster at all, it copies it into a combat slot to
  engage it, as part of a teleportation effect** (see candidate 8
  below and `file-formats.md`). Full writeup in `file-formats.md`'s "Monster approach and
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
  Tests in `tests/test_combat.c`; all 18 suites pass (`test_combat.c`
  is a new suite alongside the 17 pre-existing ones — corrected here,
  an earlier pass this session miscounted it as 19). Full writeup in
  `file-formats.md`'s "Turn-based combat: turn order" section.
  **`ProcessCombatRound` — done, same day (2026-09-24)**: reads
  `RunDungeonGameLoop` directly to place combat in the bigger picture
  — it turns out **every ordinary exploration input tick is a
  degenerate one-entry combat round** (no monsters occupied -> the
  round ends immediately after the player's own turn), which is also
  why the dungeon-exploration monster-AI pass
  (`ProcessLevelMonsters`/`ProcessSideTrapsOnMovement`) only runs once
  a full round is exhausted, not every tick — a real, previously
  fuzzy mechanism now fully explained. `combatProcessRound` handles
  the per-tick death scan (defeat-flagging, reward-granting, record
  zeroing) and turn-cursor advancement (forward-only, no wraparound).
  Deliberately not reproduced: `CompactMonsterSlots`'s physical
  slot-shifting and its `RelocateActiveMonsterPointer` pointer-fixup
  trick — both exist solely to keep the original's raw-pointer
  turn-order entries valid after a record moves in memory, a problem
  this reimplementation's index-based entries don't have. Tests in
  `tests/test_combat.c`; all 18 suites still pass. Full writeup in
  `file-formats.md`'s "Turn-based combat: turn order" section.
  **Attack resolution's two core primitives -- done, 2026-09-25**:
  `combatResolveAttack`/`combatFailsSavingThrow` in `src23/combat.c`/
  `.h` (`ResolveAttack`/`FailsSavingThrow`, both instruction-identical
  between the games, both small and fully self-contained). Confirmed a
  genuinely satisfying pair of previously-unnamed-use fields along the
  way: the defending party member's `PartyStatEquipRating5` is the
  "defense" stat against a monster's physical attack, and
  `PartyStatSurvival` is the saving-throw bonus against a monster's
  special attack. **Found a real, previously-undocumented Chapter 3
  behavioral addition while reading `ResolveAttackerActionOutcome`
  (the function that composes these two primitives) directly in both
  games rather than assuming a match**: in Chapter 2, a resisted
  special-attack saving throw is a clean miss; in Chapter 3 it instead
  falls back to a normal damage roll using the monster's ordinary
  attack effect -- see `engine-diffs.md`. `ResolveAttackerActionOutcome`
  itself, `ProcessMonsterAttackTurn`, and the player-attack path inside
  `HandleDungeonInput` remain deferred -- branch 3 (equipment
  corrosion) calls `ClassifyItemServiceTier`, which turned out
  (2026-09-25 correction, see below) to already be characterized by an
  earlier session, narrowing but not closing this gap. Tests in
  `tests/test_combat.c` (using an
  RNG-state-peek technique for exact, non-flaky assertions about
  roll-gated outcomes rather than looping for a lucky seed); all 18
  suites pass. Full writeup in `file-formats.md`'s new "Attack
  resolution" section.
  **The staged combat event's consumer -- found and reimplemented,
  same day (2026-09-25)**: reading `ProcessMonsterAttackTurn` in full
  (the actual caller of `ResolveAttackerActionOutcome`, not chased
  down the previous round) resolved last round's open blocker --
  `word_32906` is one of 4 `g_partyEffectIconSlots` entries, read by
  `ApplyEffectAndDrawIconBar`. Reimplemented its state-mutating half
  (`RollEffectMagnitude`/`RollEffectResistance`/`ApplyEffectCost`, not
  its drawing) as `effectRollMagnitude`/`effectResolveInflictedStatus`
  (`src23/effect.c`/`.h`), `combatApplyEffect`
  (`src23/combat.c`/`.h`), and `partyDeductHp`/`partyDeductMp`
  (`src23/party.c`/`.h`). Instruction-identical between the games (see
  `engine-diffs.md`). Deliberately not reimplemented: the other 2 of
  `ApplyEffectAndDrawIconBar`'s 3 dispatch variants (item expiry, stat
  delta -- different mechanisms with their own untraced call chains)
  and its own drawing/sound/UI-tier-refresh side effects (deferred to
  the SDL2 layer). Tests across `test_effect.c`/`test_party.c`/
  `test_combat.c`; all 18 suites pass. Full writeup in
  `file-formats.md`'s "The staged combat event's consumer, found"
  section.
  **The monster gold-theft mechanic fully wired up, same day
  (2026-09-25)**: while documenting branch 2 above, checked real
  `WORLD.DAT` data for the attacking monster's own field this project
  had left unnamed (`monster.record + 0x8E`, a 4-byte span) and found
  it's nonzero for a small, consistent set of monsters in both
  games (thieves/assassins), always paired with `MonsterFieldSpecialAttack`
  set to `effect.h`'s id 15 ("takes gold, rolls no magnitude" -- its
  own magnitude range is 0-0, so the amount has to come from
  somewhere else). Named `MonsterFieldGoldTheftAmount` in `monster.h`.
  Also found and corrected a 3rd nested branch-selection test this
  project had missed on the first pass through `ResolveAttackerActionOutcome`
  (an exact-zero check on this same field, gating whether branch 2 is
  actually taken or falls back to branch 1) and a real quirk in
  `SpendMaterialCounterClamped`'s own gate (strictly-greater, not
  greater-or-equal -- an exact counter/amount match still takes the
  "depleted" clamp path). Added `bcd4SubClamped` (`src23/bcd4.c`/`.h`)
  and extended `combatApplyEffect`'s signature to take a `SaveGame*`
  and a `const Bcd4 materialAmount`, wiring the gold/ore branches that
  were previously a documented no-op. Tests in `test_bcd4.c`/
  `test_combat.c` (including the gold-theft path against a real
  `SaveGame` and the ore-cost clamp); all 18 suites pass.
  **Corrected the stale "banish" reading, properly scoped
  `ApplyEncodedItemEffect`, same day**: see candidate 8 below and
  `file-formats.md`/project memory for the full correction. No source
  changes.
  **Known-ability-ids query added, same day**: while tracing
  `ApplyEncodedItemEffect`'s callers for candidate 8's scoping, found
  `BuildAlchemySpellList`'s core test (`TestRecordFlag_CA`) is the
  exact same `PartyFieldFlagBankCA` bit test `partyApplyAbilityUnlocks`
  already writes via `flagBankSet`/`SetRecordFlag_CA` — a read-back
  query for data this project already produces. Added
  `partyKnownAbilityIdMax`/`partyKnownAbilityIds` in `src23/party.c`/
  `.h` (instruction-identical between the games; only the max index
  scanned differs per game — 125 vs. 107, an `InitGlobals` constant).
  Deliberately not reimplemented: `CheckSpellCastability`'s
  affordability gate and pagination, both UI concerns layered on top.
  Tests in `test_party.c`, including a direct round-trip against
  `partyApplyAbilityUnlocks`; all 18 suites pass.
  **Equipment corrosion classification closed out, same day
  (2026-09-25)**: the previous round's correction (`ClassifyItemServiceTier`/
  `GetClassifiedItemStatField` were already characterized, not
  undecoded) left one narrow open question -- where `word_2E548`
  (the scratch value `GetClassifiedItemStatField` reads) comes from in
  this specific call path. Turned out mundane: `ClassifyItemServiceTier`
  calls `LoadItemCatalogRecord` itself as its first step, so
  `word_2E548` is just "the item being classified," no external
  lifetime question at all. Reimplemented as
  `itemClassifyServiceTier`/`itemCorrosionReplacement` in
  `src23/item.c`/`.h`, instruction-identical between the games.
  Confirmed against real data: SLING (Chapter 2 item id `0x21E`)
  classifies as category A (a weapon) and its own `ItemTargetBreakItemA`
  is `667`, matching `itemCorrosionReplacement`'s result exactly. This
  fully resolves `ResolveAttackerActionOutcome` branch 3's item-side
  down to a narrow, well-understood remainder: composing the
  attacker-flag equipment-slot selection and defender-item lookup into
  `combat.c` itself, not yet done. Tests in `tests/test_item.c`
  (all 3 tiers x both categories synthetically, plus the real SLING/
  BREAD/BAG cases); all 18 suites pass.
  **`ResolveAttackerActionOutcome` fully composed, 2026-09-26**: with
  every primitive solid, assembled `combatSelectTrapEffectVariant`
  (`SelectTrapEffectVariant`) and `combatResolveAttackerAction`
  (`ResolveAttackerActionOutcome`'s own 3-way dispatch) in
  `src23/combat.c`/`.h`. Both instruction-identical between the games
  except for the already-documented Chapter 3 resisted-special-attack
  fallback. Returns a plain `CombatAttackerAction` value rather than
  staging into an icon slot -- the caller applies
  `CombatAttackDamage`/`StatusEffect` via `combatApplyEffect` directly.
  **One piece deliberately left undone**: `CombatAttackCorrosion`
  reports the equipment slot/item/replacement-id but doesn't write the
  replacement back -- the original's own write-back
  (`HandleIconBarItemExpiry`) is defined for item-expiry-on-use/wear,
  and this project hasn't confirmed combat's corrosion staging feeds
  it with matching semantics rather than just reusing the same byte
  offsets for something else. Tests in `tests/test_combat.c` cover
  every branch (plain damage hit/miss, gold-theft fail/resist via
  RNG-peek, the "special selected but nothing configured" fallback,
  corrosion fail/resist, all 3 equipment-slot selections, an empty
  slot, and an unclassifiable equipped item); all 18 suites pass.
  This closes out candidate 7's decision-logic scope entirely --
  what's left is purely the UI-driving orchestration (drawing,
  the corrosion write-back, `ApplyEffectAndDrawIconBar`'s other 2
  dispatch variants, `ProcessMonsterAttackTurn`,
  `HandleDungeonInput`'s player-attack path).
  **`LoadCurgameRecord`'s long-open record format resolved,
  2026-09-26**: re-tried `run_ida_script.ps1` from inside this session
  after a false "blocked" conclusion the day before (see project
  memory) -- it works fine, and a single targeted read-only script
  (`yendor2/ida_scripts/check_curgame_record_buffer.py`, committed)
  immediately answered the question pure `.asm` text-grepping
  couldn't: the 2 words `LoadCurgameRecord` copies land in
  `g_lockStatusFlags` and a packed `threshold*100 + effectId` word --
  the *exact same* two globals `LoadLockState` populates for an
  ordinary lock. A CURGAME "trigger" record and a lock record are the
  same physical shape read through two different loaders. Traced the
  consumer (`UseAbilityCommand` -> `ApplySavingThrowEffect`,
  instruction-identical in both games) to confirm: it's a
  search/lockpicking-triggered magical trap, with a two-tier roll
  structure (one shared trigger roll using the attempting character's
  own `PartyStatThievery`, then the ordinary effect pipeline applied to
  either the triggering character alone or the whole party, id
  `>= 50` selecting the latter). Reimplemented the packed-value decode
  as `partyDecodeSavingThrowEffect` in `src23/party.c`/`.h`. Tests in
  `test_party.c` cover the zero case, both target scopes, and the
  49/50 id boundary. Full writeup in `file-formats.md`'s "Lock/door
  definition catalog" section (the `LoadCurgameRecord` sub-note).
  **The roll-and-apply composition itself -- done, same day**:
  `combatApplySavingThrowTrap` in `src23/combat.c`/`.h` (kept there, not
  `party.c`/`effect.c`, since `party.h` can't include `combat.h`).
  Confirmed `ApplySavingThrowEffect`/`RollEffectResistance`/
  `RollEffectMagnitude` instruction-identical in Chapter 3. Two quirks
  confirmed and reproduced exactly: `RollEffectResistance` skips its
  second saving-throw roll entirely (not just its effect) unless the
  effect both inflicts something and gates on a save, so this
  composition's RNG-draw count matches the original exactly; and the
  whole-party scan stops dead at the first unoccupied
  `SaveHeaderPartySlots` slot instead of skipping past it. Also
  confirmed the original never populates a gold/ore effect's material
  amount for this call path at all (`RollEffectMagnitude`'s own
  early-out for cost-flag bits 0-2 leaves it at the icon slot's cleared
  0), so a search/lock trap configured with a gold-cost effect id would
  always steal exactly 0 -- reproduced by always passing a zeroed
  `Bcd4` rather than inventing a source. Tests in `test_combat.c` cover
  the zero-value and avoided-trigger cases, a single-target application
  with an exact peeked-RNG magnitude check, the whole-party
  incapacitated-skip + stop-dead-at-empty-slot quirk together, and an
  out-of-range effect id. All 18 suites pass. `UseAbilityCommand`/
  `HandleSearchCommand` themselves remain unreimplemented (UI-heavy
  top-level commands needing the SDL2 layer for prompts/confirms/
  message boxes), but the entire decision-and-apply logic beneath them
  is done.

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
index" section for the full decode). Still open there: exactly which
EMS byte offset a `0x4000`/`0x8000` record's `value` indexes into
(`LoadCurgameRecord`'s own base-offset mystery, distinct from what the
loaded record *means* once read — that part is resolved 2026-09-26,
see the status entry above), and flag bits `0x2000`/`0x400`'s
consumers, if any.

**New candidate 11 (2026-10-03): the NPC dialogue/service engine
(`UseItem`)** -- data layer done (`dialog.c`, see file-formats.md's "NPC
dialogue catalog"). Next: reimplement the topic handlers
(`UseHealingItem`, `UseAttributeBoostItem`, `UseExperienceBoostItem`,
`UseKeyItem`/`CheckKeyItem`, `UseAbilityScroll`, `UseItemType_400/800` and
`ApplyItemEffectFlags`) as decide-don't-apply functions, then the topic-mask
bookkeeping. `partyApplyTraining`'s `cost` input is the price in this data.

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
~~6. **The side-trap/ambush pipeline's wall-trap half**~~ — **fully
   resolved and reimplemented 2026-09-30**: the "how does a wall/door
   trap pool entry get created" mystery is closed for good. An
   exhaustive whole-binary search (both games) for every write to
   `MonsterFieldWound` bit `0x1000` (`MonsterWoundAmbushPending`) found
   exactly one site per game, and it's `monsterApproachParty`'s own
   ambush-arming write, already reimplemented — **there is no separate
   wall/door trap creation mechanism at all.** A "wall/door trap" is
   simply an ordinary monster record in the ambush-pending state,
   presented as a wall/door effect or an ordinary encounter depending
   only on which direction the party happens to be facing relative to
   it. With that settled, reimplemented the roll/gate decision logic
   itself: `combatRollTrapAvoidanceMagnitude`/`combatResolveSideTrap`
   in `src23/combat.c`/`.h`, using the party's `PartyStatEquipRating5`
   against the trap pool entry's own (already-named, ordinary)
   `MonsterFieldRangedAccuracy`/`RangedDamage` fields. **A real Chapter
   2 vs. Chapter 3 difference found**: Chapter 2 rolls
   `RandomInRange(100)`, Chapter 3 rolls `RandomInRange(55)` instead,
   with the same final percentage formula — Chapter 3 traps trigger
   noticeably more often for the same margin. Tests in
   `test_combat.c` cover both bounds via the RNG-peek technique, a
   concrete same-seed divergence between the games, and all 4
   facing-match cases. See `file-formats.md`'s "side trap"/ambush
   section for the full writeup. **Still not reimplemented**: the
   4-slot scratch staging table and `PresentTriggeredSideTrapEffects`
   itself, both pure UI/drawing/sound sequencing deferred to the
   eventual SDL2 layer.
~~7. **Turn-based combat's turn order, round processing, attack
   resolution (fully composed), and the icon-bar effect-application
   pipeline they feed**~~ — **done 2026-09-26** (`combatBuildTurnOrder`/
   `combatSelectActiveMonster`/`combatProcessRound`/`combatResolveAttack`/
   `combatFailsSavingThrow`/`combatApplyEffect`/`combatSelectTrapEffectVariant`/
   `combatResolveAttackerAction` in `src23/combat.c`/`.h`,
   `effectRollMagnitude`/`effectResolveInflictedStatus` in
   `src23/effect.c`/`.h`, `partyDeductHp`/`partyDeductMp` in
   `src23/party.c`/`.h`, `itemClassifyServiceTier`/`itemCorrosionReplacement`
   in `src23/item.c`/`.h` -- see the status entries above and
   `file-formats.md`'s "Turn-based combat"/"Attack resolution"/"The
   staged combat event's consumer, found"/"`ResolveAttackerActionOutcome`,
   fully composed" sections). **`ApplyIconBarStatDelta` reimplemented,
   2026-09-29**: the icon-bar's capped/floored stat-delta dispatch
   variant, as `partyApplyIconBarStatDelta` (`src23/party.c`/`.h`) --
   data-driven (the field offsets it operates on come from the icon
   slot itself, populated by an untraced caller, so the specific stat
   this applies to in practice is still unconfirmed). Found **two real
   Chapter 2 bugs, both fixed in Chapter 3**: a zero `maxFieldOffset`
   ("uncapped") gets misread as a real field offset in Chapter 2, and
   `RefreshCarryCapacityAndAttributeBonuses`/`CheckForLevelUp`'s own
   internal `g_currentPartyRecord` read goes stale for a whole-party
   effect in Chapter 2 since nothing updates it per-recipient -- see
   `engine-diffs.md`. Reimplemented matching Chapter 3's corrected
   behavior for both games. Tests in `test_party.c`; all 18 suites
   pass. **`HandleIconBarItemExpiry` reimplemented too, same round --
   ApplyEffectAndDrawIconBar's full 3-way dispatch is now done**: the
   "`g_itemStatEffectTable`" prerequisite flagged as unextracted turned
   out to already exist under a different name --
   `item.h`'s own `itemEffectEntry`/`itemEffectPairs`/`itemEffectField`/
   `itemEffectAmount`, decoded by an earlier round but never connected
   to `RemoveMultiStatEffect`/`ApplyMultiStatEffectForItem`. Reading
   those two functions directly confirmed a pair's field is a *raw
   byte offset* straight into the party record (this project's own
   `isPartyEffectField` test helper had already validated the
   hypothesis against real data without the connection being made).
   Reimplemented as `partyApplyMultiStatEffect`/
   `partyRemoveMultiStatEffect`/`partyHandleIconBarItemExpiry`
   (`src23/party.c`/`.h`), all instruction-identical in Chapter 3 --
   see `engine-diffs.md`. Tests in `test_party.c` cover both the
   destroy and replace branches; all 18 suites pass. **The
   equipment-corrosion write-back resolved too, same round -- a
   long-open question finally closed**: tracing `ResolveAttackerActionOutcome`'s
   corrosion branch and `GetClassifiedItemStatField` directly (both
   instruction-identical in Chapter 3) confirmed combat's own staged
   fields map *exactly* onto `partyHandleIconBarItemExpiry`'s
   parameters -- no reinterpretation needed. Confirmed against real
   data just how rare this mechanic actually is: of both games' full
   monster rosters, only Chapter 3's CROCODILE (catalog block 70) has a
   legitimate corrosion-flag-plus-valid-effect-id combination; Chapter
   2 has none at all (its one flag-matching block is an unnamed
   placeholder with out-of-range effect ids, not a real monster).
   Reimplemented as `combatApplyCorrosion` (`src23/combat.c`/`.h`);
   tests in `test_combat.c`; all 18 suites pass.
   **A related discovery, same investigation**: `ApplyEncodedItemEffect`'s
   own corridor/ranged-attack branches (bits `0x4`/`0x2000`, already
   flagged in candidate 8 below) reach a small, genuinely separate
   attack-resolution family against *map* monsters (`g_levelMonsters`)
   -- `ApplyAttackToTarget`/`TryResolveAttackAgainstTarget`/
   `ApplyTargetResistancesToAttack`/`ApplyDamageToMapMonster`. Traced
   `ApplyTargetResistancesToAttack` fully and reimplemented it as
   `combatApplyTargetResistances` (`src23/combat.c`/`.h`) -- resolving
   the original's own "di's record type here is unknown" uncertainty
   along the way (it's a full `MonsterRecordSize` record, confirmed by
   cross-referencing every field offset against `monster.h`'s own
   already-decoded fields). Tests in `test_combat.c`; all 18 suites
   pass. The rest of the family (`ApplyAttackToTarget`/
   `ApplyDamageToMapMonster` themselves) still depends on more untraced
   caller-context globals that bits `0x4`/`0x2000` haven't been traced
   far enough to supply -- left for whoever picks up those branches
   next, see candidate 8.
   **Update 2026-10-03: `ProcessMonsterAttackTurn` is now composed as
   `combatProcessMonsterTurn`** (see file-formats.md); the player-attack path
   in `HandleDungeonInput` remains: its melee swing is now
   `combatPlayerMeleeAttack`; spell/ability use and the UI shell remain.
   **Still open, a good candidate for its own pass**: `ProcessMonsterAttackTurn`
   itself (the caller that would actually wire
   `combatSelectTrapEffectVariant`/`combatResolveAttackerAction`/
   `combatApplyEffect`/`combatApplyCorrosion` together end to end) and
   the player-attack path inside `HandleDungeonInput` (spell/ability
   use in combat, area-attack handling, and all the drawing/sound/
   UI-tier-refresh work this project has deliberately deferred to the
   eventual SDL2 layer) -- both pure orchestration/composition now, no
   remaining decision-logic gaps anywhere in this candidate.
8. **`ApplyEncodedItemEffect`** (was `sub_2C0FE`, the largest function
   in the binary at 4,210 bytes -- named and scoped by an earlier
   session, revisited 2026-09-25) -- a flat ~19-branch bitmask switch
   (`word_33302`/`word_33306`) applying an item's or container's coded
   magical effect, called from `RunAlchemyScreen` and
   `InteractWithContainer` right before `ConsumeItemChargeResource`
   spends the charge. **Two branches reimplemented, 2026-09-26**: a
   single-target and a whole-party status-effect application
   (`word_33302` bits `0x8000`/`0x4000`), both populating a
   `g_partyEffectIconSlots` entry and calling `ApplyEffectAndDrawIconBar`
   exactly like combat does -- except here the caller supplies an
   already-resolved inflicted-status/magnitude pair rather than rolling
   one (the concrete case `RollEffectMagnitude`/`RollEffectResistance`'s
   own "already resolved" early-outs exist for). A shared gate zeroes
   both values if the *acting* character is Cursed. **A real Chapter 2
   vs. Chapter 3 difference found**: Chapter 3 additionally skips any
   *recipient* who is Cursed in the whole-party branch, a check Chapter
   2 altogether lacks -- see `engine-diffs.md`. Reimplemented as
   `combatResolveEncodedItemEffectValue`/`combatApplyEncodedItemEffectSingle`/
   `combatApplyEncodedItemEffectParty` in `src23/combat.c`/`.h`, tested
   in `test_combat.c`; all 18 suites pass. Still open: the surrounding
   dispatch decision itself (which of `word_33302`'s ~19 bits fires at
   all; whether `word_33300`'s own `0x800`/`0x1000` bits -- read from an
   untraced caller context -- select this path versus skipping the
   icon-bar entirely).
   **A third branch reimplemented, 2026-09-29**: bit `0x1`, a
   "Knock"-style auto-unlock. Probes the party's own cell then the cell
   one step ahead in facing via a new shared primitive,
   `worldObjectProbeFacingTile` (`src23/worldobjects.c`/`.h`, `ProbeFacingTile`
   in the original), reusing `worldObjectFind` and `movementApply`'s own
   forward delta rather than re-deriving the direction table a third
   time; classifies whatever's found via the already-existing
   `interactClassify` and, for `InteractOutcomeLockMagical`/
   `InteractOutcomeCurgameFallbackB`, marks it unlocked/triggered via
   `interactBitmapSet` -- reimplemented as `interactKnock` in
   `src23/interact.c`/`.h`. Tests in `test_worldobjects.c`/`test_interact.c`;
   all 18 suites pass. **Surveyed the rest of the function's branches
   this round too, and found the remaining ones genuinely need
   prerequisite subsystems this project hasn't started, not just more
   reading**: bit `0x80` (world-state timers) is tangled into a whole
   not-yet-scoped "world ailments/weather/lighting" system
   (`TickWorldAilments`, day/night ambient lighting) -- also found a
   real infinite-loop hazard there for an out-of-range index, presumably
   unreachable with real data, not reproduced. Bit `0x10` (conjure an
   item onto the held-item cursor) needs the pervasive
   `g_heldItemType`/held-item UI system (~40 other call sites) as a
   prerequisite. Bit `0x2` ("rest here") is a thin wrapper around
   `RestPartyAndAdvanceClock`, whose own decision logic is now done
   (2026-09-30, see the status entry above) except for the
   item-availability-based regen-rate derivation, which still needs the
   not-yet-built inventory-search subsystem -- this bit itself is still
   unimplemented (it's UI-toolbar dispatch calling the whole command),
   but no longer blocked on *undecoded* logic the way the others here
   are. Bit `0x4` (a
   corridor/ranged-attack path, along with bit `0x2000` further down)
   confirmed to match this candidate's existing scoping exactly --
   `ApplyAttackToTarget`'s own resistance-filtering half is now traced
   and reimplemented (`combatApplyTargetResistances`, see candidate 6
   above for the full writeup). **The rest of that family's own
   caller-context mystery -- the "word_332D8-33306 encoded effect
   descriptor cluster" this project had marked "genuinely unresolved"
   after a dedicated earlier investigation -- is resolved for good,
   2026-09-30**: the fields aren't a separate, unwritten scratch region
   at all, they're named offsets *into* the same 80-byte
   `LoadClueBookSpellEntry` buffer the alchemy/spell-casting UI already
   reads, confirmed by real-mode segment arithmetic (`es:0x5A5A` with
   `es` loaded from `seg seg129` resolves to the exact same linear
   address as `word_332D0`) and cross-checked against real bytes in
   both games' actual `WORLD.DAT` files (`ida_scripts/dump_worlddat_block4_offset.py`,
   one script per game) -- the first 6 records read HEAL/MAGIC
   ATTACK/SLING SHOT/COLD SLASH/MINOR WOUNDS/MINER'S LIGHT I,
   byte-identical in both games. See `file-formats.md`'s own dedicated
   section for the complete writeup. This closes the "which caller
   populates this" side of the mystery.

   **The record's full field layout decoded, and the attack-resolution
   family composed, 2026-10-01**: new module `src23/spellrecord.c`/`.h`
   names every field this project has a confirmed consumer for (name,
   costs, the type-restriction gate, the preset inflicted-status/
   magnitude pair, drain amount, attack magnitude, the tick-timer arm,
   class eligibility, and the 3 flag words `word_33300`/`33302`/`33306`),
   cross-checked against real byte statistics from both games' full
   tables (125 records Chapter 2, 107 Chapter 3 -- both counts confirmed
   named engine constants, not estimates). Resolved three real
   previously-unexplained quirks along the way: `MonsterFieldUnknown4E`
   is the type-restriction match target; `MonsterFieldImmunities` is
   reused at runtime to mark "currently afflicted" via the exact same
   bits as "permanently immune"; and `MonsterStateSpecialAttackDisabled`/
   `MonsterStateBusy` share bit positions with `MonsterImmuneCursing`/
   `MonsterImmuneHexing`, so landing those statuses sets those state bits
   as a side effect, not a separate mechanism (a new bit,
   `MonsterStateHitFlashPending`, was also named). `ApplyAttackToTarget`/
   `TryResolveAttackAgainstTarget`/`ApplyTargetResistancesToAttack` are
   now fully composed as `combatResolveSpellAttack`/
   `combatApplySpellAttack` (`src23/combat.c`/`.h`). This also confirms
   `word_33300`/`33302`/`33306` are genuine record fields (not per-call
   caller state as every earlier round assumed) -- `ApplyEncodedItemEffect`'s
   ~19-branch dispatch is simply data-driven from whichever spell/ability
   id an item encodes. Tests in `test_spellrecord.c` (22nd suite, new)
   and extended `test_combat.c`; all 22 suites pass.

   **The full dispatch chain read and two caller-context mysteries
   closed, same day**: `ApplyEncodedItemEffect`'s dispatch is a flat
   sequential chain (not a jump table), all 19 branch targets now
   recorded by address in `file-formats.md` for whoever picks up the
   remaining ones. This read resolved two things this project had
   flagged as untraced: `g_uiScratchFlags4` bit `0x80`
   (`combatResolveSpellAttack`'s `alreadyResolved`) is simply which of
   `ApplyEncodedItemEffect`'s two callers invoked it --
   `RunAlchemyScreen` always clears it (a normal roll, for player-cast
   spells), `InteractWithContainer` always sets it (a preset value, for
   container/trap effects); and the single-target/whole-party gate
   (`SpellFieldFlagsA` bits `0x800`/`0x1000`, renamed
   `SpellFlagsAPositionReset`/`IconBarPresetAmount`) is confirmed
   all-or-nothing for the single-target branch only (nothing happens at
   all with neither bit set) but never gates the whole-party branch's
   own icon-bar call.

   **Two more branches reimplemented, caught and fixed a same-round
   self-introduced error, same day**: bits `0x2000`/`0x1000` (the
   corridor/ranged-attack family) confirmed by direct read to call
   `ApplyAttackToTarget` against `g_activeCombatMonster` alone or every
   occupied `g_monsterSlots` entry respectively -- the real caller
   `combatResolveSpellAttack`/`combatApplySpellAttack` had been built
   for two rounds ago without yet having one. Reimplemented as
   `combatMarkSpellAttackHit`/`combatApplySpellAttackToActiveSlots`
   (`src23/combat.c`/`.h`), also finally naming `monster.h`'s
   long-standing unidentified `+0x18` field (`MonsterFieldLastAttackMarker`
   -- a confirmed writer, still no confirmed reader). While doing this,
   caught and corrected a mistake in this very round's own earlier
   dispatch-chain writeup: bit `0x4`, not bit `0x20`, is the
   already-documented teleport-then-engage mechanic; bit `0x20` is
   actually a never-before-documented position bookmark
   (save/restore the party's own world position, facing, and render
   state) -- see `file-formats.md` for the full correction. Tests in
   `test_combat.c`; all 22 suites pass.

   **Bit `0x20` fully resolved and reimplemented, same day**: traced
   `g_uiScratchFlags1` bit `0x80`'s only setter to `RunAlchemyScreen`
   itself, which shows its own confirm prompt before dispatch even
   runs, picking Mark vs. Return. Real data removes all ambiguity --
   exactly one record in each game's catalog uses this branch, and it's
   literally named **"MARK OR RETURN"**. Its own bookmark-offset field
   (`spellrecord.h`'s new `SpellFieldBookmarkOffset`) lands at `0xF0` in
   `party.h`'s one genuinely unused field-map gap in both games,
   confirming it's real reserved space. Reimplemented as
   `combatSaveLocationBookmark`/`combatRestoreLocationBookmark`
   (`src23/combat.c`/`.h`); tests cover the full round-trip and the
   "never saved" failure case; all 22 suites pass.

   **Two more branches picked off, same day**: bit `0x8`
   (`interactTriggerFacingCurgameEvent`, a third user of the existing
   `interactResolveIfOutcome` primitive, qualifying set
   `{InteractOutcomeCurgameFlag10, InteractOutcomeCurgameFlag8}` --
   never resolves a door, only a curgame record) and bit `0x2`
   (`loc_2C703`, confirmed to be the already-scoped "rest here" wrapper
   from an earlier round, no new behavior, just the dispatch address
   pinned down). Tests in `test_interact.c`; all 22 suites pass.

   **One branch investigated, deliberately deferred rather than
   rushed, same day**: bit `0x2000`'s own `loc_2CF51` diversion
   (triggered by `SpellFieldResistFlags` bits `0x40`/`0x80`) is a real,
   distinct mechanic built around the already-named
   `ResolveAttackAndLatchFirstHit`, but its two outcome tails (one
   *adds* damage back to the target, one subtracts it normally) and a
   shared icon-bar step reading two more record fields
   (`word_332E4`/`word_332E6`, now at least confirmed as effect-id
   sources) aren't understood well enough yet to implement without
   guessing -- recorded in `file-formats.md` as a better-scoped
   candidate for its own pass.

   **`ApplyDamageToMapMonster` traced and reimplemented, closing a gap
   open since an earlier round, same day**: a direct read showed its
   own two unnamed callees (flagged "not traced" in an old comment) are
   simply `ApplyAttackToTarget` -- this function is exactly
   `combatResolveSpellAttack`/`combatApplySpellAttack`/
   `combatMarkSpellAttackHit` plus a death check
   (`monsterGrantRewards`/`monsterPoolRemove`, both already
   reimplemented). Reimplemented as `combatApplyDamageToMapMonster`
   (`src23/combat.c`/`.h`). Tracing its caller also corrected a
   **second, older misattribution** (predating this session): the
   "straight-line multi-target attack" mechanic belongs to word_33302
   bit `0x200` (`loc_2CEE7`), not bit `0x4` (the teleport-then-engage
   branch). Bits `0x100`/`word_33306`'s `0x4` were also checked and
   confirmed to be pure UI/rendering (a weapon-select icon redraw, a
   corridor-background animation setup respectively), not gameplay
   logic worth reimplementing. Tests in `test_combat.c` cover the
   survive/die/miss cases; all 22 suites pass.

   **`word_33306`'s remaining 3 bits checked too, same day -- all three
   reduce to the one primitive just reimplemented**: `0x2`/`0x1` are
   pure animation (a screen-shake and a fade-to-black respectively)
   that, once finished, jump straight into bit `0x200`'s own scan tail
   -- not a different mechanic at all. `0x8` is a projectile-sprite
   variant with its own targeting (`ClassifyObstacleAtViewportRow`
   along up to 5 viewport rows) but an identical hit-resolution tail,
   field for field, to `ApplyDamageToMapMonster`. `ApplyEncodedItemEffect`
   itself ends right after this branch (`yendor2.asm:52748`), confirming
   the full 19-branch dispatch is now completely accounted for. No new
   code needed -- `combatApplyDamageToMapMonster` already covers all
   four branches' gameplay logic; what remains for each is animation/
   orchestration only.

   **Bit `0x80` traced, resolving the discrepancy above, and its own
   light-source mechanic reimplemented, same day**: it's the arming
   write for `word_3295A` bit `0x800`, the gate
   `MaybeForceTickWorldAilments` tests before calling
   `TickWorldAilments` -- the older round's "world-state timers"
   description was right; this round's own first guess (day/night
   music) was wrong, now corrected. Tracing `TickWorldAilments` found a
   genuinely tangled subsystem of 3 counter families -- but one of them
   turned out to be fully, concretely resolvable: `word_36C85`/`89`/`8B`
   are 3 light-source burn-down counters, confirmed against real
   `WORLD.DAT` item data (the dispatch ids are literally CANDLE/TORCH/
   LIGHT SOURCE and their own lit variants). Reimplemented as
   `lightSourceApply`/`lightSourceTick` (new `src23/lightsource.c`/`.h`);
   tests in `test_lightsource.c` (23rd suite); all 23 suites pass.

   **`IsItemRangeAvailable` reimplemented too, same day**
   (`yendor2.asm:22764`, already correctly named by an earlier session
   -- "CORRECTED from `CheckTransportAvailability` -- too specific a
   guess"): the 6-entry-table-then-party-inventory half, as
   `partyFindItemInRange`/`itemRangeAvailable` (`src23/party.c`/`.h`,
   tests in `test_party.c`) -- exactly the "item-availability"
   dependency `RestPartyAndAdvanceClock`'s own regen-rate derivation
   has been waiting on since a much earlier round.

   **The regen-rate formula itself fully traced, and
   `ConsumeItemChargeResource`'s own default mode reimplemented, same
   day**: active member count, up to that many
   `IsItemRangeAvailable(0x36, 0x40)` + `ConsumeItemChargeResource`
   attempts (breaking at the first miss), `regenPercent = (100 /
   activeCount) * consumedCount`. The item range is confirmed against
   real `WORLD.DAT` data in both games -- literal food items (MEAT,
   BREAD, FOOD, CHEESE, ALE), not a guess. Confirmed
   `ConsumeItemChargeResource`'s "default decrement-with-auto-discard"
   mode is the only one `RestPartyAndAdvanceClock` (and every caller
   except `RepairItemCommand`) ever reaches, and reimplemented its
   party-inventory half as `partyConsumeItemCharge`
   (`src23/party.c`/`.h`) -- decrements a multi-use item's own charge
   count, or discards the slot and deducts its weight for a single-use
   one (confirmed via a previously-unnamed target-entry flag bit, real
   food items in both games all have it clear). Tests in `test_party.c`
   cover both cases plus discarding on the last charge; all 23 suites
   pass.

   **The full derivation composed, same day**: `partyDeriveRestRegenPercent`
   (`src23/party.c`/`.h`) ties the active-member count, the consume
   loop, and the truncation-order formula together exactly as
   `RestPartyAndAdvanceClock` does -- including reproducing its own
   integer-truncation quirk (`(100/activeCount)*consumedCount`, not the
   more natural `(100*consumedCount)/activeCount` -- 3 active members
   all fed yields 99%, not 100%) and the stop-dead-at-first-unoccupied-slot
   quirk for the active-member count itself. A global-table match is
   treated as a miss rather than consumed, a conservative
   under-approximation given the global-table case isn't reimplemented.
   This closes the regen-rate dependency chain `party.h` has been
   flagging since a genuinely old round. Tests in `test_party.c`; all 23
   suites pass.

   **The item-slot-level light-source transition reimplemented too,
   same day**: `TickAilmentDuration` -- confirmed genuinely separate
   from `lightSourceApply`/`Tick`'s own standalone counters (see
   `file-formats.md`'s "world ailments" section) -- as
   `lightSourceTickItemSlot` (`src23/lightsource.c`/`.h`, extending
   `LightSourceState` with a new `instanceCount[3]` field for
   `word_9425`/`9429`/`942B`). On expiry a lit item's own id advances
   by 1 (confirmed against real data: 9->10 LIT CANDLE->USED CANDLE,
   etc.) rather than being zeroed, and the shared `litFlags` bit only
   clears once every coexisting lit instance of that light source has
   burned out. Tests in `test_lightsource.c`; all 23 suites pass.

   **The 6-entry global-table consumption case reimplemented too, same
   day**: `itemSlotConsumeGlobalCharge` (`src23/party.c`/`.h`) -- the
   same decrement-or-discard logic as `partyConsumeItemCharge` (now
   sharing a `static itemSlotSpendCharge` helper), minus the weight
   deduction since the resource panel isn't weighed.
   `partyDeriveRestRegenPercent` now consumes a global-table match
   instead of treating it as a conservative miss. Tests in
   `test_party.c`; all 23 suites pass.

   **The "second timer array" resolved, 2026-10-02**: it was a
   segment-offset alias -- `TickWorldAilmentTimers`' 6 timers at
   `DS:0x9433` *are* bit `0x80`'s `word_36C93`..`9D`, and the light
   counters `0x9425`.. are `word_36C85`.. (one field, merged in
   `LightSourceState`). Bit `0x80` is the light spells (MINER'S LIGHT
   I/II, INFINITE ILLUMINATION), reimplemented as
   `lightSourceArmSpellTimer`/`lightSourceTickTimers`. **Still not
   done**: the 3 special `ConsumeItemChargeResource` modes and container
   recursion (a separate, genuinely large CURGAME-backed subsystem this
   project has no reader for). See
   `file-formats.md`'s own dedicated "world ailments" and "Quest-item
   and party-inventory range checks" sections for the full confirmed
   address/field map of what's left -- genuinely small now.

   **Bit `0x100` corrected and `TickMonsterTimer`'s gate-bit setter
   found, same day**: an earlier pass this same day misclassified
   `loc_2C92A` as pure UI after reading only its setup lines; it's
   actually a multi-row piercing projectile (optionally continuing past
   the first hit, per `word_33302` bit `0x800`) that, on a landed hit,
   arms `TickMonsterTimer`'s own gate -- the first confirmed setter this
   project has found for it, see `file-formats.md`'s dedicated section
   and `monster.h`'s updated `monsterTickTimer` doc comment. Not
   reimplemented itself (a genuinely separate mechanic from the attack
   family already done, with its own unresolved details -- what
   `MonsterFieldAnimSet` selects between, what the fixed immunity-bit
   mark means) -- corrected rather than left standing.

   **Update 2026-10-03 -- supersedes the list below**: the whole dispatch is
   now `spellSelectBranch`; every branch real data uses is implemented or
   decoded (held item, JUMP spells via `spelljump.c`, screen-wide scan,
   projectile hit/splash, LIFE FORCE). What's left of candidate 8 is
   non-logic: the viewport-row/animation shells, the monster-to-combat-slot
   pull after a jump, and the container-recursion/ConsumeItemChargeResource
   items. Detail in file-formats.md.

   **Still open** (historical): `word_33302` bits `0x10`/`0x100` (held-item cursor
   and the piercing-projectile mechanic just corrected above,
   respectively; update 2026-10-03: bit `0x100`'s per-monster hit is now
   `combatApplyProjectileHit` -- bit `0x400`'s splash is `combatApplySplashHit`
   + `combatReapDeadMapMonsters`; only the flight/animation shell remains) and bit `0x80`'s own surrounding dispatch (its
   per-target light-source counters are reimplemented, but the branch's
   own 6-slot arming write and `IsItemRangeAvailable`'s item-slot
   transition logic aren't); `loc_2CF51` (the bit `0x2000` diversion) was
   deferred here and has since been reimplemented as
   `combatApplyLifeForceSpell` (see file-formats.md); bit `0x200`/`word_33306`'s own
   surrounding scan-loop *shape* (which of the two targeting modes
   fires, and the viewport-depth bookkeeping around it) is
   orchestration, not yet composed, even though the per-target attack
   itself is done; a dozen bytes in the record's own middle (offsets
   `0x36`-`0x3F`) have no confirmed consumer.
   **A fourth branch reimplemented, same round**: bit `0x40`, the
   sibling of bit `0x1` flagged above -- confirmed instruction-identical
   in Chapter 3 and shares bit `0x1`'s exact probe-then-classify-then-mark
   shape, differing only in its qualifying outcome set
   (`LockFlag40`/`LockPriced`/`CurgameFlag40` instead of
   `LockMagical`/`CurgameFallbackB`) and a UI-only text-column choice
   (not modeled). Rather than a second named wrapper, `interactKnock`
   was refactored onto a shared, outcome-set-parameterized primitive,
   `interactResolveIfOutcome` (`src23/interact.c`/`.h`), that bit
   `0x40` uses directly -- deliberately left unwrapped since this
   project doesn't have a confident narrative for what unifies its 3
   qualifying outcomes into one spell/item effect. Tests in
   `test_interact.c`/`test_worldobjects.c`; all 18 suites pass.
   **Corrected 2026-09-25**: one branch
   (`yendor2.asm:51586`) previously described in this project's own
   docs and memory as a "banish the monster on the facing tile"
   mechanic is actually a teleportation-style effect that relocates
   the party (`g_wipeEffectX/Y` -> `g_partyWorldX/Y`, via a
   destination-search loop not yet traced) and then, if a monster
   occupies the destination cell, copies it into `g_monsterSlots` slot
   1 to engage it in turn-based combat (the same record-relocation
   idea `CompactMonsterSlots` uses, now that this project understands
   that struct) rather than despawning it — see `file-formats.md`'s
   corrected note. The remaining ~17 branches (world-state timers, a
   corridor/ranged-attack path via `ApplyDamageToMapMonster` --
   already named and reusable with `monsterGrantRewards`/
   `monsterPoolRemove` once its own `ApplyAttackToTarget` dependency is
   traced -- held-item cursor updates, weather effects, and more)
   weren't traced this round either, given the function's size; a good
   candidate for a dedicated multi-round pass rather than one-off
   attention. **One piece already peeled off cleanly**: the container
   branch's own trigger is gated by `ConfirmContainerInteraction`
   (untraced -- it's what actually sets up `word_3331A`/`word_33300`/
   `word_332DA` etc. before `ApplyEncodedItemEffect` runs), and a
   sibling branch in `InteractWithContainer` (not `ApplyEncodedItemEffect`
   itself) applies a trivial class-gated ability effect (id 3, "costs
   nothing, inflicts nothing" -- likely just a status icon) directly,
   without going through the big dispatcher at all -- surfaced while
   reading this, not the dispatcher: `partyKnownAbilityIdMax`/
   `partyKnownAbilityIds` (see the status entry above), the read-back
   half of the same `PartyFieldFlagBankCA` ability system this
   branch's own gate (`TestRecordFlag_CA`) checks.

~~9. **Party teleport/fast-travel destinations**~~ -- **lookup/gate
   logic done 2026-09-30**; found the same day while finally tracing
   `WorldObjectFlagUnknown2000` (`worldobjects.h`), a real, common
   world-object flag this project had left untested for a while; fully
   traced and both games' data extracted the same day. Resolves the
   long-open "region/town password" question (see "Open questions"
   below) -- and the name turns out to be completely literal, not a
   metaphor: a `0x2000`-flagged world object's own `value` is a 1-based
   index into a destination table (`DS:0xD40B`, 16-byte stride, exactly
   **187** entries, Chapter 2; `DS:0xBA95`, 18-byte stride, exactly
   **139** entries, Chapter 3 -- both counts *proven* by address
   arithmetic, not estimated: each table's base plus its entry count
   times its stride lands exactly on its own `IsDestinationUnlocked`
   gate table's base address, and both match `WorldObjectFlagUnknown2000`'s
   own reachable-record counts 1:1 in each game) giving the party's new
   world position/facing/music plus an optional unlock gate.
   `IsDestinationUnlocked` is a small, `0xFFFF`-terminated table
   (22-byte stride both games; 20 entries Chapter 2, 35 Chapter 3) of
   `(destinationId, flagWordAddress, bitMask, promptId, hasMsg, up to
   12 bytes of password text)` -- every flag address lands inside the
   already-reimplemented `g_globalFlags` region, so the underlying gate
   is the *same* global quest/world-state flag system `globalflags.c`
   already covers. Most rows with `hasMsg == 0` are a **literal typed
   password check**: `ShowConfirmPrompt`, then a 12-character
   byte-for-byte compare (space = end-of-word wildcard) against the
   embedded text, success sets the same flag bit the "already unlocked"
   check reads. Real extracted words confirm this beyond doubt -- Chapter
   2: `ALEXANDER`, `DOMAIN`, `OPPOSITION`, `HORSEMAN`, `WHITE`, `TREES`,
   `NORTH`/`EAST`/`SOUTH`/`WEST`, `MYSELF`, `SAFARI`, plus one
   unreachable all-null-byte row; Chapter 3: `NOBLEMAN`, `GAUNTLET`,
   `COMPASSION`, `RUSE`, `GEMSTONE`, `CALANTHA`, `ALLIANCE`, `TIMBER`,
   `SOLITAIRE`, `DELIA`, `DRAGONSKIN`. **A real Chapter 2 vs Chapter 3
   difference found**: Chapter 3's gate re-tests the destination
   record's own flags a second time to add a silent
   deny-with-a-different-message sub-case Chapter 2 doesn't have; also,
   Chapter 2 has a one-off hardcoded landing-spot override (one
   destination, one night-time window) and Chapter 3 doesn't, while
   Chapter 3 calls `ApplyMapTriggerEffect` on arrival and Chapter 2
   never does. See `file-formats.md`'s "Party teleport/fast-travel
   destinations" section for the full field-by-field writeup.
   **Reimplemented same day**: `travelDestinationLookup`/
   `travelCheckUnlock`/`travelResolvePassword` in `src23/travel.c`/`.h`,
   both games' full tables embedded as literal C data, following
   `combat.c`'s "decide, don't apply" split (`travelCheckUnlock` is a
   pure decision function; `travelResolvePassword` mutates the
   `globalflags.c` buffer only once the caller's UI has collected
   actual typed text). Exhaustively cross-checking every Chapter 3 gate
   row against its own destination record's flags confirmed the
   "neither bit set" sub-case is provably unreachable in real data
   (21/21 empty-password rows have destination bit `0x4000`; 13/13 real
   passwords have bit `0x8000`) -- reproduced anyway for fidelity.
   Tests in `test_travel.c` (19th suite) exercise `RUSE` end to end.
   **Still not reimplemented, deliberately**: the music-track/
   travel-mode side effects (`word_36CB1`/`word_36CB3`/`word_36C79`/
   `word_36CBF` Chapter 2, `ds:0xCF2F`/`0xCF31`/`0xCF33`/`0xCF3F`/
   `0xCEF9` Chapter 3) aren't traced to a confirmed consumer yet, so
   `TravelDestination` carries them as raw undecoded fields rather than
   composing behavior against unconfirmed inputs; `TravelToDestination`
   itself (the UI-driving orchestration around these two functions) is
   deferred to the eventual SDL2 layer, same as this project's other
   top-level input handlers.

~~10. **`ApplyMapTriggerEffect`/`IsPositionInTriggerList`**~~ -- found
    and reimplemented 2026-09-30 while chasing `IsPositionInTriggerList`
    for `gameClockRestAllowed`'s own dependency; both games' tables
    extracted the same day
    (`ida_scripts/dump_trigger_list_table.py`, one script per game).
    Dispatch structure fully traced for both games: a 6-way switch on a
    matched trigger-table entry's own flags (teleport / ailment-tick /
    3 fixed icon-bar effect ids / a data-driven equipment-corrosion
    event / a fully data-driven default effect id). Most of it composes
    from pieces already reimplemented elsewhere: `PrepareTrapEffectSlots`
    is `effectGetDef`; the icon-bar population matches
    `ApplyEffectAndDrawIconBar`'s existing dispatch; and the corrosion
    branch's own per-member eligibility field, resolved against real
    data (a trigger entry's `+4` value, `0x142`), turned out to be an
    already-named equipment-slot offset already consumed by
    `partyTickEquippedItemDurability`. The 3 fixed-effect-id branches
    turned out to be **gold/ore-theft traps, not HP/MP damage** --
    confirmed via the already-embedded effect table (ids
    `0xF`/`0x10`/`0x11`'s own `costFlags` are
    `EffectCostGold`/`Ore1`/`Ore2`), explaining why those branches alone
    populate *both* of the icon slot's value-field words (a Bcd4
    amount) while the data-driven default branch only populates one.

    **A real, confirmed Chapter 2 vs. Chapter 3 difference in the
    default branch, genuinely exercised by real data (corrected from an
    earlier miscount this same round)**: Chapter 3 adds a per-member
    exclusion (skip if item `0x275` is equipped in equipment code
    `0x13`'s slot) that Chapter 2 doesn't have at all -- 4 of Chapter
    3's 19 real trigger entries actually set the gating bit, not zero
    as an earlier pass this round mistakenly concluded (a flag value of
    `0x8001` was misread as "no dispatch bits set, therefore
    Chapter-2-equivalent," missing that bit `0x8000` is just the axis
    selector, not a dispatch bit at all).

    **The Chapter 3 teleport-branch convergence with `TravelToDestination`
    is confirmed, not just suspected**: its own teleport branch
    populates the *exact same globals* `TravelToDestination` does and
    calls the same `TickTravelResourceAilments` helper -- explaining why
    Chapter 3's `TravelToDestination` calls `ApplyMapTriggerEffect` on
    arrival (see candidate 9). The two tables are **not** byte-for-byte
    the same shape (20-byte vs. 18-byte records, different field
    offsets) but share every field's own *semantic* target. Teleport
    itself stays unreimplemented here -- pure UI-driving orchestration,
    the same scope boundary `TravelToDestination` itself has.

    Reimplemented as `mapTriggerFind`/`mapTriggerDecide`/
    `mapTriggerApplyEffect`/`mapTriggerApplyCorrosion` in a new
    `src23/maptrigger.c`/`.h`. Tests in `test_maptrigger.c` (21st suite)
    cover the lookup against real table entries from both games, every
    dispatch branch including Chapter 3's real (not synthetic)
    item-exclusion case, the gold-theft Bcd4 construction (necessarily
    synthetic -- no real trigger entry exercises it in either game),
    and the corrosion apply path's full eligibility chain. All 21
    suites pass. Still not reimplemented: the teleport and ailment-tick
    branches themselves (pure orchestration / a not-yet-built
    prerequisite system). See `file-formats.md`'s own dedicated section
    for the complete writeup.

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

- ~~Is `NUORE` a currency, a resource, or both?~~ — **already resolved,
  just never marked closed here**: `file-formats.md`'s "Global material
  counters and BCD arithmetic" section has had the full answer since an
  earlier session. `NUORE` is a resource, not a currency — gold
  (`g_partyGold`, `0x94B3`, labeled `"$"`/"GOLD COINS:") is the actual
  currency. `NUORE` (`0x94BB`) and `MAGIC ORE` (`0x94B7`) are a pair of
  BCD counters confirmed exactly consecutive with gold (by
  `ShowResourceDepletedOverlay`'s own scan of all three), both spent on
  alchemy/spell costs (`CastSpell`'s `0x1C` ability converts 10 units of
  one into the other) — so it's genuinely a 3-resource economy, just
  with only one of the three being spendable currency. `src23/`'s own
  `party.h`/`effect.h`/`savegame.h` already model this generically
  (`SaveHeaderGold`/`OreCounter1`/`OreCounter2`,
  `EffectCostGold`/`Ore1`/`Ore2`) without committing to which raw
  counter is `MAGIC ORE` vs `NUORE` in the C naming, since the two
  behave identically wherever this project's own logic touches them —
  reimplementing the alchemy screen itself is what would need the more
  specific labels, not blocked on anything today. `engine-diffs.md`
  still documents that Chapter 3 replaces this mechanic with a
  5-artifact quest system, so the above is Chapter 2-only.
- ~~Relationship between the "region" passwords and "town" passwords~~
  — **fully resolved 2026-09-30, both games' data extracted, mechanism
  confirmed down to real in-game password words**: not a metaphor —
  "region/town password" describes a literal typed-text unlock check.
  Found the actual consumer while tracing `WorldObjectFlagUnknown2000`
  (`worldobjects.h`) for the first time — a real, common world-object
  flag this project had flagged as untested by any traced caller.
  `start`'s own per-cell dispatcher (reached via `ProbeFacingTile`, the
  same probe `interactKnock` uses) tests it directly and, on a match,
  calls `TravelToDestination(ax = [si+4]`, the object's own `value`
  field`) — a party teleport/fast-travel handler that looks up a
  destination-id table by 1-based id: exactly 187 entries in Chapter 2
  (`DS:0xD40B`, 16-byte stride) and exactly 139 in Chapter 3
  (`DS:0xBA95`, 18-byte stride, a genuinely different record shape) —
  both counts *proven* by address arithmetic (each table's own end
  lands exactly on its `IsDestinationUnlocked` gate table's start, no
  gap), matching `WorldObjectFlagUnknown2000`'s own reachable-record
  counts 1:1. The lock check, `IsDestinationUnlocked`, is a small,
  `0xFFFF`-terminated table (22-byte stride both games, 20 entries
  Chapter 2, 35 Chapter 3) whose flag addresses all land inside the
  already-reimplemented `g_globalFlags` region — reusing that same
  system — but most rows with no canned rejection message are a real
  **typed password prompt**: `ShowConfirmPrompt` then a 12-character
  compare against text embedded right in the table row, success
  setting the same global flag bit the "already unlocked" check reads.
  Confirmed beyond doubt by extracting the actual words (Chapter 2:
  `ALEXANDER`, `DOMAIN`, `OPPOSITION`, `HORSEMAN`, `WHITE`, `TREES`,
  `NORTH`/`EAST`/`SOUTH`/`WEST`, `MYSELF`, `SAFARI`; Chapter 3:
  `NOBLEMAN`, `GAUNTLET`, `COMPASSION`, `RUSE`, `GEMSTONE`, `CALANTHA`,
  `ALLIANCE`, `TIMBER`, `SOLITAIRE`, `DELIA`, `DRAGONSKIN`) — and one
  of them, `RUSE`, is the *exact same word* as the Chapter 3 "Note"
  signed `THE PASSWORD IS` `` `RUSE~ `` this project already had
  cataloged under `file-formats.md`'s "In-world readable text" from an
  earlier, unrelated session, closing the loop this entry used to flag
  as "not yet cross-checked" — the in-world note really is how the
  player learns the literal string to type at that destination's
  prompt. See `file-formats.md`'s "Party teleport/fast-travel
  destinations" section and `roadmap.md` candidate 9 for the full
  writeup, including the confirmed Chapter 2 vs Chapter 3 structural
  differences (Chapter 2's one-off hardcoded landing override and lack
  of `ApplyMapTriggerEffect`; Chapter 3's extra silent-deny sub-case
  and lack of the override). Still open: the day/night music-track and
  travel-mode side effects, not yet traced to confirmed consumers —
  deliberately left rather than composing partial state against
  unconfirmed globals.
- What `sg0977`/`sg0ffc`/`sg1486`/`sg195C`/`sg1ABC` (the 5 segments
  with IDA hex-address names instead of sequential `segNNN`) actually
  are — likely harmless IDA bookkeeping, not investigated further
  since it hasn't blocked anything.

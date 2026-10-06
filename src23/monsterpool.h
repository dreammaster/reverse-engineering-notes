#ifndef YENDOR23_MONSTERPOOL_H
#define YENDOR23_MONSTERPOOL_H

#include <stdbool.h>
#include <stdint.h>

#include <stddef.h>

#include "bcd4.h"
#include "dungeongrid.h"
#include "game.h"
#include "globalflags.h"
#include "monster.h"
#include "party.h"
#include "random.h"
#include "savegame.h"
#include "worldmap.h"

/*
 * The 80-slot live monster pool (g_levelMonsters, monster.h's
 * MonsterPoolSize/MonsterRecordSize): both how it's kept in sync with
 * the scrolling dungeon-grid window, and how a new monster is spawned
 * into it in the first place.
 *
 * The window-sync half is RefreshDungeonMapWindow's tail
 * (yendor2.asm:29496 on, instruction-identical in Chapter 3), the part
 * not covered by dungeongrid.c's own base-window build: after that
 * build, the original walks every pool slot and either relinks a
 * still-visible monster to its new grid-relative cell (and bakes a
 * "monster here" overlay into that DungeonGridCell) or despawns one
 * that scrolled out (zeroes its record and clears its spawn flag).
 *
 * The spawn half is SpawnMonsterInFacingDirection (yendor2.asm:33008,
 * instruction-identical in Chapter 3, including its facing-offset
 * tables -- checked byte-for-byte against both real executables).
 * Its own catalog-copy, positioning, animation-start and
 * activate-by-distance steps already matched monster.h's
 * monsterRecordSpawn/monsterRecordPlace/monsterRecordStartAnimation/
 * monsterTryActivateByDistance closely enough that this module's own
 * spawn function is mostly composition of those, plus the one
 * genuinely new piece: a facing-dependent position-offset table (see
 * monsterSpawnOffsetTable). Not reimplemented: the caller-side decision
 * of *which* type id / viewport position to spawn at
 * (TryTriggerMonsterEncounterAtCell, driven by first-person viewport
 * rendering -- out of scope until the rendering layer exists).
 */

/*
 * TestCellMonsterSpawnedFlag/SetCellMonsterSpawnedFlag/
 * ClearCellMonsterSpawnedFlag's shared CURGAME bitmap
 * (SaveSectionMonsterSpawnFlags), one bit per monster *type id* -- not a
 * per-location spawn-point index, confirmed by cross-referencing both
 * call sites: TryInteractAtPosition tests it with a worldobjects.c
 * 0x800 record's own `value` field, and RefreshDungeonMapWindow's
 * despawn path clears it with the live record's own MonsterFieldType --
 * the same number space, meaning a 0x800 marker's value *is* the type
 * id it spawns. Bit-packed MSB-first within each byte (byte
 * typeId/8, bit 7-typeId%8), the same convention already confirmed for
 * the explored-map and lock "already unlocked" bitmaps.
 */
bool monsterSpawnFlagTest(SaveGame *save, GameKind game, unsigned typeId);
void monsterSpawnFlagSet(SaveGame *save, GameKind game, unsigned typeId);
void monsterSpawnFlagClear(SaveGame *save, GameKind game, unsigned typeId);

/*
 * Refreshes every occupied pool slot against grid's (already-rebuilt,
 * see dungeongrid.h) window: a monster whose world position is still
 * within the tracked range gets MonsterFieldCell updated to the new
 * grid-relative offset and a DungeonGridCellFlagOverlay marker baked
 * into that cell (skipped, not an error, if the relative offset lands
 * on the one-cell edge slack the original's own bounds check allows but
 * DungeonGrid's 78x78 array doesn't cover -- see the .c file); a
 * monster that's scrolled out has its record zeroed and its spawn flag
 * cleared via monsterSpawnFlagClear. save may be NULL (spawn flags
 * aren't touched then). pool must be MonsterPoolSize records of
 * MonsterRecordSize bytes each. Returns the number of slots still
 * occupied after the refresh.
 */
unsigned monsterPoolRefreshWindow(uint8_t *pool, DungeonGrid *grid, SaveGame *save);

/*
 * SpawnMonsterInFacingDirection's facing-dependent spawn-position offset
 * table (yendor2/ida_scripts/dump_spawn_offset_tables.py): 51 entries per
 * facing, each a signed (dx, dy) pair added to the party's world position
 * to get a spawn position. Indexed by a "viewport index" (0-50) that
 * identifies one specific rendered cell of the first-person dungeon
 * viewport -- entries are grouped by dy (deepest/widest row first,
 * narrowing to a single 3-cell-wide strip near the party), matching a
 * perspective viewing cone, not a literal row/column pair. The original
 * only calls in with an index >= 0x11 (17) -- indices 0-16 cover the
 * single widest, deepest row and are never used for a spawn -- but the
 * table itself is exposed in full; range-restricting which indices are
 * "reachable" is the caller's concern once the rendering loop exists.
 */
enum { MonsterSpawnOffsetCount = 51 };

typedef struct {
    int8_t dx, dy;
} MonsterSpawnOffset;

/* The 51-entry table for one of the 4 SaveFacing values, or NULL if facing isn't one of them. */
const MonsterSpawnOffset *monsterSpawnOffsetTable(uint16_t facing);

/*
 * SpawnMonsterInFacingDirection: finds an empty pool slot and fully
 * initializes it there --
 *   - monster.h's monsterRecordSpawn (catalog block copy, full health,
 *     on-death flags) for typeId,
 *   - a spawn position from partyWorldX/Y + monsterSpawnOffsetTable(facing)[viewportIndex],
 *     placed via monsterRecordPlace against gridOriginRow/gridOriginCol,
 *   - a random animation start via monsterRecordStartAnimation
 *     (rolls RandomInRange(5) using rng, matching the original),
 *   - monster.h's monsterTryActivateByDistance(record, viewportIndex)
 *     (may or may not set MonsterStateAware immediately, matching the
 *     original calling it with the same viewportIndex right after
 *     placement),
 *   - monsterSpawnFlagSet(typeId) if save is non-NULL.
 * Returns the slot index spawned into, or -1 if the pool has no empty
 * slot or typeId is unknown to catalog (nothing is changed in that case).
 * viewportIndex must be < MonsterSpawnOffsetCount.
 */
int monsterPoolSpawn(uint8_t *pool, const MonsterCatalog *catalog, SaveGame *save, GameKind game, uint16_t facing,
                      uint16_t partyWorldX, uint16_t partyWorldY, uint16_t gridOriginRow, uint16_t gridOriginCol,
                      unsigned viewportIndex, unsigned typeId, RandomState *rng);

/*
 * FindMonsterTypeInLevelPool (yendor2.asm:33123): the pool slot holding a live monster of type `typeId` (the first, scanning slots 0-79), or -1. The original
 * also runs TryActivateMonsterByDistance on the monster it finds, which the encounter scan below does with the cell's viewport index.
 */
int monsterPoolFindType(const uint8_t *pool, unsigned typeId);

/*
 * TryTriggerMonsterEncounterAtCell (yendor2.asm:30285; Chapter 3 the same), run once per cell of the first-person view as it is drawn: for the viewport cells
 * 17-48 (not the farthest row, not the party's own cell 49) that are not hidden (viewport.h) and carry the "monster here" marker (+6 bit 0x400, whose +4 is
 * the monster's type id: baked for a not-yet-spawned marker by windowbake.h, for a live monster by monsterPoolRefreshWindow): if no monster of that type is in
 * the pool the monster is spawned from its catalog record at that cell (monsterPoolSpawn, which also marks the type as spawned), otherwise the existing one
 * has monsterTryActivateByDistance run with the cell's index. Either way the monster is then drawn: cellMonsters[index] receives the pool record (NULL for
 * every cell without one). Returns the number of monsters spawned by the scan. `cells` are the 51 view cells (viewportBuild + viewportComputeVisibility).
 */
unsigned monsterPoolEncounterScan(uint8_t *pool, const MonsterCatalog *catalog, SaveGame *save, GameKind game, uint16_t facing, uint16_t partyWorldX,
                                   uint16_t partyWorldY, uint16_t gridOriginRow, uint16_t gridOriginCol, const DungeonGridCell cells[51],
                                   uint8_t *cellMonsters[51], RandomState *rng);

/*
 * GrantMonsterRewards (yendor2.asm:33151): a dying monster's own loot
 * fields (monster.h's MonsterLoot) are added into 4 staging BCD4
 * counters -- not permanent material totals directly; the original
 * drains these into the real counters later, in ShowLootAndAwardExperience
 * (a UI-heavy function, not reimplemented) -- plus its two on-death
 * global-flag deltas (MonsterFieldFlagOnDeath/2) get applied via
 * globalflags.h. globalFlags/globalFlagsSize may be NULL/0 to skip the
 * flag step, e.g. if the caller hasn't set up a g_globalFlags buffer
 * (its real size isn't confirmed -- see globalflags.h).
 */
typedef struct {
    Bcd4 gold;
    Bcd4 nuore;
    Bcd4 ore;
    Bcd4 experience;
} MonsterRewardStaging;

void monsterGrantRewards(MonsterRewardStaging *staging, const uint8_t *record, uint8_t *globalFlags,
                          size_t globalFlagsSize);

/*
 * The state-mutating half of ShowLootAndAwardExperience (yendor2.asm:33827,
 * instruction-identical in Chapter 3) -- the "drain the staging counters"
 * step monsterGrantRewards' own doc comment already points to. Its UI
 * half (the "treasure found" panel, sound cue, portrait redraw) is not
 * reimplemented here. Adds staging's gold/ore/nuore into the save's
 * permanent material counters (SaveHeaderGold/OreCounter1/OreCounter2 --
 * confirmed this session which staging field maps to which counter by
 * reading both GrantMonsterRewards' stage-in and this function's own
 * drain-out against the same four global scratch addresses) and,
 * for every occupied, non-incapacitated party slot (SaveHeaderPartySlots),
 * adds staging->experience to that member's PartyFieldExperience and
 * calls partyCheckForLevelUp -- called unconditionally per slot exactly
 * like the original (partyCheckForLevelUp has its own internal
 * incapacitated guard, matching CheckForLevelUp's).
 */
void monsterRewardsAward(SaveGame *save, GameKind game, const MonsterRewardStaging *staging);

/*
 * RemoveMonsterFromMap (yendor2.asm:33784): clears the "monster here"
 * overlay from the record's linked DungeonGridCell (found from the
 * record's own world position, not its raw +6 cell offset -- safe even
 * if the monster is no longer within grid's current window) and zeroes
 * the whole record. grid may be NULL to skip the overlay-clear step.
 */
void monsterPoolRemove(uint8_t *record, DungeonGrid *grid);

/*
 * ClassifyObstacleAtWorldPosition (yendor2.asm:1878, yendor3.asm:5526):
 * a monster-movement-specific passability check on a world map cell --
 * genuinely different thresholds from movement.h's own player-facing
 * ClassifyFloorType/IsCellTypeImpassable, not reusable from there.
 * Errors 1 (a literal wall) and 2 ("feature", a nonzero floor overlay --
 * doors, scenery) are both treated as blocking by every known caller;
 * they're kept distinct here only because the original does.
 */
typedef enum { MonsterObstacleClear, MonsterObstacleWall, MonsterObstacleFeature } MonsterObstacle;

MonsterObstacle monsterClassifyObstacle(GameKind game, uint16_t wallType, uint16_t floorType);

/*
 * ProcessLevelMonsters' approach/ambush check (yendor2.asm:33430 on,
 * instruction-identical in Chapter 3 including the RandomInRange(100)
 * thresholds -- confirmed by direct comparison, not assumed). A monster
 * only ever engages the party head-on, along a grid-aligned line: if
 * it's not on the exact same world row or column as the party, nothing
 * happens. Otherwise this scans map cells from one step away up to
 * adjacent-to-the-party (capped at 5 steps, see below), and if every
 * cell along the way is MonsterObstacleClear, sets the MonsterWound
 * direction bit matching the monster's side of the party and rolls
 * RandomInRange(100) against monsterAmbushThreshold(awareness); success
 * sets MonsterWoundAmbushPending too (consumed by the "side trap"/ambush
 * presentation pipeline, not reimplemented here). The monster's own
 * position is never changed by this check itself (the step toward the party
 * is monsterWalkTowardParty, the next stage of the same turn).
 *
 * **A confirmed Chapter 2 bug, not replicated here**: Chapter 2's own
 * code only explicitly bounds this scan to 5 steps when the monster is
 * on the "far" side of the party (reusing an uninitialized register
 * otherwise, which in practice inherits whatever's left in
 * ProcessLevelMonsters' own unrelated 80-slot loop counter); Chapter 3
 * adds the missing explicit bound for both sides, fixing it. This
 * reimplementation always uses Chapter 3's corrected 5-step bound for
 * both games, rather than replicate Chapter 2's incidental,
 * pool-slot-index-dependent behavior.
 */
void monsterApproachParty(uint8_t *record, GameKind game, const WorldMap *map, int partyWorldX, int partyWorldY,
                           RandomState *rng);

/*
 * ProcessLevelMonsters' full per-slot flow (yendor2.asm:33379 on),
 * composing every piece above in the original's exact order:
 *   1. Skipped entirely unless MonsterStateAware is set.
 *   2. monsterTickTimer runs. MonsterTickExpired -> grants rewards
 *      (accumulated into staging, not drained to permanent totals --
 *      see monsterGrantRewards) and removes the monster
 *      (MonsterTurnRemoved). MonsterTickOngoing -> skipped this turn,
 *      same as if it weren't aware at all (MonsterTurnSkipped) --
 *      *not* given a chance to approach, matching the original exactly.
 *   3. Otherwise (MonsterTickIdle), skipped if MonsterFieldApproachGate
 *      is 0 or MonsterStateBusy is set; otherwise monsterApproachParty
 *      runs (MonsterTurnApproached).
 * globalFlags/globalFlagsSize/grid may be NULL/0 to skip their
 * respective steps, same as the functions they're passed through to.
 * Note RemoveMonsterFromMap never clears the CURGAME spawn flag (only
 * monsterPoolRefreshWindow's scroll-out despawn does) -- a monster
 * that dies here can't be re-triggered by the same worldobjects.c
 * marker, matching the original exactly.
 */
typedef enum { MonsterTurnSkipped, MonsterTurnRemoved, MonsterTurnApproached } MonsterTurnOutcome;

/*
 * The rest of ProcessLevelMonsters' per-monster turn (yendor2.asm:33534 on, Chapter 3 identical in structure): unless the ambush
 * check above triggered (MonsterWoundAmbushPending), the monster -- whenever it is aware, not timed out, and whether or not the
 * approach gate or busy flag let the ambush check run -- clears its MonsterWound direction bits (0x1F00) and takes one step toward
 * the party along the grid. The step is chosen thus: if the party is in an adjacent row (the monster is one row above/below), try the
 * vertical step first (a "vertical-first" attempt); otherwise a horizontal step toward the party's column if it is in another
 * column; failing that (same column) a vertical step. A blocked step falls back to the other axis once (vertical-first -> horizontal ->
 * vertical; horizontal -> vertical); a blocked vertical step ends the turn. A free step whose target is the party's own cell starts
 * combat (MonsterMoveEngaged -- the caller copies the record into a combat slot); otherwise the record and the grid's "monster here"
 * markers (cell flag 0x400 and its occupant word) move to the new cell (MonsterMoveStepped).
 */
typedef enum { MonsterMoveNone, MonsterMoveStepped, MonsterMoveEngaged } MonsterMoveOutcome;

/*
 * IsMonsterStepBlocked (yendor2.asm:49530, Chapter 3 :37868): can a monster with awareness traits `traits` (MonsterFieldAwareness bits
 * 0x10 passes closed doors, 0x14 crosses the special wall range, 0x1A crosses water-type walls (types 0-1), 0x8 crosses floor type
 * 0x25 [Chapter 2]; Chapter 3: bit 2 forbids ordinary terrain) enter `target`? Cells with flags 0xC00 (another monster/object) always
 * block. *hopsTwo is set when the monster passes a door or the special wall range: the step then covers two cells.
 */
bool monsterStepBlocked(GameKind game, const DungeonGridCell *target, uint16_t traits, bool *hopsTwo);

MonsterMoveOutcome monsterWalkTowardParty(uint8_t *record, GameKind game, DungeonGrid *grid, int partyWorldX, int partyWorldY);

MonsterTurnOutcome monsterPoolProcessSlot(uint8_t *record, GameKind game, const WorldMap *map, DungeonGrid *grid,
                                           uint8_t *globalFlags, size_t globalFlagsSize,
                                           MonsterRewardStaging *staging, int partyWorldX, int partyWorldY,
                                           RandomState *rng);

/*
 * The whole per-monster turn of ProcessLevelMonsters in order: not aware -> nothing; the timer (expired: rewards + removal; still
 * running: the turn ends); the ambush check when the approach gate is nonzero and the monster is not busy, which ends the turn
 * when it triggers; then the walk toward the party. `turn` is monsterPoolProcessSlot's classification (Skipped also covers an idle
 * monster that only walked), `move` the walk's outcome.
 */
typedef struct {
    MonsterTurnOutcome turn;
    MonsterMoveOutcome move;
} MonsterFullTurn;

MonsterFullTurn monsterPoolTakeTurn(uint8_t *record, GameKind game, const WorldMap *map, DungeonGrid *grid, uint8_t *globalFlags,
                                    size_t globalFlagsSize, MonsterRewardStaging *staging, int partyWorldX, int partyWorldY, RandomState *rng);

#endif

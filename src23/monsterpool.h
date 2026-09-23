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
 * Its own catalog-copy, positioning and animation-start steps already
 * matched monster.h's monsterRecordSpawn/monsterRecordPlace/
 * monsterRecordStartAnimation closely enough that this module's own
 * spawn function is mostly composition of those, plus the one
 * genuinely new piece: a facing-dependent position-offset table (see
 * monsterSpawnOffsetTable). Not reimplemented: TryActivateMonsterByDistance
 * (awareness-on-spawn, not traced) and the caller-side decision of
 * *which* type id / viewport position to spawn at
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
 *   - monsterSpawnFlagSet(typeId) if save is non-NULL.
 * Returns the slot index spawned into, or -1 if the pool has no empty
 * slot or typeId is unknown to catalog (nothing is changed in that case).
 * viewportIndex must be < MonsterSpawnOffsetCount.
 */
int monsterPoolSpawn(uint8_t *pool, const MonsterCatalog *catalog, SaveGame *save, GameKind game, uint16_t facing,
                      uint16_t partyWorldX, uint16_t partyWorldY, uint16_t gridOriginRow, uint16_t gridOriginCol,
                      unsigned viewportIndex, unsigned typeId, RandomState *rng);

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
 * position is never changed by this check -- it only ever "notices" the
 * party from where it already stands, never actually steps closer.
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

#endif

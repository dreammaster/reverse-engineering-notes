#ifndef YENDOR23_MONSTERPOOL_H
#define YENDOR23_MONSTERPOOL_H

#include <stdbool.h>
#include <stdint.h>

#include "dungeongrid.h"
#include "game.h"
#include "monster.h"
#include "random.h"
#include "savegame.h"

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

#endif

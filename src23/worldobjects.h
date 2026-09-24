#ifndef YENDOR23_WORLDOBJECTS_H
#define YENDOR23_WORLDOBJECTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * A previously-undocumented WORLD.DAT block: a sparse per-cell "world
 * object" index -- doors/locks, searchable/found-item triggers, and
 * scripted monster-spawn markers, all keyed by exact (worldCol, worldRow).
 * This is the actual source of the door/lock flag movement.h's
 * MovementCellOutcome/movementClassifyCell API takes as a plain isDoor
 * bool, and (per RefreshDungeonMapWindow, see dungeongrid.h) what
 * TryInteractAtPosition consults while baking the in-memory dungeon
 * grid's per-cell flags.
 *
 * Found via the same "resource block setup" stub family as the master
 * palette/CURGAME-section stubs (extract_resource_stubs.py in both
 * games' ida_scripts/): PreloadWorldDataTable (yendor2.asm:3661,
 * yendor3.asm:27171) allocates a fixed 0x6768-byte (26,472) buffer and
 * reads it from WORLD.DAT in one shot -- confirmed offset 0x1A1141
 * (Chapter 2) / 0x41090D (Chapter 3), same size both games.
 *
 * In-memory/on-disk shape (traced from FindObjectAtPosition,
 * yendor2.asm:30970, and cross-checked against both real WORLD.DAT
 * files -- every column's list is sorted ascending by Y with zero bad
 * terminations in both games' real data): a 720-entry array of 16-bit
 * byte offsets (one per playable world X column, WorldObjectColumnMin
 * = 0x28 through 0x2F7), each pointing -- relative to the blob's own
 * start, not the file -- to that column's list of 6-byte records,
 * terminated by Y = 0xFFFF:
 *   +0 Y (u16, world row -- the list is sorted ascending by this field
 *      and the original's scan relies on that, stopping early once a
 *      list entry's Y exceeds the query)
 *   +2 flags (u16) -- see WorldObjectFlag
 *   +4 value (u16) -- meaning depends on which flag bit is set
 *
 * TryInteractAtPosition's bounds check (colMin/colMax/rowMin/rowMax) is
 * exactly movement.h's MovementBounds -- reused directly here via
 * worldObjectFind, rather than duplicated.
 */

enum {
    WorldObjectTableSize = 0x6768, /* 26,472 bytes: fixed both games */
    WorldObjectColumns = 720,      /* one entry per playable world X column */
    WorldObjectColumnMin = 0x28    /* worldCol - WorldObjectColumnMin = column-table index */
};

typedef struct {
    uint32_t offset; /* WORLD.DAT byte offset of this block */
} WorldObjectTableLayout;

const WorldObjectTableLayout *worldObjectTableLayout(GameKind game);

typedef struct {
    GameKind game;
    uint8_t data[WorldObjectTableSize];
} WorldObjectTable;

/* Parses WorldObjectTableSize bytes starting at region[0]; false if size is too small. */
bool worldObjectTableParse(WorldObjectTable *table, GameKind game, const uint8_t *region, size_t size);

/* Same, from a whole WORLD.DAT image already in memory. */
bool worldObjectTableParseWorldDat(WorldObjectTable *table, GameKind game, const uint8_t *worldDat, size_t size);

/*
 * Flag bits (TryInteractAtPosition, yendor2.asm:30813, yendor3's
 * equivalent instruction-identical): tested in this exact priority
 * order -- a record with more than one bit set only reaches the
 * highest-priority branch below. Real data never showed more than one
 * of these bits set on the same record in either game.
 */
typedef enum {
    /* value -> LoadLockState(value); a lock/door id. Always blocks movement (see movement.h's isDoor input). */
    WorldObjectFlagDoor = 0x8000,
    /* value -> LoadCurgameRecord(value), an index into CURGAME's SaveSectionEventState (both as the shared
       "already triggered" bitmap and, separately, an EMS-backed 4-byte record whose own format isn't
       resolved -- see lockcatalog.h's "LoadCurgameRecord" note). Outcome classified by interact.h. */
    WorldObjectFlagCurgameRecord = 0x4000,
    /* TryInteractAtPosition always reports a fixed errorCode=4 for this bit and never reads value for it. */
    WorldObjectFlagFixedResponse = 0x1000,
    /* value is a monster TYPE ID (see monsterpool.h's monsterSpawnFlagTest/Set/Clear -- confirmed by
       cross-referencing the despawn path, which clears the same CURGAME bitmap using a live monster's own
       type id, the same number space) -- a scripted/pre-placed monster-encounter marker. The majority flag
       bit in both games' real data. */
    WorldObjectFlagMonsterSpawn = 0x800,
    /*
     * A real, common bit in both games' data (187 Chapter 2 records, 139
     * Chapter 3) that TryInteractAtPosition never tests -- every check
     * above it falls through to "nothing here" for a 0x2000-only record.
     * ProbeFacingTile (yendor2.asm:30915) reaches the same underlying
     * FindObjectAtPosition record, but its callers weren't traced far
     * enough to confirm whether any of them read this bit. Not
     * interpreted by this module beyond exposing the raw flags word.
     */
    WorldObjectFlagUnknown2000 = 0x2000
} WorldObjectFlag;

typedef struct {
    uint16_t y; /* == the queried worldRow, on a successful find */
    uint16_t flags;
    uint16_t value;
} WorldObjectRecord;

/*
 * FindObjectAtPosition's exact lookup: bounds-checks (worldCol, worldRow)
 * against movementBounds(game) first (matching the original's own check),
 * then scans that column's sorted list for an exact Y match. Returns
 * false (out is untouched) if out of bounds or no record's Y matches.
 */
bool worldObjectFind(const WorldObjectTable *table, GameKind game, int worldCol, int worldRow, WorldObjectRecord *out);

#endif

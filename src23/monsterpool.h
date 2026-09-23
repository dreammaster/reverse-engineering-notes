#ifndef YENDOR23_MONSTERPOOL_H
#define YENDOR23_MONSTERPOOL_H

#include <stdbool.h>
#include <stdint.h>

#include "dungeongrid.h"
#include "game.h"
#include "monster.h"
#include "savegame.h"

/*
 * The 80-slot live monster pool (g_levelMonsters, monster.h's
 * MonsterPoolSize/MonsterRecordSize) as it's kept in sync with the
 * scrolling dungeon-grid window -- the part of RefreshDungeonMapWindow
 * (yendor2.asm:29496 on, instruction-identical in Chapter 3) not covered
 * by dungeongrid.c's own base-window build: after that build, the
 * original walks every pool slot and either relinks a still-visible
 * monster to its new grid-relative cell (and bakes a "monster here"
 * overlay into that DungeonGridCell) or despawns one that scrolled out
 * (zeroes its record and clears its spawn flag).
 *
 * Explicitly out of scope, same as worldobjects.c/dungeongrid.c already
 * note: how a new monster actually gets spawned in the first place
 * (SpawnMonsterInFacingDirection, not traced this pass) -- this module
 * only keeps *already-live* pool entries in sync with the window.
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

#endif

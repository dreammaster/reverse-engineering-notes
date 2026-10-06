#ifndef YENDOR23_WINDOWBAKE_H
#define YENDOR23_WINDOWBAKE_H

#include <stdbool.h>
#include <stdint.h>

#include "dungeongrid.h"
#include "game.h"
#include "interact.h"
#include "lockcatalog.h"
#include "savegame.h"
#include "worldobjects.h"

/*
 * The second pass of RefreshDungeonMapWindow (yendor2.asm:29286, the loop at :29437; Chapter 3 the same): after the base window is copied from the world map
 * (dungeongrid.h), every cell of the 78 x 78 window is run through TryInteractAtPosition (interact.h) and the outcome is baked into the cell:
 *
 *   outcome                                              what is written into the cell
 *   10 curgame flag 0x40, 1 curgame fallback A           flags |= 0x4000
 *   2  curgame fallback B (flag 0x20)                    flags |= 0x2000
 *   3  magical lock                                      flags |= 0x1000
 *   6  curgame flag 0x10                                 floor type (+2) = the curgame record's packed value
 *   7  curgame flag 0x8                                  wall type (+0) = the curgame record's packed value
 *   5  a monster marker not yet spawned                  flags |= 0x400 and +4 = the marker's value (the monster type id)
 *   0, 4, 8, 9                                           nothing
 *
 * This is where the cell flags that movement.h's isDoor input (0x6000) and the monster spawn check (0x400, see monsterpool.h) come from; the base copy never
 * sets them. Doors (WorldObjectFlagDoor) themselves bake nothing (3 is the only door outcome that does); a closed door is the wall type itself.
 */
typedef struct {
    uint16_t flagsOr;     /* bits for the cell's +6 */
    bool setWall, setFloor;
    uint16_t newType;     /* the value for the wall (+0) or floor (+2) type */
    bool monsterMarker;   /* the cell's +4 = monsterType, flags |= 0x400 */
    uint16_t monsterType;
} WindowBake;

/* What the outcome writes; `curgameValue` is the curgame record's packed value (outcomes 6 and 7), `object` the cell's record (outcome 5). */
WindowBake windowBakeForOutcome(InteractOutcome outcome, const WorldObjectRecord *object, uint16_t curgameValue);

void windowBakeApply(DungeonGridCell *cell, const WindowBake *bake);

/*
 * Runs the pass over the whole window: for each cell with a world object (worldobjects.h) it loads the lock or curgame record that object names, tests the
 * shared "already unlocked / triggered" bitmap and the monster-spawned flags in `save`, classifies, and applies the result. `save` must not be NULL.
 * Returns how many cells were changed.
 */
unsigned dungeonGridBakeMarkers(DungeonGrid *grid, GameKind game, const WorldObjectTable *objects, const LockCatalog *locks, SaveGame *save);

#endif

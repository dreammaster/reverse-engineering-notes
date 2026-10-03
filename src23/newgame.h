#ifndef YENDOR23_NEWGAME_H
#define YENDOR23_NEWGAME_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "savegame.h"

/*
 * InitializeNewGameWorldState (yendor2.asm:49634, yendor3.asm:37448) -- the title screen's "new game".
 *
 * The starting state is a 5000-byte template, the size of CURGAME section 0 (the 500-byte game-state block plus
 * the nine 500-byte party records), stored at the very end of Chapter 2's WORLD.DAT (offset 0x1ACCED) and at
 * 0x41D72F in Chapter 3's (ida_scripts/dump_newgame_block.py). It holds the opening position, calendar and
 * clock, and the four ready-made heroes in roster slots 6-9 (Chapter 2: SQUIRE, DIANA, YENDOR, JOSEPHINE; the
 * header is named "SMITHWARE PARTY", Chapter 3's "PRE-CREATED PARTY"); roster slots 1-5 are empty. (Chapter 3's
 * template also lists the four heroes as the active party in its party-slot table; the initializer clears it.)
 *
 * saveGameNewGame: copies the template into section 0, empties the four active party slots, clears bit 0x0800 of
 * each party record's UI-flags word (+0x15C), re-asserts the opening values the original writes after the
 * template read -- they equal the template's:
 *   Chapter 2  facing West, position (166, 36), 4 November 546, 07:00 (420 minutes), animation speed 5
 *   Chapter 3  facing North, position (460, 46), 20 March 547, 09:00 (540 minutes); and clears header word +0x1B0
 * -- and leaves every other section zero (the original writes zeros over the explored map, event state, lock
 * and shop state, spawn flags and the first item instances; a fresh SaveGame is already all zero).
 * Returns false if the WORLD.DAT image is too short.
 */
enum {
    NewGameTemplateOffsetYendor2 = 0x1ACCED,
    NewGameTemplateOffsetYendor3 = 0x41D72F,
    NewGameTemplateSize = 5000
};

bool saveGameNewGame(SaveGame *save, GameKind game, const uint8_t *worldDat, size_t size);

#endif

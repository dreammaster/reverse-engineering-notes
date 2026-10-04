#ifndef YENDOR23_LOCATION_H
#define YENDOR23_LOCATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * The names of the world's map blocks (BuildClueLocationSuffix, yendor2.asm:31620; the local area map header and the clue book's map list).
 * The world is cut into blocks of 40 x 24 cells (localmap.h), 120 in Chapter 2 and 140 in Chapter 3, numbered along rows. WORLD.DAT
 * holds, straight after the world map (144 / 168 rows of 3200 bytes), a table of 6-byte block records (Chapter 2 at 460800, Chapter 3 at
 * 537600) and then a table of 20-byte space padded names (Chapter 2 at 461520, Chapter 3 at 538440; name 0 is unused). A block record is
 *   +0..+3  four characters: a level number and a map number, see below
 *   +4      the name index (1-based) into the name table
 *   +5      a second byte, not used here
 * The suffix after the name is " LEVEL x" when character 0 is not '0' (Chapter 2: that one character; Chapter 3: characters 0-2), else
 * " MAP x" when character 1 is not a space (Chapter 2: character 1; Chapter 3: characters 1-3), else none. The suffix texts are templates read
 * from the executable (" LEVEL X", " MAP X"; Chapter 3 " LEVEL XXX", " MAP XXX") with the placeholder characters overwritten.
 * Drawn after the name, 6 pixels further on, in colour 0x5B for a map and 0xAA for a level.
 */
typedef struct {
    char name[24];
    char suffix[16];
    int kind; /* 0 none, 1 map suffix, 2 level suffix */
} LocationName;

/* False if the block number or the file is out of range. */
bool locationName(GameKind game, const uint8_t *worldDat, size_t size, unsigned block, const char *levelTemplate, const char *mapTemplate, LocationName *out);

#endif

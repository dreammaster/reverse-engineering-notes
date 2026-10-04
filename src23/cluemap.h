#ifndef YENDOR23_CLUEMAP_H
#define YENDOR23_CLUEMAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"
#include "localmap.h"
#include "location.h"
#include "viewrender.h"
#include "worldmap.h"

/*
 * The clue book's MAPS page (F1; RunClueBookMapCategory yendor2.asm:5287, LoadClueBookMapEntry :6689, DrawClueBookMapGrid :6532,
 * DrawClueBookMapLocationMarker :5430, DrawClueBookMapCategoryHeader :6655). A map is one 40 x 24 block of the world (localmap.h; map id n is block
 * n - 1) drawn full screen like the local area map -- the first line holds the block's name and suffix (location.h) in 0xD at (0, 1) and the hint
 * "SELECT LEGEND OR ESC" in 0x59 at x = 201 -- but its tiles come from a "known" bitmap in WORLD.DAT instead of the party's fog of war: one bit per
 * world cell, MSB first, 100 bytes per world row (Chapter 2 at 1396760, Chapter 3 at 3952386), so the book shows the whole map. Over it go the
 * block's legend markers: WORLD.DAT records of 4 u16 (map id, world x, world y, label number) in a table sorted by map id (Chapter 2: 405 records
 * at 1450685, Chapter 3: 250 at 4011261); a marker is category 9 picture 0x73 at (8 * (x mod 40), 8 + 8 * (y mod 24)) and is clickable (an 8 x 8
 * region). Clicking one shows its label -- 26-byte records at 1453925 / 4013261, indexed by the label number -- as picture 0x73 at (161, 0) and the
 * text at (170, 1) in 0x7B on 0. (Labels read like "BLACKWING MINE" and "REQUIRES 70 WISDOM".)
 */
enum { ClueMapMarkersMax = 64, ClueMapLabelSize = 26 };

typedef struct {
    unsigned x, y;           /* world cell */
    unsigned label;
    int screenX, screenY;    /* where the marker sits on the page */
} ClueMapMarker;

/* The markers of map `mapId` (1-based); returns how many were stored (at most `max`). */
unsigned clueMapMarkers(GameKind game, const uint8_t *worldDat, size_t size, unsigned mapId, ClueMapMarker *out, unsigned max);

/* The label text of a marker; false if out of range. */
bool clueMapLabel(GameKind game, const uint8_t *worldDat, size_t size, unsigned label, char out[ClueMapLabelSize]);

/* The page's 40 x 24 cells: world types from `map`, explored = the known bitmap's bit. */
void clueMapFill(LocalMapCell cells[LocalMapColumns * LocalMapRows], GameKind game, const WorldMap *map, const uint8_t *worldDat, size_t size, unsigned mapId);

/* The page: tiles, header (name + suffix and `hint`) and markers. */
void clueMapPageDraw(const ViewRenderer *r, const LocalMapCell cells[LocalMapColumns * LocalMapRows], const LocationName *name, const char *hint,
                     const ClueMapMarker *markers, unsigned markerCount);

/* The label shown after a marker was clicked. */
void clueMapLabelDraw(const ViewRenderer *r, const char *label);

/* Which marker (index) a click at (x, y) hits, or -1. */
int clueMapMarkerAt(const ClueMapMarker *markers, unsigned markerCount, int x, int y);

#endif

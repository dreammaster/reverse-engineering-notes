#ifndef YENDOR23_WORLDMAP_STDIO_H
#define YENDOR23_WORLDMAP_STDIO_H

#include "worldmap.h"

/* Reads just the map region out of a WORLD.DAT file; for tools and tests. */
bool worldMapReadWorldDatFile(WorldMap *map, GameKind game, const char *path);

#endif

#ifndef YENDOR23_ITEM_STDIO_H
#define YENDOR23_ITEM_STDIO_H

#include "item.h"

/* Reads just the catalog region out of a WORLD.DAT file; for tools and tests. */
bool itemCatalogReadWorldDatFile(ItemCatalog *catalog, GameKind game, const char *path);

#endif

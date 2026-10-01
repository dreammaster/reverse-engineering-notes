#ifndef YENDOR23_SPELLRECORD_STDIO_H
#define YENDOR23_SPELLRECORD_STDIO_H

#include "spellrecord.h"

/* Reads just the spell/ability catalog region out of a WORLD.DAT file; for tools and tests. */
bool spellCatalogReadWorldDatFile(SpellCatalog *catalog, GameKind game, const char *path);

#endif

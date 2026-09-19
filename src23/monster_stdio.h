#ifndef YENDOR23_MONSTER_STDIO_H
#define YENDOR23_MONSTER_STDIO_H

#include "monster.h"

/* Reads just the monster catalog region out of a WORLD.DAT file; for tools and tests. */
bool monsterCatalogReadWorldDatFile(MonsterCatalog *catalog, GameKind game, const char *path);

#endif

// The upgrade of the data of a project that was saved by an editor of an older version (the fixes of
// TVisionaireGame::UpdateVersion(), asm 1508677-1523438).
#pragma once

#include "vstables/visionaireGame.h"

/** The fixes of the versions 0x63-0xAC: all that are numbered `version` or more, oldest first. */
void applyVersionFixes(TVisionaireGame &game, int version);

/** The fixes of the versions 0xB3-0xB8 that come after the ones every version gets. */
void applyLateVersionFixes(TVisionaireGame &game, int version);

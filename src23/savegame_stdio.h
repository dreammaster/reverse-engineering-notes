#ifndef YENDOR23_SAVEGAME_STDIO_H
#define YENDOR23_SAVEGAME_STDIO_H

#include "savegame.h"

/* stdio wrappers for tools and tests; the engine proper should use savegame.h alone. */
bool saveGameReadFile(SaveGame *save, const char *path);
bool saveGameWriteFile(const SaveGame *save, const char *path);

#endif

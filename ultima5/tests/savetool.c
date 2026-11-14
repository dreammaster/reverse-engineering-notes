// Reads SAVEGAME/SAVED.GAM through the engine's loader and writes it back (SAVEGAME/OUT.GAM would be nicer, but the
// writer only knows the real names): used by tools/save_roundtrip.py to check the loader/writer are exact inverses.
#include "common/common.h"
#include "vars.h"
#include "savegame.h"

#include <stdio.h>

int main(void)
{
    if (FILE_ReadSavegameFile("SAVED.GAM") != 0)
    {
        printf("read failed\n");
        return 1;
    }
    FILE_WriteSavegameFile("SAVED.GAM");
    return 0;
}

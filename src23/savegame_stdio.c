#include "savegame_stdio.h"

#include <stdio.h>

bool saveGameReadFile(SaveGame *save, const char *path) {
    FILE *file = fopen(path, "rb");
    if (!file) {
        return false;
    }
    static uint8_t buffer[SaveFileSizeMax + 1];
    size_t size = fread(buffer, 1, sizeof(buffer), file);
    bool tooLarge = !feof(file);
    fclose(file);
    return !tooLarge && saveGameLoad(save, buffer, size);
}

bool saveGameWriteFile(const SaveGame *save, const char *path) {
    static uint8_t buffer[SaveFileSizeMax];
    size_t size = saveGameStore(save, buffer, sizeof(buffer));
    if (size == 0) {
        return false;
    }
    FILE *file = fopen(path, "wb");
    if (!file) {
        return false;
    }
    bool ok = fwrite(buffer, 1, size, file) == size;
    return fclose(file) == 0 && ok;
}

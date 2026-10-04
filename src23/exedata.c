#include "exedata.h"

#include <string.h>

size_t exeDataFileOffset(GameKind game) {
    return game == GameYendor2 ? 0x21660 : 0x21DB0;
}

bool exeDataOpen(ExeData *exe, GameKind game, const uint8_t *data, size_t size) {
    static const char kCheck[] = "QUIT \"CREATE\"";
    size_t base = exeDataFileOffset(game);
    size_t checkOffset = base + (game == GameYendor2 ? 0x79E5 : 0x7D17);
    if (size < checkOffset + sizeof(kCheck) || memcmp(data + checkOffset, kCheck, sizeof(kCheck)) != 0) {
        return false;
    }
    exe->data = data;
    exe->size = size;
    exe->base = base;
    return true;
}

bool exeDataString(const ExeData *exe, unsigned dsOffset, char *out, size_t capacity) {
    size_t at = exe->base + dsOffset;
    if (capacity == 0 || at >= exe->size) {
        return false;
    }
    size_t n = 0;
    while (at + n < exe->size && exe->data[at + n] != 0 && n + 1 < capacity) {
        out[n] = (char)exe->data[at + n];
        n++;
    }
    out[n] = 0;
    return true;
}

unsigned exeDataU8(const ExeData *exe, unsigned dsOffset) {
    size_t at = exe->base + dsOffset;
    return at < exe->size ? exe->data[at] : 0;
}

unsigned exeDataU16(const ExeData *exe, unsigned dsOffset) {
    return exeDataU8(exe, dsOffset) | (exeDataU8(exe, dsOffset + 1) << 8);
}

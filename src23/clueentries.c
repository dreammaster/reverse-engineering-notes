#include "clueentries.h"

#include <string.h>

typedef struct {
    unsigned table, classNames, itemGroups;
} Addresses;

static const Addresses kYendor2 = {0xF3F4, 0x8F04, 0x8A01};
static const Addresses kYendor3 = {0xF6A8, 0x9223, 0x8D22};

unsigned clueEntries(const ExeData *exe, GameKind game, unsigned category, ClueEntry out[ClueEntriesMax]) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    if (category < 1 || category > 17) {
        return 0;
    }
    unsigned list = exeDataU16(exe, a->table + 2 * (category - 1));
    unsigned count = exeDataU16(exe, list);
    if (!list || count > ClueEntriesMax) {
        return 0;
    }
    for (unsigned i = 0; i < count; i++) {
        out[i].id = (uint16_t)exeDataU16(exe, list + 2 + 4 * i);
        out[i].free = (exeDataU16(exe, list + 4 + 4 * i) & 0x8000) != 0;
    }
    return count;
}

bool clueEntryPackedString(const ExeData *exe, GameKind game, unsigned category, unsigned index, char out[ClueEntryTextSize]) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    if ((category != 4 && category != 11) || index < 1) {
        return false;
    }
    unsigned at = category == 4 ? a->classNames : a->itemGroups;
    for (unsigned i = 1; i < index; i++) {
        char skip[ClueEntryTextSize];
        if (!exeDataString(exe, at, skip, sizeof(skip))) {
            return false;
        }
        at += (unsigned)strlen(skip) + 1;
    }
    return exeDataString(exe, at, out, ClueEntryTextSize);
}

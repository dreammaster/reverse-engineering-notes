#include "explore.h"

static bool locate(SaveGame *save, int x, int y, uint8_t **byte, uint8_t *mask) {
    if (x < 0 || y < 0) {
        return false;
    }
    const SaveLayout *layout = saveLayoutFor(save->kind);
    unsigned index = (unsigned)x / 8;
    if (index >= layout->sections[SaveSectionExploredMap].recordSize) {
        return false;
    }
    uint8_t *row = saveGameRecord(save, SaveSectionExploredMap, (unsigned)y);
    if (!row) {
        return false;
    }
    *byte = row + index;
    *mask = (uint8_t)(0x80u >> ((unsigned)x % 8));
    return true;
}

bool exploreIsExplored(SaveGame *save, int x, int y) {
    uint8_t *byte, mask;
    return locate(save, x, y, &byte, &mask) && (*byte & mask) != 0;
}

bool exploreMarkCell(SaveGame *save, int x, int y) {
    uint8_t *byte, mask;
    if (!locate(save, x, y, &byte, &mask) || (*byte & mask)) {
        return false;
    }
    *byte |= mask;
    return true;
}

static void markRow(SaveGame *save, int x, int y, bool alongX, ExploreReveal *newly) {
    static const int offsets[3] = {-1, 1, 0};
    for (unsigned i = 0; i < 3; i++) {
        int cx = alongX ? x + offsets[i] : x;
        int cy = alongX ? y : y + offsets[i];
        if (exploreMarkCell(save, cx, cy) && newly) {
            newly->cells[newly->count].x = cx;
            newly->cells[newly->count].y = cy;
            newly->count++;
        }
    }
}

void exploreRevealAroundPlayer(SaveGame *save, int x, int y, uint16_t facing, ExploreReveal *newly) {
    if (newly) {
        newly->count = 0;
    }
    if (facing == SaveFacingNorth) {
        markRow(save, x, y - 1, true, newly);
        markRow(save, x, y, true, newly);
    } else if (facing == SaveFacingSouth) {
        markRow(save, x, y + 1, true, newly);
        markRow(save, x, y, true, newly);
    } else if (facing == SaveFacingEast) {
        markRow(save, x + 1, y, false, newly);
        markRow(save, x, y, false, newly);
    } else {
        markRow(save, x - 1, y, false, newly);
        markRow(save, x, y, false, newly);
    }
}

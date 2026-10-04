#include "intro2.h"

#include <string.h>

void introCellsInit(IntroCell cells[IntroCellCount]) {
    static const IntroCell kCells[IntroCellCount] = {
        {0xC080, 0x1A, 0x1F, 6, 0x48, 0x38, 0x5E, 0x2F, 0x1A}, {0xC080, 0x1A, 0x1F, 6, 0xF0, 0x38, 0x5E, 0x2F, 0x1C},
        {0xC080, 0x17, 0x19, 6, 0x44, 0x38, 0xED, 0x88, 0x19}, {0xC080, 0x17, 0x19, 6, 0xEC, 0x38, 0xED, 0x88, 0x17},
        {0x8040, 0x0A, 0x0F, 2, 0x69, 0x8C, 0xF2, 0x97, 0x0A}, {0x0040, 0x10, 0x13, 2, 0xB4, 0x8C, 0x30, 0x9B, 0x10},
        {0x0040, 0x49, 0x4E, 1, 0x56, 0xD2, 0x2E, 0x69, 0x49}};
    memcpy(cells, kCells, sizeof(kCells));
}

static void cellStep(IntroCell *cell) {
    bool up = (cell->flags & (IntroCellLoop | IntroCellUp)) != 0;
    int next = (int)cell->picture + (up ? 1 : -1);
    if (up ? next <= (int)cell->lastPicture : next >= (int)cell->firstPicture) {
        cell->picture = (uint16_t)next;
    } else if (cell->flags & (IntroCellUp | IntroCellOnce)) {
        cell->flags &= 0xBF9F;
    } else if (cell->flags & IntroCellLoop) {
        cell->picture = cell->firstPicture;
    }
}

unsigned introCellsFrame(IntroCell cells[IntroCellCount], int scrollX, int scrollY, bool tick, IntroCellDraw draws[IntroCellCount]) {
    unsigned count = 0;
    for (unsigned i = 0; i < IntroCellCount; i++) {
        IntroCell *cell = &cells[i];
        if (!(cell->flags & IntroCellOn)) {
            continue;
        }
        int x = (int)cell->x - scrollX;
        int y = (int)cell->y - scrollY;
        if (y > 199) {
            continue;
        }
        if ((int)cell->height + y <= 0) {
            cell->flags &= 0x7FFF;
            continue;
        }
        IntroCellDraw *draw = &draws[count++];
        draw->category = cell->category;
        draw->picture = cell->picture;
        draw->x = x;
        draw->y = y;
        draw->firstRow = 0;
        draw->rows = cell->height;
        draw->width = cell->width;
        if (y < 0) {
            draw->y = 0;
            draw->firstRow = (unsigned)(scrollY - (int)cell->y);
        } else if ((int)cell->height + y > 199) {
            draw->rows = (unsigned)(200 - y);
        }
        if ((cell->flags & IntroCellAnimating) && tick) {
            cellStep(cell);
        }
    }
    return count;
}

const IntroCard *introCards(void) {
    static const IntroCard kCards[IntroCardCount] = {
        {0xBF89, 5, 0x0A, 0x08, IntroCardNoVoice}, {0xC063, 2, 0x0A, 0xAE, IntroCardNoVoice}, {0xC0E5, 2, 0x0A, 0x08, 0x10},
        {0xC130, 1, 0x1C, 0x08, 0x13},             {0xC15D, 3, 0x0A, 0x08, 0x17},             {0xC1C9, 2, 0x1C, 0x08, 0x14},
        {0xC201, 4, 0x0A, 0x08, 0x15},             {0xC0AE, 2, 0x22, 0xAE, IntroCardNoVoice}, {0xC2A1, 2, 0x22, 0x08, 0x1C}};
    return kCards;
}

bool introCardLine(const ExeData *exe, const IntroCard *card, unsigned index, char out[IntroCardLineMax]) {
    if (index >= card->lines) {
        return false;
    }
    unsigned offset = card->textOffset;
    for (unsigned skipped = 0; skipped < index; skipped++) {
        while (exeDataU8(exe, offset) != 0) {
            offset++;
        }
        offset++;
    }
    return exeDataString(exe, offset, out, IntroCardLineMax);
}

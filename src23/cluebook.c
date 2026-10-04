#include "cluebook.h"

#include "font.h"

unsigned clueTabPicture(GameKind game, unsigned tab, uint16_t flags) {
    static const unsigned kYendor2[ClueTabCount] = {0x20, 0x145, 0x147, 0x153, 0x149, 0x14B, 0x14D};
    static const unsigned kYendor3[ClueTabCount] = {0x1E, 0x20, 0x22, 0x24, 0x26, 0x28, 0x2A};
    unsigned base = game == GameYendor3 ? kYendor3[tab] : kYendor2[tab];
    return base + ((flags & (ClueTabFirstBit >> tab)) ? 1 : 0);
}

void clueNavBarDraw(const ViewRenderer *r, uint16_t flags) {
    if (flags & ClueNavHintList) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 11, 185, "d LIST", 0xF, 0, FontTransparent);
    }
    if (flags & ClueNavHintMap) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 272, 185, "MAP c", 0xF, 0, FontTransparent);
    }
    for (unsigned tab = 0; tab < ClueTabCount; tab++) {
        viewDrawPicture(r, 8, clueTabPicture(r->game, tab, flags), 62 + 30 * (int)tab, 180, false, 0);
    }
}

void clueEntryListDraw(const ViewRenderer *r, const ClueListRow *rows, unsigned count, int selected, bool revealAll) {
    for (unsigned i = 0; i < count && i < ClueListRows; i++) {
        if (!rows[i].name) {
            continue;
        }
        bool dim = !revealAll && !rows[i].known;
        uint8_t colour = (int)i == selected ? (dim ? 0x84 : 0x8A) : (dim ? 0x05 : 0x0A);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 40, 27 + 10 * (int)i, rows[i].name, colour, 0, FontTransparent);
    }
}

ClueTextSource clueCategorySource(unsigned category) {
    switch (category) {
    case 1:
        return ClueSourceLocation;
    case 2:
        return ClueSourceMonster;
    case 3:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
        return ClueSourceSpell;
    case 4:
        return ClueSourcePackedA;
    case 11:
        return ClueSourcePackedB;
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
        return ClueSourceItem;
    default:
        return ClueSourceNone;
    }
}

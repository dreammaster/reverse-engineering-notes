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

int clueHeadingX(GameKind game, unsigned category, unsigned length) {
    static const struct {
        uint8_t category;
        uint16_t x;
    } kYendor2[] = {{1, 291}, {2, 208}, {3, 213}, {4, 183}, {11, 225}, {12, 249}, {13, 141}, {14, 195}, {15, 273}, {16, 237}, {17, 273}};
    if (game == GameYendor2) {
        for (unsigned i = 0; i < sizeof(kYendor2) / sizeof(kYendor2[0]); i++) {
            if (kYendor2[i].category == category) {
                return kYendor2[i].x;
            }
        }
    }
    return 313 - 6 * (int)length;
}

static void clueListRecompute(ClueList *list) {
    list->last = list->first + ClueListRows - 1;
    if (list->last > list->count - 1) {
        list->last = list->count - 1;
    }
}

void clueListInit(ClueList *list, unsigned count) {
    list->count = count;
    list->first = list->selected = 0;
    list->last = 0;
    if (count) {
        clueListRecompute(list);
    }
}

bool clueListCanPageUp(const ClueList *list) {
    return list->count > ClueListRows && list->first != 0;
}

bool clueListCanPageDown(const ClueList *list) {
    return list->count > ClueListRows && list->last != list->count - 1;
}

static void clueListScrollUp(ClueList *list) {
    unsigned row = list->selected - list->first;
    list->first = list->first >= ClueListRows ? list->first - ClueListRows : 0;
    list->selected = list->first + row;
    clueListRecompute(list);
}

static void clueListScrollDown(ClueList *list) {
    unsigned row = list->selected - list->first;
    list->first += ClueListRows;
    list->selected = list->first + row;
    clueListRecompute(list);
    if (list->selected > list->last) {
        list->selected = list->last;
    }
}

unsigned clueListPageKey(ClueList *list, bool down) {
    if (list->count == 0) {
        return 0;
    }
    if (!down) {
        if (clueListCanPageUp(list)) {
            clueListScrollUp(list);
            return 1;
        }
        if (list->selected != list->first) {
            list->selected = list->first;
            return 1;
        }
        return 0;
    }
    if (clueListCanPageDown(list)) {
        clueListScrollDown(list);
        return 2;
    }
    if (list->selected != list->last) {
        list->selected = list->last;
        return 2;
    }
    return 0;
}

unsigned clueListRowKey(ClueList *list, bool down) {
    if (list->count == 0) {
        return 0;
    }
    if (!down) {
        if (list->selected != list->first) {
            list->selected--;
        } else if (clueListCanPageUp(list)) {
            clueListScrollUp(list);
            list->selected = list->last;
        } else {
            list->selected = list->first;
        }
        return 1;
    }
    if (list->selected != list->last) {
        list->selected++;
    } else if (clueListCanPageDown(list)) {
        clueListScrollDown(list);
        list->selected = list->first;
    } else {
        list->selected = list->last;
    }
    return 2;
}

int clueCategoryCommand(bool extended, uint8_t key, unsigned region, uint16_t *flags, bool *playSound) {
    int command = 0;
    *playSound = false;
    if (key != 0) {
        if (!extended && key == 0x1B) {
            command = 8;
        } else if (!extended && key == 'K') {
            return (*flags & ClueNavHintList) ? 1 : 0;
        } else if (!extended && key == 'P') {
            return (*flags & ClueNavHintMap) ? 9 : 0;
        } else if (!extended && key == 9) {
            *flags &= 0xFF9F;
            return ClueCommandHelp;
        } else if (extended && key >= 0x3B && key <= 0x40) {
            command = key - 0x3B + 2;
        } else {
            return 0;
        }
    } else if (region == 0) {
        return 0;
    } else if (region == 1) {
        return (*flags & ClueNavHintList) ? 1 : 0;
    } else if (region == 9) {
        return (*flags & ClueNavHintMap) ? 9 : 0;
    } else {
        command = (int)region;
    }
    if (command < 2 || command > 8) {
        return 0;
    }
    uint16_t bit = (uint16_t)(0x8000u >> (command - 2));
    if (*flags & bit) {
        return 0;
    }
    *flags = (uint16_t)((*flags & 0x1FF) | bit);
    *playSound = true;
    return command;
}

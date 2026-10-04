#include "cluepaged.h"

#include <stdio.h>
#include <string.h>

#include "cluebook.h"
#include "font.h"

typedef struct {
    unsigned title, heading, footer;
    size_t pages;
    unsigned count;
} Layout;

static const Layout kYendor2 = {0x883D, 0x8889, 0x8EEC, 1411160, 31};
static const Layout kYendor3 = {0x8B5D, 0x8BAA, 0x920B, 3969186, 33};

bool cluePagedTextLoad(CluePagedText *t, const ExeData *exe, GameKind game) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    return exeDataString(exe, l->title, t->title, sizeof(t->title)) && exeDataString(exe, l->heading, t->heading, sizeof(t->heading)) &&
           exeDataString(exe, l->footer, t->footer, sizeof(t->footer));
}

unsigned cluePageCount(GameKind game) {
    return game == GameYendor2 ? kYendor2.count : kYendor3.count;
}

const uint8_t *cluePagedPage(GameKind game, const uint8_t *worldDat, size_t size, unsigned page) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    if (page < 1 || page > l->count) {
        return NULL;
    }
    size_t at = l->pages + (size_t)(page - 1) * CluePageSize;
    return at + CluePageSize <= size ? worldDat + at : NULL;
}

uint16_t cluePagedNavFlags(GameKind game, unsigned page) {
    return (uint16_t)((page != 1 ? 0x100 : 0) | (page != cluePageCount(game) ? 0x80 : 0));
}

CluePagedResult cluePagedNavigate(GameKind game, unsigned *page, uint8_t key, unsigned region, bool registered) {
    uint16_t flags = cluePagedNavFlags(game, *page);
    bool previous = key == 'I' || (key == 0 && region == 1), next = key == 'Q' || (key == 0 && region == 2);
    if (previous && (flags & 0x100)) {
        (*page)--;
        return CluePagedPrevious;
    }
    if (next && (flags & 0x80)) {
        if (!registered && *page > CluePagedFreePages) {
            return CluePagedNag;
        }
        (*page)++;
        return CluePagedNext;
    }
    return CluePagedNone;
}

void cluePagedDraw(const ViewRenderer *r, const CluePagedText *t, const uint8_t *pageData, unsigned page, uint16_t navFlags) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, r->game == GameYendor2 ? 13 : 6, 1, 1, false, 0);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 6, 4, t->title, 0x0D, 0, FontTransparent);
    int headingX = r->game == GameYendor2 ? 188 : 313 - 6 * (int)strlen(t->heading);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, headingX, 4, t->heading, 0x0D, 0, FontTransparent);
    char footer[16];
    snprintf(footer, sizeof(footer), "%s", t->footer);
    if (page == 1 && footer[0]) {
        footer[0] = ' ';
    }
    if (page == cluePageCount(r->game) && strlen(footer) > 9) {
        footer[9] = ' ';
    }
    if (pageData) {
        for (unsigned i = 0; i < CluePagedLines; i++) {
            char line[CluePageLineSize + 1];
            memcpy(line, pageData + i * CluePageLineSize, CluePageLineSize);
            line[CluePageLineSize] = 0;
            fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 10, 23 + 6 * (int)i, line, 0x0D, 0, FontTransparent);
        }
    }
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 129, 167, footer, 0x0D, 0, FontTransparent);
    clueNavBarDraw(r, navFlags);
}

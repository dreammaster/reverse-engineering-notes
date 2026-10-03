#include "statuspanel.h"

#include "font.h"
#include "party.h"
#include "uiregions.h"

enum {
    FaceCategory = 7,
    IconCategory = 9,
    BackgroundColour = 6,
    EntriesPerPanel = 8
};

static unsigned field(const uint8_t *record, unsigned offset) {
    return (unsigned)record[offset] | ((unsigned)record[offset + 1] << 8);
}

unsigned statusPanelBarWidth(int current, unsigned maximum) {
    if (current <= 0) {
        return 0;
    }
    unsigned value = (unsigned)current > maximum ? maximum : (unsigned)current;
    uint32_t dividend = 100u * maximum;
    unsigned high = (unsigned)(dividend >> 16), low = dividend & 0xFFFF;
    if (high != 0 && high >= value) { /* the original's guard against a 16-bit quotient overflow */
        high = value;
        value++;
    }
    if (value == 0) {
        return 0; /* the original would divide by zero (a maximum of 0 with a positive current) */
    }
    unsigned quotient = (unsigned)((((uint32_t)high << 16) | low) / value) & 0xFFFF;
    unsigned width = quotient == 0 ? 1 : 3800u / quotient;
    return width == 0 ? 1 : width;
}

static void drawBar(const ViewRenderer *r, int x, int y, int current, unsigned maximum, uint8_t colour) {
    unsigned width = 0;
    if (current > 0) {
        if ((unsigned)current > maximum) {
            colour = (uint8_t)(colour + 2);
        }
        width = statusPanelBarWidth(current, maximum);
    }
    for (int row = 0; row < StatusPanelBarHeight; row++) {
        for (int col = 0; col < StatusPanelBarWidth; col++) {
            r->screen[(y + row) * ViewScreenWidth + x + col] = (unsigned)col < width ? colour : BackgroundColour;
        }
    }
}

void statusPanelDraw(const ViewRenderer *r, unsigned panel, const uint8_t *record) {
    unsigned count;
    const uint16_t (*rows)[5] = uiRegionEntries(r->game, UiRegionsPartyPanels, &count);
    if (!record || panel >= StatusPanelCount || count < (panel + 1) * EntriesPerPanel) {
        return;
    }
    const uint16_t(*e)[5] = rows + panel * EntriesPerPanel;
    bool ch3 = r->game == GameYendor3;
    unsigned status = field(record, PartyFieldStatusFlags);
    bool dead = (status & PartyStatusDead) != 0;

    viewDrawPicture(r, FaceCategory, field(record, PartyFieldPanelFace), e[0][0], e[0][2], false, 0);
    if ((status & 0x1C40) || (field(record, 0x15E) & 0x8000)) {
        viewDrawPicture(r, FaceCategory, ch3 ? 1 : 0xE0, e[0][0], e[0][2], true, 0);
    }

    int barX = e[7][0], barY = e[7][2];
    drawBar(r, barX, barY, dead ? 0 : (int16_t)field(record, 0x52), field(record, 0x92), 0x59);
    drawBar(r, barX, barY + 5, (int16_t)field(record, 0x54), field(record, 0x94), 0xCA);
    drawBar(r, barX, barY + 10, (int16_t)field(record, 0x118), field(record, 0x56), 0x86);

    unsigned blank = ch3 ? 0x10 : 0x15;
    unsigned ready = ch3 ? 0xF : 0x14;
    viewDrawPicture(r, IconCategory, field(record, 0xB4) ? ready : blank, e[6][0], e[6][2], false, 0);

    if (!dead && field(record, PartyFieldPendingLevel) != 0) {
        fontDrawChar(r->game, 0, r->screen, ViewScreenWidth, e[4][0] + 2, e[4][2] + 2, 'T', 0xF, 0, FontTransparent);
    } else {
        viewDrawPicture(r, IconCategory, blank, e[4][0], e[4][2], false, 0);
    }

    unsigned a = status & 0x2000 ? 7 : status & 0x4000 ? 6 : status & 0x8000 ? 5 : 4;
    unsigned b = status & 0x400 ? 10 : status & 0x800 ? 9 : status & 0x1000 ? 8 : 4;
    unsigned c = status & 0x80 ? 13 : status & 0x100 ? 12 : status & 0x200 ? 11 : 4;
    viewDrawPicture(r, IconCategory, a, e[1][0], e[1][2], false, 0);
    viewDrawPicture(r, IconCategory, b, e[1][0], e[2][2], false, 0);
    viewDrawPicture(r, IconCategory, c, e[1][0], e[3][2], false, 0);
    unsigned protection = 0;
    for (unsigned i = 0; i < PartyProtectionCount; i++) {
        protection += field(record, PartyFieldProtections + 2 * i);
    }
    viewDrawPicture(r, IconCategory, (protection & 0xFFFF) != 0 ? 0xE : blank, e[5][0], e[5][2], false, 0);

    if (dead) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, barX + 8, barY, "DEAD", 0xF, 0, FontTransparent);
    }
}

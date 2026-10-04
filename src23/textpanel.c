#include "textpanel.h"

#include <string.h>

#include "font.h"

enum { PanelColour = 4, HeaderColour = 0x8A };

static void fill(const ViewRenderer *r, int x, int y, int width, int height) {
    for (int row = 0; row < height; row++) {
        memset(&r->screen[(y + row) * ViewScreenWidth + x], PanelColour, (size_t)width);
    }
}

void textPanelClear(const ViewRenderer *r, bool combat) {
    if (combat) {
        fill(r, 240, 86, 73, 109);
    } else {
        fill(r, 240, 96, 72, 60);
        fill(r, 241, 87, 72, 6);
    }
}

void textPanelMessage(const ViewRenderer *r, const char *const *lines, unsigned count, uint8_t colour) {
    for (unsigned i = 0; i < count; i++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 96 + 6 * (int)i, lines[i], colour, 0, FontTransparent);
    }
}

void textPanelHeader(const ViewRenderer *r, const char *text) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 241, 87, text, HeaderColour, PanelColour, FontOpaque);
}

void textPanelGold(const ViewRenderer *r, const Bcd4 gold) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 241, 87, "$          ", HeaderColour, PanelColour, FontOpaque);
    char text[12];
    bcd4Format(gold, text);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 247, 87, text, HeaderColour, PanelColour, FontOpaque);
}

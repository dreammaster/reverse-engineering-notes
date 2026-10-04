#include "alchemylist.h"

#include <stdio.h>

#include "font.h"

static void number(const ViewRenderer *r, int x, int y, unsigned value, uint8_t colour) {
    char text[16];
    snprintf(text, sizeof(text), "%u", value);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, text, colour, 0, FontTransparent);
}

void alchemyListDraw(const ViewRenderer *r, const AlchemyRow *rows, unsigned count, int selected) {
    bool ch3 = r->game == GameYendor3;
    viewDrawPicture(r, 1, ch3 ? 3 : 4, 15, 23, true, 0);
    for (unsigned i = 0; i < count && i < AlchemyListRows; i++) {
        int y = 37 + 6 * (int)i;
        uint8_t colour = rows[i].colour;
        if ((int)i == selected) {
            colour = colour == 0xF ? 0x8A : 0x85;
        }
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 21, y, rows[i].name, colour, 0, FontTransparent);
        number(r, ch3 ? 170 : 150, y, rows[i].mp, colour);
        if (rows[i].nuore) {
            number(r, ch3 ? 203 : 179, y, rows[i].nuore, colour);
        }
        if (rows[i].ore && !ch3) {
            number(r, 202, y, rows[i].ore, colour);
        }
    }
}

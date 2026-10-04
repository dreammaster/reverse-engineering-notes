#include "alchemylist.h"

#include <stdio.h>

#include "font.h"
#include "textpanel.h"

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

void alchemyStatusPanelDraw(const ViewRenderer *r, const AlchemyPanelText *t, const char *name, int mp, int mpMax, const Bcd4 counter1, const Bcd4 counter2) {
    textPanelClear(r, false);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 241, 87, t->blank, 0x8A, 4, FontOpaque);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 241, 87, name, 0x8A, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 96, t->magic, mp > mpMax ? 0xCC : 0xCA, 0, FontTransparent);
    char fraction[32];
    snprintf(fraction, sizeof(fraction), "%d/%d", mp & 0xFFFF, mpMax & 0xFFFF);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 102, fraction, 0x0F, 0, FontTransparent);
    char text[12];
    if (t->label1 && counter1) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 114, t->label1, 0x8A, 0, FontTransparent);
        bcd4Format(counter1, text);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 120, text, 0x0F, 0, FontTransparent);
    }
    if (t->label2 && counter2) {
        int y = t->label1 ? 132 : 114;
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y, t->label2, 0x8A, 0, FontTransparent);
        bcd4Format(counter2, text);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, y + 6, text, 0x0F, 0, FontTransparent);
    }
}

#include "monsterpanel.h"

#include <stdio.h>

#include "font.h"

enum { IconCategory = 9, BlankIcon = 4, BackgroundColour = 6 };

static unsigned word(const uint8_t *record, unsigned offset) {
    return (unsigned)record[offset] | ((unsigned)record[offset + 1] << 8);
}

int monsterPanelY(unsigned panel) {
    return panel == 0 ? 0x57 : panel == 1 ? 0x7B : 0x9F;
}

unsigned monsterPanelBarWidth(unsigned health, unsigned maximum) {
    unsigned value = health == 0 ? 1 : health;
    if (value > maximum) {
        value = maximum;
    }
    if (value == 0) {
        return 0; /* the original would divide by zero */
    }
    unsigned quotient = (100u * maximum / value) & 0xFFFF;
    unsigned width = quotient == 0 ? 1 : 4500u / quotient;
    return width == 0 ? 1 : width;
}

static void text(const ViewRenderer *r, int x, int y, const char *string, uint8_t colour) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, string, colour, 0, FontTransparent);
}

void monsterPanelDraw(const ViewRenderer *r, unsigned panel, uint8_t *monster, bool active, unsigned revealTier) {
    if (panel >= MonsterPanelCount || word(monster, 0) == 0) {
        return;
    }
    bool ch3 = r->game == GameYendor3;
    unsigned state = word(monster, 0x0C);
    uint8_t colour = (state & 0x20) ? 0xAA : active ? 0x8A : 9;
    int x = MonsterPanelX, y = monsterPanelY(panel);
    char line[14];
    for (unsigned i = 0; i < 13; i++) {
        line[i] = (char)monster[0x32 + i];
    }
    line[13] = 0;
    text(r, x, y, line, colour);
    for (unsigned i = 0; i < 13; i++) {
        line[i] = (char)monster[0x3F + i];
    }
    text(r, x, y + 6, line, colour);
    y += 12;

    if (revealTier >= (ch3 ? 60u : 55u)) {
        unsigned health = word(monster, 0x10), maximum = word(monster, 0x50);
        uint8_t fill = 0x59;
        if (health == 0) {
            health = 1;
        }
        if (health > maximum) {
            health = maximum;
            fill = 0x5B;
        }
        unsigned width = monsterPanelBarWidth(health, maximum);
        for (int row = 0; row < MonsterPanelBarHeight; row++) {
            for (int col = 0; col < MonsterPanelBarWidth; col++) {
                r->screen[(y + row) * ViewScreenWidth + x + col] = (unsigned)col < width ? fill : BackgroundColour;
            }
        }
        if (revealTier >= 75) {
            unsigned a = (state & 0x4000) ? 7 : (state & 0x8000) ? 6 : BlankIcon;
            unsigned b = (state & 0x1000) ? 9 : (state & 0x2000) ? 8 : BlankIcon;
            unsigned c = (state & 0x400) ? 13 : (state & 0x800) ? 12 : BlankIcon;
            viewDrawPicture(r, IconCategory, a, x + 46, y, true, 0);
            viewDrawPicture(r, IconCategory, b, x + 55, y, true, 0);
            viewDrawPicture(r, IconCategory, c, x + 64, y, true, 0);
            if (revealTier >= 80) {
                y += 9;
                if (state & 0x200) {
                    if (state & 0x8000) {
                        text(r, x, y, "POISONED", colour);
                        y += 6;
                    }
                    if (state & 0x4000) {
                        text(r, x, y, "DISEASED", colour);
                    }
                } else if (state & 0x100) {
                    if (state & 0x2000) {
                        text(r, x, y, "PARALYZED", colour);
                        y += 6;
                    }
                    if (state & 0x1000) {
                        text(r, x, y, "FROZEN", colour);
                    }
                } else if (state & 0x80) {
                    if (state & 0x800) {
                        text(r, x, y, "HEXED", colour);
                        y += 6;
                    }
                    if (state & 0x400) {
                        text(r, x, y, "CURSED", colour);
                    }
                } else if (state & 0x40) {
                    text(r, x, y, "HEALTH:", colour);
                    y += 6;
                    char numbers[24];
                    snprintf(numbers, sizeof(numbers), "%u/%u", word(monster, 0x10), word(monster, 0x50));
                    text(r, x, y, numbers, colour);
                }
            }
        }
    }
    state &= 0xFC1F;
    monster[0x0C] = (uint8_t)state;
    monster[0x0D] = (uint8_t)(state >> 8);
}

#include "statsheet.h"

#include <stdio.h>

#include "bcd4.h"
#include "font.h"
#include "paperdoll.h"
#include "party.h"

static int field(const uint8_t *record, unsigned offset) {
    return (int16_t)((unsigned)record[offset] | ((unsigned)record[offset + 1] << 8));
}

static void number(const ViewRenderer *r, int x, int y, int value, int maximum, uint8_t normal, uint8_t high) {
    char text[16];
    snprintf(text, sizeof(text), "%d", value);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, text, value > maximum ? high : normal, 0, FontTransparent);
}

static void stat(const ViewRenderer *r, int x, int y, const uint8_t *record, unsigned offset) {
    number(r, x, y, field(record, offset), field(record, offset + 0x40), 0xF, 0x8A);
}

void statSheetDraw(const ViewRenderer *r, const uint8_t *record, unsigned characterId, const uint16_t roles[5]) {
    static const uint8_t kLeft[] = {0x3C, 0x3E, 0x40, 0x42, 0x44, 0x46, 0x4C, 0x4E, 0x50};
    static const int kLeftY[] = {60, 70, 80, 90, 100, 110, 130, 140, 150};
    for (unsigned i = 0; i < sizeof(kLeft); i++) {
        stat(r, 203, kLeftY[i], record, kLeft[i]);
    }
    stat(r, 191, 170, record, 0x52);
    stat(r, 191, 180, record, 0x54);
    char experience[12];
    bcd4Format(record + 0x18, experience);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 203, 190, experience, 0xF, 0, FontTransparent);

    for (unsigned i = 0; i < 8; i++) {
        stat(r, 297, 60 + 10 * (int)i, record, 0x58 + 2 * i);
    }
    unsigned roleCount = r->game == GameYendor3 ? 4 : 5;
    for (unsigned i = 0; i < roleCount; i++) {
        bool holder = roles[i] == characterId;
        number(r, 297, 140 + 10 * (int)i, field(record, 0x68 + 2 * i), field(record, 0x68 + 2 * i + 0x40), holder ? 0xCB : 0xF, holder ? 0x9B : 0x8A);
    }

    if (field(record, 0x16) == 1) {
        unsigned sum = 0;
        for (unsigned i = 0; i < 6; i++) {
            sum += (unsigned)field(record, 0x3C + 2 * i) & 0xFFFF;
        }
        unsigned average = (sum & 0xFFFF) / 6;
        const char *verdict = average <= 49 ? "POOR" : average <= 52 ? "AVERAGE" : average <= 55 ? "GOOD" : "GREAT";
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 254, 26, verdict, 0xDF, 0, FontTransparent);
    }
}

void characterSheetDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint8_t *record, unsigned characterId, const uint16_t roles[5],
                        const char *title) {
    viewDrawPicture(r, 0, 3, 1, 1, false, 0);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 107, 6, title, 0xF, 0, FontTransparent);
    paperDollDraw(r, catalog, record, 116, 60);
    viewDrawPicture(r, 7, (unsigned)field(record, 0x12), 116, 19, true, 0);
    statSheetDraw(r, record, characterId, roles);
    char name[PartyNameBufferSize];
    for (unsigned i = 0; i < PartyNameMaxLength; i++) {
        name[i] = (char)record[i];
    }
    name[PartyNameMaxLength] = 0;
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 156, 26, name, 0xF, 0, FontTransparent);
    const char *className = partyClassName((unsigned)field(record, 0x0E));
    if (className) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 156, 38, className, 0xF, 0, FontTransparent);
    }
    char level[8];
    snprintf(level, sizeof(level), "%d", field(record, 0x16));
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 256, 38, level, 0xF, 0, FontTransparent);
}

void detailSheetDraw(const ViewRenderer *r, const uint8_t *record, unsigned characterId, const uint16_t roles[5]) {
    viewDrawPicture(r, 1, 1, 15, 23, false, 0);
    const char *className = partyClassName((unsigned)field(record, 0x0E));
    if (className) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 20, 27, className, 0xF, 0, FontTransparent);
    }
    number(r, 122, 27, field(record, 0x16), field(record, 0x16), 0xF, 0xF);
    char experience[12];
    bcd4Format(record + 0x18, experience);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 160, 27, experience, 0xF, 0, FontTransparent);
    for (unsigned i = 0; i < 6; i++) {
        stat(r, 42, 37 + 10 * (int)i, record, 0x3C + 2 * i);
    }
    for (unsigned i = 0; i < 3; i++) {
        stat(r, 42, 99 + 10 * (int)i, record, 0x4C + 2 * i);
    }
    for (unsigned i = 0; i < 8; i++) {
        stat(r, 116, 43 + 10 * (int)i, record, 0x58 + 2 * i);
    }
    unsigned roleCount = r->game == GameYendor3 ? 4 : 5;
    for (unsigned i = 0; i < roleCount; i++) {
        bool holder = roles[i] == characterId;
        number(r, 203, 43 + 10 * (int)i, field(record, 0x68 + 2 * i), field(record, 0x68 + 2 * i + 0x40), holder ? 0xCB : 0xF, holder ? 0x9B : 0x8A);
    }
}

#include "cluetransport.h"

#include <stdio.h>
#include <string.h>

#include "cluebook.h"
#include "font.h"

typedef struct {
    unsigned records, value, uses, time, between, sevenPm, sevenAm, anytime, title;
} Addresses;

static const Addresses kYendor2 = {0x77C6, 0x7FBD, 0x8C21, 0x8C27, 0x8C2D, 0x8C3F, 0x8C45, 0x8C4B, 0x8A6A};
static const Addresses kYendor3 = {0x7AF4, 0x82EA, 0x8F3F, 0x8F45, 0x8F4B, 0x8F5D, 0x8F63, 0x8F69, 0x8D89};

bool clueTransportLoad(ClueTransportData *d, const ExeData *exe, GameKind game) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    for (unsigned i = 0; i < ClueMountCount; i++) {
        unsigned base = a->records + 26 * i;
        if (!exeDataString(exe, base, d->mounts[i].name, sizeof(d->mounts[i].name))) {
            return false;
        }
        for (unsigned k = 0; k < 4; k++) {
            d->mounts[i].price[k] = (uint8_t)exeDataU8(exe, base + 0x0E + k);
        }
        d->mounts[i].uses = exeDataU16(exe, base + 0x16);
        d->mounts[i].flags = exeDataU16(exe, base + 0x18);
    }
    return exeDataString(exe, a->value, d->value, sizeof(d->value)) && exeDataString(exe, a->uses, d->uses, sizeof(d->uses)) &&
           exeDataString(exe, a->time, d->time, sizeof(d->time)) && exeDataString(exe, a->between, d->between, sizeof(d->between)) &&
           exeDataString(exe, a->sevenPm, d->sevenPm, sizeof(d->sevenPm)) && exeDataString(exe, a->sevenAm, d->sevenAm, sizeof(d->sevenAm)) &&
           exeDataString(exe, a->anytime, d->anytime, sizeof(d->anytime)) && exeDataString(exe, a->title, d->title, sizeof(d->title));
}

static void put(const ViewRenderer *r, int x, int y, const char *s, uint8_t colour) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, s, colour, 0, FontTransparent);
}

void clueTransportPageDraw(const ViewRenderer *r, const ClueTransportData *d, uint16_t navFlags) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, r->game == GameYendor2 ? 13 : 6, 1, 1, false, 0);
    put(r, 6, 4, d->title, 0x0D);
    clueNavBarDraw(r, navFlags);
    static const unsigned kShown[3] = {0, 1, 3};
    static const int kY[3] = {26, 74, 122};
    for (unsigned n = 0; n < 3; n++) {
        const ClueMount *m = &d->mounts[kShown[n]];
        int y = kY[n];
        put(r, 91, y, m->name, 0x0D);
        y += 12;
        put(r, 91, y, d->value, 0x0A);
        char text[16];
        bcd4Format(m->price, text);
        put(r, 127, y, text, 0x8A);
        y += 9;
        put(r, 97, y, d->uses, 0x0A);
        snprintf(text, sizeof(text), "%u", m->uses);
        put(r, 127, y, text, 0x59);
        y += 9;
        put(r, 97, y, d->time, 0x0A);
        if (m->flags & 2) {
            put(r, 127, y, d->anytime, 0xCA);
        } else {
            put(r, 127, y, d->between, 0x0D);
            put(r, 175, y, d->sevenPm, 0xCA);
            put(r, 175, y + 9, d->sevenAm, 0xCA);
        }
    }
}

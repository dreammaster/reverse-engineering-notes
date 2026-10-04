#include "clueitem.h"

#include <stdio.h>
#include <string.h>

#include "bcd4.h"
#include "cluebook.h"
#include "font.h"

typedef struct {
    unsigned baseValue, weight, absorption, fitsIn, adds, characterPanel, anyPanel, backpack, box, bag;
    unsigned twoHanded, yes, no, skill, duration, minutes, health, magic, percent, damage, protections;
    unsigned protectionNames, statNames, skillTypes;
} TextAddresses;

static const TextAddresses kYendor2 = {0x8A82, 0x8A8E, 0x8A96, 0x8AA2, 0x8AAB, 0x8AB1, 0x8AC1, 0x8ACB, 0x8AD5, 0x8ADA, 0x8ADE, 0x8AE8,
                                       0x8AEC, 0x8AEF, 0x8B45, 0x8B4F, 0x8B57, 0x8B5F, 0x8B66, 0x7C81, 0x7B24, 0x7B31, 0x7DC7, 0x7E8A};
static const TextAddresses kYendor3 = {0x8DA1, 0x8DAD, 0x8DB5, 0x8DC1, 0x8DCA, 0x8DD0, 0x8DE0, 0x8DEA, 0x8DF4, 0x8DF9, 0x8DFD, 0x8E07,
                                       0x8E0B, 0x8E0E, 0x8E62, 0x8E6C, 0, /* no HEALTH- */ 0x8E7C, 0x8E83, 0x7FAE, 0x7E56, 0x7E63, 0x80F4, 0x81B7};

static bool load(const ExeData *exe, unsigned address, char *out, size_t capacity) {
    if (address == 0) {
        out[0] = 0;
        return true;
    }
    return exeDataString(exe, address, out, capacity);
}

static bool loadPacked(const ExeData *exe, unsigned address, char (*out)[16], unsigned count) {
    for (unsigned i = 0; i < count; i++) {
        if (!exeDataString(exe, address, out[i], 16)) {
            return false;
        }
        address += (unsigned)strlen(out[i]) + 1;
    }
    return true;
}

bool clueItemTextLoad(ClueItemText *t, const ExeData *exe, GameKind game) {
    const TextAddresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    return load(exe, a->baseValue, t->baseValue, sizeof(t->baseValue)) && load(exe, a->weight, t->weight, sizeof(t->weight)) &&
           load(exe, a->absorption, t->absorption, sizeof(t->absorption)) && load(exe, a->fitsIn, t->fitsIn, sizeof(t->fitsIn)) &&
           load(exe, a->adds, t->adds, sizeof(t->adds)) && load(exe, a->characterPanel, t->characterPanel, sizeof(t->characterPanel)) &&
           load(exe, a->anyPanel, t->anyPanel, sizeof(t->anyPanel)) && load(exe, a->backpack, t->backpack, sizeof(t->backpack)) &&
           load(exe, a->box, t->box, sizeof(t->box)) && load(exe, a->bag, t->bag, sizeof(t->bag)) &&
           load(exe, a->twoHanded, t->twoHanded, sizeof(t->twoHanded)) && load(exe, a->yes, t->yes, sizeof(t->yes)) && load(exe, a->no, t->no, sizeof(t->no)) &&
           load(exe, a->skill, t->skill, sizeof(t->skill)) && load(exe, a->duration, t->duration, sizeof(t->duration)) &&
           load(exe, a->minutes, t->minutes, sizeof(t->minutes)) && load(exe, a->health, t->health, sizeof(t->health)) &&
           load(exe, a->magic, t->magic, sizeof(t->magic)) && load(exe, a->percent, t->percent, sizeof(t->percent)) &&
           load(exe, a->damage, t->damage, sizeof(t->damage)) && load(exe, a->protections, t->protections, sizeof(t->protections)) &&
           loadPacked(exe, a->protectionNames, t->protectionNames, 9) && loadPacked(exe, a->statNames, t->statNames, 27) &&
           loadPacked(exe, a->skillTypes, t->skillTypes, 5);
}

void clueFormatNumber(unsigned value, unsigned decimals, char out[16]) {
    char digits[16];
    snprintf(digits, sizeof(digits), "%u", value & 0xFFFF);
    size_t length = strlen(digits);
    if (decimals == 0 || decimals > length) {
        memcpy(out, digits, length + 1);
        return;
    }
    size_t whole = length - decimals;
    memcpy(out, digits, whole);
    out[whole] = '.';
    memcpy(out + whole + 1, digits + whole, decimals + 1);
}

static void put(const ViewRenderer *r, int x, int y, const char *s, uint8_t colour) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, s, colour, 0, FontTransparent);
}

/* DrawLabeledNumberIfNonzero: the label, then (when the number is not zero) the number at x = 157. */
static void labeledNumber(const ViewRenderer *r, int x, int y, const char *label, unsigned value, unsigned decimals, uint8_t colour) {
    char number[16];
    put(r, x, y, label, 0x0A);
    if (value & 0xFFFF) {
        clueFormatNumber(value, decimals, number);
        put(r, 157, y, number, colour);
    }
}

void clueItemPageDraw(const ViewRenderer *r, const ClueItemText *t, const uint8_t *item, uint16_t navFlags) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, 13, 1, 1, false, 0);
    char name[ItemNameBufferSize];
    itemGetName(item, name);
    put(r, 6, 4, name, 0x0D);
    clueNavBarDraw(r, navFlags);
    unsigned flags = itemGetU16(item, ItemFieldFlags);
    viewDrawPicture(r, 8, itemGetU16(item, ItemFieldIcon) + ((flags & ItemFlagAltIcon) ? 1 : 0), 68, 41, true, 0);

    put(r, 91, 39, t->baseValue, 0x0A);
    const uint8_t *value = itemBaseValue(item);
    if (bcd4AtLeastU16(value, 1)) {
        char text[12];
        bcd4Format(value, text);
        put(r, 157, 39, text, 0x8A);
    }
    labeledNumber(r, 115, 45, t->weight, itemGetU16(item, ItemFieldWeight), 1, 0x8A);

    unsigned fit = itemGetU16(item, ItemFieldFitFlags);
    put(r, 110, 69, t->fitsIn, 0x0A);
    int x = 158;
    if (!(fit & 0xE000)) {
        put(r, x, 69, (flags & ItemFlagEquipCode0B) ? t->characterPanel : t->anyPanel, 0xCA);
        return;
    }
    if (fit & ItemFitBackpack) {
        put(r, x, 69, t->backpack, 0xCA);
        x += 54;
    }
    if (fit & ItemFitBox) {
        put(r, x, 69, t->box, 0xCA);
        x += 24;
    }
    if (fit & ItemFitBag) {
        put(r, x, 69, t->bag, 0xCA);
    }
}

/* ShowArmorProtectionsList / ShowArmorAttributeBonusList: the effect pairs that belong to one of the two name tables. */
static void effectList(const ViewRenderer *r, const ClueItemText *t, const uint8_t *effect, int labelX, int labelY, const char *label, bool protections) {
    put(r, labelX, labelY, label, 0x0A);
    if (!effect) {
        return;
    }
    int y = labelY;
    unsigned pairs = itemEffectPairs(effect);
    for (unsigned i = 0; i < pairs && i < 4; i++) {
        unsigned type = itemEffectField(effect, i);
        char number[16];
        const char *name = NULL;
        if (protections) {
            if (type > 0x30) {
                continue;
            }
            unsigned index = type >= 0x20 && !(type & 1) ? (type - 0x20) / 2 : 9;
            name = index < 9 ? t->protectionNames[index] : NULL;
        } else {
            if (type < 0x7C) {
                continue;
            }
            unsigned index = (type & 1) ? 27 : (type - 0x7C) / 2;
            name = index < 27 ? t->statNames[index] : NULL;
        }
        clueFormatNumber(itemEffectAmount(effect, i), 0, number);
        put(r, labelX + (protections ? 0x48 : 0x1E), y, number, 0xA7);
        if (name) {
            put(r, labelX + (protections ? 0x48 : 0x1E) + 24, y, name, 0xA7);
        }
        y += 6;
    }
}

void clueArmorRowDraw(const ViewRenderer *r, const ClueItemText *t, const uint8_t *entry, const uint8_t *effect) {
    labeledNumber(r, 91, 57, t->absorption, itemTargetWord(entry, ItemTargetAbsorption), 0, 0x59);
    effectList(r, t, effect, 85, 81, t->protections, true);
    effectList(r, t, effect, 127, 111, t->adds, false);
}

void clueWeaponRowDraw(const ViewRenderer *r, const ClueItemText *t, const uint8_t *entry) {
    labeledNumber(r, 115, 57, t->damage, itemTargetWord(entry, 0), 0, 0x59);
    unsigned flags = itemTargetWord(entry, ItemTargetSlotFlags);
    put(r, 121, 90, t->skill, 0x0A);
    unsigned type = (flags & 0x8000) ? 0 : (flags & 0x4000) ? 1 : (flags & 0x2000) ? 2 : (flags & 0x1000) ? 3 : 0;
    put(r, 157, 90, t->skillTypes[type], 0xA7);
    put(r, 103, 120, t->twoHanded, 0x0A);
    put(r, 157, 120, (flags & 1) ? t->yes : t->no, 0xA7);
}

void clueHealingRowDraw(const ViewRenderer *r, const ClueItemText *t, GameKind game, const uint8_t *entry) {
    unsigned flags = itemTargetWord(entry, ItemTargetSlotFlags);
    unsigned percent = itemTargetWord(entry, 2);
    bool magic = (flags & 0x8000) != 0;
    if (game == GameYendor3 && !magic) {
        return;
    }
    labeledNumber(r, magic ? 121 : 115, 57, magic ? t->magic : t->health, percent, 0, 0x59);
    put(r, (int16_t)percent < 10 ? 169 : 175, 57, t->percent, 0x0D);
}

void clueDurationRowDraw(const ViewRenderer *r, const ClueItemText *t, const uint8_t *entry) {
    labeledNumber(r, 103, 57, t->duration, 10 * itemTargetWord(entry, 2), 0, 0x59);
    put(r, 181, 57, t->minutes, 0x0D);
}

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

static const struct {
    uint8_t category;
    uint16_t yendor2, yendor3;
} kHeadings[] = {{11, 0x88C0, 0x8BE1}, {12, 0x8A01, 0x8D22}, {13, 0x8A21, 0x8D44}, {14, 0x8A3F, 0x8D5D}, {15, 0x8A54, 0x8D6B}, {16, 0x8A5C, 0x8D80}, {17, 0x8A7A, 0x8D99}};

static const TextAddresses kYendor2 = {0x8A82, 0x8A8E, 0x8A96, 0x8AA2, 0x8AAB, 0x8AB1, 0x8AC1, 0x8ACB, 0x8AD5, 0x8ADA, 0x8ADE, 0x8AE8,
                                       0x8AEC, 0x8AEF, 0x8B45, 0x8B4F, 0x8B57, 0x8B5F, 0x8B66, 0x7C81, 0x7B24, 0x7B31, 0x7DC7, 0x7E8A};
static const TextAddresses kYendor3 = {0x8DA1, 0x8DAD, 0x8DB5, 0x8DC1, 0x8DCA, 0x8DD0, 0x8DE0, 0x8DEA, 0x8DF4, 0x8DF9, 0x8DFD, 0x8E07,
                                       0x8E0B, 0x8E0E, 0x8E62, 0x8E6C, 0, /* no HEALTH- */ 0x8E7C, 0x8E83, 0x7FAE, 0x7E56, 0x7E63, 0x80F4, 0x81B7};

typedef struct {
    unsigned strength, dexterity, damage, poison, undead, area, restores, percentMagic, percentHealth, cures, curesWhat, converts, convertsWhat;
} AbilityAddresses;

static const AbilityAddresses kAbility2 = {0x88D0, 0x8923, 0x893A, 0x88DA, 0x88E1, 0x8C5D, 0x8C53, 0x88FA, 0x88E8, 0x8C63, 0x8C6A, 0x8C80, 0x8C8A};
static const AbilityAddresses kAbility3 = {0x8BF1, 0x8C44, 0x8C5B, 0x8BFB, 0x8C02, 0x8F7B, 0x8F71, 0x8C1B, 0x8C09, 0x8F81, 0x8F88, 0, 0};

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
    memset(t->headings, 0, sizeof(t->headings));
    for (unsigned i = 0; i < sizeof(kHeadings) / sizeof(kHeadings[0]); i++) {
        if (!exeDataString(exe, game == GameYendor2 ? kHeadings[i].yendor2 : kHeadings[i].yendor3, t->headings[kHeadings[i].category], sizeof(t->headings[0]))) {
            return false;
        }
    }
    const AbilityAddresses *b = game == GameYendor2 ? &kAbility2 : &kAbility3;
    ClueAbilityText *ab = &t->ability;
    if (!(load(exe, b->strength, ab->strength, sizeof(ab->strength)) && load(exe, b->dexterity, ab->dexterity, sizeof(ab->dexterity)) &&
          load(exe, b->damage, ab->damage, sizeof(ab->damage)) && load(exe, b->poison, ab->poison, sizeof(ab->poison)) && load(exe, b->undead, ab->undead, sizeof(ab->undead)) &&
          load(exe, b->area, ab->area, sizeof(ab->area)) && load(exe, b->restores, ab->restores, sizeof(ab->restores)) &&
          load(exe, b->percentMagic, ab->percentMagic, sizeof(ab->percentMagic)) && load(exe, b->percentHealth, ab->percentHealth, sizeof(ab->percentHealth)) &&
          load(exe, b->cures, ab->cures, sizeof(ab->cures)) && load(exe, b->curesWhat, ab->curesWhat, sizeof(ab->curesWhat)) &&
          load(exe, b->converts, ab->converts, sizeof(ab->converts)) && load(exe, b->convertsWhat, ab->convertsWhat, sizeof(ab->convertsWhat)))) {
        return false;
    }
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

void clueItemPageDraw(const ViewRenderer *r, const ClueItemText *t, const uint8_t *item, uint16_t navFlags, unsigned category) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, r->game == GameYendor2 ? 13 : 6, 1, 1, false, 0);
    char name[ItemNameBufferSize];
    itemGetName(item, name);
    put(r, 6, 4, name, 0x0D);
    if (category < 18 && t->headings[category][0]) {
        put(r, clueHeadingX(r->game, category, (unsigned)strlen(t->headings[category])), 4, t->headings[category], 0x0D);
    }
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

typedef struct {
    unsigned id, amount;
    char kind; /* 'u' undead, 'p' poison, 'a' 3 x 3, 0 nothing -- the damage rows; 'm' percent of magic, 'h' percent of health -- the restore rows */
} AbilityRow;

static const AbilityRow kDamage2[] = {{0x1B, 50, 'u'}, {0x19, 60, 0}, {0x1A, 35, 'p'}, {0x240, 40, 'a'}};
static const AbilityRow kDamage3[] = {{0x3F, 85, 'u'}, {0x3D, 60, 0}, {0x3E, 35, 'p'}, {0x3C, 40, 'a'}};
static const AbilityRow kRestore2[] = {{0x1D, 50, 'm'}, {0x17, 100, 'm'}, {0x12, 25, 'h'}, {0x13, 50, 'm'}, {0x14, 100, 'm'}};
static const AbilityRow kRestore3[] = {{0x37, 50, 'm'}, {0x38, 100, 'm'}, {0x34, 25, 'h'}, {0x35, 50, 'm'}, {0x36, 100, 'm'}};

static const AbilityRow *findRow(const AbilityRow *rows, unsigned count, unsigned id) {
    for (unsigned i = 0; i < count; i++) {
        if (rows[i].id == id) {
            return &rows[i];
        }
    }
    return NULL;
}

static void abilityNumber(const ViewRenderer *r, unsigned amount) {
    char number[16];
    clueFormatNumber(amount, 0, number);
    put(r, 157, 57, number, 0x59);
}

/* ShowItemAbilityEffectInfo. */
static void abilityRowDraw(const ViewRenderer *r, const ClueAbilityText *t, GameKind game, unsigned id, const uint8_t *consumable, const uint8_t *effect) {
    const bool chapter2 = game == GameYendor2;
    if (effect) { /* the first pair of the effect entry: field offset, amount */
        bool dexterity = itemTargetWord(effect, 0) == 0x3E;
        labeledNumber(r, dexterity ? 97 : 103, 57, dexterity ? t->dexterity : t->strength, itemTargetWord(effect, 1), 0, 0x59);
        return;
    }
    if (consumable && (itemTargetWord(consumable, 0) & 1)) {
        const AbilityRow *row = chapter2 ? findRow(kDamage2, 4, id) : findRow(kDamage3, 4, id);
        if (row) {
            put(r, 115, 57, t->damage, 0x0A);
            abilityNumber(r, row->amount);
            const char *word = row->kind == 'u' ? t->undead : row->kind == 'p' ? t->poison : row->kind == 'a' ? t->area : NULL;
            if (word) {
                put(r, 175, 57, word, 0xA7);
            }
            return;
        }
    }
    const AbilityRow *row = chapter2 ? findRow(kRestore2, 5, id) : findRow(kRestore3, 5, id);
    if (row) {
        put(r, 103, 57, t->restores, 0x0A);
        abilityNumber(r, row->amount);
        put(r, row->amount < 100 ? 175 : 181, 57, row->kind == 'h' ? t->percentHealth : t->percentMagic, 0xA7);
    } else if (id == (chapter2 ? 0x18u : 0x39u)) {
        put(r, 121, 57, t->cures, 0x0A);
        put(r, 157, 57, t->curesWhat, 0x59);
    } else if (chapter2 && id == 0x1C) {
        put(r, 103, 57, t->converts, 0x0A);
        put(r, 157, 57, t->convertsWhat, 0x59);
    }
}

void clueInfoRowDraw(const ViewRenderer *r, const ClueItemText *t, GameKind game, unsigned itemId, const uint8_t *record, const uint8_t *consumable, const uint8_t *effect) {
    const bool chapter2 = game == GameYendor2;
    if (itemGetU16(record, ItemFieldFlags) & 0x1000) {
        if (consumable) {
            clueDurationRowDraw(r, t, consumable);
        }
    } else if (chapter2 ? (itemId >= 0x36 && itemId <= 0x46) : (itemId >= 0x1F && itemId <= 0x20)) {
        if (consumable) {
            clueHealingRowDraw(r, t, game, consumable);
        }
    } else if (itemId == (chapter2 ? 0x240u : 0x3Cu) || (chapter2 ? (itemId >= 0x12 && itemId <= 0x1D) : (itemId >= 0x34 && itemId <= 0x3F))) {
        abilityRowDraw(r, &t->ability, game, itemId, consumable, effect);
    }
}

static unsigned subIconTargetWord1(const ItemCatalog *catalog, const uint8_t *record) {
    const uint8_t *entry = record ? itemTargetEntry(catalog, record) : NULL;
    return entry ? itemTargetWord(entry, ItemTargetSlotFlags) : 0;
}

unsigned clueSubIconCount(const ItemCatalog *catalog, unsigned itemId, uint16_t *selectionMask) {
    const uint8_t *record = itemCatalogRecord(catalog, itemId);
    *selectionMask = 0;
    if (!record) {
        return 0;
    }
    unsigned flags = (unsigned)record[ItemFieldFlags] | ((unsigned)record[ItemFieldFlags + 1] << 8);
    bool ring = false;
    if (flags & 0xC000) {
        if (!(subIconTargetWord1(catalog, record) & 0x800)) {
            return 0;
        }
    } else if (flags & 0x0E00) {
        if (!(subIconTargetWord1(catalog, record) & 0x100)) {
            return 0;
        }
        ring = true;
    } else {
        return 0;
    }
    unsigned count = 2;
    for (unsigned i = 1; i <= 9; i++) {
        if (!(subIconTargetWord1(catalog, itemCatalogRecord(catalog, itemId + i)) & (ring ? 0x100u : 0x800u))) {
            break;
        }
        count++;
    }
    *selectionMask = (uint16_t)(0x8000 | (ring ? 1 : 0));
    return count;
}

void clueSubIconRegion(unsigned icon, uint16_t out[5]) {
    out[0] = (uint16_t)(0x15 + 0x1A * icon);
    out[1] = (uint16_t)(out[0] + 0x10);
    out[2] = 0x8D;
    out[3] = 0x96;
    out[4] = (uint16_t)(icon + 1);
}

unsigned clueSubIconClick(unsigned region, unsigned firstItemId, uint16_t *selectionMask) {
    unsigned icon = region - 1;
    *selectionMask = (uint16_t)((*selectionMask & 0x1F) | (0x8000u >> icon));
    return firstItemId + icon;
}

void clueSubIconRowDraw(const ViewRenderer *r, unsigned count, uint16_t selectionMask) {
    for (unsigned i = 0; i < count && i < ClueSubIconMax; i++) {
        uint16_t region[5];
        clueSubIconRegion(i, region);
        bool selected = (selectionMask & (0x8000u >> i)) != 0;
        viewDrawPicture(r, 8, 0x155 + 2 * i + (selected ? 1 : 0), region[0], region[2], true, 0);
    }
}

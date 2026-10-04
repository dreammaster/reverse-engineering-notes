#include "cluespell.h"

#include <stdio.h>
#include <string.h>

#include "chargen.h"
#include "cluebook.h"
#include "font.h"
#include "party.h"
#include "spellrecord.h"

typedef struct {
    unsigned header, costLabels, effectLabels, classNames, scroll, creation, training, all, one, visibleMonsters, visibleUndeads, monster, insect, undead,
        character, plural, handToHand, straight, area, distance, outOfHand, anytime;
    unsigned costLabelCount;
    uint32_t indexBlock, lineBlock;
} Addresses;

static const Addresses kYendor2 = {0x8C9E, 0x8CAF, 0x8CC4, 0x79A3, 0x8CE6, 0x8D83, 0x8CDD, 0x8CED, 0x8CF1, 0x8D64, 0x8FC2, 0x8CF5, 0x8D75, 0x8D7C,
                                   0x8CFD, 0x8D07, 0x8D09, 0x8D19, 0x8D2C, 0x8D3A, 0x8D48, 0x8D5C, 3, 1458267, 1458767};
static const Addresses kYendor3 = {0x8FBC, 0x8FCD, 0x8FDB, 0x7CD5, 0x8FFD, 0x909A, 0x8FF4, 0x9004, 0x9008, 0x907B, 0x92E1, 0x900C, 0x908C, 0x9093,
                                   0x9014, 0x901E, 0x9020, 0x9030, 0x9043, 0x9051, 0x905F, 0x9073, 2, 4019761, 4020193};

static bool loadPacked(const ExeData *exe, unsigned address, char (*out)[12], unsigned count, size_t width) {
    for (unsigned i = 0; i < count; i++) {
        if (!exeDataString(exe, address, out[i], width)) {
            return false;
        }
        address += (unsigned)strlen(out[i]) + 1;
    }
    return true;
}

#define LOAD(field) exeDataString(exe, a->field, t->field, sizeof(t->field))

bool clueSpellTextLoad(ClueSpellText *t, const ExeData *exe, GameKind game) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    memset(t, 0, sizeof(*t));
    t->costLabelCount = a->costLabelCount;
    unsigned address = a->costLabels;
    for (unsigned i = 0; i < t->costLabelCount; i++) {
        if (!exeDataString(exe, address, t->costLabels[i], sizeof(t->costLabels[i]))) {
            return false;
        }
        address += (unsigned)strlen(t->costLabels[i]) + 1;
    }
    return LOAD(header) && loadPacked(exe, a->effectLabels, t->effectLabels, 5, sizeof(t->effectLabels[0])) &&
           loadPacked(exe, a->classNames, t->classNames, 6, sizeof(t->classNames[0])) && LOAD(scroll) && LOAD(creation) && LOAD(training) && LOAD(all) &&
           LOAD(one) && LOAD(visibleMonsters) && LOAD(visibleUndeads) && LOAD(monster) && LOAD(insect) && LOAD(undead) && LOAD(character) &&
           LOAD(plural) && LOAD(handToHand) && LOAD(straight) && LOAD(area) && LOAD(distance) && LOAD(outOfHand) && LOAD(anytime);
}

bool spellDescriptionRange(GameKind game, const uint8_t *worldDat, size_t size, unsigned id, unsigned *first, unsigned *count) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    size_t entry = a->indexBlock + 4 * (size_t)id;
    if (entry + 4 > size || id > (game == GameYendor2 ? 124u : 107u)) {
        return false;
    }
    *first = worldDat[entry] | (worldDat[entry + 1] << 8);
    *count = worldDat[entry + 2] | (worldDat[entry + 3] << 8);
    return true;
}

const uint8_t *spellDescriptionLine(GameKind game, const uint8_t *worldDat, size_t size, unsigned index) {
    const Addresses *a = game == GameYendor2 ? &kYendor2 : &kYendor3;
    size_t at = a->lineBlock + (size_t)index * ClueSpellLineSize;
    return at + ClueSpellLineSize <= size ? worldDat + at : NULL;
}

static void put(const ViewRenderer *r, int x, int y, const char *s, uint8_t colour) {
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, x, y, s, colour, 0, FontTransparent);
}

static void classRow(const ViewRenderer *r, const ClueSpellText *t, unsigned classIndex, int *y, unsigned number, const char *word, uint8_t colour) {
    char text[16];
    put(r, 122, *y, word, colour);
    snprintf(text, sizeof(text), "%u", number);
    put(r, 104, *y, text, 0x0D);
    put(r, 44, *y, t->classNames[classIndex], 0x0D);
    *y += 6;
}

void clueSpellPageDraw(const ViewRenderer *r, const ClueSpellText *t, const uint8_t *record, unsigned spellId, const char *heading, uint16_t navFlags,
                       const ClueSpellDescription *description) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    viewDrawPicture(r, 0, r->game == GameYendor2 ? 13 : 6, 1, 1, false, 0);
    char name[SpellNameFieldSize + 1];
    spellGetName(record, name);
    put(r, 6, 4, name, 0x0D);
    put(r, clueHeadingX(r->game, 3, (unsigned)strlen(heading)), 4, heading, 0x0D);
    clueNavBarDraw(r, navFlags);

    unsigned flagsA = spellGetU16(record, SpellFieldFlagsA), flagsB = spellGetU16(record, SpellFieldFlagsB), resist = spellGetU16(record, SpellFieldResistFlags);
    unsigned target = spellGetU16(record, SpellFieldTargetTypeId), mask = spellGetU16(record, SpellFieldClassEligibility);

    put(r, 44, 28, t->header, 0x0A);
    for (unsigned i = 0; i < t->costLabelCount; i++) {
        put(r, 200, 40 + 6 * (int)i, t->costLabels[i], 0x0A);
        char number[16];
        snprintf(number, sizeof(number), "%u", spellGetU16(record, SpellFieldMpCost + 2 * i));
        put(r, 236, 40 + 6 * (int)i, number, 0x8A);
    }
    for (unsigned i = 0; i < 5; i++) {
        put(r, 44, 76 + 6 * (int)i, t->effectLabels[i], 0x0A);
    }

    int y = 34;
    const PartyAbilityUnlockRow *unlock = partyAbilityUnlockTable(r->game);
    for (unsigned k = 0; k < 6; k++) {
        uint16_t start[2];
        partyStartingAbilityIds(4 + k, start);
        if (start[0] == spellId || start[1] == spellId) {
            classRow(r, t, k, &y, 1, t->creation, 0xA7);
            continue;
        }
        unsigned level = 0;
        for (unsigned column = 0; column < PartyAbilityUnlockColumnCount && !level; column++) {
            for (unsigned slot = 0; slot < PartyAbilityUnlockSlotCount; slot++) {
                if (unlock[k][column * PartyAbilityUnlockSlotCount + slot] == spellId) {
                    level = 2 * (column + 1);
                    break;
                }
            }
        }
        if (level) {
            classRow(r, t, k, &y, level, t->training, 0x8A);
        } else if (mask & (0x20u >> k)) {
            classRow(r, t, k, &y, spellGetU16(record, SpellFieldRequiredLevel), t->scroll, 0xCA);
        }
    }

    if (!(flagsB & 0xFF)) {
        bool all = (resist & 6) || (flagsB & 0x5E00);
        put(r, 92, 76, all ? t->all : t->one, 0xCA);
        int x = 92 + 24;
        bool done = false;
        if (flagsB & 0xC000) {
            put(r, x, 76, t->character, 0x0D);
            x += 54;
        } else if ((resist & 6) || (flagsB & 0x200)) {
            put(r, x, 76, ((resist & 0x100) && target == 13) ? t->visibleUndeads : t->visibleMonsters, 0x0D);
            done = true;
        } else if ((resist & 0x100) && target == 9) {
            put(r, x, 76, t->insect, 0x0D);
            x += 36;
        } else if ((resist & 0x100) && target == 13) {
            put(r, x, 76, t->undead, 0x0D);
            x += 36;
        } else {
            put(r, x, 76, t->monster, 0x0D);
            x += 42;
        }
        if (!done) {
            if (flagsB & 0x5C00) {
                put(r, x, 76, t->plural, 0x0D);
                x += 6;
            }
            x += 6;
            const char *range = (flagsB & 0x3000) ? t->handToHand : (flagsB & 0x800) ? t->straight : (flagsB & 0x400) ? t->area : (flagsB & 0x100) ? t->distance : NULL;
            if (range) {
                put(r, x, 76, range, 0xCA);
            }
        }
    }
    put(r, 74, 88, (flagsA & 0x400) ? t->outOfHand : (flagsB & 0x3000) ? t->handToHand : t->anytime, 0xA7);

    if (description) {
        for (unsigned i = 0; i < description->lineCount && i < 16; i++) {
            if (description->lines[i]) {
                char line[ClueSpellLineSize + 1];
                memcpy(line, description->lines[i], ClueSpellLineSize);
                line[ClueSpellLineSize] = 0;
                put(r, 44, 106 + 6 * (int)i, line, 0x0D);
            }
        }
    }
}

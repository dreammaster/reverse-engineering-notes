#include "partyinfo.h"

#include <stdio.h>
#include <string.h>

#include "party.h"

enum { InfoX = 241, FirstRow = 96 };

void partyInfoFormatNumber(unsigned value, char out[8]) {
    value &= 0xFFFF;
    if (value >= 1000) {
        snprintf(out, 8, "%u,%03u", value / 1000, value % 1000);
    } else {
        snprintf(out, 8, "%u", value);
    }
}

typedef struct {
    PanelRun *out;
    unsigned max, count;
} Runs;

static void put(Runs *r, int x, int y, uint8_t colour, bool opaque, const char *text) {
    if (r->count < r->max) {
        PanelRun *run = &r->out[r->count++];
        run->x = x;
        run->y = y;
        run->colour = colour;
        run->opaque = opaque;
        snprintf(run->text, sizeof(run->text), "%s", text);
    }
}

/* DrawCharacterNameHeader: twelve blanks over the panel, then the name, at (241, 87) in 0x8A. */
static void header(Runs *r, const uint8_t *record) {
    char name[16];
    snprintf(name, sizeof(name), "%.13s", (const char *)record + PartyFieldName);
    put(r, InfoX, 87, 0x8A, true, "            ");
    put(r, InfoX, 87, 0x8A, false, name);
}

int partyInfoKindForRegion(unsigned part) {
    return part >= 2 && part <= 4 ? 0 : part == 5 ? 1 : part == 6 ? 2 : part == 7 ? 3 : part == 8 ? 4 : -1;
}

unsigned partyInfoAfflictions(const uint8_t *record, PanelRun *out, unsigned max) {
    static const struct {
        unsigned bit;
        const char *name;
    } kStates[] = {{0x2000, "DISEASED"}, {0x4000, "POISONED"}, {0x8000, "SICK"}, {0x0400, "STONED"}, {0x0800, "FROZEN"}, {0x1000, "PARALYZED"}, {0x0080, "CURSED"}, {0x0100, "HEXED"}, {0x0200, "JINXED"}};
    Runs r = {out, max, 0};
    unsigned status = partyGetU16(record, PartyFieldStatusFlags);
    int y = FirstRow;
    header(&r, record);
    put(&r, InfoX, y, 0xAA, false, "AFFLICTIONS:");
    y += 6;
    if (!(status & 0xFF80)) {
        put(&r, InfoX, y, 0x0F, false, "NONE");
        return r.count;
    }
    for (unsigned i = 0; i < sizeof(kStates) / sizeof(kStates[0]); i++) {
        if (status & kStates[i].bit) {
            put(&r, InfoX, y, 0x0F, false, kStates[i].name);
            y += 6;
        }
    }
    return r.count;
}

unsigned partyInfoTraining(const uint8_t *record, PanelRun *out, unsigned max) {
    Runs r = {out, max, 0};
    char number[8];
    header(&r, record);
    put(&r, InfoX, FirstRow, 0xAA, false, "TRAINING:");
    put(&r, InfoX, FirstRow + 6, 0x8A, false, "CURRENTLY");
    put(&r, InfoX, FirstRow + 12, 0x8A, false, "LEVEL");
    partyInfoFormatNumber(partyGetU16(record, PartyFieldLevel), number);
    put(&r, InfoX + 42, FirstRow + 12, 0x8A, false, number);
    unsigned pending = partyGetU16(record, 0x1E);
    if (pending) {
        put(&r, InfoX, FirstRow + 18, 0x0F, false, "READY FOR");
        put(&r, InfoX, FirstRow + 24, 0x0F, false, "LEVEL");
        partyInfoFormatNumber(pending, number);
        put(&r, InfoX + 42, FirstRow + 24, 0x0F, false, number);
    }
    return r.count;
}

unsigned partyInfoProtections(const uint8_t *record, PanelRun *out, unsigned max) {
    static const char *const kNames[9] = {"DISEASE", "POISON", "SICKNESS", "STONING", "FROZEN", "PARALYZE", "CURSING", "HEXING", "JINXING"};
    Runs r = {out, max, 0};
    char number[8];
    header(&r, record);
    put(&r, InfoX, FirstRow, 0xAA, false, "PROTECTIONS:");
    for (unsigned i = 0; i < 9; i++) {
        put(&r, InfoX, FirstRow + 6 + 6 * (int)i, 0x0F, false, kNames[i]);
        partyInfoFormatNumber(partyGetU16(record, 0x20 + 2 * i), number);
        put(&r, InfoX + 0x36, FirstRow + 6 + 6 * (int)i, 0x0F, false, number);
    }
    return r.count;
}

/* The four mounts of the transports table (DS:0x77C6, the clue book's transportation page): name, the charge that uses it up, and the times it can be used (1 at night, 2 by day). */
unsigned partyInfoTransports(const uint8_t *record, unsigned clockMinutes, PanelRun *out, unsigned max) {
    static const struct {
        const char *name;
        unsigned charges, times;
    } kMounts[4] = {{"PEGASUS", 1, 0x8003}, {"GIANT EAGLE", 2, 0x4003}, {"FLYING RUG", 4, 0x2002}, {"MAGIC DRAGON", 4, 0x1001}};
    Runs r = {out, max, 0};
    unsigned learned = partyGetU16(record, PartyFieldAbilities);
    if (!learned) { /* the original draws nothing at all */
        return 0;
    }
    header(&r, record);
    put(&r, InfoX, FirstRow, 0xAA, false, "TRANSPORTS:");
    int y = FirstRow + 6;
    for (unsigned i = 0; i < 4; i++) {
        if (!(learned & (0x8000u >> i))) {
            continue;
        }
        unsigned charge = partyGetU16(record, PartyFieldAbilityCharge + 2 * i);
        bool night = clockMinutes < 0x1A4 || clockMinutes > 0x474;
        bool ready = (int)charge < (int)kMounts[i].charges && (night ? (kMounts[i].times & 1) : (kMounts[i].times & 2));
        put(&r, InfoX, y, ready ? 0x0F : 0x07, false, kMounts[i].name);
        y += 6;
    }
    return r.count;
}

/* DrawThreeStatBars: the three labels 18 pixels apart (HEALTH, MAGIC, WEIGHT) in their bar colours, the numbers one row above each pair: "n/max" in white -- DEAD for HEALTH of a dead member -- and the
 * weight with one decimal (tenths). A value above its maximum brightens the label by two colours. */
unsigned partyInfoStatBars(const uint8_t *record, PanelRun *out, unsigned max) {
    Runs r = {out, max, 0};
    char text[24], a[8], b[8];
    int hp = (int16_t)partyGetU16(record, 0x52), hpMax = (int16_t)partyGetU16(record, 0x92), mp = (int16_t)partyGetU16(record, 0x54), mpMax = (int16_t)partyGetU16(record, 0x94);
    int load = (int16_t)partyGetU16(record, 0x118), capacity = (int16_t)partyGetU16(record, 0x56), capacityMax = (int16_t)partyGetU16(record, 0x96);
    header(&r, record);
    put(&r, InfoX, FirstRow, (uint8_t)(0x59 + (hp > hpMax ? 2 : 0)), false, "HEALTH:");
    put(&r, InfoX, FirstRow + 18, (uint8_t)(0xCA + (mp > mpMax ? 2 : 0)), false, "MAGIC:");
    put(&r, InfoX, FirstRow + 36, (uint8_t)(0x86 + (capacity > capacityMax ? 2 : 0)), false, "WEIGHT:");
    if (partyGetU16(record, PartyFieldStatusFlags) & 0x40) {
        put(&r, InfoX, FirstRow + 6, 0x0F, false, "DEAD");
    } else {
        partyInfoFormatNumber((unsigned)hp, a);
        partyInfoFormatNumber((unsigned)hpMax, b);
        snprintf(text, sizeof(text), "%s/%s", a, b);
        put(&r, InfoX, FirstRow + 6, 0x0F, false, text);
    }
    partyInfoFormatNumber((unsigned)mp, a);
    partyInfoFormatNumber((unsigned)mpMax, b);
    snprintf(text, sizeof(text), "%s/%s", a, b);
    put(&r, InfoX, FirstRow + 24, 0x0F, false, text);
    /* the weight: StripCommasAndSpaces, then InsertDecimalPointFromEnd with one digit */
    char w1[8], w2[8];
    snprintf(w1, sizeof(w1), "%u", (unsigned)load & 0xFFFF);
    snprintf(w2, sizeof(w2), "%u", (unsigned)capacity & 0xFFFF);
    char d1[12], d2[12];
    snprintf(d1, sizeof(d1), "%.*s.%s", (int)strlen(w1) > 1 ? (int)strlen(w1) - 1 : 0, w1, w1 + (strlen(w1) > 1 ? strlen(w1) - 1 : 0));
    snprintf(d2, sizeof(d2), "%.*s.%s", (int)strlen(w2) > 1 ? (int)strlen(w2) - 1 : 0, w2, w2 + (strlen(w2) > 1 ? strlen(w2) - 1 : 0));
    snprintf(text, sizeof(text), "%s/%s", d1, d2);
    put(&r, InfoX, FirstRow + 42, 0x0F, false, text);
    return r.count;
}

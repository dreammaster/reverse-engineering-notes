/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_clueitem test_clueitem.c ../clueitem.c ../cluebook.c ../exedata.c ../font.c ../item.c ../bcd4.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_clueitem
 *
 * The real-data check reads the labels from yendor2/game/SW.EXE and yendor3/game/REGISTER.EXE (skipped if absent; YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "clueitem.h"

static int g_failureCount = 0;
static int g_skipCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static const uint8_t *pic(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[320 * 200];
    (void)ctx;
    memset(pixels, (uint8_t)(0x40 + category * 16 + id), sizeof(pixels));
    return pixels;
}

static bool anyColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (screen[y * 320 + x] == colour) {
                return true;
            }
        }
    }
    return false;
}

static ClueItemText makeText(void) {
    ClueItemText t;
    memset(&t, 0, sizeof(t));
    strcpy(t.baseValue, "VALUE");
    strcpy(t.weight, "WEIGHT");
    strcpy(t.absorption, "ABS");
    strcpy(t.fitsIn, "FITS");
    strcpy(t.adds, "ADDS");
    strcpy(t.characterPanel, "CHAR");
    strcpy(t.anyPanel, "ANY");
    strcpy(t.backpack, "PACK ");
    strcpy(t.box, "BOX ");
    strcpy(t.bag, "BAG");
    strcpy(t.protections, "PROT");
    strcpy(t.protectionNames[0], "FIRE");
    strcpy(t.statNames[0], "STR");
    strcpy(t.statNames[1], "DEX");
    strcpy(t.skill, "SKILL");
    strcpy(t.skillTypes[0], "PROJ");
    strcpy(t.skillTypes[1], "SLASH");
    strcpy(t.twoHanded, "2H");
    strcpy(t.yes, "YES");
    strcpy(t.no, "NO");
    strcpy(t.damage, "DMG");
    strcpy(t.duration, "DUR");
    strcpy(t.minutes, "MIN");
    strcpy(t.health, "HEALTH");
    strcpy(t.magic, "MAGIC");
    strcpy(t.percent, "PCT");
    strcpy(t.headings[12], "ARMOR");
    return t;
}

static void testFormatting(void) {
    char out[16];
    clueFormatNumber(1234, 1, out);
    check("a weight of 1234 shows as 123.4", strcmp(out, "123.4") == 0);
    clueFormatNumber(5, 1, out);
    check("a weight of 5 shows as .5 (the original inserts the point without a leading zero)", strcmp(out, ".5") == 0 || strcmp(out, "0.5") == 0);
    clueFormatNumber(1234, 0, out);
    check("no decimals: plain digits", strcmp(out, "1234") == 0);
}

static void testDraw(void) {
    ClueItemText t = makeText();
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, pic, NULL, screen, NULL};
    uint8_t item[ItemRecordSize];
    memset(item, 0, sizeof(item));
    item[ItemFieldIcon] = 7;
    item[ItemFieldBaseValue] = 0x50; /* packed BCD, see bcd4.h for the byte order; any nonzero value will do */
    item[ItemFieldBaseValue + 3] = 0x01;
    item[ItemFieldWeight] = 25;
    memcpy(item + ItemFieldName1, "BOW          ", 13);
    item[ItemFieldFitFlags + 1] = 0xC0; /* backpack + box */
    memset(screen, 0xEE, sizeof(screen));
    clueItemPageDraw(&r, &t, item, 0x8000, 12);
    check("the backdrop is cleared and picture 13 of category 0 drawn at (1, 1)", screen[0] == 0 && screen[320 + 1] == 0x40 + 13);
    check("the category heading is right aligned at the top (Chapter 2: x = 249 for category 12)", anyColour(screen, 249, 4, 280, 10, 0x0D));
    check("the item name is written at (6, 4) in colour 0xD", anyColour(screen, 6, 4, 30, 10, 0x0D));
    check("the icon (category 8 picture 7) is at (68, 41)", screen[41 * 320 + 68] == 0x40 + 8 * 16 + 7);
    check("the labels are in colour 0x0A and the value and weight in 0x8A", anyColour(screen, 91, 39, 140, 45, 0x0A) && anyColour(screen, 157, 39, 200, 45, 0x8A) &&
                                                                              anyColour(screen, 157, 45, 200, 51, 0x8A));
    check("with fit bits the container list is in 0xCA, the box after the backpack", anyColour(screen, 158, 69, 190, 75, 0xCA) && anyColour(screen, 212, 69, 240, 75, 0xCA));

    memset(item + ItemFieldFitFlags, 0, 2);
    memset(screen, 0xEE, sizeof(screen));
    clueItemPageDraw(&r, &t, item, 0x8000, 12);
    check("with no fit bits ANY PANEL is shown", anyColour(screen, 158, 69, 180, 75, 0xCA) && !anyColour(screen, 212, 69, 240, 75, 0xCA));

    uint8_t armor[12] = {0}, effect[16] = {0};
    armor[0] = 9;
    effect[0] = 0x20;
    effect[2] = 3; /* protection 0, amount 3 */
    effect[4] = 0x7C;
    effect[6] = 12; /* the first attribute, amount 12 */
    effect[8] = 0x7E;
    effect[10] = 4;
    memset(screen, 0xEE, sizeof(screen));
    clueArmorRowDraw(&r, &t, armor, effect);
    check("armor: absorption value 9 in colour 0x59 at x = 157, y = 57", anyColour(screen, 157, 57, 165, 63, 0x59));
    check("armor: protection amount and name on the PROTECTIONS line (y 81), colour 0xA7", anyColour(screen, 157, 81, 165, 87, 0xA7) && anyColour(screen, 181, 81, 205, 87, 0xA7));
    check("armor: two bonuses one under the other from y 111", anyColour(screen, 157, 111, 165, 117, 0xA7) && anyColour(screen, 157, 117, 165, 123, 0xA7) && !anyColour(screen, 157, 123, 165, 129, 0xA7));

    uint8_t weapon[12] = {0};
    weapon[0] = 6;
    weapon[2] = 0x00;
    weapon[3] = 0x40; /* word 1 = 0x4000: slashing */
    weapon[2] |= 1;
    memset(screen, 0xEE, sizeof(screen));
    clueWeaponRowDraw(&r, &t, weapon);
    check("weapon: damage, skill type and the two-handed answer are drawn", anyColour(screen, 157, 57, 165, 63, 0x59) && anyColour(screen, 157, 90, 190, 96, 0xA7) &&
                                                                              anyColour(screen, 157, 120, 175, 126, 0xA7));

    uint8_t potion[8] = {0};
    potion[4] = 5; /* word 2 */
    memset(screen, 0xEE, sizeof(screen));
    clueHealingRowDraw(&r, &t, GameYendor2, potion);
    check("Chapter 2 healing item: HEALTH- row and PERCENT at x = 169 for a single digit", anyColour(screen, 115, 57, 150, 63, 0x0A) && anyColour(screen, 169, 57, 190, 63, 0x0D));
    memset(screen, 0xEE, sizeof(screen));
    clueHealingRowDraw(&r, &t, GameYendor3, potion);
    check("Chapter 3 draws nothing for a non-magic one", !anyColour(screen, 0, 50, 320, 70, 0x0A));
    potion[2] = 0x00;
    potion[3] = 0x80;
    memset(screen, 0xEE, sizeof(screen));
    clueHealingRowDraw(&r, &t, GameYendor3, potion);
    check("...and the MAGIC- row for a magic one", anyColour(screen, 121, 57, 150, 63, 0x0A));
    memset(screen, 0xEE, sizeof(screen));
    clueDurationRowDraw(&r, &t, potion);
    check("duration: DUR- label, 50 minutes in 0x59, MIN in 0xD", anyColour(screen, 103, 57, 125, 63, 0x0A) && anyColour(screen, 157, 57, 170, 63, 0x59) && anyColour(screen, 181, 57, 200, 63, 0x0D));
}

static void testReal(GameKind game, const char *envName, const char *defaultDir, const char *name, const char *label) {
    const char *dir = getenv(envName);
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", dir ? dir : defaultDir, name);
    FILE *f = fopen(path, "rb");
    if (!f) {
        printf("SKIP %s (%s not found)\n", label, path);
        g_skipCount++;
        return;
    }
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *data = malloc((size_t)size);
    bool read = data && fread(data, 1, (size_t)size, f) == (size_t)size;
    fclose(f);
    ExeData exe;
    static ClueItemText t;
    bool ok = read && exeDataOpen(&exe, game, data, (size_t)size) && clueItemTextLoad(&t, &exe, game);
    check(label, ok);
    if (ok) {
        check("...the labels and the three packed tables came out right", strcmp(t.baseValue, "BASE VALUE:") == 0 && strcmp(t.protectionNames[8], "JINXING") == 0 &&
                                                                             strcmp(t.skillTypes[4], "CASTING     ") == 0 && strncmp(t.statNames[0], "STRENGTH", 8) == 0 &&
                                                                             strncmp(t.statNames[25], "LINGUISTICS", 11) == 0 && strcmp(t.backpack, "BACKPACK ") == 0);
    }
    free(data);
}

static void setItemWord(uint8_t *base, size_t offset, uint16_t value) {
    base[offset] = (uint8_t)value;
    base[offset + 1] = (uint8_t)(value >> 8);
}

static void testSubIcons(void) {
    static ItemCatalog catalog;
    memset(&catalog, 0, sizeof(catalog));
    catalog.game = GameYendor2;
    catalog.itemCount = 20;
    catalog.consumableCount = 0;
    catalog.weaponCount = 5;
    /* items 5-9: five "+N" weapons (flags 0x8000, target word 1 has 0x800); item 10: a plain weapon without the bit; items 11-14: rings */
    for (unsigned id = 5; id <= 9; id++) {
        uint8_t *record = catalog.items + (id - 1) * ItemRecordSize;
        setItemWord(record, ItemFieldFlags, 0x8000);
        setItemWord(record, ItemFieldTargetOffset, (uint16_t)((id - 5) * ItemWeaponSize));
    }
    for (unsigned i = 0; i < 5; i++) {
        setItemWord(catalog.weapons, (size_t)i * ItemWeaponSize + ItemTargetSlotFlags * 2, 0x800);
    }
    uint16_t mask;
    unsigned count = clueSubIconCount(&catalog, 5, &mask);
    check("items 5-9 carry the next-variant bit, item 10 lacks it: item 5 shows 2 + 4 icons (5-10)", count == 6 && mask == 0x8000);
    check("an item without equip flags has no row", clueSubIconCount(&catalog, 2, &mask) == 0 && mask == 0);
    uint16_t region[5];
    clueSubIconRegion(2, region);
    check("icon 2 sits at x 0x15 + 2 * 0x1A with the click region of the original", region[0] == 0x49 && region[1] == 0x59 && region[2] == 0x8D && region[3] == 0x96 && region[4] == 3);
    uint16_t sel = 0x8001;
    check("a click selects that icon, keeps the mode bit and names item first + k - 1", clueSubIconClick(3, 5, &sel) == 7 && sel == (0x2000 | 1));
}

int main(void) {
    testFormatting();
    testDraw();
    testSubIcons();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: the item page labels load from SW.EXE");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: the item page labels load from REGISTER.EXE");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_cluemonster test_cluemonster.c ../cluemonster.c ../clueitem.c ../cluebook.c ../exedata.c ../font.c ../monster.c ../effect.c ../party.c ../savegame.c ../item.c ../bcd4.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c && ./test_cluemonster
 *
 * The real-data check loads the labels from yendor2/game/SW.EXE and yendor3/game/REGISTER.EXE (skipped if absent; YENDOR2_GAME_DIR /
 * YENDOR3_GAME_DIR override).
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cluemonster.h"
#include "effect.h"

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

static void put16(uint8_t *m, unsigned offset, unsigned value) {
    m[offset] = (uint8_t)value;
    m[offset + 1] = (uint8_t)(value >> 8);
}

static void testDraw(void) {
    static ClueMonsterText t;
    memset(&t, 0, sizeof(t));
    for (unsigned i = 0; i < ClueMonsterRows; i++) {
        strcpy(t.label[i], "LABEL");
    }
    strcpy(t.immuneMark, "IMMUNE");
    strcpy(t.resistantMark, "RESIST");
    strcpy(t.heading, "MONSTER STATISTICS");
    uint8_t m[MonsterRecordSize];
    memset(m, 0, sizeof(m));
    put16(m, 0x50, 120); /* health */
    put16(m, 0x5A, 7);   /* damage */
    put16(m, 0x96, 0x8000 | 0x0010);
    put16(m, 0x98, 0x0200);
    m[0x7E] = 0x34; /* some nonzero loot */
    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, pic, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    clueMonsterPageDraw(&r, &t, m, "GOBLIN", 0x8000);
    check("backdrop, name and the right-aligned heading", screen[321] == 0x40 + 13 && anyColour(screen, 6, 4, 40, 10, 0x0D) && anyColour(screen, 208, 4, 316, 10, 0x0D));
    check("labels sit at their own positions (HEALTH row at (233, 46))", anyColour(screen, 233, 46, 260, 52, 0x0A) && anyColour(screen, 185, 16, 200, 22, 0x0A));
    check("HEALTH shows 120 at x = 275 in 0x59 and DAMAGE its 7; a zero stat shows nothing", anyColour(screen, 275, 46, 300, 52, 0x59) && anyColour(screen, 275, 70, 285, 76, 0x59) &&
                                                                                                 !anyColour(screen, 275, 52, 300, 58, 0x59));
    check("the immunity bit 0x8000 marks the first immune row (y 94), bit 0x4000 does not (y 100)", anyColour(screen, 263, 94, 300, 100, 0xA7) && !anyColour(screen, 263, 100, 300, 106, 0xA7));
    check("resistance bit 0x0200 marks the 0x3A00 row, and [+0x96] bit 0x10 does too", anyColour(screen, 263, 154, 300, 160, 0xA7));
    check("the 0xC000 resistance row stays unmarked", !anyColour(screen, 263, 160, 300, 166, 0xA7));
    check("the gold loot is drawn in 0x8A at x = 251 on its row (y 22)", anyColour(screen, 251, 22, 290, 28, 0x8A) && !anyColour(screen, 251, 16, 290, 22, 0x8A));
}

static void testAttackWords(void) {
    static ClueMonsterText t;
    memset(&t, 0, sizeof(t));
    const char *words[ClueAttackWordCount] = {"AREA, ", "SICK, ", "POISON, ", "DISEASE, ", "PARALYZE, ", "FROZEN, ", "STONING, ", "JINXING, ", "HEXING, ", "CURSING, ",
                                               "GOLD, ", "ORE, ", "NUORE, ", "BREAK ", "DESTROY ", "PROJECTILE, ", "WEAPON, ", "SHIELD, "};
    for (unsigned i = 0; i < ClueAttackWordCount; i++) {
        strcpy(t.attackWord[i], words[i]);
    }
    uint8_t m[MonsterRecordSize];
    memset(m, 0, sizeof(m));
    char out[160];
    bool has;
    clueMonsterAttackWords(&t, GameYendor2, m, out, &has);
    check("no special attack and no area flag: no words at all", !has && out[0] == 0);
    monsterSetU16(m, MonsterFieldFlags, MonsterFlagAreaAttack);
    clueMonsterAttackWords(&t, GameYendor2, m, out, &has);
    check("the area flag alone (effect 0 inflicts nothing) shows AREA", has && strcmp(out, "AREA") == 0);
    /* find real effects: one that inflicts POISON (0x4000) and one that steals */
    unsigned poison = 0, thief = 0;
    for (unsigned id = 1; id < EffectCountYendor2; id++) {
        EffectDef def;
        if (effectGetDef(GameYendor2, id, &def)) {
            if (!poison && (def.costFlags & 0x4000)) {
                poison = id;
            }
            if (!thief && (def.costFlags & 0x0001)) {
                thief = id;
            }
        }
    }
    monsterSetU16(m, MonsterFieldFlags, 0);
    monsterSetU16(m, MonsterFieldSpecialAttack, (uint16_t)poison);
    clueMonsterAttackWords(&t, GameYendor2, m, out, &has);
    check("a poisoning special attack lists POISON among its words", has && strstr(out, "POISON") != NULL);
    monsterSetU16(m, MonsterFieldSpecialAttack, (uint16_t)thief);
    clueMonsterAttackWords(&t, GameYendor2, m, out, &has);
    check("a stealing one lists GOLD", has && strstr(out, "GOLD") != NULL);
    monsterSetU16(m, MonsterFieldFlags, 0x0800 | 0x0200);
    clueMonsterAttackWords(&t, GameYendor2, m, out, &has);
    check("the corrode flags add BREAK / DESTROY then PROJECTILE and SHIELD, with no trailing separator", strstr(out, "PROJECTILE") != NULL && strstr(out, "SHIELD") != NULL &&
                                                                                                          strstr(out, "WEAPON") == NULL && out[strlen(out) - 1] != ' ');
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
    static ClueMonsterText t;
    bool ok = read && exeDataOpen(&exe, game, data, (size_t)size) && clueMonsterTextLoad(&t, &exe, game);
    check(label, ok);
    if (ok) {
        bool filled = true;
        for (unsigned i = 0; i < ClueMonsterRows; i++) {
            filled = filled && t.label[i][0] != 0;
        }
        bool words = t.attackLabel[0] != 0;
        for (unsigned i = 0; i < ClueAttackWordCount; i++) {
            words = words && t.attackWord[i][0] != 0;
        }
        check("...the label and the 18 words of the attack line are there", words);
        check("...every row has a label, the marks and the heading are there", filled && strcmp(t.heading, "MONSTER STATISTICS") == 0 && t.immuneMark[0] && t.resistantMark[0] &&
                                                                                 strcmp(t.immuneMark, t.resistantMark) != 0);
    }
    free(data);
}

int main(void) {
    testDraw();
    testAttackWords();
    testReal(GameYendor2, "YENDOR2_GAME_DIR", "../../yendor2/game", "SW.EXE", "Chapter 2: the monster page labels load from SW.EXE");
    testReal(GameYendor3, "YENDOR3_GAME_DIR", "../../yendor3/game", "REGISTER.EXE", "Chapter 3: the monster page labels load from REGISTER.EXE");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

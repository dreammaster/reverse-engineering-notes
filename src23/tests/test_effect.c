/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_effect test_effect.c ../effect.c ../party.c ../monster.c ../monster_stdio.c ../bcd4.c && ./test_effect
 *
 * The monster cross-checks read WORLD.DAT from yendor2/game and yendor3/game
 * (gitignored; skipped if absent). Set YENDOR2_GAME_DIR / YENDOR3_GAME_DIR
 * to override where.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "effect.h"
#include "monster.h"
#include "monster_stdio.h"
#include "party.h"

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

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static EffectDef get(GameKind game, unsigned id) {
    EffectDef def;
    memset(&def, 0, sizeof(def));
    if (!effectGetDef(game, id, &def)) {
        printf("FAIL effect %u should exist\n", id);
        g_failureCount++;
    }
    return def;
}

static void testTable(void) {
    EffectDef def;
    checkU32("yendor2 defines 45 effects", effectCount(GameYendor2), 45);
    checkU32("yendor3 defines 49 effects", effectCount(GameYendor3), 49);
    check("yendor2 id 45 is past the table", !effectGetDef(GameYendor2, 45, &def));
    check("yendor3 id 48 is the last", effectGetDef(GameYendor3, 48, &def) && !effectGetDef(GameYendor3, 49, &def));
    checkU32("the inflict mask is exactly the party status bits Cursed..Sick", EffectInflictMask,
             PartyStatusCursed | PartyStatusHexed | PartyStatusJinxed | PartyStatusStoned | PartyStatusFrozen |
                 PartyStatusParalyzed | PartyStatusDiseased | PartyStatusPoisoned | PartyStatusSick);

    uint8_t raw[EffectDefSize] = {0x0D, 0x00, 0x02, 0x00, 0x01, 0x00, 0x0A, 0x00, 0x10, 0x00, 0x00, 0x10};
    effectParseDef(raw, &def);
    check("raw bytes parse little-endian",
          def.sound == 0x0D && def.icon == 2 && def.magnitudeMin == 1 && def.magnitudeMax == 10 &&
              def.costFlags == 0x10 && def.modeFlags == 0x1000);
}

static void testDecoding(void) {
    EffectDef def = get(GameYendor2, 2);
    check("effect 2 is a plain HP-damage effect", effectSpend(&def) == EffectSpendHp && effectInflictedStatus(&def) == 0);
    checkU32("effect 2 icon", def.icon, 2);
    checkU32("effect 2 sound", def.sound, 0x0D);
    check("effect 2 rolls a magnitude", effectRollsMagnitude(&def));
    checkU32("effect 2's random bound is max - min", effectRandomBound(&def), 9);
    checkU32("effect 2 at level 3 with random 4 is (4 + 1) * 3", effectMagnitude(&def, 3, 4), 15);

    def = get(GameYendor2, 3);
    check("effect 3 (the healed icon) costs nothing", effectSpend(&def) == EffectSpendNone);

    def = get(GameYendor2, 5);
    check("effect 5 inflicts poison after a resistance roll",
          effectInflictedStatus(&def) == PartyStatusPoisoned && (def.modeFlags & EffectModeRollResistance) &&
              effectSpend(&def) == EffectSpendHp);
    def = get(GameYendor2, 7);
    check("effect 7 inflicts disease", effectInflictedStatus(&def) == PartyStatusDiseased);
    def = get(GameYendor2, 30);
    check("effect 30 inflicts disease, poison and sickness",
          effectInflictedStatus(&def) == (PartyStatusDiseased | PartyStatusPoisoned | PartyStatusSick));
    def = get(GameYendor2, 11);
    check("effect 11 curses (and costs 8 MP at 0x88)",
          effectInflictedStatus(&def) == PartyStatusCursed && effectSpend(&def) == EffectSpendMp);

    def = get(GameYendor2, 14);
    check("effect 14 is the MP-drain tick", effectSpend(&def) == EffectSpendMp && def.icon == 0xE6);
    def = get(GameYendor2, 15);
    check("effect 15 takes gold and rolls no magnitude", effectSpend(&def) == EffectSpendGold && !effectRollsMagnitude(&def));
    def = get(GameYendor2, 16);
    check("effect 16 takes the first ore", effectSpend(&def) == EffectSpendOre1);
    def = get(GameYendor2, 17);
    check("effect 17 takes the second ore", effectSpend(&def) == EffectSpendOre2);
    def = get(GameYendor2, 42);
    check("effect 42 costs HP and MP together", effectSpend(&def) == EffectSpendHpAndMp);

    def = get(GameYendor2, 18);
    check("effect 18 is a fixed, capped stat delta",
          (def.modeFlags & EffectModeStatCapped) && (def.modeFlags & EffectModeMagnitudeFixed));
    def = get(GameYendor2, 19);
    check("effect 19 is a fixed, floored stat delta",
          (def.modeFlags & EffectModeStatFloor) && (def.modeFlags & EffectModeMagnitudeFixed));
    def = get(GameYendor2, 1);
    check("effect 1 destroys an expiring item", def.modeFlags == EffectModeItemDestroy);
    def = get(GameYendor2, 0);
    check("effect 0 replaces an expiring item", def.modeFlags == EffectModeItemReplace);

    checkU32("yendor2 id 0 gets the startup sound 0x28", get(GameYendor2, 0).sound, 0x28);
    checkU32("yendor2 id 22 gets the startup sound 0x28", get(GameYendor2, 22).sound, 0x28);
    checkU32("yendor3 id 0 gets the startup sound 8", get(GameYendor3, 0).sound, 8);
    checkU32("yendor3 id 22 gets the startup sound 8", get(GameYendor3, 22).sound, 8);
    check("yendor3 effect 2 has its own sound and icon", get(GameYendor3, 2).sound == 0x22 && get(GameYendor3, 2).icon == 3);
}

static void testMagnitude(void) {
    EffectDef def = {0, 0, 7, 20, EffectCostHp, EffectModeMagnitudeFixed};
    checkU32("a fixed effect ignores level and random", effectMagnitude(&def, 9, 5), 7);
    def.modeFlags = EffectModeMagnitudeScaled;
    checkU32("a scaled effect is min * level, no random", effectMagnitude(&def, 4, 5), 28);
    def.modeFlags = 0;
    checkU32("a plain effect is (random + min) * level", effectMagnitude(&def, 2, 5), 24);
    def.magnitudeMin = 1000;
    checkU32("the product truncates to 16 bits like the original mul", effectMagnitude(&def, 100, 0), 100000u & 0xFFFF);

    EffectDef degenerate = {0, 0, 5, 5, 0, 0};
    checkU32("the bound of a degenerate range is 0", effectRandomBound(&degenerate), 0);
}

static void testResistance(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldProtections + 2 * PartyProtectionPoison, 11);
    partySetU16(record, PartyFieldProtections + 2 * PartyProtectionDisease, 20);
    partySetU16(record, PartyFieldProtections + 2 * PartyProtectionSickness, 300);
    partySetU16(record, PartyFieldProtections + 2 * PartyProtectionHexing, 5);

    EffectDef def = get(GameYendor2, 5);
    checkU32("effect 5 adds only the poison protection", effectResistanceBonus(&def, record), 11);
    def = get(GameYendor2, 7);
    checkU32("effect 7 adds only the disease protection", effectResistanceBonus(&def, record), 20);
    def = get(GameYendor2, 30);
    checkU32("effect 30 adds disease + poison + sickness", effectResistanceBonus(&def, record), 331);
    def = get(GameYendor2, 2);
    checkU32("an effect with no status adds nothing", effectResistanceBonus(&def, record), 0);
    def = get(GameYendor2, 5);
    def.modeFlags = 0;
    checkU32("without the resistance mode nothing is added", effectResistanceBonus(&def, record), 0);

    EffectDef hexCurse = {0, 0, 0, 0, PartyStatusHexed | PartyStatusCursed, EffectModeRollResistance};
    checkU32("hexing and cursing map to their own protections", effectResistanceBonus(&hexCurse, record), 5);
}

static void checkTableInvariants(GameKind game, const char *name) {
    char label[96];
    bool resistOk = true;
    bool spendOk = true;
    bool materialOk = true;
    for (unsigned id = 0; id < effectCount(game); id++) {
        EffectDef def = get(game, id);
        bool inflicts = effectInflictedStatus(&def) != 0;
        bool rolls = (def.modeFlags & EffectModeRollResistance) != 0;
        if (inflicts != rolls) {
            resistOk = false;
        }
        bool expiry = (def.modeFlags & (EffectModeItemDestroy | EffectModeItemReplace)) != 0;
        bool statDelta = (def.modeFlags & (EffectModeStatFloor | EffectModeStatCapped)) != 0;
        EffectSpend spend = effectSpend(&def);
        /* Ids 3 and 43 are icon-only effects: a "healed" marker and a sound-only cue. */
        if (spend == EffectSpendNone && !expiry && !statDelta && id != 3 && id != 43) {
            spendOk = false;
        }
        bool material = spend == EffectSpendGold || spend == EffectSpendOre1 || spend == EffectSpendOre2;
        if (material == effectRollsMagnitude(&def) && spend != EffectSpendNone) {
            materialOk = false;
        }
    }
    snprintf(label, sizeof(label), "%s: an effect inflicts a status exactly when it rolls a resistance", name);
    check(label, resistOk);
    snprintf(label, sizeof(label), "%s: every effect spends something, expires an item or changes a stat", name);
    check(label, spendOk);
    snprintf(label, sizeof(label), "%s: gold and ore effects are exactly those that roll no magnitude", name);
    check(label, materialOk);
}

static MonsterCatalog g_catalog;

static bool loadReal(GameKind game, const char *envName, const char *fallbackDir) {
    char path[512];
    const char *dir = getenv(envName);
    snprintf(path, sizeof(path), "%s/WORLD.DAT", dir ? dir : fallbackDir);
    return monsterCatalogReadWorldDatFile(&g_catalog, game, path);
}

/* Every monster's ordinary attack is an HP effect; its special effect (if any) is a real effect. */
static void checkMonsters(GameKind game, const char *name, const char *envName, const char *fallbackDir) {
    if (!loadReal(game, envName, fallbackDir)) {
        printf("SKIP %s monster cross-check (no WORLD.DAT)\n", name);
        g_skipCount++;
        return;
    }
    char label[96];
    bool primaryOk = true;
    bool specialOk = true;
    unsigned checked = 0;
    for (unsigned i = 1; i < g_catalog.blockCount; i++) {
        uint8_t record[MonsterRecordSize];
        memset(record, 0, sizeof(record));
        memcpy(record + MonsterBlockOffset, monsterCatalogBlock(&g_catalog, i), MonsterBlockSize);
        if (monsterGetU16(record, MonsterFieldMaxHealth) == 0) {
            continue;
        }
        checked++;
        EffectDef def;
        if (!effectGetDef(game, monsterGetU16(record, MonsterFieldAttackEffect), &def) || effectSpend(&def) != EffectSpendHp) {
            primaryOk = false;
        }
        unsigned special = monsterGetU16(record, MonsterFieldSpecialAttack);
        if (special != 0 && !effectGetDef(game, special, &def)) {
            specialOk = false;
        }
    }
    snprintf(label, sizeof(label), "%s: every monster's primary attack effect exists and costs HP", name);
    check(label, primaryOk && checked > 60);
    snprintf(label, sizeof(label), "%s: every monster special effect id exists", name);
    check(label, specialOk);
}

int main(void) {
    testTable();
    testDecoding();
    testMagnitude();
    testResistance();
    checkTableInvariants(GameYendor2, "yendor2");
    checkTableInvariants(GameYendor3, "yendor3");
    checkMonsters(GameYendor2, "yendor2", "YENDOR2_GAME_DIR", "../../yendor2/game");
    checkMonsters(GameYendor3, "yendor3", "YENDOR3_GAME_DIR", "../../yendor3/game");

    if (g_failureCount == 0) {
        printf("\nAll tests passed (%d skipped).\n", g_skipCount);
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

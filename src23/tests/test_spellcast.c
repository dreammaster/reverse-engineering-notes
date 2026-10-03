/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_spellcast test_spellcast.c ../spellcast.c ../spellrecord.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_spellcast
 */
#include <stdio.h>
#include <string.h>

#include "bcd4.h"
#include "party.h"
#include "spellcast.h"

static int g_failureCount = 0;

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void setU16(uint8_t *record, unsigned offset, uint16_t value) {
    record[offset] = (uint8_t)(value & 0xFF);
    record[offset + 1] = (uint8_t)(value >> 8);
}

static bool counterIs(SaveGame *save, unsigned offset, uint16_t value) {
    Bcd4 expected;
    bcd4FromU16(expected, value);
    return bcd4Compare(saveHeaderBcd4(save, offset), expected) == 0;
}

static void setup(SaveGame *save, uint8_t *caster, uint8_t *spell, uint16_t mp, uint16_t nuore, uint16_t ore) {
    saveGameInit(save, GameYendor2);
    memset(caster, 0, PartyRecordSize);
    memset(spell, 0, SpellRecordSize);
    bcd4FromU16(saveHeaderBcd4(save, SaveHeaderOreCounter2), nuore);
    bcd4FromU16(saveHeaderBcd4(save, SaveHeaderOreCounter1), ore);
    partySetStat(caster, PartyStatMagicPoints, mp);
}

static void testResourceGates(void) {
    SaveGame save;
    uint8_t caster[PartyRecordSize], spell[SpellRecordSize];

    setup(&save, caster, spell, 10, 5, 7);
    setU16(spell, SpellFieldMpCost, 10);
    setU16(spell, SpellFieldNuoreCost, 5);
    setU16(spell, SpellFieldOreCost, 7);
    check("exactly enough of everything casts", spellCanCast(spell, caster, &save, false));

    setU16(spell, SpellFieldMpCost, 11);
    check("one MP short fails", !spellCanCast(spell, caster, &save, false));

    setU16(spell, SpellFieldMpCost, 10);
    setU16(spell, SpellFieldNuoreCost, 6);
    check("one NUORE short fails", !spellCanCast(spell, caster, &save, false));

    setU16(spell, SpellFieldNuoreCost, 5);
    setU16(spell, SpellFieldOreCost, 8);
    check("one MAGIC ORE short fails", !spellCanCast(spell, caster, &save, false));

    setup(&save, caster, spell, 0, 0, 0);
    check("a free spell casts with nothing", spellCanCast(spell, caster, &save, false));
}

static void testContextGates(void) {
    SaveGame save;
    uint8_t caster[PartyRecordSize], spell[SpellRecordSize];
    setup(&save, caster, spell, 10, 0, 0);

    setU16(spell, SpellFieldFlagsA, SpellFlagsANotInCombat);
    check("an exploration spell casts outside combat", spellCanCast(spell, caster, &save, false));
    check("...but not in combat", !spellCanCast(spell, caster, &save, true));

    memset(spell, 0, sizeof(spell));
    setU16(spell, SpellFieldFlagsB, SpellFlagsBAttackPath);
    check("a single-monster attack spell is refused outside combat", !spellCanCast(spell, caster, &save, false));
    check("...and fine in combat", spellCanCast(spell, caster, &save, true));

    setU16(spell, SpellFieldFlagsB, SpellFlagsBAttackAllSlots);
    check("an all-slots attack spell is refused outside combat", !spellCanCast(spell, caster, &save, false));
    check("...and fine in combat", spellCanCast(spell, caster, &save, true));

    /* in combat the FlagsB gate doesn't apply and outside combat the FlagsA gate doesn't */
    memset(spell, 0, sizeof(spell));
    setU16(spell, SpellFieldFlagsA, SpellFlagsANotInCombat);
    setU16(spell, SpellFieldFlagsB, SpellFlagsBAttackPath);
    check("each gate is only consulted in its own context: in combat, FlagsA's refuses", !spellCanCast(spell, caster, &save, true));
    check("outside combat, FlagsB's refuses", !spellCanCast(spell, caster, &save, false));
}

static void testDeduct(void) {
    SaveGame save;
    uint8_t caster[PartyRecordSize], spell[SpellRecordSize];
    setup(&save, caster, spell, 20, 9, 12);
    setU16(spell, SpellFieldMpCost, 6);
    setU16(spell, SpellFieldNuoreCost, 4);
    setU16(spell, SpellFieldOreCost, 5);

    spellDeductCosts(spell, caster, &save);

    checkU32("MP paid", partyGetStat(caster, PartyStatMagicPoints), 14);
    check("NUORE paid", counterIs(&save, SaveHeaderOreCounter2, 5));
    check("MAGIC ORE paid", counterIs(&save, SaveHeaderOreCounter1, 7));

    setU16(spell, SpellFieldNuoreCost, 50);
    spellDeductCosts(spell, caster, &save);
    check("the ore counters clamp at zero", counterIs(&save, SaveHeaderOreCounter2, 0));

    setup(&save, caster, spell, 2, 0, 0);
    setU16(spell, SpellFieldMpCost, 5);
    spellDeductCosts(spell, caster, &save);
    checkU32("MP is not clamped: the original wraps (castability is supposed to prevent it)",
             partyGetStat(caster, PartyStatMagicPoints), (uint16_t)(2 - 5));
}

static void testLearning(void) {
    uint8_t record[PartyRecordSize], spell[SpellRecordSize];
    memset(record, 0, sizeof(record));
    memset(spell, 0, sizeof(spell));
    setU16(spell, SpellFieldClassEligibility, 0x0030);
    setU16(spell, SpellFieldRequiredLevel, 5);

    partySetU16(record, PartyFieldLevel, 5);
    partySetU16(record, PartyFieldStatusFlags, 0x0010);
    check("matching class bit and enough level can learn it", spellCanLearn(spell, 7, record));

    partySetU16(record, PartyFieldLevel, 4);
    check("one level short can't", !spellCanLearn(spell, 7, record));

    partySetU16(record, PartyFieldLevel, 9);
    partySetU16(record, PartyFieldStatusFlags, 0x0004);
    check("the wrong class can't", !spellCanLearn(spell, 7, record));

    partySetU16(record, PartyFieldStatusFlags, 0x0020 | PartyStatusPoisoned);
    check("other status bits don't matter, only the low class bits", spellCanLearn(spell, 7, record));

    setU16(spell, SpellFieldClassEligibility, 0xFFC0);
    check("class bits above the low six are ignored (mask 0x3F)", !spellCanLearn(spell, 7, record));
    setU16(spell, SpellFieldClassEligibility, 0x0030);

    spellLearn(record, 7);
    check("after learning it, the ability bank has it", partyTestAbilityFlag(record, 7));
    check("...and an already-known spell can't be learned again", !spellCanLearn(spell, 7, record));
    check("...but another can", spellCanLearn(spell, 8, record));
}

static void testContextAlone(void) {
    uint8_t spell[SpellRecordSize];
    memset(spell, 0, sizeof(spell));
    setU16(spell, SpellFieldFlagsA, SpellFlagsANotInCombat);
    check("an exploration spell: usable out of combat, not in", spellUsableInContext(spell, false) && !spellUsableInContext(spell, true));
    memset(spell, 0, sizeof(spell));
    setU16(spell, SpellFieldFlagsB, SpellFlagsBAttackAllSlots);
    check("an attack spell: usable in combat, not out", !spellUsableInContext(spell, false) && spellUsableInContext(spell, true));
}

int main(void) {
    testResourceGates();
    testContextGates();
    testDeduct();
    testLearning();
    testContextAlone();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

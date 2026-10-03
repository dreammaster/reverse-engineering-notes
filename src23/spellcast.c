#include "spellcast.h"

#include "bcd4.h"
#include "party.h"

bool spellCanCast(const uint8_t *spellRecord, const uint8_t *casterRecord, SaveGame *save, bool inCombat) {
    if (inCombat) {
        if (spellGetU16(spellRecord, SpellFieldFlagsA) & SpellFlagsANotInCombat) {
            return false;
        }
    } else if (spellGetU16(spellRecord, SpellFieldFlagsB) & (SpellFlagsBAttackPath | SpellFlagsBAttackAllSlots)) {
        return false;
    }

    uint16_t nuore = spellGetU16(spellRecord, SpellFieldNuoreCost);
    if (nuore != 0 && !bcd4AtLeastU16(saveHeaderBcd4(save, SaveHeaderOreCounter2), nuore)) {
        return false;
    }
    uint16_t ore = spellGetU16(spellRecord, SpellFieldOreCost);
    if (ore != 0 && !bcd4AtLeastU16(saveHeaderBcd4(save, SaveHeaderOreCounter1), ore)) {
        return false;
    }
    return (int16_t)spellGetU16(spellRecord, SpellFieldMpCost) <= (int16_t)partyGetStat(casterRecord, PartyStatMagicPoints);
}

void spellDeductCosts(const uint8_t *spellRecord, uint8_t *casterRecord, SaveGame *save) {
    partySetStat(casterRecord, PartyStatMagicPoints,
                 (uint16_t)(partyGetStat(casterRecord, PartyStatMagicPoints) - spellGetU16(spellRecord, SpellFieldMpCost)));
    Bcd4 amount;
    bcd4FromU16(amount, spellGetU16(spellRecord, SpellFieldNuoreCost));
    bcd4SubClamped(saveHeaderBcd4(save, SaveHeaderOreCounter2), amount);
    bcd4FromU16(amount, spellGetU16(spellRecord, SpellFieldOreCost));
    bcd4SubClamped(saveHeaderBcd4(save, SaveHeaderOreCounter1), amount);
}

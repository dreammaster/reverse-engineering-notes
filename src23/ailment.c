#include "ailment.h"

uint16_t ailmentDiseaseSeverity(const uint8_t *record) {
    uint16_t status = partyGetU16(record, PartyFieldStatusFlags);
    if (status & PartyStatusIncapacitated) {
        return 0;
    }
    return (uint16_t)(((status & PartyStatusDiseased) ? 12 : 0) + ((status & PartyStatusPoisoned) ? 6 : 0) +
                      ((status & PartyStatusSick) ? 3 : 0));
}

uint16_t ailmentCurseSeverity(const uint8_t *record) {
    uint16_t status = partyGetU16(record, PartyFieldStatusFlags);
    if ((status & PartyStatusIncapacitated) || partyGetStat(record, PartyStatMagicPoints) == 0) {
        return 0;
    }
    return (uint16_t)(((status & PartyStatusCursed) ? 16 : 0) + ((status & PartyStatusHexed) ? 8 : 0) +
                      ((status & PartyStatusJinxed) ? 4 : 0));
}

uint16_t ailmentSlowSeverity(const uint8_t *record) {
    if (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead) {
        return 0;
    }
    int16_t survival = (int16_t)partyGetStat(record, PartyStatSurvival);
    if (survival <= 0x37) {
        return 12;
    }
    if (survival <= 0x4B) {
        return 9;
    }
    if (survival <= 0x50) {
        return 6;
    }
    if (survival <= 0x64) {
        return 3;
    }
    return 0; /* <= 0x7D and <= 0x96 both give 0; above that the original returns without staging */
}

static void addStage(AilmentPass *pass, unsigned slot, uint16_t partyId, uint16_t effectId, uint16_t severity, uint16_t magnitude) {
    AilmentStage *s = &pass->stages[pass->count++];
    s->slot = slot;
    s->partyId = partyId;
    s->effectId = effectId;
    s->severity = severity;
    s->magnitude = magnitude;
}

static AilmentPass runSeverityPass(SaveGame *save, AilmentPassKind kind, uint16_t effectId, uint16_t (*severity)(const uint8_t *)) {
    AilmentPass pass;
    pass.kind = kind;
    pass.count = 0;
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        const uint8_t *record = saveGamePartyRecordById(save, id);
        uint16_t amount = record ? severity(record) : 0;
        if (amount) {
            addStage(&pass, slot, id, effectId, amount, 0);
        }
    }
    return pass;
}

static AilmentPass runColdPass(SaveGame *save, RandomState *rng) {
    AilmentPass pass;
    pass.kind = AilmentPassCold;
    pass.count = 0;
    unsigned picked = randomInRange(rng, 3) + 1;
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        const uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated) ||
            partyGetU16(record, AilmentColdProtectionSlot) == AilmentColdProtectionItem) {
            continue;
        }
        unsigned loopCounter = SavePartyMemberSlots - slot;
        if (loopCounter == picked) {
            addStage(&pass, slot, id, AilmentEffectColdPicked, 0, (uint16_t)(partyGetU16(record, PartyFieldLevel) + 4));
        } else {
            addStage(&pass, slot, id, AilmentEffectCold, 0, 0);
        }
    }
    return pass;
}

unsigned ailmentTickIconBar(AilmentClock *clock, SaveGame *save, GameKind game, uint16_t uiFlags1, uint16_t travelFlags,
                            RandomState *rng, AilmentPass passes[4]) {
    unsigned n = 0;
    if (!(uiFlags1 & AilmentUiGateMask)) {
        return 0;
    }
    if (game == GameYendor3) {
        clock->fast++;
        if (clock->fast > 40) {
            clock->fast = 1;
            passes[n++] = runSeverityPass(save, AilmentPassDisease, AilmentEffectSick, ailmentDiseaseSeverity);
            passes[n++] = runSeverityPass(save, AilmentPassCurse, AilmentEffectCurse, ailmentCurseSeverity);
        }
        if (travelFlags & 1) {
            clock->slow++;
            if (clock->slow > 40) {
                clock->slow = 1;
                passes[n++] = runColdPass(save, rng);
            }
        }
        return n;
    }
    if (travelFlags & 2) {
        clock->slow++;
        if ((int16_t)clock->slow >= 0) {
            clock->slow = 1;
            passes[n++] = runSeverityPass(save, AilmentPassSlow, AilmentEffectSick, ailmentSlowSeverity);
        }
    }
    clock->fast++;
    if ((int16_t)clock->fast < 40) {
        return n;
    }
    clock->fast = 1;
    passes[n++] = runSeverityPass(save, AilmentPassDisease, AilmentEffectSick, ailmentDiseaseSeverity);
    passes[n++] = runSeverityPass(save, AilmentPassCurse, AilmentEffectCurse, ailmentCurseSeverity);
    return n;
}

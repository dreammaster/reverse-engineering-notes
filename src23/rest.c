#include "rest.h"

#include "party.h"

RestOutcome restParty(SaveGame *save, GameClock *clock, const ItemCatalog *catalog, uint8_t *globalSlots, bool heldItem, bool noRestFlag, bool inTriggerList,
                      RestMonstersTurn monstersTurn, void *ctx) {
    RestOutcome out = {0};
    if (heldItem) {
        out.refused = true;
        return out;
    }
    if (!gameClockRestAllowed(noRestFlag, inTriggerList)) {
        out.refused = true;
        out.noRest = true;
        return out;
    }
    out.hour = 1;
    for (unsigned slice = 0; slice < 8; slice++) {
        if (monstersTurn && monstersTurn(ctx)) {
            out.interrupted = true;
            break;
        }
        out.minutes += 60;
        out.hour++;
    }
    if (!out.interrupted) {
        out.hour--;
    }
    out.dayRolled = gameClockAdvance(clock, (uint16_t)out.minutes);
    if (out.interrupted) {
        return out;
    }

    unsigned active = 0;
    for (unsigned slot = 0; slot < 4; slot++) {
        unsigned id = saveHeaderGetU16(save, SaveHeaderPartySlots + 2 * slot);
        const uint8_t *record = id ? saveGamePartyRecordById(save, id) : NULL;
        if (!record) {
            break;
        }
        if (!(partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
            active++;
        }
    }
    out.regenPercent = partyDeriveRestRegenPercent(globalSlots, save, catalog);
    out.fed = active && out.regenPercent ? out.regenPercent / (100u / active) : 0;
    for (unsigned slot = 0; slot < 4; slot++) {
        unsigned id = saveHeaderGetU16(save, SaveHeaderPartySlots + 2 * slot);
        uint8_t *record = id ? saveGamePartyRecordById(save, id) : NULL;
        if (!record) {
            break;
        }
        PartyRestOutcome rested = partyApplyRestEffects(record, out.regenPercent);
        out.died += rested.died;
    }
    return out;
}

#include "dialogservice.h"

#include <string.h>

#include "bcd4.h"
#include "globalflags.h"
#include "party.h"

static uint16_t clampToTypeCap(uint16_t statOffset, uint16_t value) {
    int16_t cap = (statOffset == 0x52 || statOffset == 0x54) ? 0x270F : 0x3E7;
    return (int16_t)value > cap ? (uint16_t)cap : value;
}

DialogTomeResult dialogApplyAttributeTome(const uint8_t *npc, SaveGame *save, uint8_t *globalFlags, size_t flagsSize) {
    DialogTomeResult result = {false, 0};
    unsigned flag = dialogGetU16(npc, DialogNpcOneTimeFlag);
    if (globalFlagTest(globalFlags, flagsSize, flag)) {
        return result;
    }
    result.applied = true;
    globalFlagSet(globalFlags, flagsSize, flag);

    uint16_t statOffset = dialogGetU16(npc, DialogNpcParamA);
    uint16_t amount = dialogGetU16(npc, DialogNpcParamB);
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead)) {
            continue;
        }
        int16_t current = (int16_t)partyGetU16(record, statOffset);
        if (current <= 0) {
            continue;
        }
        partySetU16(record, statOffset, clampToTypeCap(statOffset, (uint16_t)(current + amount)));
        partySetU16(record, statOffset + 0x40,
                    clampToTypeCap(statOffset, (uint16_t)(partyGetU16(record, statOffset + 0x40) + amount)));
        partyRefreshCarryCapacityAndAttributeBonuses(record);
        result.members++;
    }
    return result;
}

DialogTomeResult dialogApplyExperienceTome(const uint8_t *npc, SaveGame *save, GameKind game, uint8_t *globalFlags,
                                           size_t flagsSize) {
    DialogTomeResult result = {false, 0};
    unsigned flag = dialogGetU16(npc, DialogNpcOneTimeFlag);
    if (globalFlagTest(globalFlags, flagsSize, flag)) {
        return result;
    }
    result.applied = true;
    globalFlagSet(globalFlags, flagsSize, flag);

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusDead)) {
            continue;
        }
        bcd4Add(record + PartyFieldExperience, npc + DialogNpcParamA);
        partyCheckForLevelUp(record, game);
        result.members++;
    }
    return result;
}

void dialogClassifyCondition(DialogState *state, const uint8_t *partyRecord) {
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    uint16_t bits = 0;
    unsigned tiers = 0;
    if (status & PartyStatusDead) {
        bits |= 0x2000;
    }
    if (status & 0xFF80) {
        bits |= 0x4000;
        tiers++;
    }
    if ((int16_t)partyGetStat(partyRecord, PartyStatHitPoints) < (int16_t)partyGetStatMax(partyRecord, PartyStatHitPoints)) {
        bits |= 0x8000;
        tiers++;
    }
    if (tiers > 1) {
        bits |= 0x1000;
    } else if (tiers < 1) {
        bits |= 0x200;
    }
    state->availA = (uint16_t)((state->availA & 0x3FF) | bits);
}

unsigned dialogAfflictionCost(const uint8_t *partyRecord) {
    static const struct {
        uint16_t bit;
        unsigned cost;
    } table[] = {{0x8000, 5},  {0x4000, 10}, {0x2000, 20}, {0x1000, 40}, {0x0800, 50},
                 {0x0400, 60}, {0x0200, 20}, {0x0100, 30}, {0x0080, 40}};
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    unsigned total = 0;
    for (unsigned i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (status & table[i].bit) {
            total += table[i].cost;
        }
    }
    return total;
}

/* ShowHealingCostPrompt's arithmetic: (base x multiplier, truncated to 16 bits) summed once per level, in BCD. */
static void priceByLevel(const uint8_t *npc, unsigned base, const uint8_t *partyRecord, Bcd4 cost) {
    uint16_t perLevel = (uint16_t)(base * dialogGetU16(npc, DialogNpcPriceMultiplier));
    memset(cost, 0, sizeof(Bcd4));
    for (unsigned i = 0; i < partyGetU16(partyRecord, PartyFieldLevel); i++) {
        bcd4AddU16(cost, perLevel);
    }
}

void dialogHealingCost(const uint8_t *npc, unsigned type, const DialogState *state, const uint8_t *partyRecord,
                       Bcd4 cost) {
    unsigned base = 0;
    if (type & DialogHealFull) {
        if (state->availA & 0x4000) {
            base = dialogAfflictionCost(partyRecord);
        }
        if (state->availA & 0x8000) {
            base += 20;
        }
        if (state->availA & 0x2000) {
            base += 100;
        }
    } else if (type & DialogHealRevive) {
        base = 100;
    } else if (type & DialogHealCure) {
        base = dialogAfflictionCost(partyRecord);
    } else if (type & DialogHealHp) {
        base = 20;
    }
    priceByLevel(npc, base, partyRecord, cost);
}

bool dialogApplyHealing(unsigned type, const Bcd4 cost, Bcd4 gold, uint8_t *partyRecord, GameKind game) {
    if (bcd4Compare(gold, cost) < 0) {
        return false;
    }
    bcd4Sub(gold, cost);
    uint16_t status = partyGetU16(partyRecord, PartyFieldStatusFlags);
    if (type & DialogHealRevive) {
        status = (uint16_t)(status & ~PartyStatusDead);
        partySetStat(partyRecord, PartyStatHitPoints, 2);
    }
    if (type & DialogHealCure) {
        status = (uint16_t)(status & 0x7F);
    }
    if (type & DialogHealHp) {
        partySetStat(partyRecord, PartyStatHitPoints, partyGetStatMax(partyRecord, PartyStatHitPoints));
    }
    if (type & DialogHealFull) {
        partySetStat(partyRecord, PartyStatHitPoints, partyGetStatMax(partyRecord, PartyStatHitPoints));
        status = (uint16_t)(status & 0x3F);
    }
    partySetU16(partyRecord, PartyFieldStatusFlags, status);
    partyCheckForLevelUp(partyRecord, game);
    return true;
}

bool dialogTrainingQuote(const uint8_t *npc, const uint8_t *partyRecord, Bcd4 cost) {
    if ((int16_t)(partyGetU16(partyRecord, PartyFieldLevel) + 1) > (int16_t)dialogGetU16(npc, DialogNpcParamB)) {
        return false;
    }
    priceByLevel(npc, 100, partyRecord, cost);
    return true;
}

DialogChallengeOutcome dialogAttemptChallenge(const uint8_t *npc, uint8_t *partyRecord, Bcd4 gold, Bcd4 reward) {
    Bcd4 fee;
    dialogNpcFee(npc, fee);
    if (bcd4Compare(gold, fee) < 0) {
        return DialogChallengeNoGold;
    }
    bcd4Sub(gold, fee);
    memset(reward, 0, sizeof(Bcd4));
    int16_t stat = (int16_t)partyGetU16(partyRecord, dialogGetU16(npc, DialogNpcParamA));
    if ((int16_t)dialogGetU16(npc, DialogNpcParamB) > stat) {
        return DialogChallengeLost;
    }
    for (unsigned i = 0; i < dialogGetU16(npc, DialogNpcPriceMultiplier); i++) {
        bcd4Add(gold, fee);
        bcd4Add(reward, fee);
    }
    flagBankSet(partyRecord + PartyFieldFlagBank10C, 6, dialogGetU16(npc, DialogNpcCharacterFlagIndex));
    return DialogChallengeWon;
}

void dialogMarkServiceAvailability(DialogState *state, const uint8_t *npc, const uint8_t *partyRecord) {
    uint16_t a = (uint16_t)((state->availA & 0x1FFF) | 0x1000);
    if (!flagBankTest(partyRecord + PartyFieldFlagBank10C, 6, dialogGetU16(npc, DialogNpcCharacterFlagIndex))) {
        a |= 0x8000;
    }
    state->availA = a;
}

const char *dialogRiddleAnswer(GameKind game, unsigned riddleId) {
    static const char *const answers2[] = {"PENTAGON", "LINGUISTIC", "WHITE POTION", "THAINE", "SHIRLEY", "GAIN"};
    static const char *const answers3[] = {"PEACEFUL", "120",   "ARCHIBALD", "OVIAS",  "WIN",   "30",
                                           "500",      "3925",  "46080",     "400000", "70"};
    const char *const *table = game == GameYendor2 ? answers2 : answers3;
    unsigned count = game == GameYendor2 ? sizeof(answers2) / sizeof(answers2[0]) : sizeof(answers3) / sizeof(answers3[0]);
    if (riddleId == 0 || riddleId > count) {
        return NULL;
    }
    return table[riddleId - 1];
}

bool dialogCheckRiddleAnswer(GameKind game, unsigned riddleId, const char *typed) {
    const char *answer = dialogRiddleAnswer(game, riddleId);
    return answer && strcmp(answer, typed) == 0;
}

bool dialogBuyOre(unsigned kind, Bcd4 gold, Bcd4 magicOre, Bcd4 nuore, const Bcd4 quantity) {
    Bcd4 affordable;
    memcpy(affordable, gold, sizeof(Bcd4));
    bcd4ShiftRightNibble(affordable);
    if (bcd4Compare(affordable, quantity) < 0) {
        return false;
    }
    bcd4Add(kind == 2 ? magicOre : nuore, quantity);
    Bcd4 cost;
    memcpy(cost, quantity, sizeof(Bcd4));
    bcd4ShiftLeftNibble(cost);
    bcd4Sub(gold, cost);
    return true;
}

bool dialogSellAccepts(uint16_t sellTopicArg, const uint8_t *itemRecord) {
    return (sellTopicArg & itemGetU16(itemRecord, ItemFieldClass)) != 0;
}

bool dialogEnhanceEligible(const uint8_t *npc, const ItemCatalog *catalog, const uint8_t *itemRecord) {
    uint16_t flags = itemGetU16(itemRecord, ItemFieldFlags);
    const uint8_t *entry = itemTargetEntry(catalog, itemRecord);
    int16_t value;
    if (flags & 0x0A00) {
        if (!entry || !(itemTargetWord(entry, 1) & 0x0100)) {
            return false;
        }
        value = (int16_t)itemTargetWord(entry, 3);
    } else if (flags & 0xC000) {
        if (!entry || !(itemTargetWord(entry, 1) & 0x0800)) {
            return false;
        }
        value = (int16_t)itemTargetWord(entry, 4);
    } else {
        return false;
    }
    return value >= (int16_t)dialogGetU16(npc, DialogNpcParamA) && value <= (int16_t)dialogGetU16(npc, DialogNpcParamB);
}

static bool priceOfItemAtPercent(const uint8_t *npc, const ItemCatalog *catalog, unsigned itemId, Bcd4 cost) {
    const uint8_t *record = itemCatalogRecord(catalog, itemId);
    if (!record) {
        return false;
    }
    memcpy(cost, itemBaseValue(record), sizeof(Bcd4));
    bcd4MulPercent(cost, dialogGetU16(npc, DialogNpcPriceMultiplier));
    return true;
}

bool dialogEnhanceCost(const uint8_t *npc, const ItemCatalog *catalog, unsigned itemId, Bcd4 cost) {
    return priceOfItemAtPercent(npc, catalog, itemId + 1, cost);
}

bool dialogRepairEligible(const ItemCatalog *catalog, const uint8_t *itemRecord) {
    uint16_t flags = itemGetU16(itemRecord, ItemFieldFlags);
    const uint8_t *entry = itemTargetEntry(catalog, itemRecord);
    if (!entry) {
        return false;
    }
    if ((flags & 0xC000) && (itemTargetWord(entry, 1) & 0x0100)) {
        return true;
    }
    return (flags & 0x0800) && (itemTargetWord(entry, 1) & 0x0040);
}

bool dialogRepairCost(const uint8_t *npc, const ItemCatalog *catalog, unsigned originalItemId, Bcd4 cost) {
    return priceOfItemAtPercent(npc, catalog, originalItemId, cost);
}

static const DialogTransport g_transports[DialogTransportCount] = {
    {0x8000, "PEGASUS", 5000}, {0x4000, "GIANT EAGLE", 15000}, {0x2000, "FLYING RUG", 25000}, {0x1000, "MAGIC DRAGON", 35000}};

const DialogTransport *dialogTransport(unsigned index) {
    return index < DialogTransportCount ? &g_transports[index] : NULL;
}

const DialogTransport *dialogTransportForMask(uint16_t mask) {
    for (unsigned i = 0; i < DialogTransportCount; i++) {
        if (mask & g_transports[i].mask) {
            return &g_transports[i];
        }
    }
    return NULL;
}

DialogTransportResult dialogLearnTransport(uint16_t mask, uint8_t *partyRecord, Bcd4 gold) {
    const DialogTransport *transport = dialogTransportForMask(mask);
    if (!transport) {
        return DialogTransportNoGold;
    }
    if (partyGetU16(partyRecord, PartyFieldAbilities) & mask) {
        return DialogTransportAlreadyKnown;
    }
    Bcd4 price;
    bcd4FromU16(price, transport->price);
    if (bcd4Compare(gold, price) < 0) {
        return DialogTransportNoGold;
    }
    bcd4Sub(gold, price);
    partySetU16(partyRecord, PartyFieldAbilities, (uint16_t)(partyGetU16(partyRecord, PartyFieldAbilities) | mask));
    for (unsigned i = 0; i < DialogTransportCount; i++) {
        if (mask & g_transports[i].mask) {
            partySetU16(partyRecord, PartyFieldAbilityCharge + i * 2, 0);
            break;
        }
    }
    return DialogTransportLearned;
}

void dialogSellTransport(uint16_t mask, uint8_t *partyRecord, Bcd4 gold) {
    const DialogTransport *transport = dialogTransportForMask(mask);
    partySetU16(partyRecord, PartyFieldAbilities, (uint16_t)(partyGetU16(partyRecord, PartyFieldAbilities) & ~mask));
    if (transport) {
        bcd4AddU16(gold, transport->price);
    }
}

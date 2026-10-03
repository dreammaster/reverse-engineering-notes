#include "relics.h"

#include "globalflags.h"
#include "monster.h"
#include "party.h"

bool relicReady(const uint8_t *globalFlags, size_t flagsSize) {
    return globalFlagTest(globalFlags, flagsSize, RelicRechargeFlag);
}

void relicAddCache(Bcd4 counter) {
    bcd4AddU16(counter, RelicCacheAmount);
}

unsigned relicMassHealAndOverheal(SaveGame *save) {
    unsigned affected = 0;
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record) {
            continue;
        }
        partySetU16(record, PartyFieldStatusFlags, (uint16_t)(partyGetU16(record, PartyFieldStatusFlags) & 0x3F));
        partySetStat(record, PartyStatHitPoints, (uint16_t)(partyGetStatMax(record, PartyStatHitPoints) << 1));
        partySetStat(record, PartyStatMagicPoints, (uint16_t)(partyGetStatMax(record, PartyStatMagicPoints) << 1));
        partyRefreshCarryCapacityAndAttributeBonuses(record);
        affected++;
    }
    return affected;
}

void relicInstantKill(uint8_t *monsterRecord) {
    monsterSetU16(monsterRecord, MonsterFieldHealth, 0);
}

RelicAction relicClassify(unsigned itemId) {
    switch (itemId) {
    case RelicCrystalBall:
        return RelicActionVision;
    case RelicLocationPotion:
        return RelicActionLocationPotion;
    case RelicEmptyBottle:
        return RelicActionAssemblePotion;
    case 0x242:
    case 0x243:
    case 0x244:
    case 0x245:
        return RelicActionDiscoveryKey;
    case RelicPanFlute:
        return RelicActionPlayFlute;
    case RelicNuoreCache:
    case RelicMagicOreCache:
    case RelicMassHeal:
    case RelicInstantKill:
        return RelicActionCharged;
    default:
        return RelicActionNone;
    }
}

static void consumeItem(uint8_t *globalSlots, SaveGame *save, const ItemCatalog *catalog, uint16_t id) {
    ItemRangeAvailability avail = itemRangeAvailable(globalSlots, save, id, id);
    if (!avail.found) {
        return;
    }
    if (avail.inGlobalTable) {
        itemSlotConsumeGlobalCharge(catalog, globalSlots + avail.slotOffset);
    } else {
        uint8_t *record = saveGamePartyRecordById(save, avail.partyRecordId);
        partyConsumeItemCharge(record, catalog, record + avail.slotOffset);
    }
}

bool relicAssemblePotion(uint8_t *globalSlots, SaveGame *save, const ItemCatalog *catalog) {
    static const uint16_t ingredients[4] = {RelicFlower, RelicCocoon, RelicFeather, RelicOrange};
    for (unsigned i = 0; i < 4; i++) {
        if (!itemRangeAvailable(globalSlots, save, ingredients[i], ingredients[i]).found) {
            return false;
        }
    }
    consumeItem(globalSlots, save, catalog, RelicOrange);
    consumeItem(globalSlots, save, catalog, RelicFeather);
    consumeItem(globalSlots, save, catalog, RelicCocoon);
    consumeItem(globalSlots, save, catalog, RelicFlower);
    consumeItem(globalSlots, save, catalog, RelicEmptyBottle);
    return true;
}

bool relicUseLocationPotion(int worldX, int worldY, uint8_t *globalFlags, size_t flagsSize) {
    if (worldX != LocationPotionX || worldY != LocationPotionY) {
        return false;
    }
    globalFlagSet(globalFlags, flagsSize, LocationPotionFlag);
    return true;
}

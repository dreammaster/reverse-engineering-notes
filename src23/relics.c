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

bool relicUseLocationPotion(int worldX, int worldY, uint8_t *globalFlags, size_t flagsSize) {
    if (worldX != LocationPotionX || worldY != LocationPotionY) {
        return false;
    }
    globalFlagSet(globalFlags, flagsSize, LocationPotionFlag);
    return true;
}

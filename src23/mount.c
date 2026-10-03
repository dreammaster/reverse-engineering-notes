#include "mount.h"

#include "party.h"

static const MountAbility g_mounts[MountCount] = {
    {"PEGASUS", 0x8000, 1, 3}, {"GIANT EAGLE", 0x4000, 2, 3}, {"FLYING RUG", 0x2000, 4, 2}, {"MAGIC DRAGON", 0x1000, 4, 1}};

const MountAbility *mountAbility(unsigned slot) {
    return slot < MountCount ? &g_mounts[slot] : NULL;
}

MountCheckResult mountCheck(const uint8_t *partyRecord, unsigned ability, uint16_t clockMinutes, unsigned *slot) {
    if (ability < 2 || ability > 5) {
        return MountCheckNotAMount;
    }
    unsigned wanted = ability - 1;
    uint16_t learned = partyGetU16(partyRecord, PartyFieldAbilities);
    unsigned found = MountCount;
    for (unsigned i = 0; i < MountCount; i++) {
        if (learned & g_mounts[i].mask) {
            if (--wanted == 0) {
                found = i;
                break;
            }
        }
    }
    if (found == MountCount) {
        return MountCheckNotLearned;
    }
    if (slot) {
        *slot = found;
    }
    int16_t charge = (int16_t)partyGetU16(partyRecord, PartyFieldAbilityCharge + found * 2);
    if (!((int16_t)g_mounts[found].dailyCharges > charge)) {
        return MountCheckNoCharges;
    }
    bool day = clockMinutes >= MountDayStartMinutes && clockMinutes <= MountDayEndMinutes;
    if (!(g_mounts[found].timeWord & (day ? 2 : 1))) {
        return MountCheckWrongTime;
    }
    return MountCheckOk;
}

void mountSpendCharge(uint8_t *partyRecord, unsigned slot) {
    unsigned offset = PartyFieldAbilityCharge + slot * 2;
    partySetU16(partyRecord, offset, (uint16_t)(partyGetU16(partyRecord, offset) + 1));
}

MountBox mountRevealBox(uint16_t navigationAverage) {
    int16_t tier = (int16_t)navigationAverage;
    MountBox box = {11, 7, 0x4C, 0x30};
    if (tier >= 0x41) {
        box = (MountBox){17, 9, 0x34, 0x28};
        if (tier >= 0x50) {
            box = (MountBox){23, 13, 0x1C, 0x18};
            if (tier >= 0x5F) {
                box = (MountBox){27, 17, 0x0C, 8};
            }
        }
    }
    return box;
}

void mountBoxOrigin(const MountBox *box, int partyX, int partyY, int *originX, int *originY) {
    *originX = partyX - (int)((box->columns - 1) >> 1);
    *originY = partyY - (int)((box->rows - 1) >> 1);
}

bool mountCellFromClick(const MountBox *box, int originX, int originY, int clickX, int clickY, int *cellX, int *cellY) {
    int col = ((clickX - box->pixelX) >> 3) + 1;
    int row = ((clickY - box->pixelY) >> 3) + 1;
    if (col < 1 || col > (int)box->columns || row < 1 || row > (int)box->rows) {
        return false;
    }
    *cellX = originX + col - 1;
    *cellY = originY + row - 1;
    return true;
}

MountTravelResult mountTravelVerdict(GameKind game, bool samePage, uint8_t originPageAttr, uint8_t destPageAttr, bool explored,
                                     bool floorTypeImpassable, bool wallTypeBlocked, InteractOutcome interact) {
    if (game == GameYendor3 && !samePage && (originPageAttr == 2 || destPageAttr == 2)) {
        return MountTravelOutside;
    }
    if (!explored) {
        return MountTravelUnexplored;
    }
    if (floorTypeImpassable) {
        return MountTravelFloorBlocked;
    }
    if (wallTypeBlocked) {
        return MountTravelWallBlocked;
    }
    if (interact == InteractOutcomeCurgameFlag10 || interact == InteractOutcomeCurgameFlag8) {
        return MountTravelInteractive;
    }
    return MountTravelOk;
}

bool mountCellVisible(bool samePage, uint8_t originPageAttr, uint8_t destPageAttr) {
    if (samePage) {
        return true;
    }
    return originPageAttr == 1 && destPageAttr != 2;
}

uint8_t mountPageAttribute(GameKind game, const uint8_t *worldDat, size_t size, unsigned page) {
    size_t base = game == GameYendor3 ? PageTableOffsetYendor3 : PageTableOffsetYendor2;
    size_t at = base + (size_t)page * PageTableRecordSize + 5;
    return at < size ? worldDat[at] : 0;
}

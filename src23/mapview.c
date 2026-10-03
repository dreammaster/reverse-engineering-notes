#include "mapview.h"

PartyStatAverages partyAverageStatTiers(SaveGame *save) {
    PartyStatAverages out = {0, 0, 0, 0};
    uint16_t mapping = 0, navigation = 0, survival = 0;
    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        uint16_t id = saveGetPartySlot(save, slot);
        if (id == 0) {
            break;
        }
        const uint8_t *record = saveGamePartyRecordById(save, id);
        if (!record || (partyGetU16(record, PartyFieldStatusFlags) & PartyStatusIncapacitated)) {
            continue;
        }
        mapping = (uint16_t)(mapping + partyGetStat(record, PartyStatMapping));
        navigation = (uint16_t)(navigation + partyGetStat(record, PartyStatNavigation));
        survival = (uint16_t)(survival + partyGetStat(record, PartyStatSurvival));
        out.counted++;
    }
    if (out.counted) {
        out.mapping = (uint16_t)(mapping / out.counted);
        out.navigation = (uint16_t)(navigation / out.counted);
        out.survival = (uint16_t)(survival / out.counted);
    }
    return out;
}

uint16_t mapviewApplyTiers(uint16_t oldFlags, uint16_t mappingAverage, bool *redrawStatusPanel) {
    uint16_t flags = (uint16_t)(oldFlags & 0x70FF);
    int16_t avg = (int16_t)mappingAverage;
    if (avg >= MapTierExtended) {
        flags |= MapFlagExtended;
        if (avg >= MapTierLargeCapable) {
            flags |= MapFlagLargeCapable;
            if (avg >= MapTierLocalMap) {
                flags |= MapFlagLocalMap;
                if (avg >= MapTierCoordinates) {
                    flags |= MapFlagCoordinates;
                    if (avg >= MapTierFullOverview) {
                        flags |= MapFlagFullOverview;
                    }
                }
            }
        }
    }
    if (redrawStatusPanel) {
        *redrawStatusPanel = false;
    }
    if ((flags & MapFlagLarge) && !(flags & MapFlagLargeCapable)) {
        flags = (uint16_t)((flags & 0x7FFF) | MapFlagHidden);
        if (redrawStatusPanel) {
            *redrawStatusPanel = true;
        }
    }
    return flags;
}

bool mapviewLocalMapAllowed(GameKind game, uint16_t mapFlags, uint16_t uiScratchFlags1) {
    if (uiScratchFlags1 & 0x0001) {
        return true;
    }
    if (game == GameYendor3 && (uiScratchFlags1 & 0x8000)) {
        return true;
    }
    return (mapFlags & MapFlagLocalMap) != 0;
}

bool mapviewOverviewAllowed(uint16_t mapFlags) {
    return (mapFlags & MapFlagFullOverview) != 0;
}

MapviewPage mapviewPage(int worldX, int worldY, uint16_t facing) {
    MapviewPage page;
    page.pageIndex = (unsigned)(worldY / MapviewPageRows) * MapviewPagesAcross + (unsigned)(worldX / MapviewPageColumns);
    page.originX = worldX / MapviewPageColumns * MapviewPageColumns;
    page.originY = worldY / MapviewPageRows * MapviewPageRows;
    page.markerPixelX = (worldX % MapviewPageColumns) * 8;
    page.markerPixelY = (worldY % MapviewPageRows + 1) * 8;
    if (facing & SaveFacingNorth) {
        page.arrowPicture = 0;
    } else if (facing & SaveFacingSouth) {
        page.arrowPicture = 2;
    } else if (facing & SaveFacingEast) {
        page.arrowPicture = 1;
    } else {
        page.arrowPicture = 3;
    }
    return page;
}

bool mapviewOverviewMarker(int worldX, int worldY, int *pixelX, int *pixelY) {
    if (worldX < 0xA0 || worldY < 0x30 || worldX > 0x27F || worldY > 0xEF) {
        return false;
    }
    *pixelX = (worldX - 0xA0) / 40 * 24 + 0x1C;
    *pixelY = (worldY - 0x30) / 24 * 20 + 0x17;
    return true;
}

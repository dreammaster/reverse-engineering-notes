#include "newgame.h"

#include <string.h>

#include "party.h"

bool saveGameNewGame(SaveGame *save, GameKind game, const uint8_t *worldDat, size_t size) {
    size_t offset = game == GameYendor3 ? NewGameTemplateOffsetYendor3 : NewGameTemplateOffsetYendor2;
    if (size < offset + NewGameTemplateSize) {
        return false;
    }
    saveGameInit(save, game);
    uint8_t *section = saveGameSection(save, SaveSectionHeaderAndParty);
    memcpy(section, worldDat + offset, NewGameTemplateSize);

    for (unsigned slot = 0; slot < SavePartyMemberSlots; slot++) {
        saveHeaderSetU16(save, SaveHeaderPartySlots + slot * 2, 0);
    }
    for (unsigned index = 0; index < 9; index++) {
        uint8_t *record = saveGameRecord(save, SaveSectionHeaderAndParty, index + 1);
        partySetU16(record, PartyFieldUiFlags, (uint16_t)(partyGetU16(record, PartyFieldUiFlags) & 0xF7FF));
    }

    if (game == GameYendor3) {
        saveHeaderSetU16(save, SaveHeaderFacing, SaveFacingNorth);
        saveHeaderSetU16(save, SaveHeaderWorldX, 460);
        saveHeaderSetU16(save, SaveHeaderWorldY, 46);
        saveHeaderSetU16(save, SaveHeaderGameMonth, 3);
        saveHeaderSetU16(save, SaveHeaderGameDay, 20);
        saveHeaderSetU16(save, SaveHeaderGameYear, 547);
        saveHeaderSetU16(save, SaveHeaderClockMinutes, 540);
        saveHeaderSetU16(save, 0x1B0, 0);
    } else {
        saveHeaderSetU16(save, SaveHeaderFacing, SaveFacingWest);
        saveHeaderSetU16(save, SaveHeaderWorldX, 166);
        saveHeaderSetU16(save, SaveHeaderWorldY, 36);
        saveHeaderSetU16(save, SaveHeaderGameDay, 4);
        saveHeaderSetU16(save, SaveHeaderGameMonth, 11);
        saveHeaderSetU16(save, SaveHeaderGameYear, 546);
        saveHeaderSetU16(save, SaveHeaderClockMinutes, 420);
        saveHeaderSetU16(save, 0x88, 5);
    }
    return true;
}

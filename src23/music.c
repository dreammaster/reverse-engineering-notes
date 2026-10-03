#include "music.h"

unsigned musicPageForPosition(int x, int y) {
    return (unsigned)(y / 24) * 20 + (unsigned)(x / 40);
}

uint16_t musicPageTrack(GameKind game, const uint8_t *worldDat, size_t size, unsigned page) {
    size_t at = (game == GameYendor3 ? MusicTableOffsetYendor3 : MusicTableOffsetYendor2) + (size_t)page * 2;
    if (at + 2 > size) {
        return 0;
    }
    return (uint16_t)(worldDat[at] | (worldDat[at + 1] << 8));
}

bool musicRegionChanged(unsigned *lastPage, int x, int y, GameKind game, const uint8_t *worldDat, size_t size, uint16_t *track) {
    unsigned page = musicPageForPosition(x, y);
    if (page == *lastPage) {
        return false;
    }
    *lastPage = page;
    *track = musicPageTrack(game, worldDat, size, page);
    return true;
}

uint16_t musicAmbientChoice(uint16_t forcedTrack, uint16_t dayTrack, uint16_t nightTrack, uint16_t clockMinutes, uint16_t uiFlags1) {
    if (forcedTrack != 0) {
        return forcedTrack;
    }
    uint16_t track = nightTrack;
    if ((int16_t)clockMinutes >= MusicDayStartMinutes && (int16_t)clockMinutes <= MusicDayEndMinutes) {
        track = dayTrack;
    }
    if (track == 0 || !(uiFlags1 & MusicFlagAmbientAllowed)) {
        return 0;
    }
    return track;
}

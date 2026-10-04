#include "titlemenu.h"

TitleAction titleKeyAction(uint8_t key) {
    switch (key) {
    case 'C':
        return TitleManageParty;
    case 'A':
        return TitleWorldMap;
    case 'E':
        return TitleEnterGame;
    case 'R':
        return TitleIntro;
    case 'I':
        return TitleCreateCharacter;
    case 0x0D:
        return TitleToggleMusic;
    case 0x13:
        return TitleToggleSoundFx;
    case 0x11:
        return TitleQuit;
    default:
        return TitleNone;
    }
}

TitleAction titleRegionAction(unsigned region) {
    static const TitleAction kByRegion[5] = {TitleManageParty, TitleWorldMap, TitleEnterGame, TitleIntro, TitleCreateCharacter};
    return region >= 1 && region <= 5 ? kByRegion[region - 1] : TitleNone;
}

bool titleCanEnter(unsigned slotAssignmentSum) {
    return slotAssignmentSum != 0;
}

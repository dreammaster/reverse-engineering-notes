#include "gamedialog.h"

#include "font.h"

GameDialogOption gameDialogKeyOption(uint8_t key) {
    switch (key) {
    case 'S':
        return GameDialogSave;
    case 'L':
        return GameDialogLoad;
    case 'N':
        return GameDialogNewGame;
    case 'D':
        return GameDialogDos;
    case 'M':
        return GameDialogMusic;
    case 'F':
        return GameDialogSoundFx;
    case 'A':
        return GameDialogAnimation;
    case 'R':
    case 0x1B:
        return GameDialogReturn;
    default:
        return GameDialogNone;
    }
}

GameDialogOption gameDialogRegionOption(unsigned region) {
    static const GameDialogOption kByRegion[8] = {GameDialogSave,  GameDialogLoad,     GameDialogNewGame,   GameDialogDos,
                                                  GameDialogMusic, GameDialogSoundFx, GameDialogAnimation, GameDialogReturn};
    return region >= 7 && region <= 14 ? kByRegion[region - 7] : GameDialogNone;
}

bool gameDialogOptionEnabled(GameDialogOption option, unsigned uiFlags, unsigned driverFlags) {
    switch (option) {
    case GameDialogSave:
        return (uiFlags & GameDialogFlagSave) != 0;
    case GameDialogLoad:
        return (uiFlags & GameDialogFlagLoad) != 0;
    case GameDialogNewGame:
        return (uiFlags & GameDialogFlagNewGame) != 0;
    case GameDialogDos:
        return (uiFlags & GameDialogFlagDos) != 0;
    case GameDialogMusic:
        return (driverFlags & DriverMusicAvailable) != 0;
    case GameDialogSoundFx:
        return (driverFlags & DriverSoundFxAvailable) != 0;
    case GameDialogAnimation:
        return (uiFlags & GameDialogFlagAnimation) != 0;
    case GameDialogReturn:
        return (uiFlags & GameDialogFlagReturn) != 0;
    default:
        return false;
    }
}

unsigned gameDialogToggleMusic(unsigned driverFlags, bool *changed) {
    *changed = (driverFlags & DriverMusicAvailable) != 0;
    return *changed ? driverFlags ^ DriverMusicOn : driverFlags;
}

unsigned gameDialogToggleSoundFx(unsigned driverFlags, bool *changed) {
    *changed = (driverFlags & DriverSoundFxAvailable) != 0;
    return *changed ? driverFlags ^ DriverSoundFxOn : driverFlags;
}

unsigned gameDialogNextAnimationSpeed(unsigned speed) {
    return speed == 1 ? 9 : speed == 5 ? 1 : 5;
}

const char *gameDialogAnimationSpeedName(unsigned speed) {
    return speed == 1 ? "FAST  " : speed == 5 ? "MEDIUM" : "SLOW  ";
}

typedef struct {
    GameDialogOption option;
    int x, y;
    const char *label;
} DialogLabel;

void gameDialogDraw(const ViewRenderer *r, unsigned uiFlags, unsigned driverFlags, unsigned animationSpeed) {
    static const DialogLabel kLabels[] = {{GameDialogSave, 44, 95, "SAVE"},    {GameDialogLoad, 81, 95, "LOAD"},     {GameDialogNewGame, 118, 95, "NEW GAME"},
                                          {GameDialogDos, 179, 95, "DOS"},     {GameDialogMusic, 64, 107, "MUSIC"},  {GameDialogSoundFx, 119, 107, "SOUND FX"},
                                          {GameDialogReturn, 166, 119, "RETURN"}};
    viewDrawPicture(r, 1, 0, 24, 23, true, 0);
    for (unsigned i = 0; i < sizeof(kLabels) / sizeof(kLabels[0]); i++) {
        if (!gameDialogOptionEnabled(kLabels[i].option, uiFlags, driverFlags)) {
            fontDrawString(r->game, 0, r->screen, ViewScreenWidth, kLabels[i].x, kLabels[i].y, kLabels[i].label, 6, 0, FontTransparent);
        }
    }
    if (uiFlags & GameDialogFlagAnimation) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 97, 119, gameDialogAnimationSpeedName(animationSpeed), 0x8A, 4, FontOpaque);
    } else {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 38, 119, "ANIMATION", 6, 0, FontTransparent);
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 97, 119, "      ", 4, 4, FontOpaque);
    }
    if (driverFlags & DriverSoundFxOn) {
        viewDrawPicture(r, 9, 0x12, 170, 106, false, 0);
    }
    if (driverFlags & DriverMusicOn) {
        viewDrawPicture(r, 9, 0x12, 97, 106, false, 0);
    }
}

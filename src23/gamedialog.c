#include "gamedialog.h"

#include "font.h"
#include "uiregions.h"

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

const char gameDialogEmptySlotName[] = "        - EMPTY -       ";

void gameDialogSlotText(const ViewRenderer *r, unsigned slot, const char *text, bool highlighted) {
    unsigned count;
    const uint16_t(*regions)[5] = uiRegionEntries(r->game, UiRegionsGameDialog, &count);
    if (slot < SaveSlotCount && slot < count) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, regions[slot][0] + 12, regions[slot][2] + 1, text, highlighted ? 0x7B : 0x0F, 4, FontOpaque);
    }
}

void gameDialogSlotsDraw(const ViewRenderer *r, const SaveSlotEntry slots[SaveSlotCount]) {
    for (unsigned i = 0; i < SaveSlotCount; i++) { /* DrawGameDialogMenuLabelsHighlighted writes all six, an empty slot with its "- EMPTY -" */
        gameDialogSlotText(r, i, slots[i].used ? slots[i].name : gameDialogEmptySlotName, slots[i].highlighted);
    }
}

void gameDialogSlotMark(const ViewRenderer *r, unsigned slot, bool checked) {
    unsigned count;
    const uint16_t(*regions)[5] = uiRegionEntries(r->game, UiRegionsGameDialog, &count);
    if (slot < SaveSlotCount && slot < count) {
        viewDrawPicture(r, 9, checked ? 0x12 : 0x11, regions[slot][0], regions[slot][2], false, 0);
    }
}

void gameDialogStyleDraw(const ViewRenderer *r, GameDialogOption lit, unsigned driverFlags) {
    static const DialogLabel kLabels[] = {{GameDialogSave, 44, 95, "SAVE"},        {GameDialogLoad, 81, 95, "LOAD"},     {GameDialogNewGame, 118, 95, "NEW GAME"},
                                          {GameDialogDos, 179, 95, "DOS"},         {GameDialogMusic, 64, 107, "MUSIC"}, {GameDialogSoundFx, 119, 107, "SOUND FX"},
                                          {GameDialogReturn, 166, 119, "RETURN"},  {GameDialogAnimation, 38, 119, "ANIMATION"}};
    viewDrawPicture(r, 1, 0, 24, 23, true, 0);
    for (unsigned i = 0; i < sizeof(kLabels) / sizeof(kLabels[0]); i++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, kLabels[i].x, kLabels[i].y, kLabels[i].label, kLabels[i].option == lit ? 0x8A : 6, 0, FontTransparent);
    }
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 97, 119, "      ", 4, 4, FontOpaque);
    if (driverFlags & DriverSoundFxOn) {
        viewDrawPicture(r, 9, 0x12, 170, 106, false, 0);
    }
    if (driverFlags & DriverMusicOn) {
        viewDrawPicture(r, 9, 0x12, 97, 106, false, 0);
    }
}

void gameDialogPromptDraw(const ViewRenderer *r, GameDialogPrompt prompt) {
    static const char *const kDos[] = {"DO YOU WANT", "TO EXIT TO", "DOS NOW?"};
    static const char *const kSave[] = {"DO YOU WANT", "TO SAVE THIS", "GAME NOW?"};
    static const char *const kLoad[] = {"DO YOU WANT", "TO LOAD THIS", "GAME NOW?"};
    static const char *const kNew[] = {"START A NEW", "GAME NOW?"};
    const char *const *lines = prompt == GameDialogPromptDos ? kDos : prompt == GameDialogPromptSave ? kSave : prompt == GameDialogPromptLoad ? kLoad : kNew;
    unsigned count = prompt == GameDialogPromptNewGame ? 2 : 3;
    for (int row = 0; row < 60; row++) { /* ClearMessageBoxArea */
        for (int x = 0; x < 72; x++) {
            r->screen[(96 + row) * ViewScreenWidth + 240 + x] = 4;
        }
    }
    for (unsigned i = 0; i < count; i++) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 96 + 6 * (int)i, lines[i], 0x8A, 0, FontTransparent);
    }
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 120, "    YES", 0xAA, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 240, 132, "    NO", 0x5B, 0, FontTransparent);
}

int gameDialogPromptKey(uint8_t key) {
    return key == 'Y' || key == 'y' || key == 0x0D ? 1 : key == 'N' || key == 'n' || key == 0x1B ? 0 : -1;
}

int gameDialogPromptClick(int x, int y) {
    if (x < 240 || x > 311 || y < 120 || y > 137) {
        return -1;
    }
    return y <= 125 ? 1 : 0;
}

int gameDialogSlotForRegion(unsigned region) {
    return region >= 1 && region <= SaveSlotCount ? (int)region - 1 : -1;
}

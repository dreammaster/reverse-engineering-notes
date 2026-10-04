#ifndef YENDOR23_GAMEDIALOG_H
#define YENDOR23_GAMEDIALOG_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "savegame.h"
#include "viewrender.h"

/*
 * The pause / options dialog (RunGameDialog, yendor2.asm:26081; Chapter 3 the same). Eight options, each with a hotkey and a click region
 * (UiRegionsGameDialog, ids 7-14; ids 1-6 are the save-slot rows shown while saving / loading):
 *
 *   id  option     key  label position  enabled when (the caller's UI flag word, g_uiScratchFlags1)
 *   7   SAVE       S    (44, 95)        0x80
 *   8   LOAD       L    (81, 95)        0x40
 *   9   NEW GAME   N    (118, 95)       0x20
 *   10  DOS        D    (179, 95)       0x10
 *   11  MUSIC      M    (64, 107)       driver flag 1 (a music device exists)
 *   12  SOUND FX   F    (119, 107)      driver flag 4 (sound effects exist)
 *   13  ANIMATION  A    (38, 119)       0x08
 *   14  RETURN     R    (166, 119)      0x04; Escape also returns
 *
 * Every label is already part of the dialog picture (PICTURES category 1 id 0, drawn at (24, 23)); an option that is not enabled is
 * marked by writing its label again in the dull colour 6, and its key / click does nothing (the original tests the flag and skips). The
 * two check boxes at (97, 106) and (170, 106) show driver flags 2 (music on) and 8 (sound effects on) with category 9 picture 0x12
 * (checked) or 0x11 (unchecked, drawn after a toggle). The animation option, when enabled, shows its speed name in colour 0x8A on 4 at
 * (97, 119); when it is not it shows the ANIMATION caption at (38, 119) in colour 6 over a blank field.
 *
 * Driver flags (g_driverStateFlags): 1 music available, 2 music on, 4 sound effects available, 8 sound effects on.
 */
typedef enum {
    GameDialogNone,
    GameDialogSave,
    GameDialogLoad,
    GameDialogNewGame,
    GameDialogDos,
    GameDialogMusic,
    GameDialogSoundFx,
    GameDialogAnimation,
    GameDialogReturn
} GameDialogOption;

enum {
    GameDialogFlagReturn = 0x04,
    GameDialogFlagAnimation = 0x08,
    GameDialogFlagDos = 0x10,
    GameDialogFlagNewGame = 0x20,
    GameDialogFlagLoad = 0x40,
    GameDialogFlagSave = 0x80,
    DriverMusicAvailable = 1,
    DriverMusicOn = 2,
    DriverSoundFxAvailable = 4,
    DriverSoundFxOn = 8
};

/* The option for an upper-case key, or GameDialogNone (Escape is GameDialogReturn). */
GameDialogOption gameDialogKeyOption(uint8_t key);

/* The option for a UiRegionsGameDialog result (7-14); 1-6 (slot rows) and anything else give GameDialogNone. */
GameDialogOption gameDialogRegionOption(unsigned region);

/* Whether the option does anything given the UI flag word and the driver flags (music / sound toggles only need their device). */
bool gameDialogOptionEnabled(GameDialogOption option, unsigned uiFlags, unsigned driverFlags);

/* ToggleMusicSetting / ToggleSoundFxSetting: the new driver flags (unchanged without the device) and whether it changed. */
unsigned gameDialogToggleMusic(unsigned driverFlags, bool *changed);
unsigned gameDialogToggleSoundFx(unsigned driverFlags, bool *changed);

/* CycleAnimationSetting: 1 (fast) -> 9 (slow) -> 5 (medium) -> 1; any other value (the title screen sets 3) -> 5. */
unsigned gameDialogNextAnimationSpeed(unsigned speed);

/* The speed name: 1 FAST, 5 MEDIUM, anything else SLOW (six characters, padded). */
const char *gameDialogAnimationSpeedName(unsigned speed);

/* The dialog: panel picture, dimmed labels of the options that are not enabled, check boxes and the animation speed. */
void gameDialogDraw(const ViewRenderer *r, unsigned uiFlags, unsigned driverFlags, unsigned animationSpeed);

/*
 * The six save slots shown by SAVE and LOAD (the dialog's regions 1-6 are their rows; the table at DS:0x6CBE has 27 bytes per slot: the slot's
 * file digit, a flag byte -- 0x80 the slot file exists, 0x40 it is the highlighted one -- and the 25 character name read from the SAVGAMEn file,
 * findSavegame yendor2.asm:3401). Each name is written opaque on colour 4 at (row x + 12, row y + 1), in 0x0F or in 0x7B for the highlighted
 * slot. Choosing an occupied slot to save asks for confirmation (ShowConfirmPrompt 3); an empty one asks for a name first (24 characters, then
 * prompt 2); LOAD works only on occupied slots (prompt 3 again).
 */
enum { SaveSlotNameSize = 26 }; /* SaveSlotCount (6) is in savegame.h */

typedef struct {
    char name[SaveSlotNameSize];
    bool used, highlighted;
} SaveSlotEntry;

void gameDialogSlotsDraw(const ViewRenderer *r, const SaveSlotEntry slots[SaveSlotCount]);

/* The slot (0-5) whose row contains the click (UiRegionsGameDialog results 1-6), or -1. */
int gameDialogSlotForRegion(unsigned region);

#endif

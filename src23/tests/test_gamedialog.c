/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_gamedialog test_gamedialog.c ../gamedialog.c ../font.c ../viewrender.c ../random.c ../pictures.c ../worldmap.c ../uiregions.c ../savegame.c && ./test_gamedialog
 */
#include <stdio.h>
#include <string.h>

#include "gamedialog.h"
#include "uiregions.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static bool anyColour(const uint8_t *screen, int x0, int y0, int x1, int y1, uint8_t colour) {
    for (int y = y0; y < y1; y++) {
        for (int x = x0; x < x1; x++) {
            if (screen[y * 320 + x] == colour) {
                return true;
            }
        }
    }
    return false;
}

static const uint8_t *flat(void *ctx, unsigned category, unsigned id) {
    static uint8_t pixels[320 * 200];
    (void)ctx;
    memset(pixels, (uint8_t)(0x40 + category * 8 + id), sizeof(pixels));
    return pixels;
}

int main(void) {
    check("hotkeys: S L N D M F A R and Escape", gameDialogKeyOption('S') == GameDialogSave && gameDialogKeyOption('L') == GameDialogLoad && gameDialogKeyOption('N') == GameDialogNewGame &&
                                                     gameDialogKeyOption('D') == GameDialogDos && gameDialogKeyOption('M') == GameDialogMusic && gameDialogKeyOption('F') == GameDialogSoundFx &&
                                                     gameDialogKeyOption('A') == GameDialogAnimation && gameDialogKeyOption('R') == GameDialogReturn && gameDialogKeyOption(0x1B) == GameDialogReturn &&
                                                     gameDialogKeyOption('X') == GameDialogNone);
    check("click regions 7-14 map to the options, 1-6 (slot rows) to none", gameDialogRegionOption(7) == GameDialogSave && gameDialogRegionOption(10) == GameDialogDos &&
                                                                               gameDialogRegionOption(13) == GameDialogAnimation && gameDialogRegionOption(14) == GameDialogReturn &&
                                                                               gameDialogRegionOption(3) == GameDialogNone && gameDialogRegionOption(15) == GameDialogNone);
    for (unsigned game = 0; game < 2; game++) {
        unsigned count;
        const uint16_t(*regions)[5] = uiRegionEntries(game ? GameYendor3 : GameYendor2, UiRegionsGameDialog, &count);
        check("the real region table has the eight option regions where the labels are", count == 14 && regions[6][0] <= 44 && 44 <= regions[6][1] && regions[6][2] <= 95 && 95 <= regions[6][3] &&
                                                                                          regions[9][4] == 14 && regions[9][0] <= 166 && 166 <= regions[9][1] &&
                                                                                          regions[8][4] == 9 && regions[8][0] <= 118 && 118 <= regions[8][1]);
    }
    check("enabling: UI flag bits for the five commands, driver flags for the two sound options",
          gameDialogOptionEnabled(GameDialogSave, 0x80, 0) && !gameDialogOptionEnabled(GameDialogSave, 0x7F, 0xF) && gameDialogOptionEnabled(GameDialogDos, 0x10, 0) &&
              gameDialogOptionEnabled(GameDialogReturn, 0x04, 0) && gameDialogOptionEnabled(GameDialogMusic, 0, 1) && !gameDialogOptionEnabled(GameDialogMusic, 0xFF, 0xE) &&
              gameDialogOptionEnabled(GameDialogSoundFx, 0, 4) && !gameDialogOptionEnabled(GameDialogSoundFx, 0xFF, 0xB));
    bool changed;
    check("music toggles bit 2 only when the device exists", gameDialogToggleMusic(1, &changed) == 3 && changed && gameDialogToggleMusic(3, &changed) == 1 && changed &&
                                                                gameDialogToggleMusic(0, &changed) == 0 && !changed);
    check("sound effects toggle bit 8 only when the device exists", gameDialogToggleSoundFx(4, &changed) == 12 && changed && gameDialogToggleSoundFx(12, &changed) == 4 && changed &&
                                                                       gameDialogToggleSoundFx(1, &changed) == 1 && !changed);
    check("animation speed cycles 1 -> 9 -> 5 -> 1, anything else (3) -> 5", gameDialogNextAnimationSpeed(1) == 9 && gameDialogNextAnimationSpeed(9) == 5 &&
                                                                                 gameDialogNextAnimationSpeed(5) == 1 && gameDialogNextAnimationSpeed(3) == 5);
    check("speed names", strcmp(gameDialogAnimationSpeedName(1), "FAST  ") == 0 && strcmp(gameDialogAnimationSpeedName(5), "MEDIUM") == 0 &&
                             strcmp(gameDialogAnimationSpeedName(9), "SLOW  ") == 0);

    static uint8_t screen[320 * 200];
    ViewRenderer r = {GameYendor2, NULL, flat, NULL, screen, NULL};
    memset(screen, 0xEE, sizeof(screen));
    gameDialogDraw(&r, 0xFC, 0xF, 5); /* everything enabled, both sounds on */
    check("the panel picture lands at (24, 23)", screen[23 * 320 + 24] == 0x48 + 0 && screen[22 * 320 + 24] == 0xEE && screen[23 * 320 + 23] == 0xEE);
    bool dim = false;
    for (int y = 95; y < 101; y++) {
        for (int x = 44; x < 70; x++) {
            dim = dim || screen[y * 320 + x] == 6;
        }
    }
    check("enabled options get no dimming overlay", !dim);
    bool speed = false;
    for (int y = 119; y < 125; y++) {
        for (int x = 97; x < 133; x++) {
            speed = speed || screen[y * 320 + x] == 0x8A;
        }
    }
    check("the animation speed shows in colour 0x8A", speed);
    check("both check boxes are ticked (category 9 picture 0x12)", screen[106 * 320 + 97] == 0x40 + 72 + 0x12 - 0 && screen[106 * 320 + 170] == 0x40 + 72 + 0x12);

    memset(screen, 0xEE, sizeof(screen));
    gameDialogDraw(&r, 0x00, 0x0, 5); /* nothing enabled */
    dim = false;
    for (int y = 95; y < 101; y++) {
        for (int x = 44; x < 70; x++) {
            dim = dim || screen[y * 320 + x] == 6;
        }
    }
    bool caption = false;
    for (int y = 119; y < 125; y++) {
        for (int x = 38; x < 90; x++) {
            caption = caption || screen[y * 320 + x] == 6;
        }
    }
    check("a disabled SAVE is overwritten in the dull colour 6, and the ANIMATION caption appears", dim && caption);
    check("...with no ticks in the check boxes", screen[106 * 320 + 97] != 0x40 + 72 + 0x12 && screen[106 * 320 + 170] != 0x40 + 72 + 0x12);

    SaveSlotEntry slots[SaveSlotCount];
    memset(slots, 0, sizeof(slots));
    strcpy(slots[0].name, "FIRST");
    slots[0].used = true;
    strcpy(slots[2].name, "THIRD");
    slots[2].used = slots[2].highlighted = true;
    memset(screen, 0xEE, sizeof(screen));
    gameDialogSlotsDraw(&r, slots);
    check("slot names are written at (row x + 12, row y + 1): 0x0F normally, 0x7B highlighted, nothing for an empty slot",
          anyColour(screen, 51, 28, 90, 34, 0x0F) && anyColour(screen, 51, 50, 90, 56, 0x7B) && !anyColour(screen, 51, 39, 90, 45, 0x0F) && screen[28 * 320 + 55] != 0xEE);
    check("slot rows map to regions 1-6", gameDialogSlotForRegion(1) == 0 && gameDialogSlotForRegion(6) == 5 && gameDialogSlotForRegion(7) == -1 && gameDialogSlotForRegion(0) == -1);
    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

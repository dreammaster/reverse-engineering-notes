/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_titlemenu test_titlemenu.c ../titlemenu.c ../uiregions.c && ./test_titlemenu
 */
#include <stdio.h>

#include "titlemenu.h"
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

int main(void) {
    check("keys C A E R I, Return, Ctrl+S, Ctrl+Q", titleKeyAction('C') == TitleManageParty && titleKeyAction('A') == TitleWorldMap && titleKeyAction('E') == TitleEnterGame &&
                                                       titleKeyAction('R') == TitleIntro && titleKeyAction('I') == TitleCreateCharacter && titleKeyAction(0x0D) == TitleToggleMusic &&
                                                       titleKeyAction(0x13) == TitleToggleSoundFx && titleKeyAction(0x11) == TitleQuit && titleKeyAction('Z') == TitleNone);
    check("regions 1-5 give the same five commands in order", titleRegionAction(1) == TitleManageParty && titleRegionAction(2) == TitleWorldMap && titleRegionAction(3) == TitleEnterGame &&
                                                                 titleRegionAction(4) == TitleIntro && titleRegionAction(5) == TitleCreateCharacter && titleRegionAction(0) == TitleNone &&
                                                                 titleRegionAction(6) == TitleNone);
    check("entering needs a hero", !titleCanEnter(0) && titleCanEnter(7));
    for (unsigned game = 0; game < 2; game++) {
        unsigned count;
        const uint16_t(*r)[5] = uiRegionEntries(game ? GameYendor3 : GameYendor2, UiRegionsTitleMenu, &count);
        bool ok = count == 5;
        for (unsigned i = 0; ok && i < 5; i++) {
            ok = r[i][4] == i + 1 && r[i][2] < r[i][3] && (i == 0 || r[i][2] > r[i - 1][2]); /* five bands from top to bottom */
        }
        check("the real title regions are five bands, top to bottom, ids 1-5", ok);
    }
    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_maininput test_maininput.c ../maininput.c ../movement.c ../worldmap.c && ./test_maininput
 */
#include <stdio.h>

#include "maininput.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static MainAction ascii(GameKind game, char key) {
    return mainCommandForKey(game, false, (uint8_t)key).action;
}

static MainAction ext(GameKind game, uint8_t scan) {
    return mainCommandForKey(game, true, scan).action;
}

int main(void) {
    for (unsigned g = 0; g < 2; g++) {
        GameKind game = g ? GameYendor3 : GameYendor2;
        check("1-4 select a party panel, F1-F4 a detail screen", ascii(game, '3') == MainActionPartyPanel && mainCommandForKey(game, false, '3').index == 2 &&
                                                                    ext(game, 0x3D) == MainActionMemberDetail && mainCommandForKey(game, true, 0x3D).index == 2);
        check("the arrows move, Ctrl+arrows strafe",
              ext(game, 0x48) == MainActionMove && mainCommandForKey(game, true, 0x48).movement == MovementForward && mainCommandForKey(game, true, 0x50).movement == MovementBackward &&
                  mainCommandForKey(game, true, 0x4B).movement == MovementTurnLeft && mainCommandForKey(game, true, 0x4D).movement == MovementTurnRight &&
                  mainCommandForKey(game, true, 0x73).movement == MovementStrafeLeft && mainCommandForKey(game, true, 0x74).movement == MovementStrafeRight);
        check("the letter commands", ascii(game, 'A') == MainActionAttack && ascii(game, 'S') == MainActionAct && ascii(game, 'C') == MainActionAlchemy &&
                                        ascii(game, 'D') == MainActionGameDialog && ascii(game, 'K') == MainActionUnlockDoor && ascii(game, 'M') == MainActionLocalMap &&
                                        ascii(game, 'T') == MainActionClock && ascii(game, 'P') == MainActionPartyInventory && ascii(game, 'R') == MainActionRest &&
                                        ascii(game, ' ') == MainActionExamine && ext(game, 0x42) == MainActionClueBook && ext(game, 0x3F) == MainActionPanelMode);
        check("unmapped keys do nothing", ascii(game, 'Q') == MainActionNone && ascii(game, 'E') == MainActionNone && ext(game, 0x59) == MainActionNone);
    }
    check("Chapter 2 has W and the debug keys, no V", ascii(GameYendor2, 'W') == MainActionToggleMapView && ascii(GameYendor2, 'X') == MainActionDebug &&
                                                         ext(GameYendor2, 0x52) == MainActionDebug && ascii(GameYendor2, 'V') == MainActionNone);
    check("Chapter 3 has V, no W and ignores the debug keys", ascii(GameYendor3, 'V') == MainActionStatusMessage && ascii(GameYendor3, 'W') == MainActionNone &&
                                                                 ascii(GameYendor3, 'X') == MainActionNone && ext(GameYendor3, 0x53) == MainActionNone);
    check("required items", mainCommandRequiredItem(GameYendor2, MainActionUnlockDoor) == 0x2F && mainCommandRequiredItem(GameYendor2, MainActionLocalMap) == 0x1E &&
                                mainCommandRequiredItem(GameYendor2, MainActionToggleMapView) == 0x1F && mainCommandRequiredItem(GameYendor2, MainActionClock) == 7 &&
                                mainCommandRequiredItem(GameYendor3, MainActionUnlockDoor) == 0x32 && mainCommandRequiredItem(GameYendor3, MainActionLocalMap) == 0x33 &&
                                mainCommandRequiredItem(GameYendor3, MainActionClock) == 0x0C && mainCommandRequiredItem(GameYendor3, MainActionRest) == 0);
    check("combat keys: A C D P 1-4, extended keys open the detail screen",
          combatCommandForKey(false, 'A').action == MainActionAttack && combatCommandForKey(false, 'C').action == MainActionAlchemy &&
              combatCommandForKey(false, 'D').action == MainActionGameDialog && combatCommandForKey(false, 'P').action == MainActionPartyInventory &&
              combatCommandForKey(false, '2').action == MainActionPartyPanel && combatCommandForKey(false, '2').index == 1 && combatCommandForKey(true, 0x48).action == MainActionMemberDetail &&
              combatCommandForKey(false, 'R').action == MainActionNone && combatCommandForKey(false, 'M').action == MainActionNone);
    MainCommand pad = mainCommandForClick(false, 5, 1);
    check("left clicks: the pad moves, the icon row runs act / alchemy / rest / dialog, the view drops an item",
          pad.action == MainActionMove && pad.movement == MovementTurnLeft && mainCommandForClick(false, 5, 2).movement == MovementForward &&
              mainCommandForClick(false, 5, 5).movement == MovementBackward && mainCommandForClick(false, 5, 6).movement == MovementStrafeRight &&
              mainCommandForClick(false, 5, 0).action == MainActionNone && mainCommandForClick(false, 3, 1).action == MainActionAct &&
              mainCommandForClick(false, 3, 2).action == MainActionAlchemy && mainCommandForClick(false, 3, 3).action == MainActionRest &&
              mainCommandForClick(false, 3, 4).action == MainActionGameDialog && mainCommandForClick(false, 1, 0).action == MainActionDropHeldItem &&
              mainCommandForClick(false, 4, 1).action == MainActionNone && mainCommandForClick(false, 6, 0).action == MainActionPortraitClick);
    check("right clicks: examine, panel, party inventory", mainCommandForClick(true, 1, 0).action == MainActionExamine && mainCommandForClick(true, 2, 0).action == MainActionPanelClick &&
                                                              mainCommandForClick(true, 6, 0).action == MainActionPartyInventory && mainCommandForClick(true, 3, 1).action == MainActionNone);
    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

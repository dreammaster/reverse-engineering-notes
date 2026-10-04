#include "maininput.h"

static MainCommand make(MainAction action, unsigned index) {
    MainCommand c = {action, index, MovementForward};
    return c;
}

static MainCommand debugCommand(GameKind game) {
    return make(game == GameYendor2 ? MainActionDebug : MainActionNone, 0);
}

MainCommand mainCommandForKey(GameKind game, bool extended, uint8_t key) {
    if (extended) {
        switch (key) {
        case 0x3B:
        case 0x3C:
        case 0x3D:
        case 0x3E:
            return make(MainActionMemberDetail, key - 0x3Bu);
        case 0x3F:
            return make(MainActionPanelMode, 0);
        case 0x42:
            return make(MainActionClueBook, 0);
        case 0x48:
        case 0x50:
        case 0x4B:
        case 0x4D:
        case 0x73:
        case 0x74: {
            MainCommand c = make(MainActionMove, 0);
            c.movement = key == 0x48   ? MovementForward
                         : key == 0x50 ? MovementBackward
                         : key == 0x4B ? MovementTurnLeft
                         : key == 0x4D ? MovementTurnRight
                         : key == 0x73 ? MovementStrafeLeft
                                       : MovementStrafeRight;
            return c;
        }
        case 0x52:
        case 0x53:
            return debugCommand(game);
        default:
            return make(MainActionNone, 0);
        }
    }
    switch (key) {
    case ' ':
        return make(MainActionExamine, 0);
    case '1':
    case '2':
    case '3':
    case '4':
        return make(MainActionPartyPanel, key - '1');
    case 'A':
        return make(MainActionAttack, 0);
    case 'S':
        return make(MainActionAct, 0);
    case 'C':
        return make(MainActionAlchemy, 0);
    case 'D':
        return make(MainActionGameDialog, 0);
    case 'K':
        return make(MainActionUnlockDoor, 0);
    case 'M':
        return make(MainActionLocalMap, 0);
    case 'W':
        return game == GameYendor2 ? make(MainActionToggleMapView, 0) : make(MainActionNone, 0);
    case 'T':
        return make(MainActionClock, 0);
    case 'P':
        return make(MainActionPartyInventory, 0);
    case 'R':
        return make(MainActionRest, 0);
    case 'V':
        return game == GameYendor3 ? make(MainActionStatusMessage, 0) : make(MainActionNone, 0);
    case 'X':
    case 'Y':
    case 'Z':
    case 'I':
    case 'L':
    case 'B':
    case '!':
    case '@':
    case '#':
    case '$':
    case '%':
    case '^':
    case '0':
    case 0x60:
    case '~':
        return debugCommand(game);
    default:
        return make(MainActionNone, 0);
    }
}

unsigned mainCommandRequiredItem(GameKind game, MainAction action) {
    if (game == GameYendor2) {
        return action == MainActionUnlockDoor ? 0x2F : action == MainActionLocalMap ? 0x1E : action == MainActionToggleMapView ? 0x1F : action == MainActionClock ? 7 : 0;
    }
    return action == MainActionUnlockDoor ? 0x32 : action == MainActionLocalMap ? 0x33 : action == MainActionClock ? 0x0C : 0;
}

MainCommand combatCommandForKey(bool extended, uint8_t key) {
    if (extended) {
        return make(MainActionMemberDetail, 0);
    }
    switch (key) {
    case 'A':
        return make(MainActionAttack, 0);
    case 'C':
        return make(MainActionAlchemy, 0);
    case 'D':
        return make(MainActionGameDialog, 0);
    case 'P':
        return make(MainActionPartyInventory, 0);
    case '1':
    case '2':
    case '3':
    case '4':
        return make(MainActionPartyPanel, key - '1');
    default:
        return make(MainActionNone, 0);
    }
}

MainCommand mainCommandForClick(bool rightButton, unsigned region, unsigned subRegion) {
    if (rightButton) {
        switch (region) {
        case 1:
            return make(MainActionExamine, 0);
        case 2:
            return make(MainActionPanelClick, 0);
        case 6:
            return make(MainActionPartyInventory, 0);
        default:
            return make(MainActionNone, 0);
        }
    }
    switch (region) {
    case 1:
        return make(MainActionDropHeldItem, 0);
    case 2:
        return make(MainActionPanelClick, 0);
    case 3:
        return subRegion == 1   ? make(MainActionAct, 0)
               : subRegion == 2 ? make(MainActionAlchemy, 0)
               : subRegion == 3 ? make(MainActionRest, 0)
               : subRegion == 4 ? make(MainActionGameDialog, 0)
                                : make(MainActionNone, 0);
    case 5: {
        static const MovementAction kPad[6] = {MovementTurnLeft, MovementForward, MovementTurnRight, MovementStrafeLeft, MovementBackward, MovementStrafeRight};
        if (subRegion < 1 || subRegion > 6) {
            return make(MainActionNone, 0);
        }
        MainCommand command = make(MainActionMove, 0);
        command.movement = kPad[subRegion - 1];
        return command;
    }
    case 6:
        return make(MainActionPortraitClick, 0);
    default:
        return make(MainActionNone, 0);
    }
}

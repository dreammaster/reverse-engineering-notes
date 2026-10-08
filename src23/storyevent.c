#include "storyevent.h"

#include "globalflags.h"
#include "savegame.h"

bool storyEndingReady(const uint8_t *globalFlags, size_t flagsSize) {
    return globalFlagTest(globalFlags, flagsSize, StoryEndingFlagA) && globalFlagTest(globalFlags, flagsSize, StoryEndingFlagB) &&
           globalFlagTest(globalFlags, flagsSize, StoryEndingFlagC);
}

#define KEEP 0xFFFF

/* yendor3.asm:21124-21500; every number is read from the instructions (the texts are in REGISTER.EXE's data segment). */
static const StoryEvent kEvents[StoryEventCountYendor3] = {
    /* 1 */ {0, 5, 0, 0x21, 2, 397, 94, SaveFacingWest, KEEP, 0, 0, 0, true, 0, 0, 0, 0, 0, 0, 0x9301, true},
    /* 2 */ {0x1B, 0, 0, 0x54, 4, 540, 121, SaveFacingSouth, 0x1FFC, 0x2000, 0, 0, false, 0, 0, 0, 0, 0, 0, 0, false},
    /* 3 */ {0x2E, 0, 0x7F00, 0, 5, 638, 36, SaveFacingWest, KEEP, 0x2000, 0, 0, false, 0, 0, 0, 0, 0, 0, 0, false},
    /* 4 */ {0, 0, 0, 0x2C, 5, 619, 37, SaveFacingNorth, KEEP, 0x2000, 0, 0, false, 0xC6, 0xCD, 0, 0, 0, 0, 0x9507, false},
    /* 5 */ {0x91, 0, 0x7F00, 0x2C, 8, 604, 123, SaveFacingEast, KEEP, 0x4000, 0x9D, 0, false, 0, 0, 0, 0, 0, 0, 0, false},
    /* 6 */ {0, 0, 0, 0x2C, 2, 332, 70, SaveFacingSouth, 0xBFFF, 0, 0, 0x9D, false, 0, 0, 0x17F, 0x181, 0x183, 0x9E, 0x9581, false},
};

const StoryEvent *storyEvent(GameKind game, unsigned value) {
    if (game != GameYendor3 || value < 1 || value > StoryEventCountYendor3) {
        return NULL;
    }
    return &kEvents[value - 1];
}

#include "gameclock.h"

bool gameClockAdvance(GameClock *clock, uint16_t deltaMinutes) {
    clock->minutes = (uint16_t)(clock->minutes + deltaMinutes);
    if (clock->minutes < 0x59F) {
        return false;
    }
    /* Underflows to 0xFFFF when clock->minutes was exactly 0x59F (1439) -- a real, shared original bug,
       reproduced faithfully rather than corrected. See gameClockAdvance's own doc comment in gameclock.h. */
    clock->minutes = (uint16_t)(clock->minutes - 0x5A0);
    if (clock->minutes == 0) {
        clock->minutes = 1;
    }

    clock->day++;
    if (clock->day != 0x1F) {
        return true;
    }
    clock->day = 1;
    clock->month++;
    if (clock->month != 0xD) {
        return true;
    }
    clock->year++;
    /* month is deliberately left at 13 here -- see gameClockAdvance's own doc comment. */
    return true;
}

bool gameClockRestAllowed(bool noRestFlag, bool isInTriggerList) {
    if (noRestFlag) {
        return false;
    }
    return !isInTriggerList;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_gameclock test_gameclock.c ../gameclock.c && ./test_gameclock
 */
#include <stdio.h>
#include <string.h>

#include "gameclock.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void checkU32(const char *label, unsigned actual, unsigned expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static GameClock freshClock(uint16_t minutes, uint16_t day, uint16_t month, uint16_t year) {
    GameClock clock;
    clock.minutes = minutes;
    clock.day = day;
    clock.month = month;
    clock.year = year;
    return clock;
}

static void testNoRolloverBelowThreshold(void) {
    GameClock clock = freshClock(100, 5, 6, 546);
    bool rolled = gameClockAdvance(&clock, 60);
    check("no rollover: didn't cross the 1439 threshold", !rolled);
    checkU32("minutes advanced by delta", clock.minutes, 160);
    checkU32("day unchanged", clock.day, 5);
}

static void testOrdinaryDayRollover(void) {
    /* 1400 + 60 = 1460 -> 1460 - 1440 = 20, an ordinary, non-edge-case rollover. */
    GameClock clock = freshClock(1400, 5, 6, 546);
    bool rolled = gameClockAdvance(&clock, 60);
    check("day rolled over", rolled);
    checkU32("minutes wrapped to 20", clock.minutes, 20);
    checkU32("day advanced by 1", clock.day, 6);
}

static void testExactMidnightClampsToOne(void) {
    /* 960 + 480 = 1440 -> 1440 - 1440 = 0 -> clamped to 1 (the original's own "avoid literal 0" fixup). */
    GameClock clock = freshClock(960, 5, 6, 546);
    gameClockAdvance(&clock, 480);
    checkU32("exact midnight (post-subtract 0) clamps to 1, not 0", clock.minutes, 1);
}

static void testExactly1439UnderflowsSharedBug(void) {
    /*
     * A real, shared original bug (see gameclock.h's own doc comment): the rollover check fires at
     * minutes >= 1439 (0x59F), one short of the real day length (1440, 0x5A0) -- so when the
     * pre-subtraction total lands exactly on 1439, subtracting 1440 underflows to 65535 instead of a
     * sane wrapped value. 1379 + 60 = 1439 is an entirely ordinary starting point and delta (an hourly
     * rest tick), not a contrived edge case.
     */
    GameClock clock = freshClock(1379, 5, 6, 546);
    bool rolled = gameClockAdvance(&clock, 60);
    check("day still counts as rolled over", rolled);
    checkU32("minutes underflows to 65535 -- the original's own bug, reproduced faithfully", clock.minutes,
             65535);
    checkU32("day still advances normally despite the underflow", clock.day, 6);
}

static void testMonthRollover(void) {
    GameClock clock = freshClock(1400, 30, 6, 546); /* day 30 -> wraps at 31 (0x1F) */
    gameClockAdvance(&clock, 60);
    checkU32("day wrapped to 1", clock.day, 1);
    checkU32("month advanced", clock.month, 7);
    checkU32("year unchanged", clock.year, 546);
}

static void testYearRolloverLeavesMonthAt13(void) {
    /*
     * A second real, shared original bug: when a month rollover also triggers a year rollover, month is
     * never reset back to 1 here -- unlike AdvanceGameClock's own per-minute tick, which does reset it.
     * Confirmed byte-for-byte identical in both games' RestPartyAndAdvanceClock. Reproduced faithfully,
     * not corrected.
     */
    GameClock clock = freshClock(1400, 30, 12, 546); /* day 30, month 12 -> both wrap */
    gameClockAdvance(&clock, 60);
    checkU32("day wrapped to 1", clock.day, 1);
    checkU32("year advanced", clock.year, 547);
    checkU32("month left at 13, NOT reset to 1 -- the original's own bug", clock.month, 13);
}

static void testRestAllowed(void) {
    check("allowed when no flag and not in a trigger cell", gameClockRestAllowed(false, false));
    check("denied by the global no-rest flag", !gameClockRestAllowed(true, false));
    check("denied by a trigger-list match", !gameClockRestAllowed(false, true));
    check("denied when both apply", !gameClockRestAllowed(true, true));
}

int main(void) {
    testNoRolloverBelowThreshold();
    testOrdinaryDayRollover();
    testExactMidnightClampsToOne();
    testExactly1439UnderflowsSharedBug();
    testMonthRollover();
    testYearRolloverLeavesMonthAt13();
    testRestAllowed();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_travel test_travel.c ../travel.c ../globalflags.c && ./test_travel
 */
#include <stdio.h>
#include <string.h>

#include "globalflags.h"
#include "travel.h"

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

static void checkI32(const char *label, int actual, int expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %d, want %d\n", label, actual, expected);
    }
}

static uint8_t g_flags[32]; /* covers global flag indices up to 256 -- both games' real gate tables stay under that */

static void reset(void) {
    memset(g_flags, 0, sizeof(g_flags));
}

static void testDestinationCounts(void) {
    checkU32("Chapter 2 destination count", travelDestinationCount(GameYendor2), TravelDestinationCountYendor2);
    checkU32("Chapter 3 destination count", travelDestinationCount(GameYendor3), TravelDestinationCountYendor3);
}

static void testDestinationLookup(void) {
    TravelDestination dest;

    check("id 0 is out of range (1-based)", !travelDestinationLookup(GameYendor2, 0, &dest));
    check("Chapter 2 id 188 is out of range", !travelDestinationLookup(GameYendor2, 188, &dest));
    check("Chapter 3 id 140 is out of range", !travelDestinationLookup(GameYendor3, 140, &dest));

    check("Chapter 2 id 1 found", travelDestinationLookup(GameYendor2, 1, &dest));
    checkI32("Chapter 2 id 1 worldX", dest.worldX, 659);
    checkI32("Chapter 2 id 1 worldY", dest.worldY, 46);
    checkU32("Chapter 2 id 1 facing", dest.facing, 0x8000);
    checkU32("Chapter 2 id 1 flags", dest.flags, 0x8000);

    check("Chapter 2 id 187 (last) found", travelDestinationLookup(GameYendor2, 187, &dest));
    checkI32("Chapter 2 id 187 worldX", dest.worldX, 61);
    checkI32("Chapter 2 id 187 worldY", dest.worldY, 83);

    check("Chapter 3 id 1 found", travelDestinationLookup(GameYendor3, 1, &dest));
    checkI32("Chapter 3 id 1 worldX", dest.worldX, 63);
    checkI32("Chapter 3 id 1 worldY", dest.worldY, 59);
    checkU32("Chapter 3 id 1 rawC (mode)", dest.rawC, 0x0002);

    check("Chapter 3 id 139 (last) found", travelDestinationLookup(GameYendor3, 139, &dest));
    checkI32("Chapter 3 id 139 worldX", dest.worldX, 485);
    checkI32("Chapter 3 id 139 worldY", dest.worldY, 134);
}

static void testCheckUnlockNoGateEntry(void) {
    reset();
    TravelUnlockCheck r = travelCheckUnlock(GameYendor2, 1, 0, g_flags, sizeof(g_flags));
    check("Chapter 2 id 1 has no gate entry -- always proceeds", r.outcome == TravelUnlockAlreadyUnlocked);

    r = travelCheckUnlock(GameYendor3, 1, 0, g_flags, sizeof(g_flags));
    check("Chapter 3 id 1 has no gate entry -- always proceeds", r.outcome == TravelUnlockAlreadyUnlocked);
}

static void testCheckUnlockPasswordRow(void) {
    /* Chapter 2 destination 72, password "NORTH", global flag index 3. */
    reset();
    TravelUnlockCheck r = travelCheckUnlock(GameYendor2, 72, 0, g_flags, sizeof(g_flags));
    check("Chapter 2 id 72 (NORTH) needs a password when flag is clear", r.outcome == TravelUnlockNeedsPassword);
    checkU32("...with the gate row's own promptId", r.promptId, 0x1a);

    globalFlagSet(g_flags, sizeof(g_flags), 3);
    r = travelCheckUnlock(GameYendor2, 72, 0, g_flags, sizeof(g_flags));
    check("Chapter 2 id 72 proceeds once its flag is set", r.outcome == TravelUnlockAlreadyUnlocked);
}

static void testCheckUnlockMessageRow(void) {
    /* Chapter 2 destination 170: hasMsg row, no password at all. */
    reset();
    TravelUnlockCheck r = travelCheckUnlock(GameYendor2, 170, 0, g_flags, sizeof(g_flags));
    check("Chapter 2 id 170 is denied with a message when flag is clear", r.outcome == TravelUnlockDeniedWithMessage);
    checkU32("...with the gate row's own hasMsg as the message id", r.messageId, 0x242);

    globalFlagSet(g_flags, sizeof(g_flags), 52);
    r = travelCheckUnlock(GameYendor2, 170, 0, g_flags, sizeof(g_flags));
    check("Chapter 2 id 170 still proceeds once its flag is set (message row isn't a hard block)",
          r.outcome == TravelUnlockAlreadyUnlocked);
}

static void testCheckUnlockChapter3Dispatch(void) {
    /* Chapter 3 destination 46 (RUSE): its own destination record has flags bit 0x8000 set. */
    reset();
    TravelUnlockCheck r = travelCheckUnlock(GameYendor3, 46, 0x8000, g_flags, sizeof(g_flags));
    check("Chapter 3 id 46 (RUSE, destFlags bit 0x8000) needs a password", r.outcome == TravelUnlockNeedsPassword);
    checkU32("...with the gate row's own promptId", r.promptId, 0x1a);

    /* Chapter 3 destination 38: its own destination record has flags bit 0x4000 set instead. */
    reset();
    r = travelCheckUnlock(GameYendor3, 38, 0x4000, g_flags, sizeof(g_flags));
    check("Chapter 3 id 38 (destFlags bit 0x4000) is denied with the fixed 3-line message, not a prompt",
          r.outcome == TravelUnlockDeniedFixedMessage);

    /*
     * Synthetic: no real Chapter 3 gate row's destination has NEITHER bit set (verified by cross-checking
     * every hasMsg==0 row against its own destination's flags: all 21 have bit 0x4000, all 13 password rows
     * have bit 0x8000) -- this exercises the branch the disassembly still contains for completeness, using
     * id 38's real gate row with a deliberately wrong destinationFlags override.
     */
    reset();
    r = travelCheckUnlock(GameYendor3, 38, 0x0000, g_flags, sizeof(g_flags));
    check("Chapter 3, neither destFlags bit set (synthetic -- not observed in real data), denies silently",
          r.outcome == TravelUnlockDeniedSilent);

    /* Chapter 2 ignores destinationFlags entirely -- passing garbage changes nothing. */
    reset();
    r = travelCheckUnlock(GameYendor2, 72, 0xFFFF, g_flags, sizeof(g_flags));
    check("Chapter 2 ignores destinationFlags (no Chapter 3-style sub-dispatch)", r.outcome == TravelUnlockNeedsPassword);
}

static void testResolvePassword(void) {
    reset();
    check("wrong password is rejected", !travelResolvePassword(GameYendor2, 72, "SOUTH", g_flags, sizeof(g_flags)));
    check("...and the flag stays clear", !globalFlagTest(g_flags, sizeof(g_flags), 3));

    check("too-short input is rejected", !travelResolvePassword(GameYendor2, 72, "NORT", g_flags, sizeof(g_flags)));

    check("exact password matches", travelResolvePassword(GameYendor2, 72, "NORTH", g_flags, sizeof(g_flags)));
    check("...and sets the destination's global flag", globalFlagTest(g_flags, sizeof(g_flags), 3));

    reset();
    check("typed text with a valid password as a PREFIX still matches (the original's own byte-loop stops "
          "at the stored word's own space terminator and never checks for extra trailing input)",
          travelResolvePassword(GameYendor2, 72, "NORTHEAST", g_flags, sizeof(g_flags)));

    reset();
    check("a destination with no gate entry at all never matches",
          !travelResolvePassword(GameYendor2, 1, "ANYTHING", g_flags, sizeof(g_flags)));

    check("the one dead Chapter 2 row (stored password is all-null bytes) can never match",
          !travelResolvePassword(GameYendor2, 125, "", g_flags, sizeof(g_flags)));

    reset();
    check("Chapter 3's RUSE matches", travelResolvePassword(GameYendor3, 46, "RUSE", g_flags, sizeof(g_flags)));
    check("...and travelCheckUnlock now reports AlreadyUnlocked",
          travelCheckUnlock(GameYendor3, 46, 0x8000, g_flags, sizeof(g_flags)).outcome == TravelUnlockAlreadyUnlocked);
}

static void testExamineKey(void) {
    /* Chapter 2's real keys: KEY OF PORT HOPE (12), PARIAH (4), TRACKING (91) have no gate row and travel at once;
     * KEY OF NUMAGIK (9) and STONY PEAK (19) are gated on their destination's flag. */
    reset();
    check("KEY OF PORT HOPE travels", travelExamineKey(GameYendor2, 12, g_flags, sizeof(g_flags), false) == TravelExamineTravels);
    check("KEY OF PARIAH travels", travelExamineKey(GameYendor2, 4, g_flags, sizeof(g_flags), false) == TravelExamineTravels);
    check("KEY OF TRACKING travels", travelExamineKey(GameYendor2, 91, g_flags, sizeof(g_flags), false) == TravelExamineTravels);
    check("KEY OF NUMAGIK only describes itself until the destination is unlocked",
          travelExamineKey(GameYendor2, 9, g_flags, sizeof(g_flags), false) == TravelExamineGateClosed);
    check("KEY OF STONY PEAK likewise", travelExamineKey(GameYendor2, 19, g_flags, sizeof(g_flags), false) == TravelExamineGateClosed);
    memset(g_flags, 0xFF, sizeof(g_flags));
    check("with every flag set both travel", travelExamineKey(GameYendor2, 9, g_flags, sizeof(g_flags), false) == TravelExamineTravels &&
                                                  travelExamineKey(GameYendor2, 19, g_flags, sizeof(g_flags), false) == TravelExamineTravels);

    /* Chapter 3's story gate. */
    reset();
    check("Chapter 3 ANKH OF PORTALS travels", travelExamineKey(GameYendor3, 3, g_flags, sizeof(g_flags), false) == TravelExamineTravels);
    globalFlagSet(g_flags, sizeof(g_flags), TravelStoryRealmFlag);
    check("in the story realm with a weapon of light, it is refused",
          travelExamineKey(GameYendor3, 3, g_flags, sizeof(g_flags), true) == TravelExamineStoryBlocked);
    check("...and the realm flag is kept", globalFlagTest(g_flags, sizeof(g_flags), TravelStoryRealmFlag));
    check("without one it travels", travelExamineKey(GameYendor3, 3, g_flags, sizeof(g_flags), false) == TravelExamineTravels);
    check("...clearing the realm flag", !globalFlagTest(g_flags, sizeof(g_flags), TravelStoryRealmFlag));
    reset();
    globalFlagSet(g_flags, sizeof(g_flags), TravelStoryRealmFlag);
    check("Chapter 2 has no such gate", travelExamineKey(GameYendor2, 12, g_flags, sizeof(g_flags), true) == TravelExamineTravels);
}

int main(void) {
    testDestinationCounts();
    testDestinationLookup();
    testCheckUnlockNoGateEntry();
    testCheckUnlockPasswordRow();
    testCheckUnlockMessageRow();
    testCheckUnlockChapter3Dispatch();
    testResolvePassword();
    testExamineKey();

    if (g_failureCount == 0) {
        printf("All tests passed.\n");
        return 0;
    }
    printf("%d test(s) failed.\n", g_failureCount);
    return 1;
}

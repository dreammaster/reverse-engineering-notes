/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_interact test_interact.c ../interact.c ../savegame.c ../lockcatalog.c ../worldobjects.c ../movement.c ../worldmap.c ../worldmap_stdio.c && ./test_interact
 */
#include <stdio.h>
#include <string.h>

#include "interact.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void checkU32(const char *label, uint32_t actual, uint32_t expected) {
    if (actual == expected) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s: got %u, want %u\n", label, actual, expected);
    }
}

static void testBitmap(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    check("lock id 1 starts clear", !interactBitmapTest(&save, interactLockBitIndex(1)));
    check("lock id 8 starts clear", !interactBitmapTest(&save, interactLockBitIndex(8)));

    interactBitmapSet(&save, interactLockBitIndex(1));
    check("lock id 1 set", interactBitmapTest(&save, interactLockBitIndex(1)));
    check("lock id 2 unaffected by lock id 1's bit", !interactBitmapTest(&save, interactLockBitIndex(2)));
    check("lock id 8 (same byte, opposite end) unaffected", !interactBitmapTest(&save, interactLockBitIndex(8)));

    interactBitmapSet(&save, interactLockBitIndex(8));
    check("lock id 8 set", interactBitmapTest(&save, interactLockBitIndex(8)));
    check("lock id 9 (next byte) unaffected", !interactBitmapTest(&save, interactLockBitIndex(9)));

    checkU32("yendor2 curgame offset is the lock count", interactCurgameIdOffset(GameYendor2), 608);
    checkU32("yendor3 curgame offset is 0 (word_2ECF8 never written)", interactCurgameIdOffset(GameYendor3), 0);
    checkU32("yendor2 curgame id 1's bit follows the last lock id", interactCurgameBitIndex(GameYendor2, 1), 608);
    checkU32("yendor3 curgame id 1's bit collides with lock id 1's", interactCurgameBitIndex(GameYendor3, 1),
             interactLockBitIndex(1));

    unsigned curgameBit = interactCurgameBitIndex(GameYendor2, 1);
    check("curgame id 1's bit is distinct from any lock id's (yendor2 offsets by the lock count)",
          curgameBit != interactLockBitIndex(1) && curgameBit != interactLockBitIndex(608));
    check("curgame id 1 starts clear (yendor2)", !interactBitmapTest(&save, curgameBit));
    interactBitmapSet(&save, curgameBit);
    check("curgame id 1 set", interactBitmapTest(&save, curgameBit));
    check("lock id 1 still set after the curgame write (different bit)", interactBitmapTest(&save, interactLockBitIndex(1)));
}

static void testSelectBranch(void) {
    WorldObjectRecord obj;

    check("no record selects None", interactSelectBranch(NULL) == InteractBranchNone);

    obj.flags = WorldObjectFlagDoor;
    check("0x8000 selects Lock", interactSelectBranch(&obj) == InteractBranchLock);

    obj.flags = WorldObjectFlagDoor | WorldObjectFlagCurgameRecord;
    check("0x8000 wins over 0x4000 when both set", interactSelectBranch(&obj) == InteractBranchLock);

    obj.flags = WorldObjectFlagCurgameRecord;
    check("0x4000 selects Curgame", interactSelectBranch(&obj) == InteractBranchCurgame);

    obj.flags = WorldObjectFlagCurgameRecord | WorldObjectFlagFixedResponse | WorldObjectFlagMonsterSpawn;
    check("0x4000 wins over 0x1000 and 0x800", interactSelectBranch(&obj) == InteractBranchCurgame);

    obj.flags = WorldObjectFlagFixedResponse;
    check("0x1000 selects FixedResponse", interactSelectBranch(&obj) == InteractBranchFixedResponse);

    obj.flags = WorldObjectFlagFixedResponse | WorldObjectFlagMonsterSpawn;
    check("0x1000 wins over 0x800", interactSelectBranch(&obj) == InteractBranchFixedResponse);

    obj.flags = WorldObjectFlagMonsterSpawn;
    check("0x800 selects MonsterSpawn", interactSelectBranch(&obj) == InteractBranchMonsterSpawn);

    obj.flags = WorldObjectFlagUnknown2000;
    check("0x2000 alone selects None (never tested)", interactSelectBranch(&obj) == InteractBranchNone);

    obj.flags = 0;
    check("no recognized bit selects None", interactSelectBranch(&obj) == InteractBranchNone);
}

static void testClassifyLock(void) {
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));

    check("already unlocked -> None regardless of flags", interactClassifyLock(&lock, true) == InteractOutcomeNone);

    lock.flags = LockFlagMagical;
    check("magical -> LockMagical", interactClassifyLock(&lock, false) == InteractOutcomeLockMagical);

    lock.flags = LockFlagMagical | LockFlagUnknown40;
    check("magical wins over flag 0x40", interactClassifyLock(&lock, false) == InteractOutcomeLockMagical);

    lock.flags = LockFlagUnknown40;
    check("flag 0x40 -> LockFlag40", interactClassifyLock(&lock, false) == InteractOutcomeLockFlag40);

    lock.flags = 0;
    lock.price = 100;
    check("nonzero price, no magic/0x40 -> LockPriced", interactClassifyLock(&lock, false) == InteractOutcomeLockPriced);

    lock.price = 0;
    check("no magic/0x40, zero price -> None", interactClassifyLock(&lock, false) == InteractOutcomeNone);
}

static void testClassifyCurgame(void) {
    check("already triggered -> None regardless of flags", interactClassifyCurgame(0xFFFF, true) == InteractOutcomeNone);
    check("flag 0x10 -> CurgameFlag10", interactClassifyCurgame(0x10, false) == InteractOutcomeCurgameFlag10);
    check("flag 0x10 wins over 0x8/0x40/0x20", interactClassifyCurgame(0x10 | 0x8 | 0x40 | 0x20, false) == InteractOutcomeCurgameFlag10);
    check("flag 0x8 -> CurgameFlag8", interactClassifyCurgame(0x8, false) == InteractOutcomeCurgameFlag8);
    check("flag 0x8 wins over 0x40/0x20", interactClassifyCurgame(0x8 | 0x40 | 0x20, false) == InteractOutcomeCurgameFlag8);
    check("flag 0x40 -> CurgameFlag40", interactClassifyCurgame(0x40, false) == InteractOutcomeCurgameFlag40);
    check("flag 0x40 wins over 0x20", interactClassifyCurgame(0x40 | 0x20, false) == InteractOutcomeCurgameFlag40);
    check("flag 0x20 alone -> CurgameFallbackB", interactClassifyCurgame(0x20, false) == InteractOutcomeCurgameFallbackB);
    check("no low bits -> CurgameFallbackA", interactClassifyCurgame(0, false) == InteractOutcomeCurgameFallbackA);
}

static void testClassifyMonsterSpawn(void) {
    check("already spawned -> None", interactClassifyMonsterSpawn(true) == InteractOutcomeNone);
    check("not yet spawned -> UnspawnedMonster", interactClassifyMonsterSpawn(false) == InteractOutcomeUnspawnedMonster);
}

static void testFullDispatch(void) {
    WorldObjectRecord obj;
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));

    obj.flags = WorldObjectFlagDoor;
    lock.flags = LockFlagMagical;
    check("full dispatch: lock branch reaches interactClassifyLock",
          interactClassify(&obj, &lock, false, 0, false, false) == InteractOutcomeLockMagical);

    obj.flags = WorldObjectFlagCurgameRecord;
    check("full dispatch: curgame branch reaches interactClassifyCurgame",
          interactClassify(&obj, NULL, false, 0x8, false, false) == InteractOutcomeCurgameFlag8);

    obj.flags = WorldObjectFlagFixedResponse;
    check("full dispatch: fixed-response branch always returns errorCode 4",
          interactClassify(&obj, NULL, false, 0, false, false) == InteractOutcomeFixedResponse);

    obj.flags = WorldObjectFlagMonsterSpawn;
    check("full dispatch: monster-spawn branch, not yet spawned",
          interactClassify(&obj, NULL, false, 0, false, false) == InteractOutcomeUnspawnedMonster);
    check("full dispatch: monster-spawn branch, already spawned",
          interactClassify(&obj, NULL, false, 0, false, true) == InteractOutcomeNone);

    check("full dispatch: no record -> None", interactClassify(NULL, NULL, false, 0, false, false) == InteractOutcomeNone);
}

static void testWorldObjectBitIndex(void) {
    WorldObjectRecord door;
    door.flags = WorldObjectFlagDoor;
    door.value = 5;
    checkU32("a door's bit index is its own lock id's", interactWorldObjectBitIndex(GameYendor2, &door),
             interactLockBitIndex(5));

    WorldObjectRecord curgame;
    curgame.flags = WorldObjectFlagCurgameRecord;
    curgame.value = 3;
    checkU32("a curgame record's bit index is its own curgame id's", interactWorldObjectBitIndex(GameYendor2, &curgame),
             interactCurgameBitIndex(GameYendor2, 3));
}

static void testKnock(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    WorldObjectRecord door;
    door.flags = WorldObjectFlagDoor;
    door.value = 12;
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));
    lock.flags = LockFlagMagical;

    check("a magical, not-yet-unlocked door: knock succeeds",
          interactKnock(&save, GameYendor2, &door, &lock, false, 0, false, false));
    check("...and marks that lock's own bit", interactBitmapTest(&save, interactLockBitIndex(12)));

    saveGameInit(&save, GameYendor2);
    check("an already-unlocked door: knock fails (nothing to do)",
          !interactKnock(&save, GameYendor2, &door, &lock, true, 0, false, false));
    check("...and leaves the bit clear", !interactBitmapTest(&save, interactLockBitIndex(12)));

    saveGameInit(&save, GameYendor2);
    lock.flags = LockFlagUnknown40;
    check("a non-magical door (flag 0x40 instead): knock fails",
          !interactKnock(&save, GameYendor2, &door, &lock, false, 0, false, false));

    saveGameInit(&save, GameYendor2);
    WorldObjectRecord curgame;
    curgame.flags = WorldObjectFlagCurgameRecord;
    curgame.value = 4;
    check("a curgame record with flag 0x20 (fallback B), not yet triggered: knock succeeds",
          interactKnock(&save, GameYendor2, &curgame, NULL, false, 0x20, false, false));
    check("...and marks that curgame id's own bit", interactBitmapTest(&save, interactCurgameBitIndex(GameYendor2, 4)));

    saveGameInit(&save, GameYendor2);
    check("a curgame record with flag 0x10 (a different outcome): knock fails",
          !interactKnock(&save, GameYendor2, &curgame, NULL, false, 0x10, false, false));

    check("no object at all: knock fails", !interactKnock(&save, GameYendor2, NULL, NULL, false, 0, false, false));
}

static void testTriggerFacingCurgameEvent(void) {
    SaveGame save;
    saveGameInit(&save, GameYendor2);

    WorldObjectRecord curgame;
    curgame.flags = WorldObjectFlagCurgameRecord;
    curgame.value = 9;

    check("a curgame record with flag 0x10: triggers",
          interactTriggerFacingCurgameEvent(&save, GameYendor2, &curgame, NULL, false, 0x10, false, false));
    check("...and marks that curgame id's own bit", interactBitmapTest(&save, interactCurgameBitIndex(GameYendor2, 9)));

    saveGameInit(&save, GameYendor2);
    check("a curgame record with flag 0x8: triggers",
          interactTriggerFacingCurgameEvent(&save, GameYendor2, &curgame, NULL, false, 0x8, false, false));

    saveGameInit(&save, GameYendor2);
    check("a curgame record with flag 0x20 (fallback B, Knock's own set): does not trigger",
          !interactTriggerFacingCurgameEvent(&save, GameYendor2, &curgame, NULL, false, 0x20, false, false));

    saveGameInit(&save, GameYendor2);
    check("a curgame record with flag 0x40 (bit 0x40's own set): does not trigger",
          !interactTriggerFacingCurgameEvent(&save, GameYendor2, &curgame, NULL, false, 0x40, false, false));

    saveGameInit(&save, GameYendor2);
    check("an already-triggered curgame record: does not fire again",
          !interactTriggerFacingCurgameEvent(&save, GameYendor2, &curgame, NULL, false, 0x10, true, false));

    WorldObjectRecord door;
    door.flags = WorldObjectFlagDoor;
    door.value = 12;
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));
    lock.flags = LockFlagMagical;
    saveGameInit(&save, GameYendor2);
    check("a magical door (a lock outcome): never qualifies for this wrapper",
          !interactTriggerFacingCurgameEvent(&save, GameYendor2, &door, &lock, false, 0, false, false));
}

/* ApplyEncodedItemEffect's word_33302 bit 0x40 sibling's own qualifying set, exercised via
 * interactResolveIfOutcome directly rather than a dedicated wrapper (see interact.h). */
static void testResolveIfOutcomeBit0x40QualifyingSet(void) {
    static const InteractOutcome qualifying[] = {InteractOutcomeLockFlag40, InteractOutcomeLockPriced,
                                                  InteractOutcomeCurgameFlag40};

    SaveGame save;
    saveGameInit(&save, GameYendor2);

    WorldObjectRecord door;
    door.flags = WorldObjectFlagDoor;
    door.value = 7;
    LockRecord lock;
    memset(&lock, 0, sizeof(lock));
    lock.flags = LockFlagUnknown40;

    check("a flag-0x40 door: resolves", interactResolveIfOutcome(&save, GameYendor2, &door, &lock, false, 0, false,
                                                                   false, qualifying, 3));
    check("...and marks that lock's own bit", interactBitmapTest(&save, interactLockBitIndex(7)));

    saveGameInit(&save, GameYendor2);
    lock.flags = 0;
    lock.price = 50;
    check("a priced door (no magic/0x40): resolves too", interactResolveIfOutcome(&save, GameYendor2, &door, &lock,
                                                                                   false, 0, false, false, qualifying,
                                                                                   3));

    saveGameInit(&save, GameYendor2);
    lock.flags = LockFlagMagical;
    lock.price = 0;
    check("a magical door: doesn't qualify for this set (that's Knock's set instead)",
          !interactResolveIfOutcome(&save, GameYendor2, &door, &lock, false, 0, false, false, qualifying, 3));

    saveGameInit(&save, GameYendor2);
    WorldObjectRecord curgame;
    curgame.flags = WorldObjectFlagCurgameRecord;
    curgame.value = 9;
    check("a curgame record with flag 0x40: resolves",
          interactResolveIfOutcome(&save, GameYendor2, &curgame, NULL, false, 0x40, false, false, qualifying, 3));
    check("...and marks that curgame id's own bit", interactBitmapTest(&save, interactCurgameBitIndex(GameYendor2, 9)));

    saveGameInit(&save, GameYendor2);
    check("a curgame record with flag 0x20 (Knock's own fallback-B): doesn't qualify for this set",
          !interactResolveIfOutcome(&save, GameYendor2, &curgame, NULL, false, 0x20, false, false, qualifying, 3));
}

static void testKeyMatching(void) {
    /* real item words: BRASS CHEST KEY 0x8090, GOLD DOOR KEY 0x0250, KEY RING 0x0030 */
    uint16_t brassChest = 0x8090, goldChest = 0x0290, brassDoor = 0x8050, goldDoor = 0x0250, ring = 0x0030;
    uint16_t brassLock = LockFlagKeyBrass, goldLock = LockFlagKeyGold;

    check("a brass chest key opens a brass chest lock", interactKeyOpens(true, brassChest, 0, brassLock));
    check("...not a gold one", !interactKeyOpens(true, brassChest, 0, goldLock));
    check("a gold chest key opens a gold lock", interactKeyOpens(true, goldChest, 0, goldLock));
    check("a door key does not open the chest path", !interactKeyOpens(true, brassDoor, 0, brassLock));
    check("a door key opens a brass door", interactKeyOpens(false, brassDoor, 0, brassLock));
    check("a chest key does not open the door path", !interactKeyOpens(false, brassChest, 0, brassLock));
    check("a lock needing several tiers opens with any one of them",
          interactKeyOpens(false, goldDoor, 0, (uint16_t)(brassLock | goldLock)));

    uint16_t held = 0;
    held |= interactKeyRingContribution(brassDoor);
    held |= interactKeyRingContribution(goldChest);
    check("placing a chest key adds nothing to the ring", held == 0x0080);
    held |= interactKeyRingContribution(goldDoor);
    check("door keys accumulate their tier bytes", held == 0x0082);

    check("the key ring opens a door whose tier it has collected", interactKeyOpens(false, ring, held, goldLock));
    check("...but not one it has not", !interactKeyOpens(false, ring, held, LockFlagKeyIron));
    check("...and it never opens the chest path (the ring's high byte is always empty)",
          !interactKeyOpens(true, ring, held, brassLock));
    check("a non-key item opens nothing", !interactKeyOpens(false, 0x0000, held, brassLock) && !interactKeyOpens(true, 0x0000, held, brassLock));
}

int main(void) {
    testBitmap();
    testSelectBranch();
    testClassifyLock();
    testClassifyCurgame();
    testClassifyMonsterSpawn();
    testFullDispatch();
    testWorldObjectBitIndex();
    testKnock();
    testTriggerFacingCurgameEvent();
    testResolveIfOutcomeBit0x40QualifyingSet();
    testKeyMatching();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

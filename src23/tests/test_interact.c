/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_interact test_interact.c ../interact.c ../savegame.c ../lockcatalog.c ../worldobjects.c && ./test_interact
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

int main(void) {
    testBitmap();
    testSelectBranch();
    testClassifyLock();
    testClassifyCurgame();
    testClassifyMonsterSpawn();
    testFullDispatch();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

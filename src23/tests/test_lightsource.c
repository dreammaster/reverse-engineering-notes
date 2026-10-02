/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_lightsource test_lightsource.c ../lightsource.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_lightsource
 */
#include <stdio.h>
#include <string.h>

#include "lightsource.h"

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
        printf("FAIL %s (got %u, expected %u)\n", label, actual, expected);
    }
}

static void testApplyRejectsUnrelatedActionIds(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("an unrelated action id is a no-op", !lightSourceApply(&state, 1));
    check("...and nothing is lit", state.litFlags == 0);
}

static void testApplyCandle(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("lighting the candle (item 8) succeeds", lightSourceApply(&state, 8));
    check("the candle's own flag bit (0x2000) is set", state.litFlags & 0x2000);
    checkU32("the candle's duration counter is incremented", state.litCount[LightSourceCandle], 1);
    checkU32("the other two counters are untouched", state.litCount[LightSourceTorch] + state.litCount[LightSourceGeneric], 0);
}

static void testApplyTorch(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("lighting the torch (item 0xE) succeeds", lightSourceApply(&state, 0xE));
    check("the torch's own flag bit (0x800) is set", state.litFlags & 0x0800);
    checkU32("the torch's duration counter is incremented", state.litCount[LightSourceTorch], 1);
}

static void testApplyGeneric(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("lighting the generic light source (item 0xB) succeeds", lightSourceApply(&state, 0xB));
    check("its own flag bit (0x400) is set", state.litFlags & 0x0400);
    checkU32("its duration counter is incremented", state.litCount[LightSourceGeneric], 1);
}

static void testApplyAgainExtendsRatherThanResets(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    lightSourceApply(&state, 8);
    lightSourceApply(&state, 8);
    lightSourceApply(&state, 8);
    checkU32("lighting an already-lit candle extends its duration rather than resetting it",
             state.litCount[LightSourceCandle], 3);
}

static void testTickRejectsUnrelatedActionIds(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceCandle] = 5;
    check("an unrelated action id is a no-op", !lightSourceTick(&state, 1));
    checkU32("...and the counter is untouched", state.litCount[LightSourceCandle], 5);
}

static void testTickDecrementsWithoutExpiring(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceCandle] = 5;
    state.litFlags = 0x2000;
    check("ticking a lit candle (item 9) succeeds", lightSourceTick(&state, 9));
    checkU32("its duration decrements by 1", state.litCount[LightSourceCandle], 4);
    check("it's still lit", state.litFlags & 0x2000);
}

static void testTickExpiresAndClearsTheFlag(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceTorch] = 1;
    state.litFlags = 0x0800;
    check("ticking a lit torch (item 0xF) with 1 minute left succeeds", lightSourceTick(&state, 0xF));
    checkU32("its duration reaches 0", state.litCount[LightSourceTorch], 0);
    check("...and it goes out (flag cleared)", !(state.litFlags & 0x0800));
}

static void testTickOnAnAlreadyExpiredSourceStaysExpired(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceGeneric] = 0;
    state.litFlags = 0;
    check("ticking an already-out generic light (item 0xC) is still a dispatched match",
          lightSourceTick(&state, 0xC));
    checkU32("its duration stays clamped at 0, no underflow", state.litCount[LightSourceGeneric], 0);
    check("...and it stays unlit", !(state.litFlags & 0x0400));
}

static void testTickingOneSourceDoesNotAffectTheOthers(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceCandle] = 3;
    state.litCount[LightSourceTorch] = 3;
    state.litCount[LightSourceGeneric] = 3;
    state.litFlags = 0x2000 | 0x0800 | 0x0400;
    lightSourceTick(&state, 9); /* candle only */
    checkU32("the candle ticked down", state.litCount[LightSourceCandle], 2);
    checkU32("the torch is untouched", state.litCount[LightSourceTorch], 3);
    checkU32("the generic light is untouched", state.litCount[LightSourceGeneric], 3);
    check("all three are still lit except what expired (none did)",
          (state.litFlags & (0x2000 | 0x0800 | 0x0400)) == (0x2000 | 0x0800 | 0x0400));
}

static void testTickItemSlotRejectsUnrelatedIds(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    uint8_t slot[4];
    itemSlotSet(slot, 1 /* plain CANDLE, not lit */, 50);

    check("an unlit item is a no-op", !lightSourceTickItemSlot(&state, slot, 5));
    checkU32("...and its own extra is untouched", itemSlotExtra(slot), 50);
}

static void testTickItemSlotDecrementsWithoutExpiring(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceCandle] = 1;
    state.litFlags = 0x2000;
    uint8_t slot[4];
    itemSlotSet(slot, 9 /* LIT CANDLE */, 20);

    check("ticking a lit candle with time remaining succeeds", lightSourceTickItemSlot(&state, slot, 7));
    checkU32("its own id is unchanged (still lit)", itemSlotId(slot), 9);
    checkU32("its own extra decrements by the elapsed time", itemSlotExtra(slot), 13);
    check("it's still lit", state.litFlags & 0x2000);
    checkU32("the instance count is untouched", state.litCount[LightSourceCandle], 1);
}

static void testTickItemSlotExpiresAdvancesIdAndDecrementsInstanceCount(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceTorch] = 1;
    state.litFlags = 0x0800;
    uint8_t slot[4];
    itemSlotSet(slot, 0xF /* LIT TORCH */, 4);

    check("ticking a lit torch past its remaining time succeeds", lightSourceTickItemSlot(&state, slot, 10));
    checkU32("its own id advances to the 'used' item (0xF -> 0x10, real USED TORCH)", itemSlotId(slot), 0x10);
    checkU32("its own extra is zeroed", itemSlotExtra(slot), 0);
    checkU32("the instance count drops to 0", state.litCount[LightSourceTorch], 0);
    check("...and the last instance burning out clears the lit flag", !(state.litFlags & 0x0800));
}

static void testTickItemSlotMultipleInstancesKeepFlagLitUntilTheLastOneExpires(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceGeneric] = 2; /* two lit instances carried at once */
    state.litFlags = 0x0400;
    uint8_t slot[4];
    itemSlotSet(slot, 0xC /* LIT LIGHT */, 1);

    check("the first of two lit instances expiring succeeds", lightSourceTickItemSlot(&state, slot, 5));
    checkU32("its own id advances (0xC -> 0xD, real USED LIGHT)", itemSlotId(slot), 0xD);
    checkU32("the instance count drops to 1, not 0", state.litCount[LightSourceGeneric], 1);
    check("the flag stays lit -- another instance is still burning", state.litFlags & 0x0400);
}

static void testTickItemSlotExactlyOnTimeExpires(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.litCount[LightSourceCandle] = 1;
    uint8_t slot[4];
    itemSlotSet(slot, 9, 5);

    lightSourceTickItemSlot(&state, slot, 5); /* elapsed exactly equals remaining */
    checkU32("an elapsed time exactly equal to the remaining duration still expires it", itemSlotId(slot), 10);
}

static void testArmSpellTimer(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("slot 3 arms", lightSourceArmSpellTimer(&state, 3, 15));
    checkU32("timer 3 holds the duration", state.timers[2], 15);
    checkU32("flags: pending + 0x40", state.litFlags, 0x8040);
    check("slot 0 rejected", !lightSourceArmSpellTimer(&state, 0, 1));
    check("slot 7 rejected", !lightSourceArmSpellTimer(&state, 7, 1));
}

static void testTickTimers(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    lightSourceTickTimers(&state, 5);
    checkU32("no pending flag: nothing happens", state.litFlags, 0);
    lightSourceArmSpellTimer(&state, 1, 12);
    lightSourceArmSpellTimer(&state, 6, 3);
    lightSourceTickTimers(&state, 5);
    checkU32("timer 1 counts down", state.timers[0], 7);
    checkU32("timer 6 expired and zeroed", state.timers[5], 0);
    check("timer 6's bit (0x8) cleared", !(state.litFlags & 0x8));
    check("timer 1's bit (0x100) still set", state.litFlags & 0x100);
    check("pending re-armed while something still runs", state.litFlags & 0x8000);
    lightSourceTickTimers(&state, 10);
    checkU32("timer 1 expires", state.timers[0], 0);
    check("nothing running: pending flag stays clear", !(state.litFlags & 0x8000));
    checkU32("all timer bits clear", state.litFlags & 0x1F8, 0);
}

int main(void) {
    testApplyRejectsUnrelatedActionIds();
    testApplyCandle();
    testApplyTorch();
    testApplyGeneric();
    testApplyAgainExtendsRatherThanResets();
    testTickRejectsUnrelatedActionIds();
    testTickDecrementsWithoutExpiring();
    testTickExpiresAndClearsTheFlag();
    testTickOnAnAlreadyExpiredSourceStaysExpired();
    testTickingOneSourceDoesNotAffectTheOthers();
    testTickItemSlotRejectsUnrelatedIds();
    testTickItemSlotDecrementsWithoutExpiring();
    testTickItemSlotExpiresAdvancesIdAndDecrementsInstanceCount();
    testTickItemSlotMultipleInstancesKeepFlagLitUntilTheLastOneExpires();
    testTickItemSlotExactlyOnTimeExpires();
    testArmSpellTimer();
    testTickTimers();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

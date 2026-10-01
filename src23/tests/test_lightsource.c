/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_lightsource test_lightsource.c ../lightsource.c && ./test_lightsource
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
    checkU32("the candle's duration counter is incremented", state.duration[LightSourceCandle], 1);
    checkU32("the other two counters are untouched", state.duration[LightSourceTorch] + state.duration[LightSourceGeneric], 0);
}

static void testApplyTorch(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("lighting the torch (item 0xE) succeeds", lightSourceApply(&state, 0xE));
    check("the torch's own flag bit (0x800) is set", state.litFlags & 0x0800);
    checkU32("the torch's duration counter is incremented", state.duration[LightSourceTorch], 1);
}

static void testApplyGeneric(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    check("lighting the generic light source (item 0xB) succeeds", lightSourceApply(&state, 0xB));
    check("its own flag bit (0x400) is set", state.litFlags & 0x0400);
    checkU32("its duration counter is incremented", state.duration[LightSourceGeneric], 1);
}

static void testApplyAgainExtendsRatherThanResets(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    lightSourceApply(&state, 8);
    lightSourceApply(&state, 8);
    lightSourceApply(&state, 8);
    checkU32("lighting an already-lit candle extends its duration rather than resetting it",
             state.duration[LightSourceCandle], 3);
}

static void testTickRejectsUnrelatedActionIds(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.duration[LightSourceCandle] = 5;
    check("an unrelated action id is a no-op", !lightSourceTick(&state, 1));
    checkU32("...and the counter is untouched", state.duration[LightSourceCandle], 5);
}

static void testTickDecrementsWithoutExpiring(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.duration[LightSourceCandle] = 5;
    state.litFlags = 0x2000;
    check("ticking a lit candle (item 9) succeeds", lightSourceTick(&state, 9));
    checkU32("its duration decrements by 1", state.duration[LightSourceCandle], 4);
    check("it's still lit", state.litFlags & 0x2000);
}

static void testTickExpiresAndClearsTheFlag(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.duration[LightSourceTorch] = 1;
    state.litFlags = 0x0800;
    check("ticking a lit torch (item 0xF) with 1 minute left succeeds", lightSourceTick(&state, 0xF));
    checkU32("its duration reaches 0", state.duration[LightSourceTorch], 0);
    check("...and it goes out (flag cleared)", !(state.litFlags & 0x0800));
}

static void testTickOnAnAlreadyExpiredSourceStaysExpired(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.duration[LightSourceGeneric] = 0;
    state.litFlags = 0;
    check("ticking an already-out generic light (item 0xC) is still a dispatched match",
          lightSourceTick(&state, 0xC));
    checkU32("its duration stays clamped at 0, no underflow", state.duration[LightSourceGeneric], 0);
    check("...and it stays unlit", !(state.litFlags & 0x0400));
}

static void testTickingOneSourceDoesNotAffectTheOthers(void) {
    LightSourceState state;
    memset(&state, 0, sizeof(state));
    state.duration[LightSourceCandle] = 3;
    state.duration[LightSourceTorch] = 3;
    state.duration[LightSourceGeneric] = 3;
    state.litFlags = 0x2000 | 0x0800 | 0x0400;
    lightSourceTick(&state, 9); /* candle only */
    checkU32("the candle ticked down", state.duration[LightSourceCandle], 2);
    checkU32("the torch is untouched", state.duration[LightSourceTorch], 3);
    checkU32("the generic light is untouched", state.duration[LightSourceGeneric], 3);
    check("all three are still lit except what expired (none did)",
          (state.litFlags & (0x2000 | 0x0800 | 0x0400)) == (0x2000 | 0x0800 | 0x0400));
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

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

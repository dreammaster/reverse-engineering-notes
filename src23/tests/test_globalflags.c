/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_globalflags test_globalflags.c ../globalflags.c && ./test_globalflags
 */
#include <stdio.h>
#include <string.h>

#include "globalflags.h"

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

static uint8_t g_buf[8]; /* 4 words = flag indices 1-64 */

static void reset(void) {
    memset(g_buf, 0, sizeof(g_buf));
}

static void testBitPacking(void) {
    /*
     * Words are stored little-endian (matching the original's real x86
     * memory layout), so the MSB-first *bit* convention lands in the
     * *second* (high) byte of each word, not the first.
     */
    reset();
    check("index 1 starts clear", !globalFlagTest(g_buf, sizeof(g_buf), 1));
    globalFlagSet(g_buf, sizeof(g_buf), 1);
    check("index 1 set", globalFlagTest(g_buf, sizeof(g_buf), 1));
    checkU32("index 1 sets the MSB of word 0, i.e. byte 1 (little-endian)", g_buf[1], 0x80);
    checkU32("...byte 0 untouched", g_buf[0], 0);

    reset();
    globalFlagSet(g_buf, sizeof(g_buf), 16);
    checkU32("index 16 sets the LSB of word 0 (byte 0), not word 1", g_buf[0], 0x01);
    checkU32("word 0's high byte is untouched", g_buf[1], 0);

    reset();
    globalFlagSet(g_buf, sizeof(g_buf), 17);
    checkU32("index 17 sets the MSB of word 1, i.e. byte 3", g_buf[3], 0x80);

    reset();
    globalFlagSet(g_buf, sizeof(g_buf), 15);
    checkU32("index 15 sets bit 1 (0x0002) of word 0, i.e. byte 0's bit 1", g_buf[0], 0x02);

    reset();
    globalFlagSet(g_buf, sizeof(g_buf), 32);
    checkU32("index 32 (last of word 1) sets word 1's LSB (byte 2)", g_buf[2], 0x01);

    reset();
    globalFlagSet(g_buf, sizeof(g_buf), 5);
    globalFlagSet(g_buf, sizeof(g_buf), 6);
    check("index 5 and 6 are both set", globalFlagTest(g_buf, sizeof(g_buf), 5) &&
                                             globalFlagTest(g_buf, sizeof(g_buf), 6));
    globalFlagClear(g_buf, sizeof(g_buf), 5);
    check("clearing index 5 leaves index 6 set", !globalFlagTest(g_buf, sizeof(g_buf), 5) &&
                                                      globalFlagTest(g_buf, sizeof(g_buf), 6));
}

static bool memcmpZero(void) {
    static const uint8_t zero[sizeof(g_buf)] = {0};
    return memcmp(g_buf, zero, sizeof(g_buf)) == 0;
}

static void testBoundsAndSigned(void) {
    reset();
    check("index 0 is invalid (1-based), reads clear", !globalFlagTest(g_buf, sizeof(g_buf), 0));
    globalFlagSet(g_buf, sizeof(g_buf), 0); /* should be a safe no-op */
    check("...and setting it is a safe no-op", memcmpZero());

    reset();
    check("an index past the buffer's capacity reads clear", !globalFlagTest(g_buf, sizeof(g_buf), 65));
    globalFlagSet(g_buf, sizeof(g_buf), 65); /* one past the 64-flag capacity of an 8-byte buffer */
    check("...and setting it is a safe no-op", memcmpZero());

    reset();
    globalFlagApplySigned(g_buf, sizeof(g_buf), 10);
    check("a positive signed index sets", globalFlagTest(g_buf, sizeof(g_buf), 10));
    globalFlagApplySigned(g_buf, sizeof(g_buf), -10);
    check("the matching negative signed index clears it", !globalFlagTest(g_buf, sizeof(g_buf), 10));
    globalFlagApplySigned(g_buf, sizeof(g_buf), 0); /* no-op */
    check("a zero signed index changes nothing", memcmpZero());
}

int main(void) {
    testBitPacking();
    testBoundsAndSigned();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

/*
 * Build and run:
 *   cc -I .. -o test_bcd4 test_bcd4.c ../bcd4.c && ./test_bcd4
 * or, from an MSVC developer prompt:
 *   cl /nologo /W4 /I .. test_bcd4.c ..\bcd4.c && test_bcd4.exe
 */
#include <stdio.h>
#include <string.h>

#include "bcd4.h"

static int g_failureCount = 0;

static void checkBcd4Equal(const char *label, const Bcd4 actual, const Bcd4 expected) {
    if (memcmp(actual, expected, sizeof(Bcd4)) != 0) {
        g_failureCount++;
        printf("FAIL %s: got %02X%02X%02X%02X, want %02X%02X%02X%02X\n", label,
               actual[0], actual[1], actual[2], actual[3],
               expected[0], expected[1], expected[2], expected[3]);
    } else {
        printf("PASS %s\n", label);
    }
}

static void checkBool(const char *label, bool actual, bool expected) {
    if (actual != expected) {
        g_failureCount++;
        printf("FAIL %s: got %d, want %d\n", label, actual, expected);
    } else {
        printf("PASS %s\n", label);
    }
}

static void testFromU16(void) {
    Bcd4 result;

    bcd4FromU16(result, 0);
    checkBcd4Equal("fromU16(0)", result, (Bcd4){0x00, 0x00, 0x00, 0x00});

    bcd4FromU16(result, 1);
    checkBcd4Equal("fromU16(1)", result, (Bcd4){0x00, 0x00, 0x00, 0x01});

    bcd4FromU16(result, 12345);
    checkBcd4Equal("fromU16(12345)", result, (Bcd4){0x00, 0x01, 0x23, 0x45});

    bcd4FromU16(result, 65535);
    checkBcd4Equal("fromU16(65535)", result, (Bcd4){0x00, 0x06, 0x55, 0x35});
}

static void testAdd(void) {
    Bcd4 counter;

    bcd4FromU16(counter, 1);
    bcd4AddU16(counter, 1);
    checkBcd4Equal("1+1=2", counter, (Bcd4){0x00, 0x00, 0x00, 0x02});

    /* Carry propagation across all 4 bytes: 9999 + 1 = 10000. */
    bcd4FromU16(counter, 9999);
    bcd4AddU16(counter, 1);
    checkBcd4Equal("9999+1=10000", counter, (Bcd4){0x00, 0x01, 0x00, 0x00});

    /* Near the 8-digit ceiling, no overflow yet. */
    Bcd4 big = {0x00, 0x01, 0x00, 0x00}; /* 10000 */
    bcd4AddU16(big, 23456);
    checkBcd4Equal("10000+23456=33456", big, (Bcd4){0x00, 0x03, 0x34, 0x56});
}

static void testSub(void) {
    Bcd4 counter;

    bcd4FromU16(counter, 100);
    bcd4SubU16(counter, 1);
    checkBcd4Equal("100-1=99", counter, (Bcd4){0x00, 0x00, 0x00, 0x99});

    /* Borrow propagation across all 4 bytes: 10000 - 1 = 9999. */
    bcd4FromU16(counter, 10000);
    bcd4SubU16(counter, 1);
    checkBcd4Equal("10000-1=9999", counter, (Bcd4){0x00, 0x00, 0x99, 0x99});

    /*
     * Raw underflow wraps like an odometer, matching the original's
     * unconditional DAS-based subtract (callers are expected to guard
     * with bcd4AtLeastU16 first, same as the disassembly's callers use
     * IsBCDCounterAtLeast before spending gold).
     */
    bcd4FromU16(counter, 0);
    bcd4SubU16(counter, 1);
    checkBcd4Equal("0-1 wraps to 99999999", counter, (Bcd4){0x99, 0x99, 0x99, 0x99});
}

static void testCompareAndThreshold(void) {
    Bcd4 counter;
    bcd4FromU16(counter, 100);

    checkBool("100 >= 100", bcd4AtLeastU16(counter, 100), true);
    checkBool("100 >= 99", bcd4AtLeastU16(counter, 99), true);
    checkBool("100 >= 101", bcd4AtLeastU16(counter, 101), false);

    Bcd4 zero;
    bcd4FromU16(zero, 0);
    checkBool("0 >= 0", bcd4AtLeastU16(zero, 0), true);
    checkBool("0 >= 1", bcd4AtLeastU16(zero, 1), false);
}

static void bcd4FromU32(Bcd4 out, uint32_t value) {
    for (int i = 3; i >= 0; i--) {
        uint8_t low = (uint8_t)(value % 10);
        value /= 10;
        uint8_t high = (uint8_t)(value % 10);
        value /= 10;
        out[i] = (uint8_t)((high << 4) | low);
    }
}

static uint32_t bcd4ToU32(const Bcd4 in) {
    uint32_t value = 0;
    for (int i = 0; i < 4; i++) {
        value = value * 100 + (uint32_t)(in[i] >> 4) * 10 + (in[i] & 0x0F);
    }
    return value;
}

static void testAddSubAgainstBinary(void) {
    const uint32_t modulus = 100000000u;
    int failures = 0;

    /* Half-carry regression: every 2-digit pair (e.g. 8+8, 99+99). */
    for (uint32_t a = 0; a < 100; a++) {
        for (uint32_t b = 0; b < 100; b++) {
            Bcd4 sum, diff, other;
            bcd4FromU32(sum, a);
            bcd4FromU32(diff, a);
            bcd4FromU32(other, b);
            bcd4Add(sum, other);
            bcd4Sub(diff, other);
            if (bcd4ToU32(sum) != a + b || bcd4ToU32(diff) != (a + modulus - b) % modulus) {
                failures++;
            }
        }
    }

    /* Pseudo-random pairs across the whole 8-digit range, incl. wraparound. */
    uint32_t state = 12345u;
    for (int i = 0; i < 200000; i++) {
        state = state * 1664525u + 1013904223u;
        uint32_t a = (state >> 3) % modulus;
        state = state * 1664525u + 1013904223u;
        uint32_t b = (state >> 3) % modulus;
        Bcd4 sum, diff, other;
        bcd4FromU32(sum, a);
        bcd4FromU32(diff, a);
        bcd4FromU32(other, b);
        bcd4Add(sum, other);
        bcd4Sub(diff, other);
        if (bcd4ToU32(sum) != (a + b) % modulus || bcd4ToU32(diff) != (a + modulus - b) % modulus) {
            if (failures++ < 5) {
                printf("FAIL add/sub %u, %u\n", a, b);
            }
        }
    }

    if (failures > 0) {
        g_failureCount++;
        printf("FAIL add/sub vs binary: %d mismatches\n", failures);
    } else {
        printf("PASS add/sub vs binary (exhaustive 2-digit + 200k random 8-digit)\n");
    }
}

static void testShifts(void) {
    Bcd4 value = {0x00, 0x12, 0x34, 0x56};
    bcd4ShiftLeftNibble(value);
    checkBcd4Equal("shiftLeft(123456)", value, (Bcd4){0x01, 0x23, 0x45, 0x60});

    /* The top digit is lost on overflow. */
    Bcd4 full = {0x91, 0x23, 0x45, 0x67};
    bcd4ShiftLeftNibble(full);
    checkBcd4Equal("shiftLeft(91234567) drops top digit", full, (Bcd4){0x12, 0x34, 0x56, 0x70});

    Bcd4 right = {0x12, 0x34, 0x56, 0x78};
    bcd4ShiftRightNibble(right);
    checkBcd4Equal("shiftRight(12345678)", right, (Bcd4){0x01, 0x23, 0x45, 0x67});
}

static void checkMulPercent(const char *label, uint32_t value, uint16_t percent, uint32_t expected) {
    Bcd4 counter;
    Bcd4 want;
    bcd4FromU32(counter, value);
    bcd4FromU32(want, expected);
    bcd4MulPercent(counter, percent);
    checkBcd4Equal(label, counter, want);
}

static void testMulPercent(void) {
    checkMulPercent("1000 * 100% = 1000", 1000, 100, 1000);
    checkMulPercent("250 * 110% = 275", 250, 110, 275);
    checkMulPercent("5 * 50% = 3 (2.5 rounds half-up)", 5, 50, 3);
    checkMulPercent("4 * 50% = 2", 4, 50, 2);
    checkMulPercent("999 * 55% = 549", 999, 55, 549);
    checkMulPercent("0 * 155% = 0", 0, 155, 0);
    checkMulPercent("12345678 * 100% = 12345678", 12345678, 100, 12345678);
    checkMulPercent("99999999 * 100% = 99999999", 99999999, 100, 99999999);
    checkMulPercent("100 * 45% = 45 (barter discount tier)", 100, 45, 45);

    /*
     * Differential sweep against the closed form. Restricted to percent
     * <= 7270 (9 * 7270 + 50 < 65536, so the original's 16-bit digit
     * products can't truncate) and results < 10^8 (no digits shifted off
     * the top).
     */
    static const uint16_t percents[] = {1, 2, 8, 15, 25, 35, 45, 55, 98, 100, 102, 108, 115, 145, 155, 199, 500, 1000, 7270};
    int sweepFailures = 0;
    for (size_t p = 0; p < sizeof(percents) / sizeof(percents[0]); p++) {
        for (uint32_t value = 0; value < 100000000u; value += 7919u) {
            /* closed form: floor((low3*W + 50)/100) + floor(V/1000)*W*10 */
            uint64_t low3 = value % 1000;
            uint64_t high = value / 1000;
            uint64_t expected = (low3 * percents[p] + 50) / 100 + high * percents[p] * 10;
            if (expected >= 100000000u) {
                continue;
            }
            Bcd4 counter;
            bcd4FromU32(counter, value);
            bcd4MulPercent(counter, percents[p]);
            if (bcd4ToU32(counter) != (uint32_t)expected) {
                if (sweepFailures++ < 5) {
                    printf("FAIL sweep %u * %u%%: got %u, want %u\n", value, percents[p],
                           bcd4ToU32(counter), (uint32_t)expected);
                }
            }
        }
    }
    if (sweepFailures > 0) {
        g_failureCount++;
        printf("FAIL mulPercent sweep: %d mismatches\n", sweepFailures);
    } else {
        printf("PASS mulPercent sweep vs closed form\n");
    }
}

int main(void) {
    testFromU16();
    testAdd();
    testSub();
    testCompareAndThreshold();
    testAddSubAgainstBinary();
    testShifts();
    testMulPercent();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

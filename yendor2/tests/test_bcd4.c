/*
 * Build and run:
 *   cc -I ../src -o test_bcd4 test_bcd4.c ../src/bcd4.c && ./test_bcd4
 * or, from an MSVC developer prompt:
 *   cl /nologo /W4 /I ..\src test_bcd4.c ..\src\bcd4.c && test_bcd4.exe
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

int main(void) {
    testFromU16();
    testAdd();
    testSub();
    testCompareAndThreshold();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

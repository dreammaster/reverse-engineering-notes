/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_random test_random.c ../random.c && ./test_random
 */
#include <stdio.h>

#include "random.h"

static int g_failureCount = 0;

static void check(const char *label, int ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

static void testBasics(void) {
    RandomState rng;
    randomStart(&rng, 12, 34);
    check("bound 0 returns 0 and doesn't seed", randomInRange(&rng, 0) == 0 && rng.seed == 0);
    randomInRange(&rng, 10);
    check("the first real call seeds the generator", rng.seed != 0);

    RandomState a, b;
    randomStart(&a, 5, 6);
    randomStart(&b, 5, 6);
    int same = 1;
    for (int i = 0; i < 1000; i++) {
        if (randomInRange(&a, 100) != randomInRange(&b, 100)) {
            same = 0;
        }
    }
    check("the same clock reading gives the same sequence", same);

    randomStart(&b, 5, 7);
    int differs = 0;
    randomStart(&a, 5, 6);
    for (int i = 0; i < 100; i++) {
        if (randomInRange(&a, 1000) != randomInRange(&b, 1000)) {
            differs = 1;
        }
    }
    check("a different clock reading changes the sequence", differs);
}

/* The result is 0..bound inclusive, and the top of the range is reachable. */
static void testRange(void) {
    static const uint16_t bounds[] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 15, 16, 17, 55, 100, 255, 256, 1000, 32768, 65535};
    int inRange = 1;
    int reachesBound = 1;
    int reachesZero = 1;
    for (size_t i = 0; i < sizeof(bounds) / sizeof(bounds[0]); i++) {
        RandomState rng;
        randomStart(&rng, 30, 77);
        int hitBound = 0;
        int hitZero = 0;
        for (int n = 0; n < 200000; n++) {
            uint16_t value = randomInRange(&rng, bounds[i]);
            if (value > bounds[i]) {
                inRange = 0;
            }
            if (value == bounds[i]) {
                hitBound = 1;
            }
            if (value == 0) {
                hitZero = 1;
            }
        }
        if (bounds[i] <= 1000 && !hitBound) {
            reachesBound = 0;
            printf("  bound %u was never returned\n", bounds[i]);
        }
        if (bounds[i] <= 1000 && !hitZero) {
            reachesZero = 0;
        }
    }
    check("every result is <= its bound", inRange);
    check("the bound itself is returned (inclusive range)", reachesBound);
    check("zero is returned", reachesZero);
}

/* With bound 4 the mask is 7, so 1..3 get two draws each while 0 and 4 get one. */
static void testDistribution(void) {
    RandomState rng;
    randomStart(&rng, 1, 2);
    long counts[5] = {0, 0, 0, 0, 0};
    const long draws = 400000;
    for (long n = 0; n < draws; n++) {
        counts[randomInRange(&rng, 4)]++;
    }
    long tail = counts[0] + counts[4];
    long middle = counts[1] + counts[2] + counts[3];
    check("bound 4: the middle values are about twice as likely as the ends",
          middle > tail * 2 * 0.9 * 3 / 2 - 1 && middle < tail * 2 * 1.1 * 3 / 2 + 1);
    printf("  counts for bound 4: %ld %ld %ld %ld %ld\n", counts[0], counts[1], counts[2], counts[3], counts[4]);
}

int main(void) {
    testBasics();
    testRange();
    testDistribution();
    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

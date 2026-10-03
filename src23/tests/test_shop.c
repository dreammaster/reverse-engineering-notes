/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_shop test_shop.c ../shop.c ../bcd4.c && ./test_shop
 */
#include <stdio.h>
#include <string.h>

#include "shop.h"

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

static bool isValue(const Bcd4 v, uint16_t expected) {
    Bcd4 e;
    bcd4FromU16(e, expected);
    return bcd4Compare(v, e) == 0;
}

static void testTiers(void) {
    checkU32("0 -> 55%", shopBarterAdjustment(0), 55);
    checkU32("54 -> 55%", shopBarterAdjustment(54), 55);
    checkU32("55 -> 45%", shopBarterAdjustment(55), 45);
    checkU32("64 -> 45%", shopBarterAdjustment(64), 45);
    checkU32("65 -> 35%", shopBarterAdjustment(65), 35);
    checkU32("79 -> 35%", shopBarterAdjustment(79), 35);
    checkU32("80 -> 25%", shopBarterAdjustment(80), 25);
    checkU32("100 -> 25%", shopBarterAdjustment(100), 25);
    checkU32("101 -> 15%", shopBarterAdjustment(101), 15);
    checkU32("124 -> 15%", shopBarterAdjustment(124), 15);
    checkU32("125 -> 8%", shopBarterAdjustment(125), 8);
    checkU32("149 -> 8%", shopBarterAdjustment(149), 8);
    checkU32("150 -> 2%", shopBarterAdjustment(150), 2);
    checkU32("999 -> 2%", shopBarterAdjustment(999), 2);
    checkU32("1000 -> back to 55% (the fall-through default)", shopBarterAdjustment(1000), 55);
    checkU32("a negative stat is 55%", shopBarterAdjustment((uint16_t)-5), 55);
}

static void testPrices(void) {
    Bcd4 base, out;
    bcd4FromU16(base, 1000);
    shopBuyPrice(base, 0, out);
    check("55% markup on 1000 gold = 1550", isValue(out, 1550));
    shopSellPrice(base, 0, out);
    check("selling back at 55% off = 450", isValue(out, 450));
    shopBuyPrice(base, 200, out);
    check("a master haggler (2%) pays 1020", isValue(out, 1020));
    shopSellPrice(base, 200, out);
    check("...and sells for 980", isValue(out, 980));
    check("the base price is not modified", isValue(base, 1000));
}

static void testPay(void) {
    Bcd4 gold, price;
    bool drained = true;
    bcd4FromU16(gold, 100);
    bcd4FromU16(price, 150);
    check("can't afford it", !shopPay(gold, price, &drained));
    check("...gold untouched", isValue(gold, 100));
    bcd4FromU16(price, 60);
    check("affordable", shopPay(gold, price, &drained));
    check("gold reduced", isValue(gold, 40));
    check("not drained", !drained);
    bcd4FromU16(price, 40);
    check("paying the exact balance works", shopPay(gold, price, &drained));
    check("...and reports the exact drain", drained && isValue(gold, 0));
}

int main(void) {
    testTiers();
    testPrices();
    testPay();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

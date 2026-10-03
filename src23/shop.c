#include "shop.h"

#include <string.h>

unsigned shopBarterAdjustment(uint16_t bartering) {
    static const struct {
        int16_t limit;
        unsigned percent;
    } tiers[] = {{0x36, 55}, {0x40, 45}, {0x4F, 35}, {0x64, 25}, {0x7C, 15}, {0x95, 8}, {0x3E7, 2}};
    for (unsigned i = 0; i < sizeof(tiers) / sizeof(tiers[0]); i++) {
        if ((int16_t)bartering <= tiers[i].limit) {
            return tiers[i].percent;
        }
    }
    return 55;
}

void shopBuyPrice(const Bcd4 base, uint16_t bartering, Bcd4 out) {
    memcpy(out, base, sizeof(Bcd4));
    bcd4MulPercent(out, (uint16_t)(100 + shopBarterAdjustment(bartering)));
}

void shopSellPrice(const Bcd4 base, uint16_t bartering, Bcd4 out) {
    memcpy(out, base, sizeof(Bcd4));
    bcd4MulPercent(out, (uint16_t)(100 - shopBarterAdjustment(bartering)));
}

bool shopPay(Bcd4 gold, const Bcd4 price, bool *drained) {
    int cmp = bcd4Compare(gold, price);
    if (cmp < 0) {
        return false;
    }
    *drained = cmp == 0;
    bcd4Sub(gold, price);
    return true;
}

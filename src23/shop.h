#ifndef YENDOR23_SHOP_H
#define YENDOR23_SHOP_H

#include <stdbool.h>
#include <stdint.h>

#include "bcd4.h"

/*
 * Shop pricing and the gold exchange (ComputeBarterPricingPreview,
 * yendor2.asm:22663; PayGoldAndAcquireItem :13049; SellClickedCatalogItem
 * :13101; instruction-identical in Chapter 3). The shop screen itself and its
 * stock are lockcatalog.h's records and RunShopScreen's UI; this is the
 * arithmetic.
 *
 * The haggling party member (g_partyRoleAssignment1, the one chosen as the
 * speaker) sets a percentage by their Bartering stat: the better the stat the
 * smaller the adjustment, in seven descending tiers:
 *
 *    Bartering <= 54   55%        <= 124   15%
 *            <= 64     45%        <= 149    8%
 *            <= 79     35%        <= 999    2%
 *            <= 100    25%        (above 999, or negative: 55%)
 *
 * (signed compares; the last case is the original's fall-through default).
 * Buying costs base x (100 + adjustment)%, selling back pays base x
 * (100 - adjustment)%, both through bcd4MulPercent's rounding.
 */
unsigned shopBarterAdjustment(uint16_t bartering);

/* base x (100 + shopBarterAdjustment)% */
void shopBuyPrice(const Bcd4 base, uint16_t bartering, Bcd4 out);

/* base x (100 - shopBarterAdjustment)% */
void shopSellPrice(const Bcd4 base, uint16_t bartering, Bcd4 out);

/*
 * PayGoldAndAcquireItem's payment: false (gold untouched) if gold < price,
 * otherwise gold -= price. *drained is set when the payment leaves exactly
 * zero (the original shows its "resource depleted" overlay on that exact-
 * equality case -- the CompareBCD4 zero flag -- once per visit).
 */
bool shopPay(Bcd4 gold, const Bcd4 price, bool *drained);

#endif

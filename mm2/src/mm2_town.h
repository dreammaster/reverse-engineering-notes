/* Town services: temple, mage guild prices (docs/shops.md). */
#ifndef MM2_TOWN_H
#define MM2_TOWN_H

#include "mm2_combat.h"
#include "mm2_state.h"

/* Compressed price code: (c & 1Fh), x10 if bit 5, x100 if bit 6, x1000 if bit 7. */
uint32_t mm2_price_decode(int code);

/* Temple: cost to restore a character's condition (0 if nothing to restore). */
uint32_t mm2_temple_restore_cost(const Mm2Char *c, int town);
/* Temple: cost to restore the original alignment (0 if unchanged). */
uint32_t mm2_temple_alignment_cost(const Mm2Char *c, int town);
uint32_t mm2_temple_donation_cost(int town);

/* Spells on sale: spell index (0-95) and price; n = number of entries (3 temple, 4 guild). */
int mm2_temple_stock(int town, int spell[4], uint32_t price[4]);
int mm2_guild_stock(int town, int spell[4], uint32_t price[4]);

typedef enum { MM2_SHOP_OK, MM2_SHOP_NO_GOLD, MM2_SHOP_WRONG_CLASS, MM2_SHOP_LEVEL_TOO_LOW, MM2_SHOP_KNOWN, MM2_SHOP_NOTHING_TO_DO } Mm2ShopResult;

/* Gold helpers (character +66h). */
int mm2_char_pay(Mm2Char *c, uint32_t amount);   /* 1 on success, 0 if not enough gold */

/* Temple services; `town` 0-4. */
Mm2ShopResult mm2_temple_restore(Mm2Char *c, int town);       /* heals fully and clears the condition */
Mm2ShopResult mm2_temple_restore_alignment(Mm2Char *c, int town);
/* Donation: 100 x multiplier gold.  *blessed is set for the 90 % case in which the party gets the blessings. */
Mm2ShopResult mm2_temple_donate(Mm2Char *payer, Mm2State *state, int town, int *blessed, const Mm2Rng *rng);

/* Buying a spell from the temple (Paladin/Cleric list) or the mage guild (Archer/Sorcerer list). */
Mm2ShopResult mm2_buy_spell(Mm2Char *c, int spell, uint32_t price);

#endif

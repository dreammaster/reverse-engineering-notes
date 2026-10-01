/* Town services: temple, mage guild prices (docs/shops.md). */
#ifndef MM2_TOWN_H
#define MM2_TOWN_H

#include "mm2_data.h"

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

#endif

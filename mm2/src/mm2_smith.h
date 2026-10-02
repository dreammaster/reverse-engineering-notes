/* Blacksmith stock and prices (docs/shops.md; 2SMITH smith_load_stock 1C8E0, smith_item_price 1C7FC). */
#ifndef MM2_SMITH_H
#define MM2_SMITH_H

#include "mm2_data.h"

typedef struct {
	uint8_t item;
	uint8_t bonus;     /* + n shown after the name (low 6 bits of the flag byte) */
	uint8_t charges;
} Mm2SmithSlot;

typedef enum { MM2_SMITH_BUY_A = 1, MM2_SMITH_BUY_B = 2, MM2_SMITH_BUY_C = 3, MM2_SMITH_BUY_D = 4, MM2_SMITH_SELL = 5, MM2_SMITH_IDENTIFY = 6 } Mm2SmithMode;

/* Six slots for category 1-4 of `town` (0-4); dayOfYear (1-180) drives the bonus of category 2. */
void mm2_smith_stock(int town, int category, int dayOfYear, Mm2SmithSlot out[6]);

/* Price in gold.  merchant = the character has the Merchant skill. */
uint32_t mm2_smith_price(const Mm2Item *item, int bonus, Mm2SmithMode mode, int merchant);

typedef enum { MM2_SMITH_DONE, MM2_SMITH_NO_GOLD, MM2_SMITH_PACK_FULL, MM2_SMITH_NO_ITEM, MM2_SMITH_DISABLED } Mm2SmithResult;

/* Buys the slot's item into the first free backpack slot (item id +3Ah, charges +40h, bonus +46h). */
Mm2SmithResult mm2_smith_buy(Mm2Char *c, const Mm2SmithSlot *slot, const Mm2Item *items, int merchant);
/* Sells backpack item `packSlot` (0-5) for the sell price. */
Mm2SmithResult mm2_smith_sell(Mm2Char *c, int packSlot, const Mm2Item *items, int merchant);

#endif

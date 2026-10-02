/* Treasure sharing and handing out items (ovl/2MISC.asm treasure_share 0x1C64A, treasure_give_item 0x1C538). */
#ifndef MM2_TREASURE_H
#define MM2_TREASURE_H

#include "mm2_reward.h"
#include "mm2_inn.h"

typedef struct {
	uint32_t gold;
	int gems;
	Mm2TreasureItem items[3];   /* item 0 = empty */
} Mm2Treasure;

typedef struct {
	uint32_t goldShare;
	int gemShare;
	int foundBy[3];             /* party slot that received item i, or -1 */
	int backpacksFull;          /* at least one item could not be given */
} Mm2ShareResult;

/* Shares gold (among the non-hireling members) and gems (among everyone), then gives each item to the first member with
 * a free backpack slot.  The treasure is cleared. */
Mm2ShareResult mm2_treasure_share(Mm2Roster *r, Mm2Treasure *t);

#endif

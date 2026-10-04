#ifndef YENDOR23_SHOPGRID_H
#define YENDOR23_SHOPGRID_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"
#include "item.h"
#include "viewrender.h"

/*
 * The shop's item grid (DrawShopItemSlotGrid, yendor2.asm:12162; Chapter 3 identical): a 72 x 35 area at (241, 160) is filled with colour
 * 4, then each of the eight slots (the CatalogSlots region table: two rows of four 17 x 17 boxes) whose item id is nonzero shows the
 * item's category 8 icon (catalog record [+8], transparent) at the box's top-left. With no item at all the word EMPTY is written at
 * (259, 179) in colour 0xF on 4 and the original sets a flag the click handlers test. Returns the number of items drawn.
 */
unsigned shopGridDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint16_t itemIds[8]);

#endif

#ifndef YENDOR23_PAPERDOLL_H
#define YENDOR23_PAPERDOLL_H

#include <stdint.h>

#include "game.h"
#include "item.h"
#include "viewrender.h"

/*
 * A character's paper doll with the equipment shown on it: DrawPartyMemberPortrait (yendor2.asm:39190, yendor3.asm:39482), drawn
 * 56 pixels wide at (8, 8), (64, 8), (120, 8) or (176, 8) -- the four party members side by side on the inventory screen.
 * Coordinates come from the InventoryGrid region table (uiregions.h: entry xMin/yMin, relative to the doll's origin):
 *   - the body: category 6 picture [+0x14], opaque
 *   - the weapon slot [+0x13A] at entry 9 (1 item); then either, when the status flag [+0x15C] & 0x1000 is set, the icon of
 *     item [+0x1C8] (+1 variant) at (+0x28, +0x40) and the two slots [+0x142] at entries 11-12, or the three slots [+0x13E]
 *     at entries 10-12
 *   - a marker (category 8 picture 0xC for male [+0x10] == 1, 0xD otherwise) at entry 12 when [+0x15C] & 0x20
 *   - the open bag (the first of [+0x17C], [+0x1A2], [+0x1C8] that is nonzero; its icon +1 variant at (+0x26, +0x24) -- not for
 *     [+0x1C8] with flag 0x1000) or, with none, the main inventory: its first 8 slots (the group's items start 2 bytes in) at
 *     entries 0-7
 *   - two ring slots [+0x14A] as category 9 pictures at entries 13-14
 *   - worn clothing on top, category 7 pictures at fixed spots: [+0x152] (12, 0x31), [+0x154] (12, 0x48), [+0x156] (12, 0x5D),
 *     [+0x158] (10, 0x68), [+0x15A] (12, 0x56); Chapter 3 draws [+0x154] in category 6 at (0, 0) and has no [+0x156]
 * Item icons are category 8 pictures: the catalog record's [+8]; rings in the ring slots use word 2 of the item's wearable entry.
 * Clothing uses word 2 of the wearable entry too, +1 for a non-male wearer.
 */
void paperDollDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint8_t *partyRecord, int originX, int originY);

#endif

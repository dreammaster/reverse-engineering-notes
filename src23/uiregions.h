#ifndef YENDOR23_UIREGIONS_H
#define YENDOR23_UIREGIONS_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * The screens' clickable regions: HitTestRegionTable (yendor2.asm:23244) scans a table of 10-byte entries (xMin, xMax, yMin, yMax,
 * result) -- screen pixels, bounds inclusive, first match wins, 0xFFFF ends the table -- and returns the entry's result (0 = no
 * match). The tables are data-segment resident and give every screen's layout (320 x 200 coordinates); dumped by
 * ida_scripts/dump_hit_regions.py. The two games' tables are identical except for InventoryGrid (Chapter 3 has one slot fewer),
 * MemberDetail and ItemService (one entry fewer each). Results are the codes the callers switch on; meanings of the main
 * screen's:
 *   DungeonMain / DungeonClick  1 the first-person viewport (8,8)-(231,143), 2 the map/compass panel, 3 the icon row, 4 the
 *                               monster panels, 5 the lower side panel, 6 the party portrait strip
 *   DungeonIconRow              the four buttons of the icon row (y 67-82, x 239..311 in steps of 19)
 *   PartyPanels                 four 58-pixel-wide panels along the bottom: portrait, six slots and a name bar each
 */
typedef enum {
    UiRegionsDungeonMain = 0,
    UiRegionsTitleMenu,
    UiRegionsDungeonIconRow,
    UiRegionsPartyRoster,
    UiRegionsGameDialog,
    UiRegionsConfirmList,
    UiRegionsMapEditor,
    UiRegionsCharacterInventory,
    UiRegionsInventoryGrid,
    UiRegionsPartyPanels,
    UiRegionsRosterPortraits,
    UiRegionsMemberDetail,
    UiRegionsStatusIconBar,
    UiRegionsCatalogSlots,
    UiRegionsDirectionPad, /* the on-screen walk pad: 1 turn left, 2 forward, 3 turn right, 4 strafe left, 5 back, 6 strafe right */
    UiRegionsShop,
    UiRegionsItemService,
    UiRegionsMonsterPanels,
    UiRegionsDungeonClick,
    UiRegionsAlchemy,
    UiRegionsClueCategories,
    UiRegionsClueEntries,
    UiRegionsClueScroll,
    UiRegionsClueNav,
    UiRegionTableCount
} UiRegionTable;

/* The result of the first entry containing (x, y), 0 if none. */
unsigned uiRegionHit(GameKind game, UiRegionTable table, int x, int y);

/* Raw entries, for layout code: *count entries of {xMin, xMax, yMin, yMax, result}. */
const uint16_t (*uiRegionEntries(GameKind game, UiRegionTable table, unsigned *count))[5];

#endif

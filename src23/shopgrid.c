#include "shopgrid.h"

#include "font.h"
#include "uiregions.h"

unsigned shopGridDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint16_t itemIds[8]) {
    for (int y = 160; y < 160 + 35; y++) {
        for (int x = 241; x < 241 + 72; x++) {
            r->screen[y * ViewScreenWidth + x] = 4;
        }
    }
    unsigned count;
    const uint16_t(*slots)[5] = uiRegionEntries(r->game, UiRegionsCatalogSlots, &count);
    unsigned drawn = 0;
    for (unsigned i = 0; i < 8 && i < count; i++) {
        if (itemIds[i] == 0) {
            continue;
        }
        drawn++;
        const uint8_t *item = itemCatalogRecord(catalog, itemIds[i]);
        if (item) {
            viewDrawPicture(r, 8, itemGetU16(item, ItemFieldIcon), slots[i][0], slots[i][2], true, 0);
        }
    }
    if (drawn == 0) {
        fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 259, 179, "EMPTY", 0xF, 4, FontOpaque);
    }
    return drawn;
}

#include "paperdoll.h"

#include "uiregions.h"

enum { BodyCategory = 6, ClothingCategory = 7, ItemCategory = 8, SmallCategory = 9 };

static unsigned field(const uint8_t *record, unsigned offset) {
    return (unsigned)record[offset] | ((unsigned)record[offset + 1] << 8);
}

typedef struct {
    const ViewRenderer *r;
    const ItemCatalog *catalog;
    const uint8_t *party;
    int ox, oy;
    const uint16_t (*grid)[5];
} Doll;

/* the wearable/weapon entry's word 2 (the original reads [+4] of the current target record) */
static unsigned targetWord2(const Doll *d, const uint8_t *item) {
    const uint8_t *entry = itemTargetEntry(d->catalog, item);
    return entry ? itemTargetWord(entry, 2) : 0;
}

/* DrawEquippedItemIcons: `count` slots of 4 bytes from `slots`, drawn at grid entries first, first + 1, ... */
static void drawSlots(const Doll *d, const uint8_t *slots, unsigned first, unsigned count, unsigned category, bool ringVariant) {
    for (unsigned i = 0; i < count; i++, slots += 4) {
        unsigned id = field(slots, 0);
        if (id == 0) {
            continue;
        }
        const uint8_t *item = itemCatalogRecord(d->catalog, id);
        if (!item) {
            continue;
        }
        unsigned picture = itemGetU16(item, ItemFieldIcon);
        if (ringVariant && (itemGetU16(item, ItemFieldFlags) & ItemFlagEquipRing)) {
            picture = targetWord2(d, item);
        }
        viewDrawPicture(d->r, category, picture, d->ox + d->grid[first + i][0], d->oy + d->grid[first + i][2], true, 0);
    }
}

static void drawOverlayIcon(const Doll *d, unsigned itemId, int dx, int dy) {
    const uint8_t *item = itemCatalogRecord(d->catalog, itemId);
    if (item) {
        viewDrawPicture(d->r, ItemCategory, itemGetU16(item, ItemFieldIcon) + 1, d->ox + dx, d->oy + dy, true, 0);
    }
}

static void drawClothing(const Doll *d, unsigned category, unsigned itemId, int dx, int dy) {
    if (itemId == 0) {
        return;
    }
    const uint8_t *item = itemCatalogRecord(d->catalog, itemId);
    if (!item) {
        return;
    }
    unsigned picture = targetWord2(d, item);
    if (field(d->party, 0x10) != 1) {
        picture++;
    }
    viewDrawPicture(d->r, category, picture, d->ox + dx, d->oy + dy, true, 0);
}

void paperDollDraw(const ViewRenderer *r, const ItemCatalog *catalog, const uint8_t *party, int originX, int originY) {
    unsigned count;
    const uint16_t(*grid)[5] = uiRegionEntries(r->game, UiRegionsInventoryGrid, &count);
    if (!party || count < 14) {
        return;
    }
    Doll d = {r, catalog, party, originX, originY, grid};
    bool ch3 = r->game == GameYendor3;
    unsigned flags = field(party, 0x15C);

    viewDrawPicture(r, BodyCategory, field(party, 0x14), originX, originY, false, 0);
    drawSlots(&d, party + 0x13A, 9, 1, ItemCategory, false);
    if (flags & 0x1000) {
        drawOverlayIcon(&d, field(party, 0x1C8), 0x28, 0x40);
        drawSlots(&d, party + 0x142, 11, 2, ItemCategory, false);
    } else {
        drawSlots(&d, party + 0x13E, 10, 3, ItemCategory, false);
    }
    if (flags & 0x20) {
        viewDrawPicture(r, ItemCategory, field(party, 0x10) == 1 ? 0xC : 0xD, originX + grid[12][0], originY + grid[12][2], true, 0);
    }

    const uint8_t *group;
    if (field(party, 0x17C)) {
        drawOverlayIcon(&d, field(party, 0x17C), 0x26, 0x24);
        group = party + 0x180;
    } else if (field(party, 0x1A2)) {
        drawOverlayIcon(&d, field(party, 0x1A2), 0x26, 0x24);
        group = party + 0x1A6;
    } else if (field(party, 0x1C8)) {
        if (!(flags & 0x1000)) {
            drawOverlayIcon(&d, field(party, 0x1C8), 0x26, 0x24);
        }
        group = party + 0x1CC;
    } else {
        group = party + 0x118;
    }
    drawSlots(&d, group + 2, 0, 8, ItemCategory, false);
    drawSlots(&d, party + 0x14A, 13, 2, SmallCategory, true);

    if (ch3) {
        drawClothing(&d, BodyCategory, field(party, 0x154), 0, 0);
        drawClothing(&d, ClothingCategory, field(party, 0x152), 12, 0x31);
        drawClothing(&d, ClothingCategory, field(party, 0x158), 10, 0x68);
        drawClothing(&d, ClothingCategory, field(party, 0x15A), 12, 0x56);
    } else {
        drawClothing(&d, ClothingCategory, field(party, 0x152), 12, 0x31);
        drawClothing(&d, ClothingCategory, field(party, 0x154), 12, 0x48);
        drawClothing(&d, ClothingCategory, field(party, 0x156), 12, 0x5D);
        drawClothing(&d, ClothingCategory, field(party, 0x158), 10, 0x68);
        drawClothing(&d, ClothingCategory, field(party, 0x15A), 12, 0x56);
    }
}

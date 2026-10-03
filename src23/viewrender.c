#include "viewrender.h"

#include <string.h>

#include "pictures.h"
#include "worldmap.h"

enum {
    ScreenSize = ViewScreenWidth * ViewScreenHeight,
    Val11 = 0x0000,
    Val12 = 0x0386,
    Val13 = 0x0BF2,
    Val14 = 0x13B6,
    Ptr1 = 0x3B76,
    Ptr2 = 0x3CA8,
    Ptr3 = 0x3DF2,
    Ptr4 = 0x41D2,
    Ptr5 = 0x4316,
    Ptr6 = 0x4460,
    Ptr7 = 0x4858,
    CategoryWalls = 1,
    CategoryObjects = 2,
    CategoryFloor = 4,
    CategoryCeiling = 5,
    CategoryFarWall = 6,
    HiddenFlag = 1,
    OverlayFlag = 0x2000,
    TorchFlag = 0x1000
};

uint32_t viewTablesOffset(GameKind game) {
    return game == GameYendor3 ? 0x40BF71 : 0x19C7A5;
}

uint8_t viewShadeColour(uint8_t colour, int8_t delta) {
    if (delta == 0 || colour >= 0xD0) {
        return colour;
    }
    uint8_t low = colour & 0xF0, high = colour | 0x0F;
    uint8_t shifted = (uint8_t)(colour + (uint8_t)delta);
    if (shifted < low) {
        return low;
    }
    if (shifted <= high) {
        return shifted;
    }
    if (colour & 0x80) {
        return high;
    }
    return (shifted & 0x80) ? low : high;
}

typedef struct {
    const ViewRenderer *r;
    const uint8_t *picture; /* NULL = nothing to draw */
    unsigned width, size;
    int8_t shade;
    bool transparent;
    bool remap;            /* RemapOrMaskColorByHueTable active (row-mask sprites only) */
    uint16_t remapWords[16];
} Blit;

static unsigned rd(const ViewRenderer *r, unsigned offset) {
    if (offset + 2 > ViewTablesSize) {
        return 0;
    }
    return (unsigned)r->tables[offset] | ((unsigned)r->tables[offset + 1] << 8);
}

static int rds(const ViewRenderer *r, unsigned offset) {
    return (int16_t)rd(r, offset);
}

static void put(const Blit *b, long dest, uint8_t pixel) {
    if (dest >= 0 && dest < ScreenSize) {
        b->r->screen[dest] = viewShadeColour(pixel, b->shade);
    }
}

/* One source pixel, with the 0xFF colour key; returns the new destination position. */
static long emit(const Blit *b, long dest, unsigned source) {
    if (source < b->size) {
        uint8_t pixel = b->picture[source];
        if (!(b->transparent && pixel == 0xFF)) {
            put(b, dest, pixel);
        }
    }
    return dest + 1;
}

/* RemapOrMaskColorByHueTable: replaces the hue group (high nibble) of a colour found in the table of {source << 8 | target}
 * words, sorted by source; a target of 0xF makes the colour transparent (0xFF). */
static uint8_t remapColour(const Blit *b, uint8_t colour) {
    unsigned hue = colour >> 4;
    for (unsigned i = 0; i < 16; i++) {
        unsigned word = b->remapWords[i], source = word >> 8, target = word & 0xFF;
        if (word == 0 || hue < source) {
            break;
        }
        if (hue == source) {
            if (target == 0xF) {
                return 0xFF;
            }
            hue = target;
            break;
        }
    }
    return (uint8_t)((hue << 4) | (colour & 0xF));
}

/* DrawRleMaskedShadedRun's pixel: colour key, shade, hue remap. */
static long emitRow(const Blit *b, long dest, unsigned source) {
    if (source < b->size) {
        uint8_t pixel = b->picture[source];
        if (!(b->transparent && pixel == 0xFF)) {
            pixel = viewShadeColour(pixel, b->shade);
            if (b->remap) {
                pixel = remapColour(b, pixel);
            }
            if (pixel != 0xFF || !b->remap) {
                if (dest >= 0 && dest < ScreenSize) {
                    b->r->screen[dest] = pixel;
                }
            }
        }
    }
    return dest + 1;
}

static bool openBlit(Blit *b, const ViewRenderer *r, unsigned category, unsigned id, int8_t shade, bool transparent) {
    const PictureCategory *c = pictureCategory(r->game, category);
    b->r = r;
    b->picture = c ? r->picture(r->pictureCtx, category, id) : NULL;
    b->width = c ? c->width : 0;
    b->size = c ? (unsigned)c->width * c->height : 0;
    b->shade = shade;
    b->transparent = transparent;
    b->remap = false;
    return b->picture != NULL;
}

/* DrawRleMaskedShadedRun: one scanline's run records. */
static void drawRleRow(const Blit *b, unsigned records, unsigned source, long dest) {
    for (;; records += 6) {
        unsigned count = rd(b->r, records), run = rd(b->r, records + 2), skip = rd(b->r, records + 4);
        if (count == 0) {
            return;
        }
        for (unsigned k = 0; k < count; k++) {
            for (unsigned j = 0; j < run; j++) {
                dest = emitRow(b, dest, source++);
            }
            source += skip;
        }
    }
}

/* Layers 0, 7, 8, 9-14. */
static void rowMaskSprite(const Blit *b, unsigned table, unsigned depth) {
    unsigned entry = table + 6 * depth, x = rd(b->r, entry), y = rd(b->r, entry + 2), ptr = rd(b->r, entry + 4);
    if (x == 0) {
        return;
    }
    unsigned groups = rd(b->r, ptr), records = ptr + 2, source = 0;
    long dest = (long)y * ViewScreenWidth + x;
    for (unsigned guard = 0; guard < 1024; guard++, groups += 6) {
        unsigned repeat = rd(b->r, groups);
        if (repeat == 0) {
            return;
        }
        unsigned rows = rd(b->r, groups + 2), skip = rd(b->r, groups + 4);
        for (unsigned k = 0; k < repeat; k++) {
            for (unsigned row = 0; row < rows; row++) {
                drawRleRow(b, records, source, dest);
                dest += ViewScreenWidth;
                source += b->width;
            }
            source += skip * b->width;
        }
    }
}

/* Layers 1 and 2: a polygon patch of the 224-wide floor/ceiling picture. */
static void patchSprite(const Blit *b, unsigned table, unsigned depth, int offsetY) {
    unsigned entry = table + 6 * depth, x = rd(b->r, entry), y = rd(b->r, entry + 2), ptr = rd(b->r, entry + 4);
    if (ptr == 0) {
        return;
    }
    unsigned source = y * b->width + x;
    long dest = (long)(y + offsetY) * ViewScreenWidth + x + 8;
    for (unsigned list = ptr, guard = 0; guard < 512; list += 6, guard++) {
        unsigned run = rd(b->r, list);
        if (run == 0) {
            return;
        }
        for (unsigned j = 0; j < run; j++) {
            emit(b, dest + j, source + j);
        }
        int shift = rds(b->r, list + 2);
        source += b->width + shift;
        dest += ViewScreenWidth + shift;
    }
}

/* DrawRleScaledSpriteColumn: one destination column, drawn down the screen from a source column. */
static void drawRleColumn(const Blit *b, unsigned records, unsigned source, long dest) {
    for (;; records += 6) {
        unsigned count = rd(b->r, records), run = rd(b->r, records + 2), skip = rd(b->r, records + 4);
        if (count == 0) {
            return;
        }
        for (unsigned k = 0; k < count; k++) {
            for (unsigned j = 0; j < run; j++) {
                emit(b, dest, source);
                source += b->width;
                dest += ViewScreenWidth;
            }
            source += skip * b->width;
        }
    }
}

/* Layers 3 (left walls, groups step down) and 4 (right walls, step up). */
static void columnSprite(const Blit *b, unsigned depth, bool stepDown) {
    unsigned entry = Val14 + 6 * depth, x = rd(b->r, entry), y = rd(b->r, entry + 2), header = rd(b->r, entry + 4);
    if (x == 0) {
        return;
    }
    unsigned source = 0;
    long dest = (long)y * ViewScreenWidth + x;
    for (unsigned guard = 0; guard < 1024; guard++) {
        unsigned columns = rd(b->r, header), records = header + 4;
        int advance = rds(b->r, header + 2);
        if (rd(b->r, records) == 0) {
            source += advance;
            header += 6;
            if (columns == 0) {
                return;
            }
            continue;
        }
        if (columns == 0) {
            return;
        }
        for (unsigned c = 0; c < columns; c++) {
            drawRleColumn(b, records, source, dest);
            source += 1 + advance;
            dest += 1;
        }
        dest += stepDown ? ViewScreenWidth : -ViewScreenWidth;
        /* the record list ends with a one-word terminator; the next header follows it */
        unsigned end = records;
        while (rd(b->r, end) != 0) {
            end += 6;
        }
        header = end + 2;
    }
}

/* Layer 6: the 7-pixel-wide, 113-row strips beside the party. */
static void stripSprite(const Blit *b, unsigned depth, unsigned frame) {
    unsigned entry = Val14 + 6 * depth, x = rd(b->r, entry), y = rd(b->r, entry + 2);
    unsigned source = frame;
    long dest = (long)y * ViewScreenWidth + x;
    for (unsigned row = 0; row < 0x71; row++) {
        for (unsigned j = 0; j < 7; j++) {
            emit(b, dest + j, source + j);
        }
        source += b->width;
        dest += ViewScreenWidth;
    }
}

static unsigned rowMaskTable(unsigned layer) {
    switch (layer) {
    case 0:
    case 8:
        return Val11;
    case 7:
        return Ptr1;
    case 9:
        return Ptr2;
    case 10:
        return Ptr3;
    case 11:
        return Ptr4;
    case 12:
        return Ptr5;
    case 13:
        return Ptr6;
    default:
        return Ptr7;
    }
}

static void drawSpriteRemapped(const ViewRenderer *r, unsigned layer, unsigned category, unsigned id, unsigned depth, int8_t shade,
                               bool transparent, unsigned frame, const uint8_t *remap) {
    Blit b;
    if (!openBlit(&b, r, category, id, shade, transparent)) {
        return;
    }
    if (remap) {
        memset(b.remapWords, 0, sizeof(b.remapWords));
        for (unsigned i = 0; i < 6; i++) {
            b.remapWords[i] = (uint16_t)(((remap[i] >> 4) << 8) | (remap[i] & 0xF));
        }
        b.remap = b.remapWords[0] != 0;
    }
    switch (layer) {
    case 1:
        patchSprite(&b, Val12, depth, 70);
        break;
    case 2:
        patchSprite(&b, Val13, depth, 8);
        break;
    case 3:
        columnSprite(&b, depth, true);
        break;
    case 4:
        columnSprite(&b, depth, false);
        break;
    case 6:
        stripSprite(&b, depth, frame);
        break;
    default:
        rowMaskSprite(&b, rowMaskTable(layer), depth);
        break;
    }
}

void viewDrawSprite(const ViewRenderer *r, unsigned layer, unsigned category, unsigned id, unsigned depth, int8_t shade, bool transparent,
                    unsigned frame) {
    drawSpriteRemapped(r, layer, category, id, depth, shade, transparent, frame, NULL);
}

static unsigned monsterWord(const uint8_t *monster, unsigned offset) {
    return (unsigned)monster[offset] | ((unsigned)monster[offset + 1] << 8);
}

void viewDrawMonster(const ViewRenderer *r, unsigned depth, uint8_t *monster, int8_t shade) {
    unsigned flags = monsterWord(monster, 0x92), state = monsterWord(monster, 0x0C);
    unsigned base = monsterWord(monster, 0x4C), frame = monsterWord(monster, 0x08), layer = monsterWord(monster, 0x0A);
    unsigned category = (flags & 1) ? 3 : 2;
    if (state & 2) {
        state &= ~2u;
        frame = base + 9;
        monster[0x0C] = (uint8_t)state;
        monster[0x0D] = (uint8_t)(state >> 8);
    } else if ((state & 4) && frame < base + 6) {
        frame = base + 6;
        monster[0x08] = (uint8_t)frame;
        monster[0x09] = (uint8_t)(frame >> 8);
    }
    drawSpriteRemapped(r, layer, category, frame, depth, shade, true, 0, (flags & 4) ? monster + 0x72 : NULL);
    monster[0x18] = monster[0x19] = 0;
    if (state & 0x10) {
        drawSpriteRemapped(r, layer, category, monsterWord(monster, 0x1A), depth, shade, true, 0, NULL);
    }
}

void viewDrawPicture(const ViewRenderer *r, unsigned category, unsigned id, int x, int y, bool transparent, int8_t shade) {
    Blit b;
    if (!openBlit(&b, r, category, id, shade, transparent)) {
        return;
    }
    const PictureCategory *c = pictureCategory(r->game, category);
    for (unsigned py = 0; py < c->height; py++) {
        for (unsigned px = 0; px < c->width; px++) {
            long dest = (long)(y + (int)py) * ViewScreenWidth + x + (int)px;
            if (x + (int)px < 0 || x + (int)px >= ViewScreenWidth) {
                continue;
            }
            uint8_t pixel = b.picture[py * c->width + px];
            if (!(transparent && pixel == 0xFF)) {
                put(&b, dest, pixel);
            }
        }
    }
}

/* ---- the scene ---- */

static const uint16_t kEmptyLegend[6] = {0, 0, 0, 0, 0, 0};

static const uint16_t *wallLegend(GameKind game, uint16_t type) {
    const uint16_t *words;
    return worldMapWallLegend(game, type, &words) ? words : kEmptyLegend;
}

static const uint16_t *floorLegend(GameKind game, uint16_t type) {
    const uint16_t *words;
    return worldMapFloorLegend(game, type, &words) ? words : kEmptyLegend;
}

typedef struct {
    const ViewRenderer *r;
    const ViewScene *s;
    unsigned firstCell[ViewportRowCount], cellCount[ViewportRowCount];
} Scene;

static const unsigned kRowFirst[ViewportRowCount] = {0, 17, 34, 39, 42, 45, 48};
static const unsigned kRowCount[ViewportRowCount] = {17, 17, 5, 3, 3, 3, 3};

static int8_t rowShade(const ViewScene *s, unsigned row) {
    return (int8_t)s->gradient[row];
}

static bool pairedMatch(unsigned a, unsigned b) {
    if (a == b) {
        return true;
    }
    return (a & 1) ? a - 1 == b : a + 1 == b;
}

static void shadeBands(const ViewRenderer *r, const ViewScene *s, bool floor) {
    /* ApplyDistanceShadingToFloorOrCeiling: bands of rows, each shaded by one gradient entry */
    static const unsigned kFloorRows[7] = {4, 9, 9, 10, 10, 11, 21}, kFloorGradient[7] = {0, 1, 2, 3, 4, 5, 6};
    static const unsigned kCeilingRows[7] = {10, 11, 10, 10, 9, 9, 3}, kCeilingGradient[7] = {6, 5, 4, 3, 2, 1, 0};
    const unsigned *rows = floor ? kFloorRows : kCeilingRows, *grad = floor ? kFloorGradient : kCeilingGradient;
    unsigned y = floor ? 70 : 8;
    for (unsigned band = 0; band < 7; band++) {
        for (unsigned row = 0; row < rows[band]; row++, y++) {
            for (unsigned x = 8; x < 8 + 224; x++) {
                uint8_t *p = &r->screen[y * ViewScreenWidth + x];
                *p = viewShadeColour(*p, rowShade(s, grad[band]));
            }
        }
    }
}

static void copyPicture(const ViewRenderer *r, unsigned category, unsigned id, unsigned destX, unsigned destY) {
    const PictureCategory *c = pictureCategory(r->game, category);
    const uint8_t *pic = c ? r->picture(r->pictureCtx, category, id) : NULL;
    if (!pic) {
        return;
    }
    for (unsigned y = 0; y < c->height; y++) {
        memcpy(&r->screen[(destY + y) * ViewScreenWidth + destX], &pic[y * c->width], c->width);
    }
}

static unsigned facingWord(uint16_t facing) {
    return facing & 0x8000 ? 0 : facing & 0x4000 ? 1 : facing & 0x1000 ? 2 : 3;
}

static void drawFloorAndCeiling(const ViewRenderer *r, const ViewScene *s) {
    bool ch3 = r->game == GameYendor3;
    const DungeonGridCell *party = &s->cells[49];
    const uint16_t *legend = wallLegend(r->game, party->wallType);
    unsigned ceiling = legend[1];
    if (ceiling == 0 && !(s->facing & 0x8000) && !(s->facing & 0x4000)) {
        ceiling = 1;
    }
    unsigned floor = legend[0];
    unsigned ceilingBase = ch3 ? ceiling & 1 : ceiling, floorBase = ch3 ? floor & 1 : floor;

    copyPicture(r, CategoryCeiling, ceilingBase, 8, 8);
    shadeBands(r, s, false);
    copyPicture(r, CategoryFloor, floorBase, 8, 70);
    shadeBands(r, s, true);

    for (unsigned pass = 0; pass < 2; pass++) { /* floor patches first, then the ceiling pass */
        bool isFloor = pass == 0;
        for (unsigned row = 0; row < ViewportRowCount; row++) {
            for (unsigned i = kRowFirst[row]; i < kRowFirst[row] + kRowCount[row]; i++) {
                const DungeonGridCell *cell = &s->cells[i];
                if (cell->flags & HiddenFlag) {
                    continue;
                }
                const uint16_t *cl = wallLegend(r->game, cell->wallType);
                unsigned id;
                if (isFloor) {
                    id = cl[0];
                    if (ch3) {
                        id = (id & 0xFFFE) | floorBase;
                    } else if (pairedMatch(floorBase, id)) {
                        continue;
                    }
                } else {
                    id = cl[1];
                    if (ch3) {
                        id = id == 0 ? ((s->facing & 0xC000) ? 0 : 1) : ((id & 0xFFFE) | ceilingBase);
                    } else if (pairedMatch(ceilingBase, id)) {
                        continue;
                    }
                }
                viewDrawSprite(r, isFloor ? 1 : 2, isFloor ? CategoryFloor : CategoryCeiling, id, i, rowShade(s, row), false, 0);
            }
        }
    }
}

static void drawWall(const Scene *sc, unsigned index, unsigned row) {
    const ViewRenderer *r = sc->r;
    const DungeonGridCell *cell = &sc->s->cells[index];
    if (cell->wallType == 0) {
        return;
    }
    const uint16_t *legend = wallLegend(r->game, cell->wallType);
    if (legend[2] == 0) {
        return;
    }
    viewDrawSprite(r, 0, CategoryWalls, legend[2], index, rowShade(sc->s, row), true, 0);
    if (cell->flags & OverlayFlag) {
        viewDrawSprite(r, 0, CategoryWalls, r->game == GameYendor3 ? legend[0] : 5, index, rowShade(sc->s, row), true, 0);
    }
}

static void drawSideFeature(const Scene *sc, unsigned index, unsigned row) {
    const ViewRenderer *r = sc->r;
    const DungeonGridCell *cell = &sc->s->cells[index];
    if (cell->floorType == 0) {
        return;
    }
    unsigned pic = floorLegend(r->game, cell->floorType)[facingWord(sc->s->facing)];
    if (pic == 0) {
        return;
    }
    int8_t shade = rowShade(sc->s, row);
    unsigned type = cell->floorType;
    if (r->game == GameYendor3) {
        if (type <= 99) {
            viewDrawSprite(r, 13, CategoryObjects, pic, index, shade, true, 0);
            return;
        }
        unsigned layer = type <= 199 ? 7 : 8;
        viewDrawSprite(r, layer, CategoryWalls, pic, index, shade, true, 0);
        if ((cell->flags & TorchFlag) && pic == 0x3F) {
            viewDrawSprite(r, 7, CategoryWalls, 5, index, shade, true, 0);
        }
        return;
    }
    unsigned layer = (type <= 22 || (type >= 51 && type <= 67)) ? 7 : 8;
    viewDrawSprite(r, layer, CategoryWalls, pic, index, shade, true, 0);
    if ((cell->flags & TorchFlag) && pic == 0x1C) {
        viewDrawSprite(r, 7, CategoryWalls, 6, index, shade, true, 0);
    }
}

/* A side wall: this cell's wall seen from an open neighbour on the near side of the centre. */
static void drawSideWall(const Scene *sc, unsigned index, unsigned neighbour, unsigned layer, unsigned row) {
    const ViewRenderer *r = sc->r;
    const DungeonGridCell *cell = &sc->s->cells[index], *next = &sc->s->cells[neighbour];
    if (next->flags & HiddenFlag) {
        return;
    }
    const uint16_t *nl = wallLegend(r->game, next->wallType);
    if (nl[2] != 0 || nl[3] != 0) {
        return;
    }
    const uint16_t *legend = wallLegend(r->game, cell->wallType);
    if (legend[2] != 0) {
        viewDrawSprite(r, layer, CategoryWalls, legend[2], index, rowShade(sc->s, row), true, 0);
    }
}

static void drawCellMonster(const Scene *sc, unsigned index, unsigned row) {
    if (index >= 17 && index < 49 && sc->s->cellMonsters[index]) { /* TryTriggerMonsterEncounterAtCell: not the farthest row */
        viewDrawMonster(sc->r, index, sc->s->cellMonsters[index], rowShade(sc->s, row));
    }
}

static void drawRow(const Scene *sc, unsigned row) {
    unsigned first = kRowFirst[row], count = kRowCount[row], half = (count - 1) / 2;
    const DungeonGridCell *cells = sc->s->cells;
    for (unsigned i = first; i < first + half; i++) {
        if (cells[i].flags & HiddenFlag) {
            continue;
        }
        drawWall(sc, i, row);
        drawSideWall(sc, i, i + 1, 3, row);
        drawSideFeature(sc, i, row);
        drawCellMonster(sc, i, row);
    }
    for (unsigned i = first + count - 1; i > first + half; i--) {
        if (cells[i].flags & HiddenFlag) {
            continue;
        }
        drawWall(sc, i, row);
        drawSideWall(sc, i, i - 1, 4, row);
        drawSideFeature(sc, i, row);
        drawCellMonster(sc, i, row);
    }
    unsigned mid = first + half;
    if (!(cells[mid].flags & HiddenFlag)) {
        drawWall(sc, mid, row);
        drawSideFeature(sc, mid, row);
        drawCellMonster(sc, mid, row);
    }
}

static void drawVanishingPoint(const Scene *sc) {
    const ViewRenderer *r = sc->r;
    const DungeonGridCell *cells = sc->s->cells;
    for (unsigned k = 0; k < 2; k++) {
        unsigned index = k == 0 ? 48 : 50;
        if (cells[index].flags & HiddenFlag) {
            continue;
        }
        const uint16_t *legend = wallLegend(r->game, cells[index].wallType);
        if (legend[3] != 0) {
            viewDrawSprite(r, 6, CategoryFarWall, legend[3], index, rowShade(sc->s, 6), true, legend[4] + (k ? 7 : 0));
        }
    }
    drawSideFeature(sc, 49, 6);
    for (unsigned i = 0; i < 3; i++) { /* RenderActiveMonsterSprites */
        if (sc->s->combatMonsters[i]) {
            viewDrawMonster(r, 49, sc->s->combatMonsters[i], rowShade(sc->s, 6));
        }
    }
}

void viewRender(const ViewRenderer *r, const ViewScene *scene) {
    Scene sc = {r, scene, {0}, {0}};
    drawFloorAndCeiling(r, scene);
    for (unsigned row = 0; row < ViewportRowCount - 1; row++) {
        drawRow(&sc, row);
    }
    drawVanishingPoint(&sc);
}

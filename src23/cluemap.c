#include "cluemap.h"

#include <stdio.h>
#include <string.h>

#include "font.h"

typedef struct {
    size_t known, markers, labels;
    unsigned markerCount, rows;
} Layout;

static const Layout kYendor2 = {1396760, 1450685, 1453925, 405, 144};
static const Layout kYendor3 = {3952386, 4011261, 4013261, 250, 168};

static unsigned u16(const uint8_t *p) {
    return p[0] | (unsigned)(p[1] << 8);
}

unsigned clueMapMarkers(GameKind game, const uint8_t *worldDat, size_t size, unsigned mapId, ClueMapMarker *out, unsigned max) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    unsigned count = 0;
    for (unsigned i = 0; i < l->markerCount && count < max; i++) {
        size_t at = l->markers + 8 * (size_t)i;
        if (at + 8 > size) {
            break;
        }
        if (u16(worldDat + at) != mapId) {
            continue;
        }
        ClueMapMarker *m = &out[count++];
        m->x = u16(worldDat + at + 2);
        m->y = u16(worldDat + at + 4);
        m->label = u16(worldDat + at + 6);
        m->screenX = (int)(m->x % LocalMapColumns) * LocalMapTileSize;
        m->screenY = LocalMapTop + (int)(m->y % LocalMapRows) * LocalMapTileSize;
    }
    return count;
}

bool clueMapLabel(GameKind game, const uint8_t *worldDat, size_t size, unsigned label, char out[ClueMapLabelSize]) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    size_t at = l->labels + ClueMapLabelSize * (size_t)label;
    if (at + ClueMapLabelSize > size) {
        return false;
    }
    memcpy(out, worldDat + at, ClueMapLabelSize);
    out[ClueMapLabelSize - 1] = 0;
    size_t length = strlen(out);
    while (length > 0 && out[length - 1] == ' ') {
        out[--length] = 0;
    }
    return true;
}

void clueMapFill(LocalMapCell cells[LocalMapColumns * LocalMapRows], GameKind game, const WorldMap *map, const uint8_t *worldDat, size_t size, unsigned mapId) {
    const Layout *l = game == GameYendor2 ? &kYendor2 : &kYendor3;
    unsigned block = mapId ? mapId - 1 : 0;
    int firstColumn = (int)(block % LocalMapBlocksPerRow) * LocalMapColumns, firstRow = (int)(block / LocalMapBlocksPerRow) * LocalMapRows;
    for (int row = 0; row < LocalMapRows; row++) {
        for (int column = 0; column < LocalMapColumns; column++) {
            LocalMapCell *cell = &cells[row * LocalMapColumns + column];
            int x = firstColumn + column, y = firstRow + row;
            bool inside = x < WorldMapColumns && y < (int)map->rowCount;
            cell->wallType = inside ? worldMapTileA(map, (unsigned)y, (unsigned)x) : 0;
            cell->floorType = inside ? worldMapTileB(map, (unsigned)y, (unsigned)x) : 0;
            size_t at = l->known + (size_t)y * (WorldMapColumns / 8) + (size_t)x / 8;
            cell->explored = inside && at < size && (worldDat[at] & (0x80 >> (x % 8)));
        }
    }
}

void clueMapPageDraw(const ViewRenderer *r, const LocalMapCell cells[LocalMapColumns * LocalMapRows], const LocationName *name, const char *hint,
                     const ClueMapMarker *markers, unsigned markerCount) {
    memset(r->screen, 0, (size_t)ViewScreenWidth * ViewScreenHeight);
    localMapDraw(r, cells, 0, 0, 0);
    char title[48];
    snprintf(title, sizeof(title), "%s%s", name->name, name->suffix);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 0, 1, title, 0x0D, 0, FontTransparent);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 201, 1, hint, 0x59, 0, FontTransparent);
    for (unsigned i = 0; i < markerCount; i++) {
        viewDrawPicture(r, LocalMapCategory, 0x73, markers[i].screenX, markers[i].screenY, false, 0);
    }
}

void clueMapLabelDraw(const ViewRenderer *r, const char *label) {
    viewDrawPicture(r, LocalMapCategory, 0x73, 161, 0, false, 0);
    fontDrawString(r->game, 0, r->screen, ViewScreenWidth, 170, 1, label, 0x7B, 0, FontOpaque);
}

int clueMapMarkerAt(const ClueMapMarker *markers, unsigned markerCount, int x, int y) {
    for (unsigned i = 0; i < markerCount; i++) {
        if (x >= markers[i].screenX && x <= markers[i].screenX + 8 && y >= markers[i].screenY && y <= markers[i].screenY + 8) {
            return (int)i;
        }
    }
    return -1;
}

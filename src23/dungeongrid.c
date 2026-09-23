#include "dungeongrid.h"

#include <string.h>

#include "movement.h"

void dungeonGridComputeOrigin(GameKind game, int partyWorldX, int partyWorldY, int *outOriginCol, int *outOriginRow) {
    const MovementBounds *b = movementBounds(game);

    int col = partyWorldX - 39;
    if (col >= (int)b->colMin) {
        int upper = (int)b->colMax - 15;
        col = (col <= upper) ? col : upper;
    } else {
        col = (int)b->colMin - 15;
    }
    *outOriginCol = col;

    int row = partyWorldY - 39;
    if (row >= (int)b->rowMin) {
        int upper = (int)b->rowMax - 15;
        row = (row <= upper) ? row : upper;
    } else {
        row = (int)b->rowMin - 15;
    }
    *outOriginRow = row;
}

static bool cellExplored(SaveGame *save, GameKind game, int worldRow, int worldCol) {
    if (save == NULL || worldRow < 0 || worldCol < 0) {
        return false;
    }
    const SaveLayout *layout = saveLayoutFor(game);
    unsigned byteIndex = (unsigned)worldCol / 8;
    if (byteIndex >= layout->sections[SaveSectionExploredMap].recordSize) {
        return false;
    }
    uint8_t *row = saveGameRecord(save, SaveSectionExploredMap, (unsigned)worldRow);
    if (row == NULL) {
        return false;
    }
    unsigned bitInByte = (unsigned)worldCol % 8;
    return (row[byteIndex] >> (7 - bitInByte)) & 1;
}

void dungeonGridBuild(DungeonGrid *grid, GameKind game, const WorldMap *map, SaveGame *save, int partyWorldX,
                       int partyWorldY) {
    memset(grid, 0, sizeof(*grid));
    grid->game = game;
    dungeonGridComputeOrigin(game, partyWorldX, partyWorldY, &grid->originCol, &grid->originRow);

    for (int r = 0; r < DungeonGridSize; r++) {
        int worldRow = grid->originRow + r;
        for (int c = 0; c < DungeonGridSize; c++) {
            int worldCol = grid->originCol + c;
            DungeonGridCell *cell = &grid->cells[r][c];
            cell->wallType = worldMapTileA(map, (unsigned)worldRow, (unsigned)worldCol);
            cell->floorType = worldMapTileB(map, (unsigned)worldRow, (unsigned)worldCol);
            cell->reserved4 = 0;
            cell->flags = cellExplored(save, game, worldRow, worldCol) ? 0x8000 : 0;
        }
    }
}

const DungeonGridCell *dungeonGridCell(const DungeonGrid *grid, int row, int col) {
    if (row < 0 || row >= DungeonGridSize || col < 0 || col >= DungeonGridSize) {
        return NULL;
    }
    return &grid->cells[row][col];
}

const DungeonGridCell *dungeonGridCellAtWorldPos(const DungeonGrid *grid, int worldCol, int worldRow) {
    return dungeonGridCell(grid, worldRow - grid->originRow, worldCol - grid->originCol);
}

bool dungeonGridCellIsExplored(const DungeonGridCell *cell) {
    return (cell->flags & 0x8000) != 0;
}

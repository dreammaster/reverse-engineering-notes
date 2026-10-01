/* Map metadata: graphics style per map (2PLAY map_style_for_id / enter_map) and movement. */
#ifndef MM2_MAP_H
#define MM2_MAP_H

#include <stdint.h>

/* Style 0 town, 1 cave, 2 and 5 castle, 3, 4 and 6 outdoor (docs/file-formats.md "Map styles"). */
int mm2_map_style(int map);
int mm2_style_is_outdoor(int style);
/* Bank name prefix for an indoor style: "TOWN", "CAVE" or "CASTLE". */
const char *mm2_style_name(int style);
/* Terrain bank for outdoor maps from the attribute byte 04 (low nibble): "DESERT", "OCEAN", ... */
const char *mm2_special_bank(int attr04);

/* Flag byte bit that blocks movement through the given side (docs/file-formats.md). */
int mm2_side_block_bit(char side);

/* Returns 1 if the party at (x, y) can step in direction 'facing' (flag layer only: a drawn wall
 * blocks movement exactly when its blocked bit is set).  map = 512 bytes (walls, then flags). */
int mm2_can_step(const uint8_t *map, int x, int y, char facing);

#endif

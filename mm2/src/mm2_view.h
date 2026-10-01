/* First-person maze view (docs/view.md).  Port of tools/mm2_view.py. */
#ifndef MM2_VIEW_H
#define MM2_VIEW_H

#include "mm2_gfx.h"

typedef struct {
	Mm2Bank walls, floor, sky;         /* indoor: <style>.16, <style>F.16, SKY.16 */
	Mm2Bank tiles[3], special, ground; /* outdoor: OUTDOOR1-3, terrain bank, OUTF */
} Mm2View;

/* style: "TOWN", "CAVE" or "CASTLE".  Returns 0 on failure. */
int mm2_view_load_indoor(Mm2View *v, const Mm2Game *g, const char *style);
/* special: "DESERT", "TUNDRA", "SWAMP" or "OCEAN". */
int mm2_view_load_outdoor(Mm2View *v, const Mm2Game *g, const char *special);
void mm2_view_free(Mm2View *v);

/* Facing is 'N', 'E', 'S' or 'W'; y grows to the north.  walls = 256 bytes of one map. */
void mm2_view_render_indoor(Mm2View *v, uint8_t *canvas, const uint8_t *walls, int x, int y, char facing);
void mm2_view_render_outdoor(Mm2View *v, uint8_t *canvas, const uint8_t *walls, int x, int y, char facing);

/* Wall byte helper: each side has two bits (0 open, 1 wall, 2 door, 3 wall with object). */
int mm2_wall_side(uint8_t wallByte, char side);
/* Cell step for a facing. */
void mm2_facing_delta(char facing, int *dx, int *dy);

#endif

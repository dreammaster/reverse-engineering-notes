/*
 * rooms.h - floor tiles (LEVn.MP3), floor maps (LEVn.MP1) and the isometric floor renderer.
 * Formats: docs/formats.md sections 6.5/6.6, tools/roomview.py.
 */
#ifndef CADAVER_ROOMS_H
#define CADAVER_ROOMS_H

#include <stddef.h>
#include <stdint.h>

#define TILE_W 32
#define TILE_H 18
#define TILE_TRANSPARENT 0xFF

typedef struct {
	int count;
	uint8_t *pixels;           /* count * TILE_W * TILE_H colour indices 0..15, TILE_TRANSPARENT = empty */
} TileSet;

typedef struct {
	int count;                 /* one map per room */
	uint8_t *buf;              /* owned copy of the unpacked MP1 */
	const uint8_t **map;       /* pointer to each room's w*h tile ids (NULL if size 0) */
	int *len;
} FloorMaps;

int tiles_load(const uint8_t *mp3, size_t len, TileSet *ts);
void tiles_free(TileSet *ts);

int floormaps_parse(const uint8_t *mp1, size_t len, FloorMaps *fm);
void floormaps_free(FloorMaps *fm);

/* The level palette (16 Atari-ST words, 0x0RGB) stored after MP1's three sections. Returns 0 on success. */
int mp1_palette(const uint8_t *mp1, size_t len, uint16_t pal[16]);

/* Size of the canvas needed for a w x h room floor. */
static inline void floor_canvas_size(int w, int h, int *cw, int *ch) {
	*cw = 16 * (w + h) + 16;
	*ch = 8 * (w + h) + 18;
}

/* Draw the floor of a room into a caller-provided canvas (cw*ch bytes, pre-filled with TILE_TRANSPARENT):
 * tile (col,row) is placed at (16*h + 16*(col-row), 8*(col+row)), rows from the top. */
void floor_render(const TileSet *ts, const uint8_t *map, int w, int h, uint8_t *canvas);

#endif

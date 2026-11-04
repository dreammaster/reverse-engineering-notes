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
	int w, h;                  /* 32x18 floor tiles, 32x32 wall tiles */
	uint8_t *pixels;           /* count * w * h colour indices 0..15, TILE_TRANSPARENT = empty */
} TileSet;

typedef struct {
	int count;                 /* one map per room */
	uint8_t *buf;              /* owned copy of the unpacked MP1 */
	const uint8_t **map;       /* pointer to each room's w*h tile ids (NULL if size 0) */
	int *len;
} FloorMaps;

int tiles_load(const uint8_t *mp3, size_t len, TileSet *ts);      /* floor tiles: first MP3 block */
int walltiles_load(const uint8_t *mp3, size_t len, TileSet *ts);  /* wall blocks: second MP3 block */
void tiles_free(TileSet *ts);

int floormaps_parse(const uint8_t *mp1, size_t len, FloorMaps *fm);   /* MP1 section 0 */
int wallrecs_parse(const uint8_t *mp1, size_t len, FloorMaps *fm);    /* MP1 section 1 (same shape: one record per room) */
void floormaps_free(FloorMaps *fm);

/* The level palette (16 Atari-ST words, 0x0RGB) stored after MP1's three sections. Returns 0 on success. */
int mp1_palette(const uint8_t *mp1, size_t len, uint16_t pal[16]);

/* Screen pixel position of tile (0,0) of a w x h room (3..10 each) as the game places it in its
 * 320-pixel wide viewport (table at CAD.EXE ss:4D30). Returns 0 on success. */
int floor_origin(int w, int h, int *x, int *y);

#define VIEW_W 320
#define VIEW_H 136

/* The room as the game draws it into its 320x136 viewport: floor tiles, then the wall columns
 * (record layout: docs/formats.md 6.5). `canvas` is VIEW_W*VIEW_H bytes, pre-filled with TILE_TRANSPARENT.
 * w and h must be 3..10; wrec may be NULL. */
void room_render_shell(const TileSet *floor, const TileSet *walls, const uint8_t *fmap, const uint8_t *wrec, int wrec_len,
                       int w, int h, uint8_t *canvas);

/* Size of the canvas needed for a w x h room floor. */
static inline void floor_canvas_size(int w, int h, int *cw, int *ch) {
	*cw = 16 * (w + h) + 16;
	*ch = 8 * (w + h) + 18;
}

/* Draw the floor of a room into a caller-provided canvas (cw*ch bytes, pre-filled with TILE_TRANSPARENT):
 * tile (col,row) is placed at (16*h + 16*(col-row), 8*(col+row)), rows from the top. */
void floor_render(const TileSet *ts, const uint8_t *map, int w, int h, uint8_t *canvas);

#endif

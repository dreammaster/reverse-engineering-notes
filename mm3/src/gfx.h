/* MM3 graphics: master palette and the literal/skip/fill sprite codec (docs/mm3-re.md sections 4, 5).
 * C port of tools/mm3_gfx.py. */
#ifndef MM3_GFX_H
#define MM3_GFX_H

#include <stddef.h>
#include <stdint.h>

#include "cc.h"

#define MM3_SCREEN_W 320
#define MM3_SCREEN_H 200
#define MM3_RAW_SIZE (MM3_SCREEN_W * MM3_SCREEN_H)

typedef struct {
	uint8_t rgb[256][3]; /* 8-bit RGB (6-bit VGA DAC value * 4) */
} Mm3Palette;

typedef struct {
	int x_off, y_off, w, h;
	uint8_t *pixels; /* w*h palette indices, 0 = transparent; owned */
} Mm3Layer;

typedef struct {
	int w, h;
	uint8_t *pixels; /* union rectangle of the layers, w*h palette indices, 0 = transparent; owned */
	int x0, y0;      /* position of the union rectangle relative to the draw position (min of the layer offsets) */
	int nlayers;
	Mm3Layer layer[2];
} Mm3Frame;

typedef struct {
	Mm3Frame *frames;
	unsigned count;
} Mm3Sprite;

/* Palette from the code module (member 8F99h of MM3.CC, offset 39Ch). */
int mm3_palette_load(Mm3Palette *pal, const Mm3Cc *mm3cc);

/* Decode a sprite resource (frames composite their two layers into the union rectangle).  0 on success. */
int mm3_sprite_decode(Mm3Sprite *spr, const uint8_t *data, size_t len);
void mm3_sprite_free(Mm3Sprite *spr);

/* Blit a frame (index 0 transparent) into an 8-bit surface of size surf_w x surf_h at (x, y), clipped. */
void mm3_blit(uint8_t *surf, int surf_w, int surf_h, const Mm3Frame *f, int x, int y);

/* Draw as the video module does: each layer at (x + its own offset), optionally mirrored horizontally within the layer. */
void mm3_blit_layers(uint8_t *surf, int surf_w, int surf_h, const Mm3Frame *f, int x, int y, int mirror);

#endif

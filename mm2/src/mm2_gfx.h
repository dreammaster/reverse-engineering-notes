/* Image banks (*.16 = 4 bpp, *.4 = 2 bpp); see docs/file-formats.md.  Port of tools/mm2_gfx.py. */
#ifndef MM2_GFX_H
#define MM2_GFX_H

#include "mm2_files.h"

#define MM2_SCREEN_W 320
#define MM2_SCREEN_H 200

typedef struct {
	int w, h;
	uint8_t *pix;    /* w*h palette indices */
	uint8_t *mask;   /* w*h, 1 = opaque; NULL = fully opaque */
} Mm2Image;

typedef struct {
	Mm2Blob raw;
	int count;
	int bpp;
	Mm2Image *cache;   /* decoded lazily */
	uint8_t *decoded;
} Mm2Bank;

/* Loads NAME.16 (bpp 4) or NAME.4 (bpp 2).  Returns 0 on failure. */
int mm2_bank_load(const Mm2Game *g, const char *file, int bpp, Mm2Bank *b);
void mm2_bank_free(Mm2Bank *b);
/* Decoded image (owned by the bank); NULL for a bad index. */
const Mm2Image *mm2_bank_image(Mm2Bank *b, int idx);

/* Default EGA palette as 0xRRGGBB. */
extern const uint32_t MM2_EGA_PALETTE[16];

/* Draws with the mask (transparent pixels keep the canvas). */
void mm2_blit(uint8_t *canvas, const Mm2Image *img, int x, int y);

#endif

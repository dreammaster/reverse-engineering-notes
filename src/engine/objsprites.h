/*
 * objsprites.h - object sprites in LEVn.MP1 section 2 (docs/formats.md 6.5, tools/objsprites.py).
 */
#ifndef CADAVER_OBJSPRITES_H
#define CADAVER_OBJSPRITES_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int w, h;                   /* pixels */
	uint8_t *pixels;            /* w*h colour indices 0..15, 0 = transparent */
} ObjFrame;

typedef struct {
	int block;                  /* record index in section 2 */
	uint8_t header[32];         /* not yet decoded */
	int nframes;
	ObjFrame *frames;
} ObjSprite;

typedef struct {
	int count;
	ObjSprite *sprites;         /* only blocks that parse as sprite blocks */
} ObjSprites;

int objsprites_load(const uint8_t *mp1, size_t len, ObjSprites *out);
void objsprites_free(ObjSprites *o);

#endif

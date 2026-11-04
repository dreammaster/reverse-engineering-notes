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
	uint8_t header[32];         /* words h[1], h[2], h[8..10] decoded: docs/formats.md 6.12 */
	int nframes;
	ObjFrame *frames;
	int extra_len;              /* bytes between the header and the first frame (animation script of animated blocks) */
	uint8_t *extra;
} ObjSprite;

typedef struct {
	int count;
	ObjSprite *sprites;         /* only blocks that parse as sprite blocks */
	int nhdr;                   /* raw 32-byte headers of every non-empty block (index = block number) */
	uint8_t (*hdr)[32];
	uint8_t *has_hdr;
} ObjSprites;

int objsprites_load(const uint8_t *mp1, size_t len, ObjSprites *out);
void objsprites_free(ObjSprites *o);

#endif

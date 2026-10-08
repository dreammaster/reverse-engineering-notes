#include "recomp.h"

#include <stdio.h>

uint8_t DG[65536];
uint8_t STK[65536];

#define MAX_EXTRA_SEGS 8
static struct { uint16_t seg; uint8_t *mem; } extra[MAX_EXTRA_SEGS];

void recomp_register_segment(uint16_t seg, uint8_t *mem) {
	for (int i = 0; i < MAX_EXTRA_SEGS; i++)
		if (!extra[i].mem || extra[i].seg == seg) {
			extra[i].seg = seg;
			extra[i].mem = mem;
			return;
		}
}

uint8_t *recomp_seg_mem(uint16_t seg) {
	if (seg == DSEG) return DG;
	if (seg == STACK_SEG) return STK;
	for (int i = 0; i < MAX_EXTRA_SEGS; i++)
		if (extra[i].mem && extra[i].seg == seg) return extra[i].mem;
	fprintf(stderr, "recomp: access through unmapped segment %04X\n", seg);
	abort();
}

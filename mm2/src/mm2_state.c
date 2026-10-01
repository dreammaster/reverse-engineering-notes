#include "mm2_state.h"

/* (DGROUP offset, length, state offset) from the table at DGROUP:5309 (docs/save-format.md). */
static const struct {
	unsigned dg, len, off;
} MAP[] = {
	{0x03A2, 20, 0x0000}, {0x03B6, 20, 0x0014}, {0x0416, 16, 0x0028}, {0x0426, 2, 0x0038}, {0x03CA, 2, 0x003A},
	{0x03CC, 2, 0x003C},  {0x040E, 2, 0x003E},  {0x0410, 2, 0x0040},  {0x0412, 2, 0x0042}, {0x968C, 1920, 0x0044},
	{0x03EC, 10, 0x07C4}, {0x03F6, 24, 0x07CE}, {0x03DC, 4, 0x07E6},  {0x03CE, 1, 0x07EA}, {0x03D0, 12, 0x07EB},
	{0x03E0, 11, 0x07F7}, {0x0414, 1, 0x0802},  {0x0415, 1, 0x0803},
};

uint8_t *mm2_state_ptr(Mm2State *s, unsigned dg) {
	unsigned i;
	for (i = 0; i < sizeof(MAP) / sizeof(MAP[0]); i++)
		if (dg >= MAP[i].dg && dg < MAP[i].dg + MAP[i].len)
			return s->blk + MAP[i].off + (dg - MAP[i].dg);
	return 0;
}

unsigned mm2_event_var_dgroup(int v) {
	if (v >= 0 && v < 0x18) return 0x3F6u + (unsigned)v;
	if (v == 0x23) return 0x3D8;
	if (v == 0x2B) return 0x3E0;
	if (v == 0x2C) return 0x3E1;
	if (v == 0x32) return 0x3EA;
	if (v == 0x33) return 0x3F1;
	if (v >= 0x27 && v <= 0x2A) return 0x3B5u + (unsigned)v;
	if (v >= 0x3B && v <= 0x3E) return 0x3B7u + (unsigned)v;
	if (v == 0x84) return 0x3CA;
	if (v >= 0x80 && v < 0x84) return 0x36Cu + (unsigned)v;
	return 0;
}

static unsigned rd16(Mm2State *s, unsigned dg) {
	uint8_t *p = mm2_state_ptr(s, dg);
	return p ? (unsigned)(p[0] | (p[1] << 8)) : 0;
}

unsigned mm2_state_era(const Mm2State *s) {
	return rd16((Mm2State *)s, 0x3CA);
}

unsigned mm2_state_party_size(const Mm2State *s) {
	return rd16((Mm2State *)s, 0x426);
}

unsigned mm2_state_party_id(const Mm2State *s, int slot) {
	return rd16((Mm2State *)s, 0x416u + 2u * (unsigned)slot);
}

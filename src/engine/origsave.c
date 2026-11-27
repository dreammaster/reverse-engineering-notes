#include "origsave.h"

#include <stdlib.h>
#include <string.h>

static unsigned u16(const uint8_t *p) { return p[0] | (p[1] << 8); }

/* one section: u16 length, data, count index entries; fills recs; returns the offset after it or 0 */
static size_t section(const uint8_t *d, size_t n, size_t p, int count, const uint8_t **rp, int *rl, int maxn) {
	if (p + 2 > n) return 0;
	size_t len = u16(d + p);
	p += 2;
	if (p + len + 4 * (size_t)count > n) return 0;
	const uint8_t *data = d + p, *idx = d + p + len;
	for (int i = 0; i < count && i < maxn; i++) {
		unsigned rlen = u16(idx + 4 * i), off = u16(idx + 4 * i + 2);
		if (off + rlen > len) return 0;
		rp[i] = data + off; rl[i] = (int)rlen;
	}
	return p + len + 4 * (size_t)count;
}

int origsave_parse(OrigSave *o, uint8_t *d, size_t n) {
	memset(o, 0, sizeof(*o));
	if (n < 8 + ORIGSAVE_BLOCK_LEN || memcmp(d, "mjm0DAC", 7)) return -1;
	o->level = d[7] + 1;
	const uint8_t *rp[ORIGSAVE_THINGS];
	int rl[ORIGSAVE_THINGS];
	size_t p = 8;
	int c[3] = {ORIGSAVE_ROOMS, ORIGSAVE_DOORS, ORIGSAVE_LISTS};
	for (int s = 0; s < 3; s++) {
		p = section(d, n, p, c[s], rp, rl, ORIGSAVE_THINGS);
		if (!p) return -2;
		if (s == 0) for (int i = 0; i < ORIGSAVE_ROOMS; i++) { o->room[i].p = rp[i]; o->room[i].len = rl[i]; }
		if (s == 1) for (int i = 0; i < ORIGSAVE_DOORS; i++) { o->door[i].p = rp[i]; o->door[i].len = rl[i]; }
	}
	/* section 8 (the carried list) has a variable number of index entries: find the one that makes section 6 end exactly before the data block */
	size_t end6 = n - ORIGSAVE_BLOCK_LEN;
	if (p + 2 > n) return -3;
	size_t l8 = u16(d + p);
	int ok = 0;
	for (int c8 = 0; c8 < 512 && !ok; c8++) {
		size_t q = p + 2 + l8 + 4 * (size_t)c8;
		if (q + 2 > end6) break;
		size_t l6 = u16(d + q);
		if (q + 2 + l6 + 4 * (size_t)ORIGSAVE_THINGS == end6) {
			if (!section(d, n, q, ORIGSAVE_THINGS, rp, rl, ORIGSAVE_THINGS)) return -4;
			for (int i = 0; i < ORIGSAVE_THINGS; i++) { o->thing[i].p = rp[i]; o->thing[i].len = rl[i]; }
			ok = 1;
		}
	}
	if (!ok) return -5;
	o->block = d + end6;
	o->buf = d; o->n = n;
	return 0;
}

void origsave_free(OrigSave *o) { free(o->buf); memset(o, 0, sizeof(*o)); }

unsigned origsave_b(const OrigSave *o, int ds) { return ds >= ORIGSAVE_BLOCK_BASE && ds < ORIGSAVE_BLOCK_BASE + ORIGSAVE_BLOCK_LEN ? o->block[ds - ORIGSAVE_BLOCK_BASE] : 0; }
unsigned origsave_w(const OrigSave *o, int ds) { return origsave_b(o, ds) | (origsave_b(o, ds + 1) << 8); }
uint32_t origsave_d(const OrigSave *o, int ds) { return origsave_w(o, ds) | ((uint32_t)origsave_w(o, ds + 2) << 16); }

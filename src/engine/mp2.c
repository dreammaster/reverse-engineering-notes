#include "mp2.h"

#include <stdlib.h>
#include <string.h>

static uint32_t be32(const uint8_t *p) { return ((uint32_t)p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3]; }
static unsigned be16(const uint8_t *p) { return (unsigned)((p[0] << 8) | p[1]); }

int mp2_parse(const uint8_t *data, size_t len, Mp2 *m) {
	memset(m, 0, sizeof(*m));
	m->buf = (uint8_t *)malloc(len ? len : 1);
	if (!m->buf)
		return -3;
	memcpy(m->buf, data, len);
	m->buf_len = len;

	size_t p = 0;
	for (int s = 0; s < MP2_SECTIONS; s++) {
		if (p + 10 > len)
			goto bad;
		Mp2Section *sec = &m->sec[s];
		sec->data_len = be32(m->buf + p);
		sec->alloc_len = be32(m->buf + p + 4);
		sec->count = (int)be16(m->buf + p + 8);
		size_t dir = p + 10, body = dir + 4 * (size_t)sec->count;
		if (sec->count == 0 || body > len)
			goto bad;
		sec->rec = (const uint8_t **)calloc((size_t)sec->count, sizeof(uint8_t *));
		sec->len = (int *)calloc((size_t)sec->count, sizeof(int));
		if (!sec->rec || !sec->len)
			goto bad;
		uint32_t total = 0;
		for (int i = 0; i < sec->count; i++) {
			unsigned size = be16(m->buf + dir + 4 * i), off = be16(m->buf + dir + 4 * i + 2);
			if (off != total)
				goto bad;                 /* directory must be contiguous */
			total += size;
			sec->len[i] = (int)size;
			if (body + off + size <= len)
				sec->rec[i] = m->buf + body + off;
			else
				sec->truncated = 1;
		}
		if (total != sec->data_len)
			goto bad;
		p = body + sec->data_len;
	}
	return 0;
bad:
	mp2_free(m);
	return -2;
}

void mp2_free(Mp2 *m) {
	for (int s = 0; s < MP2_SECTIONS; s++) {
		free(m->sec[s].rec);
		free(m->sec[s].len);
	}
	free(m->buf);
	memset(m, 0, sizeof(*m));
}

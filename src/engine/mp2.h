/*
 * mp2.h - LEVn.MP2 section tables (docs/formats.md section 6.4).
 *
 * Four sections: [0] script records, [1] 8-byte records, [2] room lists
 * (big-endian thing ids), [3] things (record index = thing id = language
 * string id).
 */
#ifndef CADAVER_MP2_H
#define CADAVER_MP2_H

#include <stddef.h>
#include <stdint.h>

#define MP2_SECTIONS 4

typedef struct {
	int count;
	uint32_t data_len, alloc_len;
	int truncated;           /* data runs past the end of the file (TATA/2 in the retail dump) */
	const uint8_t **rec;     /* count pointers into the owner's buffer; NULL for out-of-file records */
	int *len;                /* record sizes (0 = empty) */
} Mp2Section;

typedef struct {
	uint8_t *buf;            /* owned copy of the unpacked file */
	size_t buf_len;
	Mp2Section sec[MP2_SECTIONS];
} Mp2;

/* `data` = unpacked LEVn.MP2. Returns 0 on success. */
int mp2_parse(const uint8_t *data, size_t len, Mp2 *m);
void mp2_free(Mp2 *m);

/* Convenience for the thing table: record i is thing i (NULL if empty). */
static inline const uint8_t *mp2_thing(const Mp2 *m, int id, int *len) {
	if (id < 0 || id >= m->sec[3].count || m->sec[3].len[id] <= 0 || !m->sec[3].rec[id])
		return NULL;
	if (len)
		*len = m->sec[3].len[id];
	return m->sec[3].rec[id];
}

#endif

#include "cc.h"

#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <string.h>

#define LZ_N 4096
#define LZ_F 60
#define LZ_THRESHOLD 2
#define LZ_N_CHAR (256 - LZ_THRESHOLD + LZ_F)
#define LZ_T (LZ_N_CHAR * 2 - 1)
#define LZ_R (LZ_T - 1)
#define LZ_MAX_FREQ 0x8000

static uint8_t d_code[256], d_len[256];
static int tables_ready;

static void init_tables(void) {
	static const struct { int bits, ncodes, rep; } spec[] = {
		{3, 1, 32}, {4, 3, 16}, {5, 8, 8}, {6, 12, 4}, {7, 24, 2}, {8, 16, 1}
	};
	int c = 0, n = 0;
	for (unsigned s = 0; s < sizeof(spec) / sizeof(spec[0]); s++)
		for (int i = 0; i < spec[s].ncodes; i++, c++)
			for (int r = 0; r < spec[s].rep; r++, n++) {
				d_code[n] = (uint8_t)c;
				d_len[n] = (uint8_t)spec[s].bits;
			}
	tables_ready = 1;
}

typedef struct {
	const uint8_t *d;
	size_t len, p;
	unsigned bits, nbits;
	uint16_t freq[LZ_T + 1];
	int16_t prnt[LZ_T + LZ_N_CHAR];
	int16_t son[LZ_T];
} Lzhuf;

static void lz_init(Lzhuf *z, const uint8_t *d, size_t len, size_t pos) {
	int i, j;
	z->d = d; z->len = len; z->p = pos; z->bits = 0; z->nbits = 0;
	memset(z->freq, 0, sizeof(z->freq));
	memset(z->prnt, 0, sizeof(z->prnt));
	memset(z->son, 0, sizeof(z->son));
	for (i = 0; i < LZ_N_CHAR; i++) {
		z->freq[i] = 1;
		z->son[i] = (int16_t)(i + LZ_T);
		z->prnt[i + LZ_T] = (int16_t)i;
	}
	i = 0; j = LZ_N_CHAR;
	while (j <= LZ_R) {
		z->freq[j] = z->freq[i] + z->freq[i + 1];
		z->son[j] = (int16_t)i;
		z->prnt[i] = z->prnt[i + 1] = (int16_t)j;
		i += 2; j++;
	}
	z->freq[LZ_T] = 0xFFFF;
	z->prnt[LZ_R] = 0;
}

static int lz_getbit(Lzhuf *z) {
	if (z->nbits == 0) {
		z->bits = z->p < z->len ? z->d[z->p] : 0;
		z->p++;
		z->nbits = 8;
	}
	z->nbits--;
	return (z->bits >> z->nbits) & 1;
}

static int lz_getbyte(Lzhuf *z) {
	int v = 0;
	for (int i = 0; i < 8; i++)
		v = (v << 1) | lz_getbit(z);
	return v;
}

static void lz_reconst(Lzhuf *z) {
	int i, j = 0, k;
	for (i = 0; i < LZ_T; i++)
		if (z->son[i] >= LZ_T) {
			z->freq[j] = (uint16_t)((z->freq[i] + 1) / 2);
			z->son[j] = z->son[i];
			j++;
		}
	i = 0; j = LZ_N_CHAR;
	while (j < LZ_T) {
		unsigned f;
		int n;
		k = i + 1;
		f = z->freq[j] = (uint16_t)(z->freq[i] + z->freq[k]);
		k = j - 1;
		while (f < z->freq[k])
			k--;
		k++;
		n = j - k;
		memmove(&z->freq[k + 1], &z->freq[k], n * sizeof(z->freq[0]));
		z->freq[k] = (uint16_t)f;
		memmove(&z->son[k + 1], &z->son[k], n * sizeof(z->son[0]));
		z->son[k] = (int16_t)i;
		i += 2; j++;
	}
	for (i = 0; i < LZ_T; i++) {
		k = z->son[i];
		if (k >= LZ_T)
			z->prnt[k] = (int16_t)i;
		else
			z->prnt[k] = z->prnt[k + 1] = (int16_t)i;
	}
}

static void lz_update(Lzhuf *z, int c) {
	if (z->freq[LZ_R] == LZ_MAX_FREQ)
		lz_reconst(z);
	c = z->prnt[c + LZ_T];
	do {
		int k, l;
		k = ++z->freq[c];
		l = c + 1;
		if (k > z->freq[l]) {
			int i, j;
			while (k > z->freq[l + 1])
				l++;
			z->freq[c] = z->freq[l];
			z->freq[l] = (uint16_t)k;
			i = z->son[c];
			z->prnt[i] = (int16_t)l;
			if (i < LZ_T)
				z->prnt[i + 1] = (int16_t)l;
			j = z->son[l];
			z->son[l] = (int16_t)i;
			z->prnt[j] = (int16_t)c;
			if (j < LZ_T)
				z->prnt[j + 1] = (int16_t)c;
			z->son[c] = (int16_t)j;
			c = l;
		}
		c = z->prnt[c];
	} while (c != 0);
}

static int lz_decode_char(Lzhuf *z) {
	int c = z->son[LZ_R];
	while (c < LZ_T)
		c = z->son[c + lz_getbit(z)];
	c -= LZ_T;
	lz_update(z, c);
	return c;
}

static int lz_decode_position(Lzhuf *z) {
	int i = lz_getbyte(z);
	int c = d_code[i] << 6;
	int j = d_len[i] - 2;
	while (j--)
		i = (i << 1) | lz_getbit(z);
	return c | (i & 0x3F);
}

uint8_t *mm3_lzhuf_decode(const uint8_t *blob, size_t len, size_t *out_len) {
	Lzhuf *z;
	uint8_t *ring, *out;
	size_t size, n = 0;
	int r;

	if (!tables_ready)
		init_tables();
	if (len < 5 || blob[0] != blob[1])
		return NULL;
	size = ((size_t)blob[2] << 8) | blob[3];
	if (size == 0)
		return NULL;
	z = malloc(sizeof(*z));
	ring = malloc(LZ_N);
	out = malloc(size + LZ_F);
	if (!z || !ring || !out) {
		free(z); free(ring); free(out);
		return NULL;
	}
	lz_init(z, blob, len, 4);
	memset(ring, blob[0], LZ_N);
	r = LZ_N - LZ_F;
	while (n < size) {
		int c;
		if (z->p > len + 2)  /* ran off the payload: not an LZHUF stream */
			goto fail;
		c = lz_decode_char(z);
		if (c < 256) {
			out[n++] = (uint8_t)c;
			ring[r] = (uint8_t)c;
			r = (r + 1) & (LZ_N - 1);
		} else {
			int pos = (r - lz_decode_position(z) - 1) & (LZ_N - 1);
			int ln = c - 255 + LZ_THRESHOLD;
			for (int k = 0; k < ln; k++) {
				uint8_t b = ring[(pos + k) & (LZ_N - 1)];
				out[n++] = b;
				ring[r] = b;
				r = (r + 1) & (LZ_N - 1);
			}
		}
	}
	if (z->p > len + 2 || (len > z->p && len - z->p > 2))
		goto fail;
	free(z); free(ring);
	if (out_len)
		*out_len = size;
	return out;
fail:
	free(z); free(ring); free(out);
	return NULL;
}

uint16_t mm3_name_id(const char *name) {
	unsigned t;
	const char *p = name;
	char c = *p++;
	if (c >= 'a' && c <= 'z') c -= 32;
	t = (unsigned char)c;
	for (; *p; p++) {
		c = *p;
		if (c >= 'a' && c <= 'z') c -= 32;
		t = ((t & 0x7F) << 9) | ((t & 0xFF80) >> 7);
		t = (t + (unsigned char)c) & 0xFFFF;
	}
	return (uint16_t)t;
}

int mm3_cc_open_mem(Mm3Cc *cc, const uint8_t *data, size_t len) {
	unsigned n;
	memset(cc, 0, sizeof(*cc));
	if (len < 2)
		return -1;
	n = data[0] | (data[1] << 8);
	if (len < 2 + (size_t)n * 8)
		return -1;
	cc->data = malloc(len);
	cc->entries = calloc(n ? n : 1, sizeof(Mm3CcEntry));
	if (!cc->data || !cc->entries) {
		mm3_cc_close(cc);
		return -1;
	}
	memcpy(cc->data, data, len);
	cc->len = len;
	cc->count = n;
	for (unsigned i = 0; i < n; i++) {
		uint8_t t[8];
		for (int k = 0; k < 8; k++) {
			unsigned idx = i * 8 + k;
			unsigned c = data[2 + idx];
			t[k] = (uint8_t)((((c << 2) | (c >> 6)) & 255) - (0x54 + 0x99 * idx));
		}
		cc->entries[i].id = (uint16_t)(t[0] | (t[1] << 8));
		cc->entries[i].offset = t[2] | (t[3] << 8) | ((uint32_t)t[4] << 16);
		cc->entries[i].size = (uint16_t)(t[5] | (t[6] << 8));
	}
	return 0;
}

int mm3_cc_open(Mm3Cc *cc, const char *path) {
	FILE *f = fopen(path, "rb");
	uint8_t *buf;
	long sz;
	int rc;
	if (!f)
		return -1;
	fseek(f, 0, SEEK_END);
	sz = ftell(f);
	fseek(f, 0, SEEK_SET);
	buf = malloc(sz > 0 ? (size_t)sz : 1);
	if (!buf || sz < 0 || fread(buf, 1, (size_t)sz, f) != (size_t)sz) {
		free(buf); fclose(f);
		return -1;
	}
	fclose(f);
	rc = mm3_cc_open_mem(cc, buf, (size_t)sz);
	free(buf);
	return rc;
}

void mm3_cc_close(Mm3Cc *cc) {
	free(cc->data);
	free(cc->entries);
	memset(cc, 0, sizeof(*cc));
}

int mm3_cc_find(const Mm3Cc *cc, const char *name) {
	uint16_t id = mm3_name_id(name);
	for (unsigned i = 0; i < cc->count; i++)
		if (cc->entries[i].id == id)
			return (int)i;
	return -1;
}

uint8_t *mm3_cc_read_index(const Mm3Cc *cc, int index, size_t *out_len) {
	const Mm3CcEntry *e;
	uint8_t *res;
	if (index < 0 || (unsigned)index >= cc->count)
		return NULL;
	e = &cc->entries[index];
	if ((size_t)e->offset + e->size > cc->len)
		return NULL;
	res = mm3_lzhuf_decode(cc->data + e->offset, e->size, out_len);
	if (res)
		return res;
	res = malloc(e->size ? e->size : 1);
	if (!res)
		return NULL;
	memcpy(res, cc->data + e->offset, e->size);
	if (out_len)
		*out_len = e->size;
	return res;
}

uint8_t *mm3_cc_read(const Mm3Cc *cc, const char *name, size_t *out_len) {
	if (getenv("MM3_RESLOG")) fprintf(stderr, "[res %6.2f] %s\n", clock() / (double)CLOCKS_PER_SEC, name);
	return mm3_cc_read_index(cc, mm3_cc_find(cc, name), out_len);
}

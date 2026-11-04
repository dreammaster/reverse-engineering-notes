/*
 * mjm.c - decoders for the "mjm" container, PKWARE DCL and RNC method 1.
 *
 * The DCL decoder follows the canonical-Huffman construction used by the
 * public "blast" reference implementation; the code tables are the ones
 * defined by the PKWARE format.  The game itself ships a hand-written
 * assembler version of the same algorithm (CAD.EXE seg001:0CAA..).
 */
#include "mjm.h"

#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* growable output buffer                                              */
/* ------------------------------------------------------------------ */
typedef struct {
	uint8_t *data;
	size_t len, cap;
} Buf;

static int buf_put(Buf *b, uint8_t v) {
	if (b->len == b->cap) {
		size_t nc = b->cap ? b->cap * 2 : 65536;
		uint8_t *nd = (uint8_t *)realloc(b->data, nc);
		if (!nd)
			return MJM_ERR_MEMORY;
		b->data = nd;
		b->cap = nc;
	}
	b->data[b->len++] = v;
	return MJM_OK;
}

/* ------------------------------------------------------------------ */
/* PKWARE DCL                                                          */
/* ------------------------------------------------------------------ */
typedef struct {
	short count[16];
	short symbol[256];
} Huff;

typedef struct {
	const uint8_t *in;
	size_t len, pos;
	unsigned bitbuf;
	int bitcnt;
	int err;
} Bits;

static const uint8_t litlen[] = {
	11, 124, 8, 7, 28, 7, 188, 13, 76, 4, 10, 8, 12, 10, 12, 10, 8, 23, 8,
	9, 7, 6, 7, 8, 7, 6, 55, 8, 23, 24, 12, 11, 7, 9, 11, 12, 6, 7, 22, 5,
	7, 24, 6, 11, 9, 6, 7, 22, 7, 11, 38, 7, 9, 8, 25, 11, 8, 11, 9, 12,
	8, 12, 5, 38, 5, 38, 5, 11, 7, 5, 6, 21, 6, 10, 53, 8, 7, 24, 10, 27,
	44, 253, 253, 253, 252, 252, 252, 13, 12, 45, 12, 45, 12, 61, 12, 45,
	44, 173};
static const uint8_t lenlen[] = {2, 35, 36, 53, 38, 23};
static const uint8_t distlen[] = {2, 20, 53, 230, 247, 151, 248};
static const short lenbase[16] = {3, 2, 4, 5, 6, 7, 8, 9, 10, 12, 16, 24, 40, 72, 136, 264};
static const short extra[16] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8};

static void huff_build(Huff *h, const uint8_t *rep, int n) {
	short length[256];
	short offs[16];
	int sym = 0;
	for (int i = 0; i < n; i++) {
		int run = (rep[i] >> 4) + 1, l = rep[i] & 15;
		while (run--)
			length[sym++] = (short)l;
	}
	memset(h->count, 0, sizeof(h->count));
	for (int s = 0; s < sym; s++)
		h->count[length[s]]++;
	offs[1] = 0;
	for (int l = 1; l < 15; l++)
		offs[l + 1] = (short)(offs[l] + h->count[l]);
	for (int s = 0; s < sym; s++)
		if (length[s])
			h->symbol[offs[length[s]]++] = (short)s;
}

static unsigned get_bits(Bits *b, int need) {
	unsigned val = b->bitbuf;
	while (b->bitcnt < need) {
		if (b->pos >= b->len) {
			b->err = 1;
			return 0;
		}
		val |= (unsigned)b->in[b->pos++] << b->bitcnt;
		b->bitcnt += 8;
	}
	b->bitbuf = val >> need;
	b->bitcnt -= need;
	return val & ((1u << need) - 1);
}

static int huff_decode(Bits *b, const Huff *h) {
	int code = 0, first = 0, index = 0;
	for (int l = 1; l <= 15; l++) {
		code |= (int)get_bits(b, 1) ^ 1;
		if (b->err)
			return -1;
		int count = h->count[l];
		if (code - count < first)
			return h->symbol[index + (code - first)];
		index += count;
		first += count;
		first <<= 1;
		code <<= 1;
	}
	return -1;
}

int dcl_explode(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len) {
	static Huff lit_h, len_h, dist_h;
	static int init;
	if (!init) {
		huff_build(&lit_h, litlen, sizeof(litlen));
		huff_build(&len_h, lenlen, sizeof(lenlen));
		huff_build(&dist_h, distlen, sizeof(distlen));
		init = 1;
	}
	if (in_len < 2 || in[0] > 1 || in[1] < 4 || in[1] > 6)
		return MJM_ERR_FORMAT;
	int ascii = in[0], dbits = in[1];
	Bits b = {in, in_len, 2, 0, 0, 0};
	Buf o = {0, 0, 0};
	int rc = MJM_OK;

	for (;;) {
		if (get_bits(&b, 1)) {
			int sym = huff_decode(&b, &len_h);
			if (sym < 0) { rc = MJM_ERR_DATA; break; }
			int len = lenbase[sym] + (int)get_bits(&b, extra[sym]);
			if (b.err) { rc = MJM_ERR_DATA; break; }
			if (len == 519)
				break;
			int sh = len == 2 ? 2 : dbits;
			int ds = huff_decode(&b, &dist_h);
			if (ds < 0) { rc = MJM_ERR_DATA; break; }
			size_t dist = ((size_t)ds << sh) + get_bits(&b, sh) + 1;
			if (b.err || dist > o.len) { rc = MJM_ERR_DATA; break; }
			for (int i = 0; i < len && rc == MJM_OK; i++)
				rc = buf_put(&o, o.data[o.len - dist]);
		} else {
			int c;
			if (ascii) {
				c = huff_decode(&b, &lit_h);
				if (c < 0) { rc = MJM_ERR_DATA; break; }
			} else {
				c = (int)get_bits(&b, 8);
			}
			if (b.err) { rc = MJM_ERR_DATA; break; }
			rc = buf_put(&o, (uint8_t)c);
		}
		if (rc != MJM_OK)
			break;
	}
	if (b.err && rc == MJM_OK)
		rc = MJM_ERR_DATA;
	if (rc != MJM_OK) {
		free(o.data);
		return rc;
	}
	*out = o.data ? o.data : (uint8_t *)malloc(1);
	*out_len = o.len;
	return MJM_OK;
}

/* ------------------------------------------------------------------ */
/* RNC method 1 ("Pro-Pack")                                           */
/* ------------------------------------------------------------------ */
uint16_t crc16_arc(const uint8_t *data, size_t len) {
	uint16_t crc = 0;
	for (size_t i = 0; i < len; i++) {
		crc ^= data[i];
		for (int k = 0; k < 8; k++)
			crc = (uint16_t)((crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1);
	}
	return crc;
}

typedef struct {
	const uint8_t *d;
	size_t len, pos;
	uint32_t buf;
	int cnt;
	int err;
} RBits;

static unsigned r_word(RBits *r) {
	unsigned w = 0;
	if (r->pos < r->len) w = r->d[r->pos];
	if (r->pos + 1 < r->len) w |= (unsigned)r->d[r->pos + 1] << 8;
	r->pos += 2;
	return w;
}

static int r_bit(RBits *r) {
	if (r->cnt == 0) {
		r->buf = r_word(r);
		r->cnt = 16;
	}
	int b = r->buf & 1;
	r->buf >>= 1;
	r->cnt--;
	return b;
}

static unsigned r_bits(RBits *r, int n) {
	unsigned v = 0;
	for (int i = 0; i < n; i++)
		v |= (unsigned)r_bit(r) << i;
	return v;
}

typedef struct {
	int n;
	uint8_t len[32];
	uint16_t code[32];
} RTable;

static void r_table(RBits *r, RTable *t) {
	t->n = (int)r_bits(r, 5);
	if (t->n > 32) { r->err = 1; t->n = 0; }
	for (int i = 0; i < t->n; i++)
		t->len[i] = (uint8_t)r_bits(r, 4);
	unsigned code = 0;
	for (int l = 1; l <= 16; l++) {
		for (int i = 0; i < t->n; i++)
			if (t->len[i] == l)
				t->code[i] = (uint16_t)code++;
		code <<= 1;
	}
}

static int r_huff(RBits *r, const RTable *t) {
	unsigned code = 0;
	for (int l = 1; l <= 16; l++) {
		code = (code << 1) | (unsigned)r_bit(r);
		for (int i = 0; i < t->n; i++) {
			if (t->len[i] == l && t->code[i] == code) {
				if (i < 2)
					return i;
				return (int)((1u << (i - 1)) | r_bits(r, i - 1));
			}
		}
		if (r->pos > r->len + 4) break;
	}
	r->err = 1;
	return 0;
}

int rnc1_unpack(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len) {
	if (in_len < 18 || memcmp(in, "RNC", 3) != 0 || in[3] != 1)
		return MJM_ERR_FORMAT;
	uint32_t ulen = ((uint32_t)in[4] << 24) | (in[5] << 16) | (in[6] << 8) | in[7];
	uint32_t plen = ((uint32_t)in[8] << 24) | (in[9] << 16) | (in[10] << 8) | in[11];
	uint16_t ucrc = (uint16_t)((in[12] << 8) | in[13]);
	uint16_t pcrc = (uint16_t)((in[14] << 8) | in[15]);
	if (18 + (size_t)plen > in_len || ulen > (64u << 20))
		return MJM_ERR_FORMAT;
	if (crc16_arc(in + 18, plen) != pcrc)
		return MJM_ERR_CRC;

	uint8_t *o = (uint8_t *)malloc(ulen ? ulen : 1);
	if (!o)
		return MJM_ERR_MEMORY;
	RBits r = {in, in_len, 18, 0, 0, 0};
	r_bits(&r, 2);
	size_t n = 0;
	while (n < ulen && !r.err) {
		RTable raw, dist, len;
		r_table(&r, &raw);
		r_table(&r, &dist);
		r_table(&r, &len);
		int chunks = (int)r_bits(&r, 16);
		for (;;) {
			int cnt = r_huff(&r, &raw);
			while (cnt-- > 0 && !r.err) {
				if (r.pos >= in_len || n >= ulen) { r.err = 1; break; }
				o[n++] = in[r.pos++];
			}
			if (--chunks <= 0 || r.err)
				break;
			size_t pos = (size_t)r_huff(&r, &dist) + 1;
			int l = r_huff(&r, &len) + 2;
			if (pos > n) { r.err = 1; break; }
			while (l-- > 0 && n < ulen)
				{ o[n] = o[n - pos]; n++; }
		}
	}
	if (r.err || n != ulen) {
		free(o);
		return MJM_ERR_DATA;
	}
	if (crc16_arc(o, ulen) != ucrc) {
		free(o);
		return MJM_ERR_CRC;
	}
	*out = o;
	*out_len = ulen;
	return MJM_OK;
}

/* ------------------------------------------------------------------ */
int mjm_unpack(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len) {
	if (in_len < 4 || in[0] != 'm' || in[1] != 'j' || in[2] != 'm')
		return MJM_ERR_FORMAT;
	switch (in[3]) {
	case '0': {
		uint8_t *o = (uint8_t *)malloc(in_len - 4 ? in_len - 4 : 1);
		if (!o)
			return MJM_ERR_MEMORY;
		memcpy(o, in + 4, in_len - 4);
		*out = o;
		*out_len = in_len - 4;
		return MJM_OK;
	}
	case '1':
		return dcl_explode(in + 4, in_len - 4, out, out_len);
	case '2':
		return rnc1_unpack(in + 4, in_len - 4, out, out_len);
	}
	return MJM_ERR_FORMAT;
}

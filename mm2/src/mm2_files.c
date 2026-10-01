#include "mm2_files.h"
#include "mm2_lzw.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_DIR "D:/GOG Games/Might and Magic 2"

void mm2_game_init(Mm2Game *g, const char *dir) {
	if (!dir) dir = getenv("MM2_DIR");
	if (!dir) dir = DEFAULT_DIR;
	snprintf(g->dir, sizeof(g->dir), "%s", dir);
}

Mm2Blob mm2_read_file(const Mm2Game *g, const char *name) {
	Mm2Blob b = {NULL, 0};
	char path[1024];
	FILE *f;
	long n;
	snprintf(path, sizeof(path), "%s/%s", g->dir, name);
	f = fopen(path, "rb");
	if (!f) return b;
	fseek(f, 0, SEEK_END);
	n = ftell(f);
	fseek(f, 0, SEEK_SET);
	b.data = (uint8_t *)malloc(n > 0 ? (size_t)n : 1);
	if (b.data && fread(b.data, 1, (size_t)n, f) == (size_t)n)
		b.size = (size_t)n;
	else {
		free(b.data);
		b.data = NULL;
	}
	fclose(f);
	return b;
}

void mm2_blob_free(Mm2Blob *b) {
	free(b->data);
	b->data = NULL;
	b->size = 0;
}

Mm2Blob mm2_lzw_blob(const uint8_t *in, size_t n) {
	Mm2Blob b = {NULL, 0};
	uint32_t size;
	long got;
	if (n < 4) return b;
	size = in[0] | (in[1] << 8) | (in[2] << 16) | ((uint32_t)in[3] << 24);
	b.data = (uint8_t *)malloc(size ? size : 1);
	if (!b.data) return b;
	got = mm2_lzw_decode(in + 4, n - 4, b.data, size);
	if (got != (long)size) {
		free(b.data);
		b.data = NULL;
		return b;
	}
	b.size = size;
	return b;
}

Mm2Blob mm2_load_lzw_file(const Mm2Game *g, const char *name) {
	Mm2Blob raw = mm2_read_file(g, name), out = {NULL, 0};
	if (raw.data) {
		out = mm2_lzw_blob(raw.data, raw.size);
		mm2_blob_free(&raw);
	}
	return out;
}

static uint32_t rd32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

int mm2_load_map(const Mm2Game *g, int map, uint8_t out[512]) {
	Mm2Blob d, c;
	size_t from, to;
	if (map < 0 || map >= MM2_MAPS) return 0;
	d = mm2_read_file(g, "MAP.DAT");
	if (!d.data) return 0;
	from = d.data[map * 2] | (d.data[map * 2 + 1] << 8);
	to = map + 1 < MM2_MAPS ? (size_t)(d.data[map * 2 + 2] | (d.data[map * 2 + 3] << 8)) : d.size;
	c = mm2_lzw_blob(d.data + from, to - from);
	mm2_blob_free(&d);
	if (!c.data || c.size != 512) {
		mm2_blob_free(&c);
		return 0;
	}
	memcpy(out, c.data, 512);
	mm2_blob_free(&c);
	return 1;
}

int mm2_load_attrib(const Mm2Game *g, int map, uint8_t out[64]) {
	Mm2Blob b = mm2_load_lzw_file(g, "ATTRIB.DAT");
	int ok = b.data && map >= 0 && (size_t)(map + 1) * 64 <= b.size;
	if (ok) memcpy(out, b.data + map * 64, 64);
	mm2_blob_free(&b);
	return ok;
}

Mm2Blob mm2_load_events(const Mm2Game *g, int map) {
	Mm2Blob none = {NULL, 0}, d, c;
	uint32_t offs[71] = {0}, first;
	int n, i;
	size_t nxt;
	d = mm2_read_file(g, (map >= 5 && map <= 16) ? "EVENTSO.DAT" : "EVENTSI.DAT");
	if (!d.data) return none;
	first = rd32(d.data);
	if (!first) first = rd32(d.data + 20);
	n = (int)(first / 4);
	if (n > 71) n = 71;
	for (i = 0; i < n; i++) offs[i] = rd32(d.data + 4 * i);
	if (map >= n || !offs[map]) {
		mm2_blob_free(&d);
		return none;
	}
	nxt = d.size;
	for (i = 0; i < n; i++)
		if (offs[i] > offs[map] && offs[i] < nxt) nxt = offs[i];
	c = mm2_lzw_blob(d.data + offs[map], nxt - offs[map]);
	mm2_blob_free(&d);
	return c;
}

int mm2_parse_events(const Mm2Blob *chunk, Mm2EventChunk *out) {
	const uint8_t *d = chunk->data;
	size_t p = 0, size;
	int n = 0;
	if (!d) return 0;
	out->triggers = d;
	while (p + 3 <= chunk->size && !(d[p] == 0 && d[p + 1] == 0 && d[p + 2] == 0)) {
		p += 3;
		n++;
	}
	out->nTriggers = n;
	p += 3;
	if (p + 2 > chunk->size) return 0;
	size = d[p] | (d[p + 1] << 8);
	if (p + size > chunk->size) return 0;
	out->scripts = d + p + 2;
	out->scriptsLen = size - 2;
	out->messages = d + p + size;
	out->messagesLen = chunk->size - (p + size);
	return 1;
}

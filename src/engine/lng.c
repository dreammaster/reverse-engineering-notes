#include "lng.h"

#include <stdlib.h>
#include <string.h>

/* CAD.EXE text6_alphabet (1F17:F853), 1-based; code 0 ends a string. */
static const char alphabet[64] = "\0ABCDEFGHIJKLMNOPQRSTUVWXYZ .,0123456789\r\r-?\"<=>";

size_t lng_decode_string(const uint8_t *data, size_t len, size_t pos, char *out, size_t outsz) {
	size_t n = 0;
	while (pos < len) {
		uint8_t b[3] = {0, 0, 0};
		for (int i = 0; i < 3 && pos + i < len; i++)
			b[i] = data[pos + i];
		unsigned codes[4] = {b[0] >> 2u, ((b[0] & 3u) << 4) | (b[1] >> 4u), ((b[1] & 15u) << 2) | (b[2] >> 6u),
		                     b[2] & 0x3Fu};
		for (int i = 0; i < 4; i++) {
			if (codes[i] == 0)
				goto done;
			if (codes[i] <= 47 && n + 1 < outsz)
				out[n++] = alphabet[codes[i]];
		}
		pos += 3;
	}
done:
	if (outsz)
		out[n] = 0;
	return n;
}

int lng_parse(const uint8_t *data, size_t len, LngTable *t) {
	memset(t, 0, sizeof(*t));
	if (len < 4)
		return -1;
	int count = (data[0] << 8) | data[1];
	size_t base = 2 + 2 * (size_t)count + 2;
	if (base > len)
		return -1;
	t->strings = (char **)calloc((size_t)count, sizeof(char *));
	if (!t->strings)
		return -1;
	t->count = count;
	for (int i = 0; i < count; i++) {
		size_t off = ((size_t)data[2 + 2 * i] << 8) | data[3 + 2 * i];
		char buf[2048];
		if (base + off > len) {
			lng_free(t);
			return -1;
		}
		lng_decode_string(data, len, base + off, buf, sizeof(buf));
		t->strings[i] = strdup(buf);
		if (!t->strings[i]) {
			lng_free(t);
			return -1;
		}
	}
	return 0;
}

void lng_free(LngTable *t) {
	for (int i = 0; i < t->count; i++)
		free(t->strings[i]);
	free(t->strings);
	memset(t, 0, sizeof(*t));
}

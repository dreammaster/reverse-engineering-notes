/*
 * lng.h - language string tables (ENGn.LNG / FREn.LNG / GERn.LNG).
 * Format: see docs/formats.md section 5 and tools/lng.py.
 */
#ifndef CADAVER_LNG_H
#define CADAVER_LNG_H

#include <stddef.h>
#include <stdint.h>

typedef struct {
	int count;
	char **strings;  /* NUL terminated, 'A'-'Z' plus the game's glyph codes; '\r' = line break */
} LngTable;

/* Parse an already mjm-unpacked language file. Returns 0 on success. */
int lng_parse(const uint8_t *data, size_t len, LngTable *out);
void lng_free(LngTable *t);

/* Decode one 6-bit packed string; returns bytes written (without the NUL). */
size_t lng_decode_string(const uint8_t *data, size_t len, size_t pos, char *out, size_t outsz);

#endif

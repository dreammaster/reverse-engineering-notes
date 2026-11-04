/*
 * mjm.h - Cadaver "mjm" container + the two compressors it wraps.
 *
 * File layout (see docs/formats.md):
 *      'm' 'j' 'm' <method>
 *        method '0': payload is stored raw
 *        method '1': payload is a PKWARE DCL "implode" stream
 *        method '2': payload is a Rob Northen "RNC\x01" (Pro-Pack) stream
 */
#ifndef CADAVER_MJM_H
#define CADAVER_MJM_H

#include <stddef.h>
#include <stdint.h>

/* Error codes (negative) */
enum {
	MJM_OK = 0,
	MJM_ERR_FORMAT = -1,   /* bad magic / header */
	MJM_ERR_DATA = -2,     /* corrupt stream */
	MJM_ERR_MEMORY = -3,
	MJM_ERR_CRC = -4
};

/* Decompress a whole mjm file image. On success *out is malloc()ed (caller
 * frees) and *out_len set. */
int mjm_unpack(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len);

/* The underlying codecs, exposed for tests and for other file types. */
int dcl_explode(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len);
int rnc1_unpack(const uint8_t *in, size_t in_len, uint8_t **out, size_t *out_len);

uint16_t crc16_arc(const uint8_t *data, size_t len);

#endif

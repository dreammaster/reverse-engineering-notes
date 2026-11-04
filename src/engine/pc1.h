/*
 * pc1.h - *.PC1 picture decoder (LOGO.PC1, LOADING2.PC1).
 * Format documented in docs/formats.md section 3.
 */
#ifndef CADAVER_PC1_H
#define CADAVER_PC1_H

#include <stddef.h>
#include <stdint.h>

#define PC1_WIDTH 320
#define PC1_HEIGHT 200

typedef struct {
	uint8_t palette[16][3];              /* RGB, 0-255 */
	uint8_t pixels[PC1_WIDTH * PC1_HEIGHT]; /* colour indices 0-15 */
} Pc1Image;

/* Returns 0 on success, negative on malformed data. */
int pc1_decode(const uint8_t *data, size_t len, Pc1Image *img);

/* The raw LZ stage, exposed for tests. Returned buffer is malloc()ed. */
int pc1_lz(const uint8_t *data, size_t len, uint8_t **out, size_t *out_len);

#endif

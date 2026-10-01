/* Minimal PNG writer (stored deflate blocks) for debugging output. */
#ifndef MM2_PNG_H
#define MM2_PNG_H

#include <stdint.h>

/* indexed: w*h palette indices, palette: 0xRRGGBB entries (n <= 256).  Returns 1 on success. */
int mm2_write_png(const char *path, const uint8_t *indexed, int w, int h, const uint32_t *palette, int n);

#endif

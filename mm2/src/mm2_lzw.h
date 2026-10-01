/* LZW decoder for the MM2 data files (docs/file-formats.md): 9-12 bit codes, LSB first,
 * 100h = clear, 101h = end.  Port of tools/mm2_lzw.py. */
#ifndef MM2_LZW_H
#define MM2_LZW_H

#include <stddef.h>
#include <stdint.h>

/* Decodes into out (capacity outcap); returns the number of bytes written, or -1 on a corrupt
 * stream / overflow. */
long mm2_lzw_decode(const uint8_t *in, size_t inlen, uint8_t *out, size_t outcap);

#endif

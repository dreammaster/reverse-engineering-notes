/* ags/lzw.h -- M5 ("Open a room", see src/PLAN.md): the two low-level
 * decompression primitives room-loading needs, ported near-verbatim
 * from Common/lzw.cpp (lzwexpand/lzwexpand_to_mem, the LZSS-variant
 * background-image compressor) and Common/compress.cpp (cunpackbitl,
 * the PackBits-style per-row RLE decoder used for the walkable/walk-
 * behind/hotspot/region masks). Both are genuinely AGS-owned Common/
 * code (not a third-party library), already confirmed by matches.json
 * at the header/structural level (malloc size, macro-expansion getc,
 * putbytes reset for lzwexpand; the ferror/cx==-128 sentinel fix for
 * cunpackbitl) -- ported directly from the still-present reference
 * source rather than re-derived instruction-by-instruction, per that
 * entry's own note that further bit-level tracing "would mostly
 * re-verify a well-known, unremarkable LZSS-variant algorithm already
 * fully present in this repo's own Common/lzw.cpp."
 */
#ifndef AGS_LZW_H
#define AGS_LZW_H

#include <stdio.h>

/* Common/lzw.cpp's lzwexpand_to_mem(FILE*), fused with lzwexpand's own
 * body (the file-output path is never used by room loading, so isn't
 * ported). `maxsize` is the exact decompressed byte count the caller
 * already knows (load_lzw's own on-disk `maxsize` field, read right
 * before calling this) -- decompression stops once that many bytes
 * have been produced, matching source's own `putbytes>=maxsize`
 * early-exit. Returns a malloc'd buffer of `maxsize+10` bytes (source's
 * own small safety margin), or NULL on allocation failure. Caller
 * frees it. */
unsigned char *ags_lzw_expand_to_mem(FILE *f, long maxsize);

/* Common/compress.cpp:215's cunpackbitl(unsigned char*,int,FILE*) --
 * a direct, unmodified port. Decodes one row of `size` bytes into
 * `line` (caller-owned). Returns 0 on success, ferror(infile)'s own
 * nonzero value on a stream error, or -1 on a decoded-run/sequence
 * overflowing `size` (source's own buffer-overflow guard). */
int ags_cunpackbitl(unsigned char *line, int size, FILE *infile);

#endif /* AGS_LZW_H */

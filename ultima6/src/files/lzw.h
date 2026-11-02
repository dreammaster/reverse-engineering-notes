#ifndef ULTIMA6_FILES_LZW_H
#define ULTIMA6_FILES_LZW_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * The LZW decoder of GAME.EXE (sub_3ACB4, driven by sub_3AEDC/sub_3AC50): a
 * little-endian u32 with the decompressed size, then codes of 9 to 12 bits packed
 * LSB first. 0x100 resets the dictionary (the code that follows is a literal and
 * adds no entry), 0x101 ends the stream, entries are numbered from 0x102 and the
 * code width grows when the next entry number reaches 1 << width.
 *
 * Decodes the stream starting at src[offset]; returns false if it is truncated or
 * malformed or does not produce the announced size. Used for LZMAP, LZOBJBLK,
 * LZDNGBLK and the other *.lz? / library files.
 */
bool lzwDecompress(const Bytes &src, size_t offset, Bytes &out);

} // namespace Ultima6

#endif

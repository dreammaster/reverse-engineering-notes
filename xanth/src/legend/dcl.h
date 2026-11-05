/* PKWARE DCL "implode" stream decoder (the algorithm of Mark Adler's blast.c). Used for the pixel data of Xanth's .PIC files.
 *
 * ScummVM port: common/dcl.h (Common::decompressDCL) does the same job and can replace this file.
 */
#ifndef LEGEND_DCL_H
#define LEGEND_DCL_H

#include "common/types.h"

namespace Legend {

/** Decodes a DCL stream. Returns false on corrupt or truncated input. `consumed` (optional) receives the number of input bytes used. */
bool dclExplode(const byte *data, size_t size, Common::Bytes &out, size_t *consumed = nullptr);

} // namespace Legend

#endif

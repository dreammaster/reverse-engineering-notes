#pragma once
#include "fod/bytes.h"

namespace fod {

// LZSS as used by every compressed FoD resource (KEH lzssDecode): u32 size, then flag-byte groups,
// 4096-byte ring pre-filled with 0x20, start position 0xFEE, 12-bit offset / 4-bit length+3.
Bytes lzssDecode(const uint8_t *src, size_t srcSize);
inline Bytes lzssDecode(const Bytes &b) { return lzssDecode(b.data(), b.size()); }

} // namespace fod

/* Little-endian field access on raw byte buffers (the on-disk and in-memory
 * format of the original DOS game). Never cast a struct over file bytes. */

#ifndef YENDOR1_SHARED_BYTES_H
#define YENDOR1_SHARED_BYTES_H

#include "shared/Types.h"

namespace Yendor1 {
namespace Shared {

inline uint16 readLE16(const byte *p) {
	return (uint16)(p[0] | (p[1] << 8));
}

inline int16 readLE16s(const byte *p) {
	return (int16)readLE16(p);
}

inline uint32 readLE32(const byte *p) {
	return (uint32)p[0] | ((uint32)p[1] << 8) | ((uint32)p[2] << 16) | ((uint32)p[3] << 24);
}

inline void writeLE16(byte *p, uint16 v) {
	p[0] = (byte)(v & 0xFF);
	p[1] = (byte)(v >> 8);
}

inline void writeLE32(byte *p, uint32 v) {
	p[0] = (byte)(v & 0xFF);
	p[1] = (byte)((v >> 8) & 0xFF);
	p[2] = (byte)((v >> 16) & 0xFF);
	p[3] = (byte)(v >> 24);
}

} // namespace Shared
} // namespace Yendor1

#endif

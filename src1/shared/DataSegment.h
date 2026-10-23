/* Shared::DataSegment -- the 64 KB data segment of one of the original programs, for the programs whose state lives in tables and
 * records that are addressed by offset all over the code (the character screens, the main engine). It starts as the initial contents
 * of the program's data segment, read from the executable at run time; the port's code reads and writes it with the original's offsets,
 * which are named where their meaning is known. */

#ifndef YENDOR1_SHARED_DATASEGMENT_H
#define YENDOR1_SHARED_DATASEGMENT_H

#include "data/ExeImage.h"
#include "shared/Types.h"

#include <string.h>

namespace Yendor1 {
namespace Shared {

class DataSegment {
public:
	DataSegment() { memset(_m, 0, sizeof(_m)); }

	/** Load the initial data; false when the file is not there. */
	bool load(const Data::ExeImage &exe) { return exe.copyDataSegment(_m, sizeof(_m)) != 0; }

	byte &b(uint16 off) { return _m[off]; }
	byte b(uint16 off) const { return _m[off]; }
	uint16 w(uint16 off) const { return (uint16)(_m[off] | (_m[(uint16)(off + 1)] << 8)); }
	void setW(uint16 off, uint16 v) { _m[off] = (byte)v; _m[(uint16)(off + 1)] = (byte)(v >> 8); }
	void addW(uint16 off, uint16 v) { setW(off, (uint16)(w(off) + v)); }
	byte *p(uint16 off) { return &_m[off]; }
	const byte *p(uint16 off) const { return &_m[off]; }
	/** The NUL-terminated string at `off`. */
	const char *str(uint16 off) const { return (const char *)&_m[off]; }
	void fill(uint16 off, byte v, uint16 count) { for (uint16 i = 0; i < count; i++) _m[(uint16)(off + i)] = v; }
	void copy(uint16 dst, uint16 src, uint16 count) { for (uint16 i = 0; i < count; i++) _m[(uint16)(dst + i)] = _m[(uint16)(src + i)]; }

private:
	byte _m[65536];
};

} // namespace Shared
} // namespace Yendor1

#endif

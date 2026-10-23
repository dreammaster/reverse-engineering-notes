/* Read-only access to the load image of one of the game's MZ executables, for text and tables that live in an exe's data
 * segment. The reimplementation reads such strings from the original files at run time instead of copying them into source.
 * Offsets are data-segment offsets (the DS-relative addresses the disassembly shows); `dataSegmentImageOffset` is where that
 * segment starts in the load image (documented per program in docs1). */

#ifndef YENDOR1_DATA_EXEIMAGE_H
#define YENDOR1_DATA_EXEIMAGE_H

#include "shared/Blob.h"
#include "shared/Types.h"

namespace Yendor1 {
namespace Data {

using Shared::byte;

class ExeImage {
public:
	ExeImage() : _imageStart(0), _dataBase(0) {}

	/** Load an MZ file; returns false if it is not one. */
	bool load(const char *path, uint32_t dataSegmentImageOffset);
	/** NUL-terminated string at a data-segment offset, or 0 if out of range. */
	const char *stringAt(uint32_t dsOffset) const;
	const byte *bytesAt(uint32_t dsOffset, uint32_t length) const;
	/** Copy the initial contents of the data segment (from its start to the end of the load image) to `dst`; returns the number of bytes. */
	uint32_t copyDataSegment(byte *dst, uint32_t capacity) const;

private:
	Shared::Blob _file;
	uint32_t _imageStart;    // file offset of the load image (header size)
	uint32_t _dataBase;      // image offset of the data segment
};

} // namespace Data
} // namespace Yendor1

#endif

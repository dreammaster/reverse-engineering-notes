#include "data/Palette.h"

#include <stdio.h>

namespace Yendor1 {
namespace Data {

Rgb egaToRgb(byte reg) {
	const int r = ((reg >> 2) & 1) * 2 + ((reg >> 5) & 1);
	const int g = ((reg >> 1) & 1) * 2 + ((reg >> 4) & 1);
	const int b = (reg & 1) * 2 + ((reg >> 3) & 1);
	Rgb c;
	c.r = (byte)(r * 85);
	c.g = (byte)(g * 85);
	c.b = (byte)(b * 85);
	return c;
}

bool PaletteFile::load(const char *gameDir, int fileNumber) {
	if (fileNumber < 0 || fileNumber > 6)
		return false;
	char name[16], path[512];
	snprintf(name, sizeof(name), "FILE%02d.PAL", fileNumber);
	if (!Shared::joinPath(path, sizeof(path), gameDir, name))
		return false;
	if (!_blob.load(path))
		return false;
	countRecords();
	return true;
}

void PaletteFile::assign(const byte *data, size_t size) {
	_blob.assign(data, size);
	countRecords();
}

void PaletteFile::countRecords() {
	const size_t whole = _blob.size() / kPaletteRecordBytes;
	_count = 0;
	for (size_t r = 0; r < whole; r++) {
		const byte *rec = _blob.data() + r * kPaletteRecordBytes;
		bool ok = rec[kPaletteColors] == 0;
		for (int c = 0; ok && c < kPaletteColors; c++)
			ok = rec[c] <= 0x3F;
		if (!ok)
			break;
		_count++;
	}
}

const byte *PaletteFile::record(size_t index) const {
	if (index >= count())
		return 0;
	return _blob.data() + index * kPaletteRecordBytes;
}

} // namespace Data
} // namespace Yendor1

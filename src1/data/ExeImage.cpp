#include "data/ExeImage.h"

namespace Yendor1 {
namespace Data {

bool ExeImage::load(const char *path, uint32_t dataSegmentImageOffset) {
	if (!_file.load(path) || _file.size() < 0x1C || _file.data()[0] != 'M' || _file.data()[1] != 'Z')
		return false;
	const byte *h = _file.data();
	_imageStart = (uint32_t)(h[8] | (h[9] << 8)) * 16;
	_dataBase = dataSegmentImageOffset;
	return _imageStart < _file.size();
}

const byte *ExeImage::bytesAt(uint32_t dsOffset, uint32_t length) const {
	const uint32_t pos = _imageStart + _dataBase + dsOffset;
	if (pos + length > _file.size())
		return 0;
	return _file.data() + pos;
}

const char *ExeImage::stringAt(uint32_t dsOffset) const {
	const uint32_t pos = _imageStart + _dataBase + dsOffset;
	if (pos >= _file.size())
		return 0;
	for (uint32_t i = pos; i < _file.size(); i++)
		if (_file.data()[i] == 0)
			return (const char *)(_file.data() + pos);
	return 0;
}

} // namespace Data
} // namespace Yendor1

#include "files/lib_n.h"

namespace Ultima6 {

static uint32_t get32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool LibN::load(const Bytes &data, bool hasSizeHeader) {
	_items.clear();
	_data = data;
	size_t tableStart = hasSizeHeader ? 4 : 0;
	if (data.size() < tableStart + 4)
		return false;

	// The table ends at the lowest non-zero offset.
	size_t count = (size_t)-1;
	for (size_t i = 0; tableStart + i * 4 + 4 <= data.size() && i < count; ++i) {
		uint32_t offset = get32(&data[tableStart + i * 4]) & 0xFFFFFF;
		if (offset != 0 && (offset - tableStart) / 4 < count)
			count = (offset - tableStart) / 4;
	}
	if (count == (size_t)-1 || tableStart + count * 4 > data.size())
		return false;

	_items.resize(count);
	for (size_t i = 0; i < count; ++i) {
		uint32_t v = get32(&data[tableStart + i * 4]);
		_items[i].offset = v & 0xFFFFFF;
		_items[i].flag = (uint8_t)(v >> 24);
		_items[i].size = 0;
	}
	for (size_t i = 0; i < count; ++i) {
		if (!_items[i].offset)
			continue;
		uint32_t next = (uint32_t)data.size();
		for (size_t j = i + 1; j < count; ++j) {
			if (_items[j].offset) {
				next = _items[j].offset;
				break;
			}
		}
		if (next > _items[i].offset && next <= data.size())
			_items[i].size = next - _items[i].offset;
	}
	return true;
}

const uint8_t *LibN::item(size_t i, size_t &size) const {
	if (!present(i) || _items[i].size == 0) {
		size = 0;
		return nullptr;
	}
	size = _items[i].size;
	return &_data[_items[i].offset];
}

} // namespace Ultima6

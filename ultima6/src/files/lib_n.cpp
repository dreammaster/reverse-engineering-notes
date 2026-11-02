#include "files/lib_n.h"

#include "files/lzw.h"

namespace Ultima6 {

static uint32_t get32(const uint8_t *p) {
	return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24);
}

bool LibN::load(const Bytes &data, bool hasSizeHeader, int entrySize) {
	_items.clear();
	_data = data;
	size_t tableStart = hasSizeHeader ? 4 : 0;
	if ((entrySize != 2 && entrySize != 4) || data.size() < tableStart + entrySize)
		return false;

	// The table ends at the lowest non-zero offset.
	size_t count = (size_t)-1;
	for (size_t i = 0; tableStart + (i + 1) * entrySize <= data.size() && i < count; ++i) {
		uint32_t offset = entrySize == 2 ? get16(&data[tableStart + i * 2]) : get32(&data[tableStart + i * 4]) & 0xFFFFFF;
		if (offset != 0 && (offset - tableStart) / entrySize < count)
			count = (offset - tableStart) / entrySize;
	}
	if (count == (size_t)-1 || tableStart + count * entrySize > data.size())
		return false;

	_items.resize(count);
	for (size_t i = 0; i < count; ++i) {
		uint32_t v = entrySize == 2 ? get16(&data[tableStart + i * 2]) : get32(&data[tableStart + i * 4]);
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

bool LibN::get(size_t i, Bytes &out) const {
	size_t size = 0;
	const uint8_t *p = item(i, size);
	if (!p)
		return false;
	uint8_t f = _items[i].flag;
	for (size_t j = i; f == 0xFF && j < _items.size(); ++j)
		if (_items[j].flag != 0xFF)
			f = _items[j].flag;
	if (f == 0x01 || f == 0x20)
		return lzwDecompress(_data, _items[i].offset, out);
	if (f == 0xC1) { // stored, with the item's own size in front
		if (size < 4)
			return false;
		out.assign(p + 4, p + size);
		return true;
	}
	out.assign(p, p + size);
	return true;
}

} // namespace Ultima6

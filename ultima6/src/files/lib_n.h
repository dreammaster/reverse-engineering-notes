#ifndef ULTIMA6_FILES_LIB_N_H
#define ULTIMA6_FILES_LIB_N_H

#include "files/data_file.h"

namespace Ultima6 {

/**
 * The offset-table archive used by PORTRAIT.*, CONVERSE.* and, inside the LZW stream
 * of the *.SHP files, by the graphics: an array of little-endian u32 (low 24 bits the
 * offset of the item in the file, high byte a flag), then the items. The table runs up
 * to the first non-zero offset, so the item count is that offset / 4 (an item with
 * offset 0 is absent). An item ends where the next present one starts.
 *
 * Inside .SHP files the archive starts with one extra u32, the archive's own size,
 * before the table (hasSizeHeader = true).
 *
 * The flag byte is 0 in every Ultima VI archive checked; the compressed-item flags
 * (1 and 0x20) belong to the spin-offs' *.LZC files and are not decoded here.
 */
class LibN {
public:
	bool load(const Bytes &data, bool hasSizeHeader);

	size_t count() const {
		return _items.size();
	}
	bool present(size_t i) const {
		return i < _items.size() && _items[i].offset != 0;
	}
	uint8_t flag(size_t i) const {
		return _items[i].flag;
	}
	/** Pointer to and size of an item's bytes (nullptr / 0 for an absent item). */
	const uint8_t *item(size_t i, size_t &size) const;

private:
	struct Item {
		uint32_t offset;
		uint32_t size;
		uint8_t flag;
	};
	Bytes _data;
	std::vector<Item> _items;
};

} // namespace Ultima6

#endif

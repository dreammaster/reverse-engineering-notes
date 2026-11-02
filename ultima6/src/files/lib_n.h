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
	/** entrySize is 4 for most archives and 2 for BOOK.DAT (u16 offsets, no flag byte). */
	bool load(const Bytes &data, bool hasSizeHeader, int entrySize = 4);

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
	/**
	 * The item as the spin-offs' *.LZC / *.VGA files hold it: flags 1 and 0x20 mean an LZW stream (u32 decompressed size first, then the codes, as in Ultima VI's
	 * LZ files), 0xFF shares the flag of the next item that has another, 0xC1 is stored plain behind a u32 holding the item's own size, and everything else (0, 2, 0xE0) is stored plain.
	 */
	bool get(size_t i, Bytes &out) const;

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

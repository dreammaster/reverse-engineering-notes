#pragma once
#include "fod/bytes.h"

namespace fod {

// A `*MAP`/`*SCR`/`*ANI` file: table of {u16 offset (16-byte paragraphs), u16 size (bytes)}.
class Archive {
public:
	Archive(const std::string &path, size_t entries);
	size_t entries() const { return _toc.size(); }
	bool has(size_t idx) const { return idx < _toc.size() && _toc[idx].size != 0; }
	Bytes chunk(size_t idx) const;             // raw bytes
	Bytes chunkLzss(size_t idx) const;         // LZSS-decoded
private:
	struct Entry { uint32_t offset; uint32_t size; };
	Bytes _data;
	std::vector<Entry> _toc;
};

// Which disk (file set) each map number lives on, and the file-name prefix for that set.
const char *mapFilePrefix(int mapNo);            // "k", "f" or "d" (lower case; caller adds map/scr/ani)
int mapDisk(int mapNo);

} // namespace fod

#include "fod/archive.h"
#include "fod/lzss.h"
#include <fstream>

namespace fod {

Bytes readFile(const std::string &path) {
	std::ifstream f(path, std::ios::binary);
	if (!f) throw std::runtime_error("cannot open " + path);
	return Bytes((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

Archive::Archive(const std::string &path, size_t entries) : _data(readFile(path)) {
	Reader r(_data);
	for (size_t i = 0; i < entries; ++i) {
		Entry e;
		e.offset = uint32_t(r.u16()) * 16;
		e.size = r.u16();
		_toc.push_back(e);
	}
}

Bytes Archive::chunk(size_t idx) const {
	if (!has(idx)) throw std::runtime_error("empty archive entry");
	const Entry &e = _toc[idx];
	if (e.offset + e.size > _data.size()) throw std::runtime_error("archive entry out of range");
	return Bytes(_data.begin() + e.offset, _data.begin() + e.offset + e.size);
}

Bytes Archive::chunkLzss(size_t idx) const {
	Bytes raw = chunk(idx);
	return lzssDecode(raw);
}

// KEH dseg:0125h, indexed by 1-based map number. Disk set 1 = K files, 2 = F files, 3 = D files;
// the archive chunk index is mapNo - 1.
static const uint8_t MAP_DISK[29] = {0, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 2, 3, 2,
                                     3, 3, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2};
int mapDisk(int mapNo) { return mapNo >= 0 && mapNo < 29 ? MAP_DISK[mapNo] : 0; }

const char *mapFilePrefix(int mapNo) {
	switch (mapDisk(mapNo)) {
	case 2: return "f";
	case 3: return "d";
	default: return "k";
	}
}

} // namespace fod

#include "legend/rgn.h"

#include <algorithm>

#include "common/stream.h"

namespace Legend {

bool RgnFile::load(const Common::Bytes &data) {
	_scenes.clear();
	Common::ReadStream s(data);
	size_t limit = data.size();                  // the directory ends where the first region data starts
	for (size_t i = 0; i * 6 + 6 <= limit; i++) {
		s.seek(i * 6);
		uint16 off = s.readUint16LE();
		Scene sc;
		sc.baseId = s.readUint16LE();
		uint16 count = s.readUint16LE();
		if (off) {
			limit = std::min<size_t>(limit, off);
			s.seek(off);
			for (unsigned r = 0; r < count; r++) {
				Region rg;
				rg.id = s.readSint16LE();
				int16 type = s.readSint16LE();
				rg.rect = type == 2;
				int n = type == 2 ? 4 : 2 * type;
				if (n < 0 || n > 512)
					return false;
				for (int k = 0; k < n; k++)
					rg.pts.push_back(s.readSint16LE());
				sc.regions.push_back(rg);
			}
		}
		_scenes.push_back(sc);
	}
	return !s.err() && !_scenes.empty();
}

bool RgnFile::Region::contains(int x, int y) const {
	if (rect) {
		int x1 = std::min(pts[0], pts[2]), x2 = std::max(pts[0], pts[2]);
		int y1 = std::min(pts[1], pts[3]), y2 = std::max(pts[1], pts[3]);
		return x >= x1 && x <= x2 && y >= y1 && y <= y2;
	}
	bool in = false;
	size_t n = pts.size() / 2;
	for (size_t i = 0, j = n - 1; i < n; j = i++) {
		int xi = pts[2 * i], yi = pts[2 * i + 1], xj = pts[2 * j], yj = pts[2 * j + 1];
		if (((yi > y) != (yj > y)) && x < (xj - xi) * (y - yi) / (double)(yj - yi) + xi)
			in = !in;
	}
	return in;
}

const RgnFile::Region *RgnFile::hit(size_t i, int x, int y) const {
	if (i >= _scenes.size())
		return nullptr;
	for (size_t r = 0; r < _scenes[i].regions.size(); r++)
		if (_scenes[i].regions[r].contains(x, y))
			return &_scenes[i].regions[r];
	return nullptr;
}

} // namespace Legend

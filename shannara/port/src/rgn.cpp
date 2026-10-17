#include "rgn.h"
#include <algorithm>

namespace shannara {

RgnFile::RgnFile(const Bytes &d) {
	std::vector<uint16_t> offs;
	size_t limit = d.size();
	for (size_t i = 0; i * 6 + 6 <= limit; i++) {
		Cursor c(d, i * 6);
		uint16_t off = c.u16();
		RgnEntry e;
		e.offset = off;
		e.baseId = c.u16();
		e.count = c.u16();
		_entries.push_back(e);
		offs.push_back(off);
		if (off)
			limit = std::min<size_t>(limit, off);
	}
	for (size_t i = 0; i < _entries.size(); i++) {
		if (!offs[i])
			continue;
		Cursor c(d, offs[i]);
		for (int r = 0; r < _entries[i].count; r++) {
			Region g;
			g.id = c.s16();
			int type = c.s16();
			if (type == 2) {
				g.rect = true;
				for (int k = 0; k < 4; k++)
					g.pts.push_back(c.s16());
			} else {
				if (type < 0)
					throw FormatError("bad region type");
				for (int k = 0; k < 2 * type; k++)
					g.pts.push_back(c.s16());
			}
			_entries[i].regions.push_back(g);
		}
	}
}

} // namespace shannara

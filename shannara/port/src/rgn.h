// SHANxxx.RGN picture hotspot files (verified on all 12 retail files).
//   directory: 6-byte entries {u16 dataOffset (0 = none), u16 baseId, u16 count}; ends at the smallest non-zero offset
//   at dataOffset: count regions {s16 id, s16 type, ...}: type 2 = rectangle {x1,y1,x2,y2}, otherwise a polygon with `type` {x,y} vertices
#pragma once
#include "common.h"

namespace shannara {

struct Region {
	int id = 0;
	bool rect = false;
	std::vector<int16_t> pts; // rect: x1 y1 x2 y2; polygon: x0 y0 x1 y1 ...
};

struct RgnEntry {
	uint16_t offset = 0, baseId = 0, count = 0;
	std::vector<Region> regions;
};

class RgnFile {
public:
	explicit RgnFile(const Bytes &data);
	const std::vector<RgnEntry> &entries() const { return _entries; }
private:
	std::vector<RgnEntry> _entries;
};

} // namespace shannara

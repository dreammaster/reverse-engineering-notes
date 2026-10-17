#include "le.h"
#include <cstring>

namespace shannara {

LeExe::LeExe(Bytes file) : _d(std::move(file)) {
	// find the LE header: an MZ stub whose e_lfanew leads to "LE\0\0" (bound DOS/4GW executables embed one after the loader stub)
	size_t stub = (size_t)-1, hdr = 0;
	for (size_t o = 0; o + 0x40 < _d.size(); o++) {
		if (_d[o] != 'M' || _d[o + 1] != 'Z')
			continue;
		Cursor c(_d, o + 0x3C);
		size_t h = o + c.u32();
		if (h + 0x84 < _d.size() && _d[h] == 'L' && _d[h + 1] == 'E' && _d[h + 2] == 0 && _d[h + 3] == 0) {
			stub = o;
			hdr = h;
			break;
		}
	}
	if (stub == (size_t)-1)
		throw FormatError("no LE header found");
	auto u32at = [&](size_t off) { Cursor c(_d, off); return c.u32(); };
	uint32_t pages = u32at(hdr + 0x14);
	_pageSize = u32at(hdr + 0x28);
	_eip = u32at(hdr + 0x1C);
	uint32_t objTab = u32at(hdr + 0x40), nObj = u32at(hdr + 0x44), pageMap = u32at(hdr + 0x48);
	_dataPages = stub + u32at(hdr + 0x80);
	for (uint32_t i = 0; i < nObj; i++) {
		Cursor c(_d, hdr + objTab + 24 * i);
		Object o;
		o.size = c.u32();
		o.base = c.u32();
		c.u32(); // flags
		o.firstPage = c.u32();
		o.pages = c.u32();
		_objects.push_back(o);
	}
	for (uint32_t i = 0; i < pages; i++) {
		uint32_t e = u32at(hdr + pageMap + 4 * i);
		_pageMap.push_back(((e & 0xFF) << 16) | (((e >> 8) & 0xFF) << 8) | ((e >> 16) & 0xFF)); // big-endian 24-bit page number
	}
}

void LeExe::read(uint32_t addr, void *dst, size_t n) const {
	uint8_t *out = (uint8_t *)dst;
	for (size_t k = 0; k < n; k++) {
		uint32_t a = addr + (uint32_t)k;
		bool found = false;
		for (const Object &o : _objects) {
			if (a < o.base || a >= o.base + o.pages * _pageSize)
				continue;
			uint32_t rel = a - o.base;
			uint32_t logical = o.firstPage - 1 + rel / _pageSize;
			if (logical >= _pageMap.size())
				throw FormatError("page index outside the page map");
			size_t off = _dataPages + (size_t)(_pageMap[logical] - 1) * _pageSize + rel % _pageSize;
			if (off >= _d.size())
				throw FormatError("page data past end of file");
			out[k] = _d[off];
			found = true;
			break;
		}
		if (!found)
			throw FormatError("address outside the executable's objects");
	}
}

} // namespace shannara

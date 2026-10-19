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
	_pages = pages;
	_hdr = hdr;
	_fixupPageTab = hdr + u32at(hdr + 0x68);
	_fixupRecTab = hdr + u32at(hdr + 0x6C);
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

void LeExe::mapInto(uint8_t *mem, size_t memSize, uint32_t delta) const {
	for (const Object &o : _objects) {
		for (uint32_t p = 0; p < o.pages; p++) {
			uint32_t logical = o.firstPage - 1 + p;
			size_t off = _dataPages + (size_t)(_pageMap[logical] - 1) * _pageSize;
			uint32_t dst = o.base + delta + p * _pageSize;
			for (uint32_t k = 0; k < _pageSize; k++) {
				if (dst + k >= memSize)
					throw FormatError("image does not fit in memory");
				mem[dst + k] = off + k < _d.size() ? _d[off + k] : 0;
			}
		}
	}
	// internal-reference fixups, page by page
	for (const Object &o : _objects) {
		for (uint32_t p = 0; p < o.pages; p++) {
			uint32_t logical = o.firstPage - 1 + p;
			Cursor a(_d, _fixupPageTab + 4 * logical), b(_d, _fixupPageTab + 4 * (logical + 1));
			size_t pos = _fixupRecTab + a.u32(), end = _fixupRecTab + b.u32();
			uint32_t pageAddr = o.base + delta + p * _pageSize;
			Cursor c(_d, pos);
			while (c.pos() < end) {
				uint8_t src = c.u8(), flags = c.u8();
				bool list = (src & 0x20) != 0;
				int count = 0;
				int16_t single = 0;
				if (list)
					count = c.u8();
				else
					single = c.s16();
				if (flags & 3)
					throw FormatError("unsupported fixup target kind");
				int obj = (flags & 0x40) ? c.u16() : c.u8();
				int kind = src & 0xF;
				uint32_t tgt = 0;
				if (kind != 2)
					tgt = (flags & 0x10) ? c.u32() : c.u16();
				if (flags & 4)
					tgt += (flags & 0x20) ? c.u32() : c.u16();
				std::vector<int16_t> offs;
				if (list) {
					for (int i = 0; i < count; i++)
						offs.push_back(c.s16());
				} else {
					offs.push_back(single);
				}
				if (obj < 1 || obj > (int)_objects.size())
					throw FormatError("fixup target object out of range");
				uint32_t value = _objects[obj - 1].base + delta + tgt;
				for (int16_t so : offs) {
					int64_t at = (int64_t)pageAddr + so;
					if (at < 0 || at + 4 > (int64_t)memSize)
						continue;
					if (kind == 7) {
						for (int k = 0; k < 4; k++)
							mem[at + k] = (value >> (8 * k)) & 0xFF;
					} else if (kind == 8) {
						uint32_t v = value - (pageAddr + so + 4);
						for (int k = 0; k < 4; k++)
							mem[at + k] = (v >> (8 * k)) & 0xFF;
					} else if (kind == 5) {
						mem[at] = value & 0xFF;
						mem[at + 1] = (value >> 8) & 0xFF;
					}
				}
			}
		}
	}
}

void LeExe::read(uint32_t addr, void *dst, size_t n) const {
	uint8_t *out = (uint8_t *)dst;
	for (size_t k = 0; k < n; k++) {
		uint32_t a = addr + (uint32_t)k;
		bool found = false;
		for (const Object &o : _objects) {
			if (a < o.base || a >= o.base + (o.size > o.pages * _pageSize ? o.size : o.pages * _pageSize))
				continue;
			uint32_t rel = a - o.base;
			if (rel >= o.pages * _pageSize) { // virtual size beyond the file-backed pages: zero-initialised data (BSS)
				out[k] = 0;
				found = true;
				break;
			}
			uint32_t logical = o.firstPage - 1 + rel / _pageSize;
			if (logical >= _pageMap.size())
				throw FormatError("page index outside the page map");
			size_t off = _dataPages + (size_t)(_pageMap[logical] - 1) * _pageSize + rel % _pageSize;
			out[k] = off < _d.size() ? _d[off] : 0; // the final page is only partly stored; the rest is zero-filled
			found = true;
			break;
		}
		if (!found)
			throw FormatError("address outside the executable's objects");
	}
}

} // namespace shannara

// Shared helpers for the Shannara resource readers (little-endian, bounds-checked).
#pragma once
#include <cstdint>
#include <cstdio>
#include <stdexcept>
#include <string>
#include <vector>

namespace shannara {

using Bytes = std::vector<uint8_t>;

struct FormatError : std::runtime_error {
	explicit FormatError(const std::string &m) : std::runtime_error(m) {}
};

inline Bytes readFile(const std::string &path) {
	FILE *f = fopen(path.c_str(), "rb");
	if (!f)
		throw FormatError("cannot open " + path);
	Bytes b;
	uint8_t buf[65536];
	size_t n;
	while ((n = fread(buf, 1, sizeof buf, f)) > 0)
		b.insert(b.end(), buf, buf + n);
	fclose(f);
	return b;
}

// Bounds-checked little-endian reads from a byte buffer.
class Cursor {
public:
	Cursor(const uint8_t *d, size_t n, size_t pos = 0) : _d(d), _n(n), _p(pos) {}
	explicit Cursor(const Bytes &b, size_t pos = 0) : _d(b.data()), _n(b.size()), _p(pos) {}
	size_t pos() const { return _p; }
	size_t size() const { return _n; }
	void seek(size_t p) { _p = p; }
	void skip(size_t n) { need(n); _p += n; }
	uint8_t u8() { need(1); return _d[_p++]; }
	uint16_t u16() { need(2); uint16_t v = _d[_p] | (_d[_p + 1] << 8); _p += 2; return v; }
	int16_t s16() { return (int16_t)u16(); }
	uint32_t u32() { uint32_t lo = u16(); return lo | ((uint32_t)u16() << 16); }
	const uint8_t *ptr() const { return _d + _p; }
	size_t left() const { return _n - _p; }
private:
	void need(size_t n) const {
		if (_p + n > _n)
			throw FormatError("read past end of data");
	}
	const uint8_t *_d;
	size_t _n, _p;
};

} // namespace shannara

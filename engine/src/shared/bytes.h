#pragma once
// Little-endian byte helpers: file reading and a bounds checked reader (shared by both games).
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <string>
#include <vector>

namespace shared {

using Bytes = std::vector<uint8_t>;

Bytes readFile(const std::string &path);

class Reader {
public:
	Reader(const uint8_t *data, size_t size, size_t pos = 0) : _d(data), _n(size), _p(pos) {}
	explicit Reader(const Bytes &b, size_t pos = 0) : Reader(b.data(), b.size(), pos) {}
	size_t pos() const { return _p; }
	void seek(size_t p) { _p = p; }
	size_t size() const { return _n; }
	bool eof() const { return _p >= _n; }
	uint8_t u8() { need(1); return _d[_p++]; }
	int8_t s8() { return int8_t(u8()); }
	uint16_t u16() { need(2); uint16_t v = _d[_p] | (_d[_p + 1] << 8); _p += 2; return v; }
	int16_t s16() { return int16_t(u16()); }
	uint32_t u32() { uint32_t lo = u16(); return lo | (uint32_t(u16()) << 16); }
	void bytes(void *dst, size_t n) { need(n); std::memcpy(dst, _d + _p, n); _p += n; }
	std::string cstr(size_t fixed) {
		need(fixed);
		std::string s(reinterpret_cast<const char *>(_d + _p), fixed);
		_p += fixed;
		s.resize(std::strlen(s.c_str()));
		return s;
	}
private:
	void need(size_t n) const { if (_p + n > _n) throw std::runtime_error("read past end of data"); }
	const uint8_t *_d;
	size_t _n, _p;
};

} // namespace shared

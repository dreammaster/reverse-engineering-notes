/* A read-only little-endian byte stream over memory, with sticky error reporting instead of exceptions.
 *
 * ScummVM port: replace with Common::SeekableReadStream / Common::MemoryReadStream (same readUint16LE() etc.; use err() / eos()).
 */
#ifndef COMMON_STREAM_H
#define COMMON_STREAM_H

#include "common/types.h"

namespace Common {

class ReadStream {
public:
	ReadStream() : _data(nullptr), _size(0), _pos(0), _err(false) {}
	ReadStream(const byte *data, size_t size) : _data(data), _size(size), _pos(0), _err(false) {}
	explicit ReadStream(const Bytes &b) : _data(b.empty() ? nullptr : &b[0]), _size(b.size()), _pos(0), _err(false) {}

	size_t size() const { return _size; }
	size_t pos() const { return _pos; }
	size_t remaining() const { return _size - _pos; }
	bool eos() const { return _pos >= _size; }
	/** True once a read went past the end (reads then return 0); stays set. */
	bool err() const { return _err; }
	const byte *data() const { return _data; }

	void seek(size_t p) {
		if (p > _size) {
			_err = true;
			_pos = _size;
		} else {
			_pos = p;
		}
	}
	void skip(size_t n) { seek(_pos + n); }

	byte readByte() { return need(1) ? _data[_pos++] : 0; }
	uint16 readUint16LE() {
		if (!need(2))
			return 0;
		uint16 v = (uint16)(_data[_pos] | (_data[_pos + 1] << 8));
		_pos += 2;
		return v;
	}
	int16 readSint16LE() { return (int16)readUint16LE(); }
	uint16 readUint16BE() {
		if (!need(2))
			return 0;
		uint16 v = (uint16)((_data[_pos] << 8) | _data[_pos + 1]);
		_pos += 2;
		return v;
	}
	uint32 readUint32LE() {
		if (!need(4))
			return 0;
		uint32 v = (uint32)_data[_pos] | ((uint32)_data[_pos + 1] << 8) | ((uint32)_data[_pos + 2] << 16) | ((uint32)_data[_pos + 3] << 24);
		_pos += 4;
		return v;
	}
	uint32 readUint32BE() {
		if (!need(4))
			return 0;
		uint32 v = ((uint32)_data[_pos] << 24) | ((uint32)_data[_pos + 1] << 16) | ((uint32)_data[_pos + 2] << 8) | (uint32)_data[_pos + 3];
		_pos += 4;
		return v;
	}
	/** Copies n bytes; returns false (and sets err()) if fewer are left. */
	bool read(void *dst, size_t n) {
		if (!need(n))
			return false;
		memcpy(dst, _data + _pos, n);
		_pos += n;
		return true;
	}

private:
	bool need(size_t n) {
		if (_err || _pos + n > _size) {
			_err = true;
			return false;
		}
		return true;
	}

	const byte *_data;
	size_t _size, _pos;
	bool _err;
};

} // namespace Common

#endif

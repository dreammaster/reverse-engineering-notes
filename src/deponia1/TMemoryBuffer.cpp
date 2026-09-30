#include "TMemoryBuffer.h"

#include <cstdlib>
#include <cstring>
#include <string>

#include "MD5.h"

TMemoryBuffer::~TMemoryBuffer() {
	delete[] _data;
}

void TMemoryBuffer::ReleaseMemory() {
	delete[] _data;
	_data = nullptr;
	_len = 0;
	_capacity = 0;
}

void TMemoryBuffer::ClearMemory() {
	_len = 0;
}

void TMemoryBuffer::Reserve(unsigned long size) {
	if (_capacity < size || !_data) {
		delete[] _data;
		_data = new unsigned char[size];
		_capacity = size;
		_len = 0;
	}
}

void TMemoryBuffer::EnsureBufferSize(unsigned long size) {
	if (_capacity < size) {
		unsigned char *newData = new unsigned char[size];
		if (_data && _len != 0)
			std::memcpy(newData, _data, _len);
		delete[] _data;
		_data = newData;
		_capacity = size;
	} else if (!_data) {
		_data = new unsigned char[size];
		_capacity = size;
	}
}

void TMemoryBuffer::Init(unsigned long size) {
	if (_capacity < size || !_data) {
		delete[] _data;
		_data = new unsigned char[size];
		_capacity = size;
	}
	_len = 0;
}

void TMemoryBuffer::growForAppend(unsigned long extraBytes) {
	unsigned long newLen = _len + extraBytes;
	if (newLen > _capacity || !_data) {
		unsigned long newCapacity = newLen + 0x400000;
		unsigned char *newData = new unsigned char[newCapacity];
		if (_data && _len != 0)
			std::memcpy(newData, _data, _len);
		delete[] _data;
		_data = newData;
		_capacity = newCapacity;
	}
}

void TMemoryBuffer::AppendData(const void *data, unsigned long size) {
	growForAppend(size);
	if (size != 0)
		std::memcpy(_data + _len, data, size);
	_len += size;
}

void TMemoryBuffer::AppendByte(unsigned char value) {
	growForAppend(1);
	_data[_len] = value;
	_len += 1;
}

TMemoryBuffer &TMemoryBuffer::operator<<(char value) {
	AppendByte(static_cast<unsigned char>(value));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(const char *value) {
	AppendData(value, static_cast<unsigned long>(std::strlen(value)));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(unsigned char value) {
	AppendByte(value);
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(short value) {
	AppendData(&value, sizeof(value));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(unsigned short value) {
	AppendData(&value, sizeof(value));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(long value) {
	AppendData(&value, sizeof(value));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(unsigned long value) {
	AppendData(&value, sizeof(value));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(const wxFileName &value) {
	std::string narrow(static_cast<const char *>(value.GetFullPath().mb_str()));
	AppendData(narrow.data(), static_cast<unsigned long>(narrow.size()));
	return *this;
}

TMemoryBuffer &TMemoryBuffer::operator<<(const wxString &value) {
	std::string narrow(static_cast<const char *>(value.mb_str()));
	AppendData(narrow.data(), static_cast<unsigned long>(narrow.size()));
	return *this;
}

void TMemoryBuffer::AppendStringWithLen(const wxString &value) {
	std::string narrow(static_cast<const char *>(value.mb_str()));
	*this << static_cast<short>(narrow.size());
	AppendData(narrow.data(), static_cast<unsigned long>(narrow.size()));
}

bool TMemoryBuffer::Uncompress(TMemoryBuffer &/*dest*/, long /*expectedSize*/) {
	// Not reversed beyond its real-zlib uncompress() call shape - see the
	// class header comment.
	return false;
}

bool TMemoryBuffer::Uncompress(long /*expectedSize*/) {
	return false;
}

bool TMemoryBuffer::Compress() {
	// Not reversed beyond its real-zlib compress() call shape - see the
	// class header comment.
	return false;
}

bool TMemoryBuffer::Decrypt(const wxString &key, unsigned long *cursor, unsigned long offset) {
	if (key.IsEmpty())
		return true;

	std::string narrowKey(static_cast<const char *>(key.mb_str()));
	unsigned char *digest = MD5String(narrowKey.c_str(), static_cast<unsigned long>(narrowKey.size()));

	unsigned char *target = _data + offset;
	unsigned long pos = cursor ? *cursor : 0;
	for (unsigned long i = 0; i < _len; i++) {
		if (pos == 16)
			pos = 0;
		target[i] ^= digest[pos];
		pos++;
	}
	if (cursor)
		*cursor = pos;

	std::free(digest);
	return true;
}

bool TMemoryBuffer::Encrypt(const wxString &key, unsigned long *cursor, unsigned long offset) {
	Decrypt(key, cursor, offset);
	return true;
}

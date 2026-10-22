#include "TCharHolder.h"

#include <cctype>
#include <cstring>

void TCharHolder::assignCString(const char *value) {
	delete[] _data;
	_data = nullptr;
	_size = 0;
	if (value && *value) {
		_size = std::strlen(value) + 1;
		_data = new char[_size];
		std::memcpy(_data, value, _size);
	}
}

TCharHolder::TCharHolder(const TCharHolder &other) {
	copy(other._data, static_cast<int>(other._size));
}

TCharHolder::TCharHolder(const wchar_t *value) {
	assignCString(static_cast<const char *>(wxString(value).mb_str()));
}

TCharHolder::TCharHolder(const char *value) {
	assignCString(value);
}

TCharHolder::TCharHolder(const char *first, const char *last) {
	Assign(first, last);
}

TCharHolder::TCharHolder(const wxFileName &value) {
	assignCString(static_cast<const char *>(value.GetFullPath().mb_str()));
}

TCharHolder::TCharHolder(const wxString &value) {
	assignCString(static_cast<const char *>(value.mb_str()));
}

TCharHolder::~TCharHolder() {
	delete[] _data;
}

void TCharHolder::copy(const char *value) {
	assignCString(value);
}

void TCharHolder::copy(const char *value, int length) {
	delete[] _data;
	_data = nullptr;
	_size = 0;
	if (value && *value && length != 0) {
		_size = static_cast<unsigned long>(length);
		_data = new char[_size];
		std::memcpy(_data, value, _size);
	}
}

void TCharHolder::resize(unsigned long size) {
	delete[] _data;
	_data = nullptr;
	_size = 0;
	if (size != 0) {
		_data = new char[size];
		_size = size;
	}
}

wxString TCharHolder::GetFullPath() const {
	return _data ? wxString(_data) : wxString();
}

wxString TCharHolder::GetFullPath(int /*format*/) const {
	return GetFullPath();
}

int TCharHolder::Cmp(const TCharHolder &other) const {
	if (!_data)
		return other._data ? -1 : 0;
	if (!other._data)
		return 1;
	int result = std::strcmp(_data, other._data);
	return (result > 0) - (result < 0);
}

int TCharHolder::CmpNoCase(const TCharHolder &other) const {
	if (!_data)
		return other._data ? -1 : 0;
	if (!other._data)
		return 1;
	const unsigned char *a = reinterpret_cast<const unsigned char *>(_data);
	const unsigned char *b = reinterpret_cast<const unsigned char *>(other._data);
	while (*a && *b) {
		int ca = std::tolower(*a);
		int cb = std::tolower(*b);
		if (ca != cb)
			return ca < cb ? -1 : 1;
		++a;
		++b;
	}
	if (*a)
		return 1;
	if (*b)
		return -1;
	return 0;
}

wxString TCharHolder::c_str() const {
	return GetFullPath();
}

const char *TCharHolder::mb_str() const {
	return _data ? _data : "";
}

wxString TCharHolder::Lower() const {
	wxString result = GetFullPath();
	result.MakeLower();
	return result;
}

bool TCharHolder::ToDouble(double *out) const {
	if (!_data)
		return false;
	return GetFullPath().ToDouble(out);
}

void TCharHolder::exchange(TCharHolder &other) {
	delete[] _data;
	_data = other._data;
	_size = other._size;
	other._data = nullptr;
	other._size = 0;
}

TCharHolder &TCharHolder::operator=(const char *value) {
	assignCString(value);
	return *this;
}

TCharHolder &TCharHolder::operator=(const wxFileName &value) {
	assignCString(static_cast<const char *>(value.GetFullPath().mb_str()));
	return *this;
}

TCharHolder &TCharHolder::operator=(const TCharHolder &other) {
	copy(other._data, static_cast<int>(other._size));
	return *this;
}

void TCharHolder::Assign(const char *first, const char *last) {
	delete[] _data;
	_data = nullptr;
	_size = 0;
	if (!first)
		return;
	long len = static_cast<long>(last - first);
	if (len <= 0)
		return;
	if (last[-1] != 0) {
		_size = static_cast<unsigned long>(len) + 1;
		_data = new char[_size];
		std::memcpy(_data, first, static_cast<std::size_t>(len));
		_data[len] = 0;
	} else {
		_size = static_cast<unsigned long>(len);
		_data = new char[_size];
		std::memcpy(_data, first, _size);
	}
}

void TCharHolder::Assign(const wxString &value) {
	assignCString(static_cast<const char *>(value.mb_str()));
}

bool TCharHolder::operator==(const TCharHolder &other) const {
	if (!_data)
		return !other._data;
	if (!other._data)
		return false;
	if (_size != other._size)
		return false;
	return std::strcmp(_data, other._data) == 0;
}

bool TCharHolder::operator==(const wxString &other) const {
	// Real wxCharBuffer::data() is never null, so the original's
	// null-converted-string branch is unreachable here in practice - a
	// null _data is thus never equal to a wxString, even an empty one.
	if (!_data)
		return false;
	return std::strcmp(_data, static_cast<const char *>(other.mb_str())) == 0;
}

bool TCharHolder::operator==(const wchar_t *other) const {
	if (!_data)
		return false;
	wxString converted(other);
	return std::strcmp(_data, static_cast<const char *>(converted.mb_str())) == 0;
}

bool TCharHolder::operator==(const char *other) const {
	if (!_data)
		return !other;
	if (!other)
		return false;
	return std::strcmp(_data, other) == 0;
}

bool TCharHolder::operator!=(const TCharHolder &other) const {
	return !(*this == other);
}

bool TCharHolder::SameAs(const TCharHolder &other) const {
	return *this == other;
}

TCharHolder::operator wxString() const {
	return GetFullPath();
}

TCharHolder::operator wxFileName() const {
	if (!_data)
		return wxFileName();
	wxFileName result(GetFullPath().ToStdWstring());
	result.NormalizePath();
	return result;
}

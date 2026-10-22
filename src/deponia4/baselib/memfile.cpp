#include "baselib/memfile.h"

#include "Diagnostics.h"
#include "WxStub.h"

TMemoryFile::~TMemoryFile() {
	delete[] _data;
}

unsigned char *TMemoryFile::GetBuffer() {
	x_assert(_data != nullptr, "m_pData", "src/baselib/memfile.cpp", 0x52);
	if (_data == nullptr && wxLog::loglevel >= 0)
		wxLog::logexpanded(L"A");
	return _data;
}

void TMemoryFile::ReleaseMemory() {
	delete[] _data;
	_data = nullptr;
	_size = 0;
}

bool TMemoryFile::Reserve(long size) {
	if (static_cast<long>(_size) >= size)
		return _data != nullptr;
	delete[] _data;
	_size = size;
	_data = new unsigned char[size];
	return true;
}

void TMemoryFile::DetachBuffer() {
	_data = nullptr;
	_size = 0;
}

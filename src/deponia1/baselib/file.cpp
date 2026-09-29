#include "baselib/file.h"

TFile::~TFile() {
	if (_handle)
		std::fclose(_handle);
}

bool TFile::OpenRead(const wxFileName &path) {
	_handle = std::fopen(static_cast<const char *>(path.GetFullPath().mb_str()), "rb");
	return _handle != nullptr;
}

unsigned long TFile::ReadToBuf(TMemoryBuffer &buffer, unsigned long size) {
	buffer.Init(size);
	if (!_handle)
		return 0;
	return static_cast<unsigned long>(std::fread(buffer.GetData(), 1, size, _handle));
}

bool TFile::ReadByte(char &out) {
	if (!_handle)
		return false;
	int c = std::fgetc(_handle);
	if (c == EOF)
		return false;
	out = static_cast<char>(c);
	return true;
}

void TFile::Close() {
	if (_handle) {
		std::fclose(_handle);
		_handle = nullptr;
	}
}

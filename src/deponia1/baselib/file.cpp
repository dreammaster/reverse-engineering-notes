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

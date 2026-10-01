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

bool TFile::DecryptHeader(const wxFileName &path, const wxString &key) {
	wxFile file;
	if (!file.Open(path.GetFullPath(), 2))
		return false;

	unsigned char buffer[0x12C];
	unsigned long bytesRead = file.Read(reinterpret_cast<char *>(buffer), sizeof(buffer));
	if (bytesRead == 0) {
		file.Close();
		return false;
	}

	TMemoryBuffer mem;
	mem.Reserve(bytesRead);
	mem.AppendData(buffer, bytesRead);
	mem.Decrypt(key, nullptr, 0);

	file.Seek(0, 0);
	file.Write(mem.GetData(), bytesRead);
	file.Flush();
	file.Close();
	return true;
}

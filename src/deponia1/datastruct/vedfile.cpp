#include "datastruct/vedfile.h"

// Confirmed (asm lines 525613-525685).
void TVedFile::CheckBinary() {
	if (_isBinary)
		return;

	char magic[4] = {0, 0, 0, 0};
	ReadByte(magic[0]);
	ReadByte(magic[1]);
	ReadByte(magic[2]);
	ReadByte(magic[3]);
	if (magic[0] == 'V' && magic[1] == 'B' && magic[2] == 'I' && magic[3] == 'N')
		_isBinary = true;

	if (_mode != 0) {
		_position = 0;
		_file.Seek(static_cast<unsigned long>(_start), 0);
	}
}

// Confirmed (asm lines 525685-525740).
bool TVedFile::GetVersionOk(int versionIn, int versionOut) const {
	int version;
	if (_mode == 2)
		version = _writeVersion;
	else if (_mode <= 1)
		version = _readVersion;
	else
		return false;

	if (version < versionIn)
		return false;
	return version < versionOut || versionOut == -1;
}

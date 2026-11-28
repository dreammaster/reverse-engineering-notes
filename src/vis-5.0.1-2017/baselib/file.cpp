#include "baselib/file.h"

#include <sys/stat.h>

#include <algorithm>
#include <cstring>

#include "Diagnostics.h"

static const unsigned long kChunkSize = 0x80000;
static const unsigned long kCompressedChunkSize = 0x200000;
// A composed file's header: the part kContentEncryptedHeader encrypts.
static const unsigned long kHeaderSize = 0x12C;

TFile::TFile() {
}

TFile::~TFile() {
}

// Confirmed (asm lines 522546-522611).
bool TFile::Seek(unsigned long offset, int mode) {
	if (_mode == 0)
		return false;

	switch (mode) {
	case 0:
		_position = static_cast<long long>(offset);
		break;
	case 1:
		_position += static_cast<long long>(offset);
		break;
	case 2:
		_position = _length - static_cast<long long>(offset);
		break;
	default:
		break;
	}
	return _file.Seek(static_cast<unsigned long>(_position + _start), 0);
}

// Confirmed (asm lines 522717-522746): `GetTimes()`'s modification time, or
// 0 if it can't be read.
long long TFile::GetFileTime(const wxFileName &path) {
	struct stat st;
	if (::stat(static_cast<const char *>(path.GetFullPath().mb_str()), &st) != 0)
		return 0;
	return static_cast<long long>(st.st_mtime) * 1000;
}

// Confirmed (asm lines 522746-522896): reopening closes whatever was open.
bool TFile::OpenRead(const wxFileName &path) {
	if (_file.IsOpened()) {
		if (_mode != 0) {
			_file.Flush();
			_file.Close();
		}
		_mode = 0;
	}

	_path = path;
	if (!_file.Open(_path.GetFullPath(), wxString(L"r")))
		return false;

	_mode = 1;
	_flag30 = 0;
	_start = 0;
	_position = 0;
	_length = _file.Length();
	return true;
}

// Confirmed (asm lines 522896-523061).
bool TFile::OpenReadFromComposedFile(const wxFileName &composedFile, const wxString &name, long offset,
                                     long length) {
	if (_file.IsOpened()) {
		if (_mode != 0) {
			_file.Flush();
			_file.Close();
		}
		_mode = 0;
	}

	_path = composedFile;
	_composedName = name;
	if (!_file.Open(_path.GetFullPath(), wxString(L"r")))
		return false;

	_mode = 1;
	_flag30 = 1;
	_start = offset;
	_position = 0;
	_length = length;
	_file.Seek(static_cast<unsigned long>(offset), 0);
	return true;
}

// Confirmed (asm lines 523174-523325): the path is made absolute before
// opening; the window is unbounded.
bool TFile::OpenWrite(const wxFileName &path) {
	if (_file.IsOpened()) {
		if (_mode != 0) {
			_file.Flush();
			_file.Close();
		}
		_mode = 0;
	}

	_path = path;
	_path.MakeAbsolute();
	if (!_file.Open(_path.GetFullPath(), wxString(L"w")))
		return false;

	_mode = 2;
	_flag30 = 1;
	_start = 0;
	_position = 0;
	_length = -1;
	return true;
}

// Confirmed (asm lines 523325-523362).
void TFile::Close() {
	if (_mode != 0) {
		_file.Flush();
		_file.Close();
	}
	_mode = 0;
}

bool TFile::IsOpened() const {
	return _file.IsOpened();
}

// Confirmed (asm lines 523078-523120): at the end once the position has
// reached a known window length, when nothing's open, or when the file
// itself is at its end.
bool TFile::Eof() const {
	if (_length == -1 || _length > _position) {
		if (_file.IsOpened() && !_file.Eof())
			return false;
	}
	return true;
}

// Confirmed (asm lines 523152-523174).
long TFile::GetCurrentPos() const {
	return _mode == 0 ? 0 : static_cast<long>(_position);
}

void TFile::SetOriginalPath(const wxFileName &path) {
	_originalPath = path;
	_originalPath.NormalizePath();
}

// Confirmed (asm lines 523836-524018): the window is copied out to `path`
// (which must already exist - it's opened read-write), 512 KiB at a time,
// from the window's start.
bool TFile::WriteToFile(const wxFileName &path) {
	std::vector<char> chunk(kChunkSize);
	wxFile out;

	if (_mode != 0) {
		_position = 0;
		_file.Seek(static_cast<unsigned long>(_start), 0);
	}

	if (!out.Open(path.GetFullPath(), 2))
		return false;

	while ((_length == -1 || _length > _position) && _file.IsOpened() && !_file.Eof()) {
		unsigned long toRead = kChunkSize;
		if (_length != -1 && static_cast<unsigned long long>(_position) + kChunkSize >
		                         static_cast<unsigned long long>(_length))
			toRead = static_cast<unsigned long>(_length - _position);

		unsigned long read = _file.Read(chunk.data(), toRead);
		if (_mode != 0) {
			_position += static_cast<long long>(read);
			_file.Seek(static_cast<unsigned long>(_position + _start), 0);
		}
		if (read == 0)
			break;
		out.Write(chunk.data(), read);
	}
	return true;
}

// Confirmed (asm lines 524018-524140).
unsigned long TFile::ReadToBuf(TMemoryBuffer &buffer, unsigned long size) {
	x_assert(_length >= 0, "m_fileLength >= 0",
	         "/home/simon/Documents/jenkins/branchPillars/src/baselib/file.cpp", 0x126);

	std::vector<char> chunk(kChunkSize);
	if (static_cast<long long>(size) > _length || size == 0)
		size = static_cast<unsigned long>(_length);
	buffer.Init(size);

	unsigned long total = 0;
	unsigned long requested = kChunkSize;
	while ((_length == -1 || _length > _position) && _file.IsOpened() && !_file.Eof() &&
	       requested == kChunkSize) {
		requested = (size < total + kChunkSize) ? size - total : kChunkSize;
		long long left = _length - _position;
		if (static_cast<long long>(requested) > left)
			requested = static_cast<unsigned long>(left);

		unsigned long read = _file.Read(chunk.data(), requested);
		_position += static_cast<long long>(read);
		buffer.AppendData(chunk.data(), read);
		total += read;
	}
	return total;
}

// Confirmed (asm lines 524140-524189).
unsigned long TFile::WriteFromBuf(const TMemoryBuffer &buffer, bool keepPosition) {
	unsigned long written = _file.Write(buffer.GetData(), buffer.GetLen());
	if (!keepPosition)
		_position += static_cast<long long>(written);
	return written;
}

unsigned long TFile::readClamped(void *dest, unsigned long size) {
	if (_length != -1 && _length <= _position)
		return 0;
	if (!_file.IsOpened() || _file.Eof())
		return 0;

	if (static_cast<unsigned long long>(_position) + size > static_cast<unsigned long long>(_length))
		size = static_cast<unsigned long>(_length - _position);
	unsigned long read = _file.Read(dest, size);
	if (_mode != 0) {
		_position += static_cast<long long>(read);
		_file.Seek(static_cast<unsigned long>(_position + _start), 0);
	}
	return read;
}

// Confirmed (asm lines 524189-524353).
bool TFile::ReadByte(char &out) {
	return readClamped(&out, 1) == 1;
}

bool TFile::ReadByte(unsigned char &out) {
	return readClamped(&out, 1) == 1;
}

// Confirmed (asm lines 524353-524535): big-endian.
bool TFile::ReadShort(short &out) {
	unsigned char bytes[2] = {0, 0};
	bool ok = readClamped(bytes, 2) == 2;
	out = static_cast<short>((bytes[0] << 8) | bytes[1]);
	return ok;
}

bool TFile::ReadShort(unsigned short &out) {
	unsigned char bytes[2] = {0, 0};
	bool ok = readClamped(bytes, 2) == 2;
	out = static_cast<unsigned short>((bytes[0] << 8) | bytes[1]);
	return ok;
}

// Confirmed (asm lines 524535-524737): big-endian; the signed version
// sign-extends the 32-bit value (movsxd) into the 64-bit long.
bool TFile::ReadLong(long &out) {
	unsigned char bytes[4] = {0, 0, 0, 0};
	bool ok = readClamped(bytes, 4) == 4;
	out = static_cast<long>(static_cast<int>((static_cast<unsigned int>(bytes[0]) << 24) | (bytes[1] << 16) |
	                                          (bytes[2] << 8) | bytes[3]));
	return ok;
}

bool TFile::ReadLong(unsigned long &out) {
	unsigned char bytes[4] = {0, 0, 0, 0};
	bool ok = readClamped(bytes, 4) == 4;
	out = (static_cast<unsigned long>(bytes[0]) << 24) | (bytes[1] << 16) | (bytes[2] << 8) | bytes[3];
	return ok;
}

// Confirmed (asm lines 524737-524821).
unsigned long TFile::ReadMem(void *dest, unsigned long size) {
	return readClamped(dest, size);
}

// Confirmed (asm lines 524821-525436) - see the header comment for the three
// modes.
bool TFile::PasteFile(const wxFileName &source, const wxString &password, long flags,
                      unsigned long &outLength) {
	TFile in;

	if (flags & kContentCompressedChunks) {
		x_assert(!(flags & kContentEncryptedHeader), "!(flags & CONTENTFLAG_ENCRYPTED_HEADER)",
		         "/home/simon/Documents/jenkins/branchPillars/src/baselib/file.cpp", 0x19E);
		outLength = 0;
		if (!in.OpenRead(source))
			return false;

		TMemoryBuffer data;
		TMemoryBuffer header;
		header.Reserve(8);
		bool ok = true;
		while ((in._length == -1 || in._length > in._position) && in._file.IsOpened() && !in._file.Eof()) {
			in.ReadToBuf(data, kCompressedChunkSize);
			unsigned long rawLength = data.GetLen();
			if (!data.Compress() || ((flags & kContentEncrypted) && !data.Encrypt(password, nullptr, 0))) {
				ok = false;
				break;
			}

			unsigned long storedLength = data.GetLen();
			header.ClearMemory();
			header << static_cast<unsigned long>(rawLength);
			header << static_cast<unsigned long>(storedLength);
			_position += static_cast<long long>(_file.Write(header.GetData(), header.GetLen()));
			_position += static_cast<long long>(_file.Write(data.GetData(), data.GetLen()));
			outLength += storedLength + header.GetLen();
		}

		// The footer goes out whether or not a chunk failed.
		header.ClearMemory();
		header << static_cast<unsigned long>(0xFFEEFFEE);
		header << static_cast<long>(0);
		_position += static_cast<long long>(_file.Write(header.GetData(), header.GetLen()));
		outLength += header.GetLen();
		return ok;
	}

	if (flags & kContentCompressed) {
		TMemoryBuffer data;
		if (!in.OpenRead(source))
			return false;

		in.ReadToBuf(data, 0);
		in.Close();
		if (!data.Compress())
			return false;
		outLength = data.GetLen();

		// Only the encrypted variant is actually written out here (the
		// original's unencrypted compressed whole-file case returns success
		// without writing anything - presumably its caller writes the buffer
		// itself through PasteData()).
		if (flags & kContentEncrypted) {
			if (!data.Encrypt(password, nullptr, 0))
				return false;
			_position += static_cast<long long>(_file.Write(data.GetData(), data.GetLen()));
		}
		return true;
	}

	if (!in.OpenRead(source))
		return false;

	TMemoryBuffer data;
	unsigned long total = 0;
	bool ok = true;
	while ((in._length == -1 || in._length > in._position) && in._file.IsOpened() && !in._file.Eof()) {
		in.ReadToBuf(data, kChunkSize);

		if (flags & kContentEncrypted) {
			if (!data.Encrypt(password, nullptr, 0)) {
				ok = false;
				break;
			}
		} else if ((flags & kContentEncryptedHeader) && total <= kHeaderSize - 1) {
			if (total + data.GetLen() <= kHeaderSize) {
				if (!data.Encrypt(password, nullptr, 0)) {
					ok = false;
					break;
				}
			} else {
				// Only the chunk's first (0x12C - total) bytes are in the header.
				TMemoryBuffer copy;
				copy.Reserve(data.GetLen());
				copy.AppendData(data.GetData(), data.GetLen());
				if (!copy.Encrypt(password, nullptr, 0)) {
					ok = false;
					break;
				}
				std::memcpy(data.GetData(), copy.GetData(), kHeaderSize - total);
			}
		}

		total += data.GetLen();
		_position += static_cast<long long>(_file.Write(data.GetData(), data.GetLen()));
	}
	return ok;
}

// Confirmed (asm lines 525436-525526).
bool TFile::PasteData(TMemoryBuffer &data, const wxString &password, long flags, unsigned long &outLength) {
	if (flags & kContentCompressed) {
		bool compressed = data.Compress();
		outLength = data.GetLen();
		if (!compressed)
			return false;
	}
	if ((flags & kContentEncrypted) && !data.Encrypt(password, nullptr, 0))
		return false;

	_position += static_cast<long long>(_file.Write(data.GetData(), data.GetLen()));
	return true;
}

bool TFile::DecryptHeader(const wxFileName &path, const wxString &key) {
	wxFile file;
	if (!file.Open(path.GetFullPath(), 2))
		return false;

	unsigned char buffer[kHeaderSize];
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

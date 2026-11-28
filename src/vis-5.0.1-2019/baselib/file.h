// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/baselib/file.cpp - see manifest/source_layout.tsv.
//
// Implemented in full (Deponia_Linux.asm lines 522315-525526, all 37
// manifest-listed methods). TFile is the engine's file wrapper: a wxFile plus
// a *window* onto it - a start offset and length within the underlying file,
// and a position relative to that window - so the same class reads a plain
// file (the window is the whole file) and an entry packed inside a composed
// container file (TFile::OpenReadFromComposedFile(); the window is that
// entry's byte range). All reads are clamped to the window. A TFile can also
// be opened for writing, and has the "paste" helpers the composed-file
// writer uses to append (optionally compressed and/or encrypted) data.
//
// Real layout (0x68 bytes, TVedFile adds to it): vtable (+0x00), the wxFile
// (+0x08), the path opened (+0x20, a wxFileName), the original path (+0x28,
// a wxFileName - what the caller asked for, before any composed-file
// indirection), a flag (+0x30: 0 for a plain read, 1 for a composed-file
// entry or a file opened for writing), the open mode (+0x34: 0 closed, 1
// read, 2 write), the content flags (+0x38, see IsContent*()), the window's
// start (+0x40), the position within the window (+0x48), the window's length
// (+0x50, -1 = unbounded), the uncompressed length (+0x58) and, for a
// composed-file entry, its name (+0x60).
#pragma once

#include "TMemoryBuffer.h"
#include "WxStub.h"

// Content-flag bits (TFile::SetContentFlags(); asm lines 522611-522717 and
// PasteFile()'s own tests). Named for their observed role; the original's
// own CONTENTFLAG_* names are known only for the one in an assert string.
enum TFileContentFlags {
	kContentCompressed = 1,
	kContentEncrypted = 2,
	// The first 0x12C bytes (a composed file's header) are encrypted even if
	// the rest isn't - the original's "CONTENTFLAG_ENCRYPTED_HEADER".
	kContentEncryptedHeader = 4,
	kContentPNGHeaderEncrypted = 8,
	kContentCompressedChunks = 0x10
};

class TFile {
public:
	TFile();
	virtual ~TFile();

	// Seek within the window (wxFromStart = 0, wxFromCurrent = 1, wxFromEnd =
	// 2; any other mode just re-seeks to the current position). Fails if the
	// file isn't open.
	bool Seek(unsigned long offset, int mode);

	void SetContentFlags(long flags) {
		_contentFlags = flags;
	}
	void SetUncompressedLength(long length) {
		_uncompressedLength = length;
	}
	bool IsContentCompressed() const {
		return (_contentFlags & kContentCompressed) != 0;
	}
	bool IsContentCompressedChunks() const {
		return (_contentFlags & kContentCompressedChunks) != 0;
	}
	bool IsContentEncrypted() const {
		return (_contentFlags & kContentEncrypted) != 0;
	}
	bool IsPNGHeaderEncrypted() const {
		return (_contentFlags & kContentPNGHeaderEncrypted) != 0;
	}

	// The file's last-modified time in milliseconds since the epoch (the raw
	// value inside a wxDateTime), or 0 if it can't be read. Static in effect
	// - the body never touches `this`.
	static long long GetFileTime(const wxFileName &path);

	bool OpenRead(const wxFileName &path);
	// Opens `composedFile` and positions at `offset`; the window is the
	// `length` bytes from there. `name` is the entry's name.
	bool OpenReadFromComposedFile(const wxFileName &composedFile, const wxString &name, long offset, long length);
	bool OpenWrite(const wxFileName &path);
	void Close();
	bool IsOpened() const;
	bool Eof() const;

	const wxFile &GetFilePointer() const {
		return _file;
	}
	long GetFileLength() const {
		return static_cast<long>(_length);
	}
	long GetCurrentPos() const;
	const wxFileName &GetPath() const {
		return _path;
	}
	// Confirmed (Deponia_Linux.asm lines 523378-523642): stores the path
	// (through the same "\\" -> "/" wxString::Replace() call seen elsewhere
	// in this project, which real wxString::Replace() no-ops on the empty
	// search string the binary actually passes - so this is just the path
	// unchanged) - not read back by anything reversed so far, kept purely for
	// parity with the original's own field.
	void SetOriginalPath(const wxFileName &path);
	const wxFileName &GetOriginalPath() const {
		return _originalPath;
	}

	// Confirmed in full (Deponia_Linux.asm lines 523658-523820): reads the
	// first 0x12C (300) bytes of `path`, XOR-decrypts them in place via the
	// real TMemoryBuffer::Decrypt() cipher keyed by `key`, then writes them
	// straight back - decrypting (or, since the cipher is its own inverse,
	// equally encrypting) a composed-file's on-disk header.
	bool DecryptHeader(const wxFileName &path, const wxString &key);
	// Confirmed (asm lines 523821-523826): a pure tail call to
	// DecryptHeader() - the cipher is its own inverse, so encryption and
	// decryption are the same operation.
	bool EncryptHeader(const wxFileName &path, const wxString &key) {
		return DecryptHeader(path, key);
	}

	// Copies this file's window out to an existing file (opened read-write),
	// 512 KiB at a time.
	bool WriteToFile(const wxFileName &path);
	// Reads up to `size` bytes (the whole window if 0 or more than is left)
	// into `buffer`, 512 KiB at a time; returns how many were read.
	unsigned long ReadToBuf(TMemoryBuffer &buffer, unsigned long size);
	unsigned long WriteFromBuf(const TMemoryBuffer &buffer, bool keepPosition);

	// Fixed-size reads (clamped to the window); true only if every byte was
	// read. The 2- and 4-byte ones are big-endian. A failed or short read
	// leaves the missing bytes zero (the original composes whatever was on
	// the stack).
	bool ReadByte(char &out);
	bool ReadByte(unsigned char &out);
	bool ReadShort(short &out);
	bool ReadShort(unsigned short &out);
	bool ReadLong(long &out);
	bool ReadLong(unsigned long &out);
	unsigned long ReadMem(void *dest, unsigned long size);

	// Appends the contents of `source` to this (write-mode) file the way the
	// composed-file writer needs, and reports the bytes the entry takes in
	// `outLength`. `flags` is a TFileContentFlags mask:
	//  - chunked compression: 2 MiB chunks, each compressed (and encrypted if
	//    asked) and preceded by an (uncompressed length, stored length) pair,
	//    ended by a (0xFFEEFFEE, 0) footer written even if a chunk failed;
	//  - whole-file compression: the file compressed (then optionally
	//    encrypted) in one piece;
	//  - otherwise a plain copy in 512 KiB chunks, optionally encrypting
	//    everything or just the first 0x12C bytes (the encrypted header);
	//    `outLength` is not set in this mode.
	bool PasteFile(const wxFileName &source, const wxString &password, long flags, unsigned long &outLength);
	// The same for data already in memory (always whole, never chunked).
	bool PasteData(TMemoryBuffer &data, const wxString &password, long flags, unsigned long &outLength);

protected:
	// Reads up to `size` bytes at the window position, clamped to the window;
	// advances the position if the file is open. Returns the byte count (0 if
	// nothing can be read).
	unsigned long readClamped(void *dest, unsigned long size);

	wxFile _file;
	wxFileName _path;
	wxFileName _originalPath;
	int _flag30 = 0;
	int _mode = 0;
	long long _contentFlags = 0;
	long long _start = 0;
	long long _position = 0;
	long long _length = -1;
	long long _uncompressedLength = 0;
	wxString _composedName;
};

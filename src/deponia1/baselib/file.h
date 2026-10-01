// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/baselib/file.cpp - see manifest/source_layout.tsv.
#pragma once

#include <cstdio>

#include "TMemoryBuffer.h"
#include "WxStub.h"

class TFile {
public:
	TFile() = default;
	~TFile();

	bool OpenRead(const wxFileName &path);
	unsigned long ReadToBuf(TMemoryBuffer &buffer, unsigned long size);
	// Confirmed call shapes only (TGameControl::PreLoad, Deponia_Linux.asm
	// lines 463954-463966) - not reversed beyond that.
	bool ReadByte(char &out);
	void Close();
	// Confirmed (TComposedFileManager::Open/TComposedFile::Open, Deponia_Linux
	// .asm line 523404+): stores the path (through the same "/" vs "\\"
	// wxString::Replace() call seen elsewhere in this project, which real
	// wxString::Replace() no-ops on for an empty search string - so this is
	// just the path unchanged) - not read back by anything reversed so far,
	// kept purely for parity with the original's own field.
	void SetOriginalPath(const wxFileName &path) {
		_originalPath = path.GetFullPath().ToStdWstring();
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

private:
	std::FILE *_handle = nullptr;
	std::wstring _originalPath;
};

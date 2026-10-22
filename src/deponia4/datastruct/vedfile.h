// Original path not yet confirmed; stays alongside type.h in datastruct/.
//
// Implemented in full (Deponia_Linux.asm lines 522419-525756, all 10
// manifest-listed methods). TVedFile is a Visionaire project/savegame file
// being read or written: a TFile plus its format version, whether it's a
// binary ("VBIN" magic) rather than XML project, whether it's a savegame,
// and a reference path.
//
// Real layout (TFile occupies +0x00-0x67): binary flag (+0x68), read-side
// format version (+0x6C), write-side format version (+0x70) - both -1
// until SetVersion() - the savegame flag (+0x74) and the reference path
// (+0x78).
#pragma once

#include "baselib/file.h"

class TVedFile : public TFile {
public:
	TVedFile() = default;
	~TVedFile() override = default;

	void SetReferencePath(const wxFileName &path) {
		_referencePath = path;
	}
	// Sets the version a file being read (first) / written (second) is in.
	void SetVersion(int readVersion, int writeVersion) {
		_readVersion = readVersion;
		_writeVersion = writeVersion;
	}
	bool IsBinary() const {
		return _isBinary;
	}
	// Reads the first four bytes: "VBIN" marks a binary project. The file is
	// rewound to its window's start either way (if it's open).
	void CheckBinary();
	// Whether a field/type that exists from file version `versionIn` up to
	// (but not including) `versionOut` (-1 = open-ended) is present in this
	// file's format version: the write-side version for a file open for
	// writing, the read-side one otherwise (and never for a mode above 2).
	bool GetVersionOk(int versionIn, int versionOut) const;
	void SetSaveGame(bool saveGame) {
		_isSaveGame = saveGame;
	}
	bool IsSaveGame() const {
		return _isSaveGame;
	}

private:
	bool _isBinary = false;
	int _readVersion = -1;
	int _writeVersion = -1;
	bool _isSaveGame = false;
	wxFileName _referencePath;
};

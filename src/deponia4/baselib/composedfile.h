// Reconstructed from Deponia_Linux.asm, TComposedFile methods at asm lines
// 541459-548209+ (address range 0x649E70-0x64FB70+). See NOTES.md.
//
// Original path confirmed via an x_assert() call inside
// TComposedFile::GetComposedFileExtension(): src/baselib/composedfile.cpp -
// see manifest/source_layout.tsv.
//
// TComposedFile is Visionaire's asset-archive ("container") format reader/
// writer. The on-disk format (confirmed from InitEntries, which is the only
// method traced in full depth):
//   bytes 0-3:    magic "VIS3"
//   bytes 4-7:    entry count (big-endian, via ByteStreamToLong; at most
//                 0x1869F)
//   bytes 8-...:  entryCount*16 + 6 bytes, ENCRYPTED with TMemoryBuffer::
//                 Decrypt(key, ...) where key is _encryptionKey - a real,
//                 confirmed cipher (see TMemoryBuffer.h): a repeating
//                 16-byte XOR keystream of MD5(key).
//                 Decrypted layout: "HDR", then entryCount 16-byte records
//                 {offset, stored size, uncompressed length, content flags},
//                 each a big-endian 32-bit value, then "END". Entry offsets
//                 are relative to the end of this table (InitEntries() adds
//                 the table's end position to each).
// The entries carry no names: they're addressed by index, through the
// "<ext>#<type letter>#<offset>#<index>#" sprite/file reference syntax (see
// GetContainerFileExtension()).
//
// All 20 methods are implemented in full (Deponia_Linux.asm lines 541459-
// 548209) - the read path (InitEntries, Open, GetMemoryFile) and the
// authoring path (InitForWrite, AddData, AddFile, AddComposedFile, WriteToDisk,
// Export) the savegame writer uses.
#pragma once

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include "TMemoryBuffer.h"
#include "WxStub.h"

class TFile;
class TMemoryFile;
class EventHandler;

// Only two ordinal values are confirmed (InitForWrite(long, wxFileName,
// bool, bool) picks between 7 and 8 based on its last bool parameter, likely
// "embed in a 32-bit vs 64-bit executable" or "compressed vs not"); the
// GetComposedFileExtension switch handles values 0-8 (9 cases total) so at
// least that many exist. Real enumerator names are not recoverable without
// the header that declares them.
enum class TContainerTypeEnum {
	kType0 = 0,
	kType1 = 1,
	kType2 = 2,
	kType3 = 3,
	kType4 = 4,
	kType5 = 5,
	kType6 = 6,
	kEmbeddedInExecutable = 7,
	kEmbeddedInExecutableAlt = 8,
};

// 48 bytes in the original (0x30): the entry's file name (+0x00, a wxFileName -
// a single-pointer COW std::wstring handle, see NOTES.md on this binary's
// pre-C++11 string ABI; entries read from a container carry none, since the
// directory stores only offsets), a pointer to in-memory source data (+0x08,
// set by AddData() for an entry written from a buffer), the entry's byte
// offset (+0x10 - relative to the end of the directory table in a container
// being read, which InitEntries() makes absolute), its stored byte size
// (+0x18), its uncompressed length (+0x20) and its content flags (+0x28, a
// TFileContentFlags mask - see baselib/file.h).
struct SEntryInfo {
	std::wstring filename;
	TMemoryBuffer *memoryData = nullptr;
	long long containerOffset = 0;
	long long byteSize = 0;
	long long uncompressedLength = 0;
	long long contentFlags = 0;
};

// The binary's own StringHashMap<wxString,wxString,wchar_t const*,StringHash<
// wchar_t const*>> (a custom chained hash map keyed by a wide string) - only
// its use as a "file path -> name already assigned" cache by TComposedFile::
// AddFile() is confirmed, so it's modeled with the standard container.
typedef std::unordered_map<std::wstring, std::wstring> StringHashMap;

class TComposedFile {
public:
	TComposedFile();
	~TComposedFile();

	static std::function<void()> onRetryLoad;
	// Registered by TGameControl's constructor with a handler that logs the
	// failing archive's exe-filename plus an error string (confirmed from
	// the lambda's _M_invoke thunk; the handler body itself wasn't traced
	// in detail). Called from InitEntries()'s open-failure path when a
	// handler is registered (see the `qword_11F8F10` check there).
	static std::function<void(TComposedFile *, std::string)> onError;

	void InitForWrite(long offset, TContainerTypeEnum containerType, bool flag);
	void InitForWrite(long offset, const wxFileName &exeFile, bool flag, bool useAltContainerType);
	void ClearEntries();
	void SetExeFile(const wxFileName &exeFile);

	int GetNumberOfEntries() const;
	SEntryInfo *GetEntry(int index);
	// Confirmed direct field read (TComposedFileManager::GetComposedMovieFileName,
	// Deponia_Linux.asm line 519672, `+0x18` - matching _composedFilePath's
	// own documented original offset).
	const std::wstring &GetComposedFilePath() const {
		return _composedFilePath;
	}
	wxString GetComposedFileExtension() const;
	wxString GetContainerFileExtension(wxString base, long index) const;

	bool Init(const wxFileName &composedFilePath, const wxString &encryptionKey, long offset);
	bool Open(TFile &outFile, const wxFileName &composedFilePath, long index);
	bool GetMemoryFile(TMemoryFile &outFile, const wxFileName &composedFilePath, long index);

	// Writes the composed file to `path` (the container extension is applied;
	// the directory is created if need be): the optional embedded exe, the
	// directory placeholder, every entry's data (from its buffer or file,
	// compressed/encrypted per its content flags), then the real directory
	// (encrypted with `password`) and, when embedding, the exe-length footer.
	// `log` (optional) receives a progress trace; `handler` (optional) gets a
	// BuildProgressEvent per entry.
	bool WriteToDisk(const wxFileName &path, const wxString &password, wxFile *log, EventHandler *handler);
	bool WriteToDisk(const wxString &password, wxFile *log, EventHandler *handler);
	// Extracts entry `index` to a new temp file (named from `a` and `b`) and
	// returns its path in `outPath`.
	bool Export(long index, wxFileName &outPath, const wxString &a, const wxString &b);
	// Appends an entry whose data is `data` (kept by pointer - the buffer must
	// outlive WriteToDisk(); it's compressed/encrypted in place there); returns
	// true and `outName` = `name` with its extension replaced by the
	// "<ext>#<type letter>#<offset>#<index>#" reference this entry is looked up
	// by.
	int AddData(const TMemoryBuffer &data, wxFileName name, wxFileName &outName, int flags);
	// The same for a file on disk. `names` caches the references already
	// handed out (by file path), so a file added twice is stored once.
	int AddFile(wxFileName file, wxFileName &outName, StringHashMap &names, int flags);
	// Appends the path a composed file would be written to (name + container
	// extension) to `outList`.
	bool AddComposedFile(const wxFileName &file, std::vector<wxFileName> &outList);

private:
	bool InitEntries();

	std::vector<SEntryInfo> _entries;
	std::wstring _composedFilePath;  // +0x18 in the original
	std::wstring _exeFile;           // +0x20 in the original
	std::wstring _encryptionKey;     // +0x38 in the original - passed to TMemoryBuffer::Decrypt
	long _offset = 0;
	TContainerTypeEnum _containerType = TContainerTypeEnum::kType0;
	bool _flag34 = false;
	bool _needsInit = false;    // +0x40: set true by Init()/Open() to request a (re)load
	bool _hadOpenError = false; // +0x41: sticky flag once TFile::OpenRead() has failed once
	wxCriticalSection _criticalSection;
};

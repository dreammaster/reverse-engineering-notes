#include "baselib/composedfile.h"

#include "Diagnostics.h"
#include "baselib/file.h"
#include "baselib/memfile.h"

std::function<void()> TComposedFile::onRetryLoad;
std::function<void(TComposedFile *, std::string)> TComposedFile::onError;

namespace {
// ByteStreamToLong(unsigned char*) - reads a little-endian uint32 from a raw
// byte pointer; used to decode the entry count right after the "VIS3" magic.
unsigned long ByteStreamToLong(const unsigned char *p) {
	return static_cast<unsigned long>(p[0]) | (static_cast<unsigned long>(p[1]) << 8) |
	       (static_cast<unsigned long>(p[2]) << 16) | (static_cast<unsigned long>(p[3]) << 24);
}
}  // namespace

TComposedFile::TComposedFile() = default;
TComposedFile::~TComposedFile() = default;

void TComposedFile::InitForWrite(long offset, TContainerTypeEnum containerType, bool flag) {
	_entries.clear();
	_offset = offset;
	_containerType = containerType;
	_flag34 = flag;
}

void TComposedFile::InitForWrite(long offset, const wxFileName &exeFile, bool flag, bool useAltContainerType) {
	_entries.clear();
	_composedFilePath = exeFile.GetFullPath().ToStdWstring();
	_offset = offset;
	_flag34 = flag;
	_containerType = useAltContainerType ? TContainerTypeEnum::kEmbeddedInExecutableAlt
	                 : TContainerTypeEnum::kEmbeddedInExecutable;
}

void TComposedFile::ClearEntries() {
	_entries.clear();
}

void TComposedFile::SetExeFile(const wxFileName &exeFile) {
	_exeFile = exeFile.GetFullPath().ToStdWstring();
}

int TComposedFile::GetNumberOfEntries() const {
	return static_cast<int>(_entries.size());
}

SEntryInfo *TComposedFile::GetEntry(int index) {
	return &_entries[static_cast<size_t>(index)];
}

wxString TComposedFile::GetComposedFileExtension() const {
	// 9-case switch on _containerType in the original, each producing a
	// distinct extension - cases with a valid (>=0) _offset format it in
	// ("v" + offset), else fall back to a fixed 2-letter code. The exact
	// wx-internal formatting call (wxString::privFormat with a bare "v"
	// format string and one integer vararg) couldn't be pinned down at the
	// byte level (see NOTES.md) - approximated as "v<offset>" since that
	// matches the observable intent (a numbered container extension).
	switch (_containerType) {
	case TContainerTypeEnum::kType0:
		return wxString(L"vis");
	case TContainerTypeEnum::kType1:
		return _offset >= 0 ? wxString(L"v" + std::to_wstring(_offset)) : wxString(L"vs");
	case TContainerTypeEnum::kType2:
		return _offset >= 0 ? wxString(L"v" + std::to_wstring(_offset)) : wxString(L"vc");
	case TContainerTypeEnum::kType3:
		return _offset >= 0 ? wxString(L"v" + std::to_wstring(_offset)) : wxString(L"vv");
	case TContainerTypeEnum::kType5:
		return _offset >= 0 ? wxString(L"v" + std::to_wstring(_offset)) : wxString(L"vi");
	case TContainerTypeEnum::kType6:
		return wxString(L"vo");
	default:
		x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/baselib/composedfile.cpp", 0x21B);
		return wxString();
	}
}

wxString TComposedFile::GetContainerFileExtension(wxString base, long index) const {
	// Builds a per-volume filename incorporating _offset, `base`, a
	// container-type-specific format character, and `index` - the exact
	// dynamic format-character table (CSWTCH_289) wasn't resolved; this
	// reproduces the observable inputs/output shape only.
	return wxString(base.ToStdWstring() + L"." + std::to_wstring(index));
}

bool TComposedFile::InitEntries() {
	wxCriticalSectionLocker locker(_criticalSection);

	if (!_needsInit && !_hadOpenError)
		return true;

	TFile file;
	if (!file.OpenRead(wxFileName(_composedFilePath))) {
		_hadOpenError = true;
		_needsInit = false;
		return false;
	}

	TMemoryBuffer header;
	if (file.ReadToBuf(header, 8) != 8) {
		return false;  // "The file ... is not a composed file."
	}

	const unsigned char *headerData = header.GetData();
	if (headerData[0] != 'V' || headerData[1] != 'I' || headerData[2] != 'S' || headerData[3] != '3') {
		return false;  // "The file ... is not a composed file."
	}

	unsigned long entryCount = ByteStreamToLong(headerData + 4);
	constexpr unsigned long kMaxEntries = 0x1869F;  // sanity limit confirmed in the disassembly
	if (entryCount > kMaxEntries)
		return false;

	if (entryCount > _entries.size())
		_entries.resize(entryCount);

	unsigned long tableSize = entryCount * 16 + 6;
	TMemoryBuffer table;
	if (file.ReadToBuf(table, tableSize) != tableSize) {
		_needsInit = false;
		return false;  // "expected N bytes, file may be truncated"
	}

	if (!table.Decrypt(_encryptionKey, nullptr, 0)) {
		_needsInit = false;
		return false;
	}

	const unsigned char *decrypted = table.GetData();
	if (decrypted[0] != 'H') {
		// Wrong key or corrupt data.
		_needsInit = false;
		return false;
	}

	LoadEntriesFromDisk(decrypted + 3, entryCount);
	_needsInit = false;
	return true;
}

void TComposedFile::LoadEntriesFromDisk(const unsigned char */*data*/, unsigned long /*entryCount*/) {
	// Not reversed - see the class header comment. Real implementation
	// would populate _entries[i].filename/containerOffset/byteSize/field20/
	// field28 from each 16-byte on-disk record.
}

bool TComposedFile::Init(const wxFileName &composedFilePath, const wxString &encryptionKey, long offset) {
	_composedFilePath = composedFilePath.GetFullPath().ToStdWstring();
	_encryptionKey = encryptionKey.ToStdWstring();
	_offset = offset;
	_needsInit = true;
	_hadOpenError = false;
	return InitEntries();
}

bool TComposedFile::Open(TFile &/*outFile*/, const wxFileName &/*composedFilePath*/, long index) {
	if (!InitEntries())
		return false;
	if (index < 0 || index >= GetNumberOfEntries())
		return false;

	SEntryInfo &entry = _entries[static_cast<size_t>(index)];
	wxFileName entryPath(entry.filename);
	entryPath.NormalizePath();
	// Real implementation opens `outFile` at the byte offset within the
	// composed file recorded for this entry (containerOffset/byteSize) -
	// not reversed (see LoadEntriesFromDisk).
	return true;
}

bool TComposedFile::GetMemoryFile(TMemoryFile &outFile, const wxFileName &/*composedFilePath*/, long index) {
	// Confirmed (Deponia_Linux.asm lines 545837-546088). Unlike Open() above,
	// the original ignores InitEntries()'s own return value here and always
	// falls through to the bounds check below, which fails closed on its own
	// if entries weren't actually (re)loaded.
	if (_needsInit || _hadOpenError)
		InitEntries();
	if (index < 0 || index >= GetNumberOfEntries())
		return false;

	const SEntryInfo &entry = _entries[static_cast<size_t>(index)];
	wxFile file;
	if (!file.Open(wxFileName(_composedFilePath).GetFullPath(), 1))
		return false;

	if (!outFile.Reserve(static_cast<long>(entry.byteSize))) {
		if (wxLog::loglevel >= 0) {
			wxString fmt;
			toUTF(&fmt, "Memory for memory file %s with length %d could not be reserved");
			wxLog::logexpanded(fmt.wc_str(), wxFileName(_composedFilePath).GetFullPath().wc_str(),
			                   static_cast<int>(entry.byteSize));
		}
		return false;
	}

	unsigned char *buffer = outFile.GetBuffer();
	file.Seek(static_cast<unsigned long>(entry.containerOffset), 0);

	// The original's read loop re-issues wxFile::Read() for the *full*
	// byteSize into the *same* buffer start on every iteration rather than
	// resuming from the current position/remaining count - almost certainly
	// harmless in practice since a local-file Read() satisfies the whole
	// request in one call, but not something worth reproducing literally
	// (see this project's "behavioral over binary fidelity" convention).
	// This reads once and treats any short read as failure, matching the
	// original's success/failure outcome either way.
	long totalRead = static_cast<long>(
	                     file.Read(reinterpret_cast<char *>(buffer), static_cast<unsigned long>(entry.byteSize)));
	file.Close();

	if (totalRead != entry.byteSize) {
		outFile.ReleaseMemory();
		return false;
	}
	return true;
}

bool TComposedFile::WriteToDisk(const wxFileName &/*path*/, const wxString &/*password*/, wxFile */*file*/,
                                EventHandler */*handler*/) {
	// Authoring-side (Visionaire Studio export); not reversed - see class
	// header comment.
	return false;
}

bool TComposedFile::WriteToDisk(const wxString &password, wxFile *file, EventHandler *handler) {
	return WriteToDisk(wxFileName(_composedFilePath), password, file, handler);
}

bool TComposedFile::Export(long /*index*/, wxFileName &/*outPath*/, const wxString &/*a*/, const wxString &/*b*/) {
	return false;
}

int TComposedFile::AddData(const TMemoryBuffer &/*data*/, wxFileName /*name*/, wxFileName &/*outName*/,
                           int /*flags*/) {
	return -1;
}

int TComposedFile::AddFile(wxFileName /*file*/, wxFileName &/*baseDir*/, int /*flags*/) {
	return -1;
}

bool TComposedFile::AddComposedFile(const wxFileName &/*file*/, std::vector<wxFileName> &/*outList*/) {
	return false;
}

#include "baselib/composedfile.h"

#include <cstdio>
#include <cstring>

#include "Diagnostics.h"
#include "EventHandler.h"
#include "TTempFile.h"
#include "baselib/BuildProgressEvent.h"
#include "baselib/file.h"
#include "baselib/memfile.h"

std::function<void()> TComposedFile::onRetryLoad;
std::function<void(TComposedFile *, std::string)> TComposedFile::onError;

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/baselib/composedfile.cpp";

namespace {
// ByteStreamToLong/ByteStreamToULong(unsigned char*) (Deponia_Linux.asm lines
// 540653-540700 - identical bodies): a big-endian 32-bit value, sign-extended
// into the 64-bit result (even the "unsigned" one).
long ByteStreamToLong(const unsigned char *p) {
	return static_cast<long>(static_cast<int>((static_cast<unsigned int>(p[0]) << 24) | (p[1] << 16) |
	                                          (p[2] << 8) | p[3]));
}

// wxString::Format(L"%03ld", n) / L"%05ld".
std::wstring padded(long value, const char *format) {
	char text[32];
	std::snprintf(text, sizeof(text), format, value);
	return std::wstring(text, text + std::char_traits<char>::length(text));
}

// The one-letter code CSWTCH_289/CSWTCH_404 (Deponia_Linux.asm) maps container
// types 1-6 to, 'v' for any other.
wchar_t containerLetter(TContainerTypeEnum type) {
	static const wchar_t kLetters[] = {L's', L'c', L'm', L'g', L'i', L'o'};
	int index = static_cast<int>(type) - 1;
	return (index >= 0 && index <= 5) ? kLetters[index] : L'v';
}
}  // namespace

TComposedFile::TComposedFile() = default;
TComposedFile::~TComposedFile() = default;

// Confirmed (asm lines 541459-541543).
void TComposedFile::InitForWrite(long offset, TContainerTypeEnum containerType, bool flag) {
	_entries.clear();
	_offset = offset;
	_containerType = containerType;
	_flag34 = flag;
}

// Confirmed (asm lines 541543-541639): note it's the *composed file path* this
// stores the given file in (the embedded-exe layout writes the container into
// the exe itself) - not SetExeFile()'s separate field.
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

// Confirmed (asm lines 541812-542348): the container type picks the
// extension; the numbered ones ("v<letter>NNN") carry the container's volume
// number when it has one (>= 0), else just the bare 2-letter code. Types 4, 7
// and 8 (the embedded-in-exe layouts) have no extension.
wxString TComposedFile::GetComposedFileExtension() const {
	switch (_containerType) {
	case TContainerTypeEnum::kType0:
		return wxString(L"vis");
	case TContainerTypeEnum::kType1:
		return wxString(_offset >= 0 ? L"vs" + padded(_offset, "%03ld") : std::wstring(L"vs"));
	case TContainerTypeEnum::kType2:
		return wxString(_offset >= 0 ? L"vc" + padded(_offset, "%03ld") : std::wstring(L"vc"));
	case TContainerTypeEnum::kType3:
		return wxString(_offset >= 0 ? L"vv" + padded(_offset, "%03ld") : std::wstring(L"vv"));
	case TContainerTypeEnum::kType5:
		return wxString(_offset >= 0 ? L"vi" + padded(_offset, "%03ld") : std::wstring(L"vi"));
	case TContainerTypeEnum::kType6:
		return wxString(L"vo");
	case TContainerTypeEnum::kType4:
	case TContainerTypeEnum::kEmbeddedInExecutable:
	case TContainerTypeEnum::kEmbeddedInExecutableAlt:
		return wxString();
	default:
		x_assert(false, "false", kSourceFile, 0x21B);
		return wxString();
	}
}

// Confirmed (asm lines 541702-541791): "<ext>#<type letter>#<volume>#<index>#"
// - how an entry is referred to by name (also what TSprite paths embed, e.g.
// "x.webp#g#-01#00001#").
wxString TComposedFile::GetContainerFileExtension(wxString base, long index) const {
	std::wstring text = base.ToStdWstring();
	text += L'#';
	text += containerLetter(_containerType);
	text += L'#' + padded(_offset, "%03ld") + L'#' + padded(index, "%05ld") + L'#';
	return wxString(text);
}

// Confirmed (asm lines 544356-545563). The directory lives right after the
// 8-byte "VIS3" + count header, encrypted; see the header comment for its
// layout. Entries come out with no names and offsets made absolute.
bool TComposedFile::InitEntries() {
	wxCriticalSectionLocker locker(_criticalSection);

	if (!_needsInit && !_hadOpenError)
		return true;

	wxFileName path(_composedFilePath);
	TFile file;
	TMemoryBuffer buffer;

	// The "can't open" outcome - also what a too-large entry count reports.
	auto couldNotOpen = [&]() {
		_hadOpenError = true;
		if (onError)
			onError(this, "not found");
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Failed to initialize the composed file '%s'. Could not open.",
			                   path.GetFullPath().wc_str());
		_needsInit = false;
		return false;
	};
	auto notComposed = [&]() {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"The file %s is not a composed file.", path.GetFullPath().wc_str());
	};
	auto invalidHeader = [&]() {
		_needsInit = false;
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Failed to initialize the composed file '%s'. Invalid header.",
			                   path.GetFullPath().wc_str());
		return false;
	};

	if (!file.OpenRead(path))
		return couldNotOpen();

	if (file.ReadToBuf(buffer, 8) != 8) {
		notComposed();
		return false;
	}
	unsigned char magic[4];
	std::memcpy(magic, buffer.GetData(), 4);
	if (buffer.GetLen() != 8 || magic[0] != 'V' || magic[1] != 'I' || magic[2] != 'S' || magic[3] != '3') {
		notComposed();
		_needsInit = false;
		return false;
	}

	unsigned long entryCount = static_cast<unsigned long>(ByteStreamToLong(buffer.GetData() + 4));
	if (entryCount > 0x1869F)
		return couldNotOpen();

	_entries.clear();
	_entries.reserve(entryCount);

	unsigned long tableSize = entryCount * 16 + 6;
	buffer.Init(tableSize);
	if (file.ReadToBuf(buffer, tableSize) != tableSize || !buffer.Decrypt(wxString(_encryptionKey), nullptr, 0)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Failed to initialize the composed file '%s'. Invalid header. Should be VIS3, but "
			                   L"was %s",
			                   path.GetFullPath().wc_str(), std::wstring(magic, magic + 4).c_str());
		_needsInit = false;
		return false;
	}

	const unsigned char *data = buffer.GetData();
	if (data[0] != 'H' || data[1] != 'D' || data[2] != 'R')
		return invalidHeader();

	const unsigned char *record = data + 3;
	for (unsigned long i = 0; i < entryCount; i++, record += 16) {
		SEntryInfo entry;
		entry.containerOffset = ByteStreamToLong(record);
		entry.byteSize = ByteStreamToLong(record + 4);
		entry.uncompressedLength = ByteStreamToLong(record + 8);
		entry.contentFlags = ByteStreamToLong(record + 12);
		_entries.push_back(entry);
	}

	if (record[0] != 'E' || record[1] != 'N' || record[2] != 'D')
		return invalidHeader();

	// Offsets in the directory are relative to its own end.
	long long base = file.GetCurrentPos();
	for (SEntryInfo &entry : _entries)
		entry.containerOffset += base;
	file.Close();

	if (wxLog::loglevel > 2)
		wxLog::logexpanded(L"The composed file '%s' contains %ld files.", path.GetFullPath().wc_str(),
		                   static_cast<long>(entryCount));

	if (_hadOpenError && onRetryLoad) {
		onRetryLoad();
		_hadOpenError = false;
	}
	_needsInit = false;
	return true;
}

// Confirmed (asm lines 546098-546304): (re)points the container at a file and
// loads it; with an explicit volume number the file's extension becomes the
// numbered "<ext><NNN>" form first.
bool TComposedFile::Init(const wxFileName &composedFilePath, const wxString &encryptionKey, long offset) {
	_composedFilePath = composedFilePath.GetFullPath().ToStdWstring();
	_encryptionKey = encryptionKey.ToStdWstring();
	if (offset == -1) {
		_needsInit = true;
		return InitEntries();
	}

	wxFileName file(_composedFilePath);
	std::wstring extension = file.GetExt().ToStdWstring();
	if (offset >= 0)
		extension += padded(offset, "%03ld");
	file.SetExt(wxString(extension));
	_composedFilePath = file.GetFullPath().ToStdWstring();
	_needsInit = true;
	return true;
}

// Confirmed (asm lines 545563-545837): opens `outFile` onto entry `index`'s
// byte range within the container, carrying the entry's content flags and
// uncompressed length (and the container's key, which the file keeps for
// decrypting) - and remembering the name it was asked for.
bool TComposedFile::Open(TFile &outFile, const wxFileName &composedFilePath, long index) {
	if (_needsInit || _hadOpenError)
		InitEntries();

	if (index < 0 || index >= GetNumberOfEntries())
		return false;

	SEntryInfo entry = _entries[static_cast<size_t>(index)];

	wxFileName container(_composedFilePath);
	if (wxLog::loglevel > 2)
		wxLog::logexpanded(L"Loading file '%s' from composed file '%s'.", composedFilePath.GetFullPath().wc_str(),
		                   container.GetFullPath().wc_str());

	bool opened = outFile.OpenReadFromComposedFile(container, wxString(_encryptionKey),
	                                               static_cast<long>(entry.containerOffset),
	                                               static_cast<long>(entry.byteSize));
	outFile.SetContentFlags(static_cast<long>(entry.contentFlags));
	outFile.SetUncompressedLength(static_cast<long>(entry.uncompressedLength));
	outFile.SetOriginalPath(composedFilePath);
	return opened;
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

// Confirmed (asm lines 542364-543915) - see the declaration's comment for
// the overall shape. File layout written: [embedded exe, if its file exists]
// [8-byte "VIS3"+count header][directory placeholder][entry data...], then the
// real encrypted directory ("HDR", per-entry {offset, size, uncompressed
// length, flags}, "END") overwrites the placeholder and, for an embedded exe,
// a (exe length, "COM") footer is appended. Entry offsets in the directory
// are cumulative stored sizes, relative to the end of the directory.
// Entries given data by AddFile() are read from disk and run through
// TFile::PasteFile(); AddData() ones through PasteData(); a failed entry
// logs and is recorded with zero sizes rather than aborting the write. A
// PNG entry flagged "header encrypted" (8) has the width and height fields of
// its IHDR chunk (at +0x10/+0x14 of the file) overwritten with the encrypted
// dimensions, keyed by the file's own name.
bool TComposedFile::WriteToDisk(const wxFileName &path, const wxString &password, wxFile *log,
                                EventHandler *handler) {
	TFile out;
	wxFileName target = path;
	target.NormalizePath();
	if (!target.DirExists())
		target.Mkdir();

	wxString extension = GetComposedFileExtension();
	if (!extension.IsEmpty())
		target.SetExt(extension);

	if (log) {
		log->Write(wxString(L"C"));
		log->Write(wxString(L"-"));
	}

	if (!out.OpenWrite(target)) {
		out.Close();
		return false;
	}

	// An existing exe file is copied in verbatim first (and its length
	// recorded for the footer).
	wxFile exeFile;
	wxFileName exePath(_exeFile);
	int exeLength = 0;
	if (wxFile::Exists(exePath.GetFullPath()) && exeFile.Open(exePath.GetFullPath(), 1)) {
		exeLength = static_cast<int>(exeFile.Length());
		unsigned long copied = 0;
		out.PasteFile(exePath, password, 0, copied);
	}

	TMemoryBuffer directory;
	unsigned long directoryLength = 0;
	long directoryPosition = 0;
	if (_flag34) {
		directoryLength = static_cast<unsigned long>(_entries.size()) * 16 + 6;
		directory.Init(8);
		directory << 'V' << 'I' << 'S' << '3';
		directory << static_cast<long>(_entries.size());
		out.WriteFromBuf(directory, false);

		// Reserve the directory's space; it's overwritten below once every
		// entry's offset is known.
		directory.Init(directoryLength);
		static const unsigned char kBlank[16] = {0};
		directory.AppendData(kBlank, 3);
		for (size_t i = 0; i < _entries.size(); i++)
			directory.AppendData(kBlank, 16);
		directory.AppendData(kBlank, 3);
		out.WriteFromBuf(directory, false);
		directoryPosition = 8;
	}

	int index = 0;
	for (SEntryInfo &entry : _entries) {
		wxFileName name(entry.filename);
		name.NormalizePath();
		unsigned long storedLength = static_cast<unsigned long>(entry.byteSize);
		long long uncompressedLength = entry.byteSize;
		long long flags = entry.contentFlags;

		if (log)
			log->Write(name.GetFullPath());

		if (entry.memoryData) {
			if (!out.PasteData(*entry.memoryData, password, static_cast<long>(flags), storedLength)) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"F");
				storedLength = 0;
				uncompressedLength = 0;
			}
		} else {
			long position = out.GetCurrentPos();
			if (!out.PasteFile(name, password, static_cast<long>(flags), storedLength)) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"F");
				storedLength = 0;
				uncompressedLength = 0;
			} else if (flags & kContentPNGHeaderEncrypted) {
				long resume = out.GetCurrentPos();
				TFile source;
				if (!source.OpenRead(name)) {
					if (wxLog::loglevel >= 0)
						wxLog::logexpanded(L"F");
					flags &= ~static_cast<long long>(kContentPNGHeaderEncrypted);
				} else {
					unsigned long width = 0, height = 0;
					source.Seek(0x10, 0);
					source.ReadLong(width);
					source.Seek(0x14, 0);
					source.ReadLong(height);
					source.Close();

					TMemoryBuffer dimensions;
					dimensions << width;
					dimensions << height;
					dimensions.Encrypt(name.GetName(), nullptr, 0);
					const unsigned char *encrypted = dimensions.GetData();

					TMemoryBuffer part;
					part.AppendData(encrypted, 4);
					out.Seek(static_cast<unsigned long>(position + 0x10), 0);
					out.WriteFromBuf(part, true);
					part.ReleaseMemory();
					part.AppendData(encrypted + 4, 4);
					out.Seek(static_cast<unsigned long>(position + 0x14), 0);
					out.WriteFromBuf(part, true);
					part.ReleaseMemory();
					dimensions.ReleaseMemory();
					out.Seek(static_cast<unsigned long>(resume), 0);
				}
			}
		}

		entry.filename = name.GetFullPath().ToStdWstring();
		entry.byteSize = static_cast<long long>(storedLength);
		entry.uncompressedLength = uncompressedLength;
		entry.contentFlags = flags;

		if (handler) {
			BuildProgressEvent progress(3, index);
			handler->AddPendingEvent(&progress);
		}
		index++;
	}

	if (_flag34) {
		out.Seek(static_cast<unsigned long>(directoryPosition), 0);
		directory.Init(directoryLength);
		directory << 'H' << 'D' << 'R';
		long offset = 0;
		for (SEntryInfo &entry : _entries) {
			entry.containerOffset = offset;
			directory << static_cast<unsigned long>(offset);
			directory << static_cast<unsigned long>(entry.byteSize);
			directory << static_cast<unsigned long>(entry.uncompressedLength);
			directory << static_cast<long>(entry.contentFlags);
			offset += static_cast<long>(entry.byteSize);
		}
		directory << 'E' << 'N' << 'D';

		if (!directory.Encrypt(password, nullptr, 0)) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"F");
			out.Close();
			return false;
		}
		out.WriteFromBuf(directory, true);
	}

	if (exeLength != 0) {
		TMemoryBuffer footer;
		footer.Init(8);
		footer << static_cast<long>(exeLength);
		footer << 'C' << 'O' << 'M';
		out.WriteFromBuf(footer, false);
	}

	out.Close();
	if (log)
		log->Write(wxString(L"-"));
	return true;
}

bool TComposedFile::WriteToDisk(const wxString &password, wxFile *log, EventHandler *handler) {
	return WriteToDisk(wxFileName(_composedFilePath), password, log, handler);
}

// Confirmed (asm lines 543953-544169).
bool TComposedFile::Export(long index, wxFileName &outPath, const wxString &a, const wxString &b) {
	if (index < 0 || index >= GetNumberOfEntries())
		return false;

	const SEntryInfo &entry = _entries[static_cast<size_t>(index)];
	TFile file;
	if (!file.OpenReadFromComposedFile(wxFileName(_composedFilePath), wxString(_encryptionKey),
	                                   static_cast<long>(entry.containerOffset), static_cast<long>(entry.byteSize)))
		return false;
	file.SetContentFlags(static_cast<long>(entry.contentFlags));

	outPath = TTempFile::AddTempFile(a, b);
	return file.WriteToFile(outPath);
}

// Confirmed (asm lines 546304-546627).
int TComposedFile::AddData(const TMemoryBuffer &data, wxFileName name, wxFileName &outName, int flags) {
	long index = static_cast<long>(_entries.size());

	SEntryInfo entry;
	entry.filename = name.GetFullPath().ToStdWstring();
	entry.memoryData = const_cast<TMemoryBuffer *>(&data);
	entry.byteSize = static_cast<long long>(data.GetLen());
	entry.contentFlags = flags;
	_entries.push_back(entry);

	outName = name;
	outName.SetExt(GetContainerFileExtension(name.GetExt(), index));
	return 1;
}

// Confirmed (asm lines 546627-548209). The reference extension depends on the
// container type: the embedded-exe layouts use "<ext>#<volume>#m00000#" (type
// 8) and "<ext>#<volume>#<index, 6 digits>#" (type 7); every other type
// "<ext>#<letter>#<volume>#<index>#" as GetContainerFileExtension() does. A
// file already in `names` is not added again - its earlier reference is
// returned. A file that doesn't exist (or has no name) is logged and refused.
int TComposedFile::AddFile(wxFileName file, wxFileName &outName, StringHashMap &names, int flags) {
	std::wstring key = file.GetFullPath().ToStdWstring();

	auto known = names.find(key);
	if (known != names.end()) {
		outName.Assign(wxString(known->second));
		return 1;
	}

	if (!file.IsOk() || file.GetName().IsEmpty() || !wxFile::Exists(wxString(key))) {
		if (!key.empty() && wxLog::loglevel > 0)
			wxLog::logexpanded(L"Failed to load file '%s'. File was not added.", file.GetFullPath().wc_str());
		return 0;
	}

	long index = static_cast<long>(_entries.size());
	SEntryInfo entry;
	entry.filename = key;
	entry.byteSize = wxFile(wxString(key)).Length();
	entry.contentFlags = flags;
	_entries.push_back(entry);

	std::wstring extension = file.GetExt().ToStdWstring();
	if (_containerType == TContainerTypeEnum::kEmbeddedInExecutableAlt)
		extension += L'#' + padded(_offset, "%03ld") + L"#m00000#";
	else if (_containerType == TContainerTypeEnum::kEmbeddedInExecutable)
		extension += L'#' + padded(_offset, "%03ld") + L'#' + padded(index, "%06ld") + L'#';
	else
		extension = GetContainerFileExtension(wxString(extension), index).ToStdWstring();

	outName = file;
	outName.SetExt(wxString(extension));
	names[key] = outName.GetFullPath().ToStdWstring();
	return 1;
}

// Confirmed (asm lines 548209-548597).
bool TComposedFile::AddComposedFile(const wxFileName &file, std::vector<wxFileName> &outList) {
	if (file.GetName().IsEmpty())
		return false;

	if (!file.IsOk()) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Invalid file path '%s'. Composed file was not added.", file.GetFullPath().wc_str());
		return false;
	}

	wxFileName composed = file;
	composed.NormalizePath();
	composed.SetExt(GetComposedFileExtension());
	outList.push_back(composed);
	return true;
}

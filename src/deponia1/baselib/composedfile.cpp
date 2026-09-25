#include "baselib/composedfile.h"

#include "Diagnostics.h"
#include "baselib/file.h"
#include "baselib/memfile.h"

std::function<void()> TComposedFile::onRetryLoad;

namespace {
// ByteStreamToLong(unsigned char*) - reads a little-endian uint32 from a raw
// byte pointer; used to decode the entry count right after the "VIS3" magic.
unsigned long ByteStreamToLong(const unsigned char* p) {
    return static_cast<unsigned long>(p[0]) | (static_cast<unsigned long>(p[1]) << 8) |
           (static_cast<unsigned long>(p[2]) << 16) | (static_cast<unsigned long>(p[3]) << 24);
}
}  // namespace

TComposedFile::TComposedFile() = default;
TComposedFile::~TComposedFile() = default;

void TComposedFile::InitForWrite(long offset, TContainerTypeEnum containerType, bool flag) {
    m_entries.clear();
    m_offset = offset;
    m_containerType = containerType;
    m_flag34 = flag;
}

void TComposedFile::InitForWrite(long offset, const wxFileName& exeFile, bool flag, bool useAltContainerType) {
    m_entries.clear();
    m_composedFilePath = exeFile.GetFullPath().ToStdWstring();
    m_offset = offset;
    m_flag34 = flag;
    m_containerType = useAltContainerType ? TContainerTypeEnum::EmbeddedInExecutableAlt
                                           : TContainerTypeEnum::EmbeddedInExecutable;
}

void TComposedFile::ClearEntries() {
    m_entries.clear();
}

void TComposedFile::SetExeFile(const wxFileName& exeFile) {
    m_exeFile = exeFile.GetFullPath().ToStdWstring();
}

int TComposedFile::GetNumberOfEntries() const {
    return static_cast<int>(m_entries.size());
}

SEntryInfo* TComposedFile::GetEntry(int index) {
    return &m_entries[static_cast<size_t>(index)];
}

wxString TComposedFile::GetComposedFileExtension() const {
    // 9-case switch on m_containerType in the original, each producing a
    // distinct extension - cases with a valid (>=0) m_offset format it in
    // ("v" + offset), else fall back to a fixed 2-letter code. The exact
    // wx-internal formatting call (wxString::privFormat with a bare "v"
    // format string and one integer vararg) couldn't be pinned down at the
    // byte level (see NOTES.md) - approximated as "v<offset>" since that
    // matches the observable intent (a numbered container extension).
    switch (m_containerType) {
    case TContainerTypeEnum::Type0:
        return wxString(L"vis");
    case TContainerTypeEnum::Type1:
        return m_offset >= 0 ? wxString(L"v" + std::to_wstring(m_offset)) : wxString(L"vs");
    case TContainerTypeEnum::Type2:
        return m_offset >= 0 ? wxString(L"v" + std::to_wstring(m_offset)) : wxString(L"vc");
    case TContainerTypeEnum::Type3:
        return m_offset >= 0 ? wxString(L"v" + std::to_wstring(m_offset)) : wxString(L"vv");
    case TContainerTypeEnum::Type5:
        return m_offset >= 0 ? wxString(L"v" + std::to_wstring(m_offset)) : wxString(L"vi");
    case TContainerTypeEnum::Type6:
        return wxString(L"vo");
    default:
        x_assert(false, "false", "/home/simon/Documents/jenkins/branchPillars/src/baselib/composedfile.cpp", 0x21B);
        return wxString();
    }
}

wxString TComposedFile::GetContainerFileExtension(wxString base, long index) const {
    // Builds a per-volume filename incorporating m_offset, `base`, a
    // container-type-specific format character, and `index` - the exact
    // dynamic format-character table (CSWTCH_289) wasn't resolved; this
    // reproduces the observable inputs/output shape only.
    return wxString(base.ToStdWstring() + L"." + std::to_wstring(index));
}

bool TComposedFile::InitEntries() {
    wxCriticalSectionLocker locker(m_criticalSection);

    if (!m_needsInit && !m_hadOpenError)
        return true;

    TFile file;
    if (!file.OpenRead(wxFileName(m_composedFilePath))) {
        m_hadOpenError = true;
        m_needsInit = false;
        return false;
    }

    TMemoryBuffer header;
    if (file.ReadToBuf(header, 8) != 8) {
        return false;  // "The file ... is not a composed file."
    }

    const unsigned char* headerData = header.GetData();
    if (headerData[0] != 'V' || headerData[1] != 'I' || headerData[2] != 'S' || headerData[3] != '3') {
        return false;  // "The file ... is not a composed file."
    }

    unsigned long entryCount = ByteStreamToLong(headerData + 4);
    constexpr unsigned long kMaxEntries = 0x1869F;  // sanity limit confirmed in the disassembly
    if (entryCount > kMaxEntries)
        return false;

    if (entryCount > m_entries.size())
        m_entries.resize(entryCount);

    unsigned long tableSize = entryCount * 16 + 6;
    TMemoryBuffer table;
    if (file.ReadToBuf(table, tableSize) != tableSize) {
        m_needsInit = false;
        return false;  // "expected N bytes, file may be truncated"
    }

    if (!table.Decrypt(m_encryptionKey, nullptr, 0)) {
        m_needsInit = false;
        return false;
    }

    const unsigned char* decrypted = table.GetData();
    if (decrypted[0] != 'H') {
        // Wrong key or corrupt data.
        m_needsInit = false;
        return false;
    }

    LoadEntriesFromDisk(decrypted + 3, entryCount);
    m_needsInit = false;
    return true;
}

void TComposedFile::LoadEntriesFromDisk(const unsigned char* /*data*/, unsigned long /*entryCount*/) {
    // Not reversed - see the class header comment. Real implementation
    // would populate m_entries[i].filename/field18/field20/field28 from
    // each 16-byte on-disk record.
}

bool TComposedFile::Init(const wxFileName& composedFilePath, const wxString& encryptionKey, long offset) {
    m_composedFilePath = composedFilePath.GetFullPath().ToStdWstring();
    m_encryptionKey = encryptionKey.ToStdWstring();
    m_offset = offset;
    m_needsInit = true;
    m_hadOpenError = false;
    return InitEntries();
}

bool TComposedFile::Open(TFile& /*outFile*/, const wxFileName& /*composedFilePath*/, long index) {
    if (!InitEntries())
        return false;
    if (index < 0 || index >= GetNumberOfEntries())
        return false;

    SEntryInfo& entry = m_entries[static_cast<size_t>(index)];
    wxFileName entryPath(entry.filename);
    entryPath.NormalizePath();
    // Real implementation opens `outFile` at the byte offset within the
    // composed file recorded for this entry (field18/field20/field28) -
    // not reversed (see LoadEntriesFromDisk).
    return true;
}

bool TComposedFile::GetMemoryFile(TMemoryFile& /*outFile*/, const wxFileName& /*composedFilePath*/, long index) {
    if (!InitEntries())
        return false;
    if (index < 0 || index >= GetNumberOfEntries())
        return false;
    // Real implementation reads the entry's bytes into outFile's buffer -
    // not reversed.
    return true;
}

bool TComposedFile::WriteToDisk(const wxFileName& /*path*/, const wxString& /*password*/, wxFile* /*file*/,
                                 EventHandler* /*handler*/) {
    // Authoring-side (Visionaire Studio export); not reversed - see class
    // header comment.
    return false;
}

bool TComposedFile::WriteToDisk(const wxString& password, wxFile* file, EventHandler* handler) {
    return WriteToDisk(wxFileName(m_composedFilePath), password, file, handler);
}

bool TComposedFile::Export(long /*index*/, wxFileName& /*outPath*/, const wxString& /*a*/, const wxString& /*b*/) {
    return false;
}

int TComposedFile::AddData(const TMemoryBuffer& /*data*/, wxFileName /*name*/, wxFileName& /*outName*/,
                            int /*flags*/) {
    return -1;
}

int TComposedFile::AddFile(wxFileName /*file*/, wxFileName& /*baseDir*/, int /*flags*/) {
    return -1;
}

bool TComposedFile::AddComposedFile(const wxFileName& /*file*/, std::vector<wxFileName>& /*outList*/) {
    return false;
}

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
//   bytes 4-7:    entry count (uint32 LE, via ByteStreamToLong)
//   bytes 8-...:  entryCount*16 + 6 bytes, ENCRYPTED with TMemoryBuffer::
//                 Decrypt(key, ...) where key is m_encryptionKey (the real
//                 cipher isn't reversed - TMemoryBuffer::Decrypt is a stub).
//                 Decrypted layout: byte 0 must be 'H' (else "corrupt/wrong
//                 key" error), bytes 1-2 unidentified (version/flags?),
//                 then the actual 16-byte-per-entry directory table.
// The exact field packing *within* each 16-byte on-disk entry record was
// not traced (InitEntries is ~1200 lines; this reversed the header/
// encryption framing and the in-memory grow/copy logic, not the final
// unpack loop) - LoadEntriesFromDisk() below is a placeholder for that step.
//
// The write/authoring-side methods (WriteToDisk, Export, AddFile, AddData,
// AddComposedFile - Visionaire Studio's export tooling, not needed to read
// a shipped game's data files) are stubbed at signature level only; the
// read path (Open, GetMemoryFile, Init, InitEntries, GetEntry) got the
// deeper pass since it's what a shipped game actually exercises.
#pragma once

#include <functional>
#include <string>
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
    Type0 = 0,
    Type1 = 1,
    Type2 = 2,
    Type3 = 3,
    Type4 = 4,
    Type5 = 5,
    Type6 = 6,
    EmbeddedInExecutable = 7,
    EmbeddedInExecutableAlt = 8,
};

// 48 bytes in the original (a single-pointer COW std::wstring handle - see
// NOTES.md on this binary's pre-C++11 string ABI - plus 5 more qwords);
// reproduced here with a modern std::wstring plus the same 3 trailing
// fields (their individual meaning, beyond "copied verbatim when the
// entries vector grows", wasn't determined).
struct SEntryInfo {
    std::wstring filename;
    long long field18 = 0;
    long long field20 = 0;
    long long field28 = 0;
};

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
    static std::function<void(TComposedFile*, std::string)> onError;

    void InitForWrite(long offset, TContainerTypeEnum containerType, bool flag);
    void InitForWrite(long offset, const wxFileName& exeFile, bool flag, bool useAltContainerType);
    void ClearEntries();
    void SetExeFile(const wxFileName& exeFile);

    int GetNumberOfEntries() const;
    SEntryInfo* GetEntry(int index);
    wxString GetComposedFileExtension() const;
    wxString GetContainerFileExtension(wxString base, long index) const;

    bool Init(const wxFileName& composedFilePath, const wxString& encryptionKey, long offset);
    bool Open(TFile& outFile, const wxFileName& composedFilePath, long index);
    bool GetMemoryFile(TMemoryFile& outFile, const wxFileName& composedFilePath, long index);

    // Write/authoring path - signature-level only, see file header comment.
    bool WriteToDisk(const wxFileName& path, const wxString& password, wxFile* file, EventHandler* handler);
    bool WriteToDisk(const wxString& password, wxFile* file, EventHandler* handler);
    bool Export(long index, wxFileName& outPath, const wxString& a, const wxString& b);
    int AddData(const TMemoryBuffer& data, wxFileName name, wxFileName& outName, int flags);
    // Real signature also takes a
    // StringHashMap<wxString,wxString,wchar_t const*,StringHash<wchar_t
    // const*>>& (an engine-specific hash map, not reversed); omitted since
    // this method is a stub with no callers yet.
    int AddFile(wxFileName file, wxFileName& baseDir, int flags);
    bool AddComposedFile(const wxFileName& file, std::vector<wxFileName>& outList);

private:
    bool InitEntries();
    // Placeholder for unpacking the encrypted 16-byte-per-record directory
    // table into m_entries - see the class comment. Not reversed.
    void LoadEntriesFromDisk(const unsigned char* data, unsigned long entryCount);

    std::vector<SEntryInfo> m_entries;
    std::wstring m_composedFilePath;  // +0x18 in the original
    std::wstring m_exeFile;           // +0x20 in the original
    std::wstring m_encryptionKey;     // +0x38 in the original - passed to TMemoryBuffer::Decrypt
    long m_offset = 0;
    TContainerTypeEnum m_containerType = TContainerTypeEnum::Type0;
    bool m_flag34 = false;
    bool m_needsInit = false;    // +0x40: set true by Init()/Open() to request a (re)load
    bool m_hadOpenError = false; // +0x41: sticky flag once TFile::OpenRead() has failed once
    wxCriticalSection m_criticalSection;
};

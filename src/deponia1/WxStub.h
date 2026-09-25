// Stand-in for the (customized) wxWidgets subset used by main().
//
// The real wxString in this binary is NOT stock wxWidgets: cross-referencing
// the disassembly shows a class whose data is layout-compatible with
// std::wstring (main+0x37D constructs a std::wstring directly from a
// wxString's address) but whose destructor symbol resolves to
// `std::pair<std::string const,ulong>::~pair`. That symbol is almost
// certainly an artifact of linker identical-code-folding (ICF): many
// unrelated small destructors compile to the same instruction sequence at
// -O2 and get merged into one symbol, so the demangled name IDA shows is not
// reliable evidence of the type's real fields. This matches the
// "descendant of templated base" confusion mentioned for this class. See
// NOTES.md for the full writeup.
//
// For this stub phase we only need something that compiles and behaves
// plausibly; exact field layout can be revisited once we're implementing
// wxString for real.
#pragma once

#include <cstdio>
#include <string>

class wxString {
public:
    wxString() = default;
    wxString(const wchar_t* s) : m_data(s) {}
    wxString(const char* s) : m_data(s, s + std::char_traits<char>::length(s)) {}
    wxString(const std::wstring& s) : m_data(s) {}

    const std::wstring& ToStdWstring() const { return m_data; }
    const wchar_t* wc_str() const { return m_data.c_str(); }
    const wchar_t* c_str() const { return m_data.c_str(); }

    bool IsEmpty() const { return m_data.empty(); }

    wxString& operator+=(const wxString& rhs) {
        m_data += rhs.m_data;
        return *this;
    }
    friend wxString operator+(wxString lhs, const wxString& rhs) {
        lhs += rhs;
        return lhs;
    }

    // Stands in for wxString::mb_str(); real wxWidgets returns a
    // wxScopedCharBuffer that is implicitly convertible to const char*.
    class CharBuffer {
    public:
        explicit CharBuffer(std::string s) : m_narrow(std::move(s)) {}
        operator const char*() const { return m_narrow.c_str(); }

    private:
        std::string m_narrow;
    };

    CharBuffer mb_str() const {
        std::string narrow(m_data.begin(), m_data.end());
        return CharBuffer(std::move(narrow));
    }

private:
    std::wstring m_data;
};

class wxFileName {
public:
    wxFileName() = default;
    explicit wxFileName(const std::wstring& fullPath) : m_fullPath(fullPath) {}

    wxString GetFullPath() const { return wxString(m_fullPath); }
    bool IsOk() const { return !m_fullPath.empty(); }
    void NormalizePath() {}

    static bool Mkdir(const wxString& dir, int permissions = 0777, int flags = 0);
    static wxString GetCwd();

private:
    std::wstring m_fullPath;
};

class wxDir {
public:
    static bool Exists(const wxString& path);
};

struct wxSize {
    int width = 0;
    int height = 0;
};

struct wxPoint {
    int x = 0;
    int y = 0;
};

struct wxRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;

    int GetWidth() const { return width; }
    int GetLeft() const { return x; }
    int GetTop() const { return y; }
};

class wxCmdLineParser {
public:
    wxCmdLineParser() = default;
    ~wxCmdLineParser() = default;

    // Stub always reports the option as not present.
    bool Found(const wxString& /*name*/, wxString* /*value*/) const { return false; }
};

class wxFile {
public:
    static bool Exists(const wxString& path);
};

class wxLog {
public:
    // Signed so `js` (jump-if-negative) checks against it stay meaningful.
    static int loglevel;

    static void SetVerbose(bool verbose);
    static void SetActiveTarget(class wxLogStderr* target);
    static void logexpanded(const wchar_t* fmt, ...);
};

class wxLogStderr {
public:
    explicit wxLogStderr(std::FILE* fp) : m_fp(fp) {}

private:
    std::FILE* m_fp;
};

bool wxInitialize();
wxString wxConvertMB2WX(const char* s);

// toUTF(wxString*, const char*) - converts a narrow (assumed UTF-8) C string
// into a wxString, writing into the caller-provided output parameter (this
// matches the calling convention seen at every reversed call site so far).
void toUTF(wxString* out, const char* utf8);

// Stand-in for wxWidgets' wxStandardPathsBase, which real TStandardPaths
// (see TStandardPaths.h) holds a pointer to and forwards most calls to.
// TODO: replace with the real wxWidgets wxStandardPaths singleton once
// wxWidgets is vendored as a real dependency (manifest/README.md) - these
// path rules are placeholders, not reversed from the binary.
class wxStandardPathsBase {
public:
    virtual ~wxStandardPathsBase() = default;
    virtual wxString GetUserDataDir() const;
    virtual wxString GetUserLocalDataDir() const;
    virtual wxString GetTempDir() const;
    virtual wxString GetExecutablePath() const;
};

// The real binary initializes TStandardPaths's wxStandardPathsBase pointer
// from a fixed global (`wxGUIAppTraits::base`) rather than a dynamic lookup;
// this mirrors that with a single process-wide stub instance.
wxStandardPathsBase& wxGetAppTraitsStandardPaths();

// wxCriticalSection is a plain mutex wrapper in real wxWidgets; several
// classes (e.g. TComposedFile) embed one as an instance member.
class wxCriticalSection {
public:
    void Enter();
    void Leave();
};

class wxCriticalSectionLocker {
public:
    explicit wxCriticalSectionLocker(wxCriticalSection& cs) : m_cs(cs) { m_cs.Enter(); }
    ~wxCriticalSectionLocker() { m_cs.Leave(); }

private:
    wxCriticalSection& m_cs;
};

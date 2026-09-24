#include "WxStub.h"

#include <cerrno>
#include <cstdarg>
#include <cstdio>
#include <direct.h>
#include <sys/stat.h>

int wxLog::loglevel = 0;

void wxLog::SetVerbose(bool /*verbose*/) {
}

void wxLog::SetActiveTarget(wxLogStderr* /*target*/) {
    // Real wxWidgets takes ownership of the previous target and deletes it;
    // the stub intentionally does nothing with the pointer.
}

void wxLog::logexpanded(const wchar_t* fmt, ...) {
    // The call sites reversed so far all use plain "%s" for a wchar_t*
    // argument, following glibc's wide-printf convention (where wide %s
    // *is* wchar_t*). MSVCRT/UCRT's wide-printf instead treats %s as
    // char* and %ls as wchar_t*, so left as-is this silently misreads the
    // argument and truncates output to one character. Normalize %s -> %ls
    // so the reversed format strings behave the same on this platform.
    std::wstring portableFmt;
    for (const wchar_t* p = fmt; *p; ++p) {
        portableFmt += *p;
        if (*p == L'%' && p[1] == L's') {
            portableFmt += L'l';
        }
    }

    va_list args;
    va_start(args, fmt);
    std::vfwprintf(stderr, portableFmt.c_str(), args);
    va_end(args);
    std::fputws(L"\n", stderr);
}

bool wxInitialize() {
    return true;
}

wxString wxConvertMB2WX(const char* s) {
    return wxString(s);
}

void toUTF(wxString* out, const char* utf8) {
    *out = wxString(utf8);
}

bool wxFile::Exists(const wxString& path) {
    struct stat st;
    return ::stat(static_cast<const char*>(path.mb_str()), &st) == 0;
}

bool wxDir::Exists(const wxString& path) {
    struct stat st;
    return ::stat(static_cast<const char*>(path.mb_str()), &st) == 0 && (st.st_mode & _S_IFDIR);
}

bool wxFileName::Mkdir(const wxString& dir, int /*permissions*/, int /*flags*/) {
    // `flags` (wxPATH_MKDIR_FULL for recursive creation) is ignored here,
    // matching the one call site we've reversed so far (TStandardPaths
    // always passes flags=0); revisit if a later class needs recursive
    // creation.
    return _wmkdir(dir.wc_str()) == 0 || errno == EEXIST;
}

wxString wxFileName::GetCwd() {
    wchar_t buf[1024];
    return wxString(_wgetcwd(buf, 1024) ? buf : L".");
}

// TStandardPaths::BuildDir (TStandardPaths.cpp) concatenates these directly
// with "CompanyName/" et al and no separator in between - reversed straight
// from the disassembly - so real wxStandardPathsBase::GetUserDataDir() must
// itself return a trailing-separator path. Matching that contract here so
// the stub produces sane nested directories.
//
// TStandardPaths::BuildDir also only ever Mkdir()s non-recursively (matching
// the one flags=0 call site reversed so far), which is fine in production
// where this returns an already-existing system directory - but means our
// placeholder base dir needs to already exist too, so the stub creates it
// on first use.
static wxString EnsureBaseDir(const wchar_t* path) {
    _wmkdir(path);
    return wxString(path);
}

wxString wxStandardPathsBase::GetUserDataDir() const {
    return EnsureBaseDir(L"./userdata/");
}

wxString wxStandardPathsBase::GetUserLocalDataDir() const {
    return EnsureBaseDir(L"./userdata_local/");
}

wxString wxStandardPathsBase::GetTempDir() const {
    return EnsureBaseDir(L"./tmp/");
}

wxString wxStandardPathsBase::GetExecutablePath() const {
    wchar_t buf[1024];
    return wxString(_wgetcwd(buf, 1024) ? buf : L".");
}

wxStandardPathsBase& wxGetAppTraitsStandardPaths() {
    static wxStandardPathsBase instance;
    return instance;
}

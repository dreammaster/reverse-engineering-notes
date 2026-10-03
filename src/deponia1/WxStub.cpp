#include "WxStub.h"

#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <direct.h>
#include <filesystem>
#include <sys/stat.h>
#include <thread>

int wxLog::loglevel = 0;

void wxLog::SetVerbose(bool /*verbose*/) {
}

void wxLog::SetActiveTarget(wxLogStderr */*target*/) {
	// Real wxWidgets takes ownership of the previous target and deletes it;
	// the stub intentionally does nothing with the pointer.
}

void wxLog::logexpanded(const wchar_t *fmt, ...) {
	// The call sites reversed so far all use plain "%s" for a wchar_t*
	// argument, following glibc's wide-printf convention (where wide %s
	// *is* wchar_t*). MSVCRT/UCRT's wide-printf instead treats %s as
	// char* and %ls as wchar_t*, so left as-is this silently misreads the
	// argument and truncates output to one character. Normalize %s -> %ls
	// so the reversed format strings behave the same on this platform.
	std::wstring portableFmt;
	for (const wchar_t *p = fmt; *p; ++p) {
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

wxString wxConvertMB2WX(const char *s) {
	return wxString(s);
}

long long wxGetLocalTimeMillis() {
	using namespace std::chrono;
	return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

void wxMilliSleep(long ms) {
	std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void toUTF(wxString *out, const char *utf8) {
	*out = wxString(utf8);
}

wxString CONVTOSTR(const int &value) {
	return wxString(std::to_wstring(value));
}

bool wxFile::Exists(const wxString &path) {
	struct stat st;
	return ::stat(static_cast<const char *>(path.mb_str()), &st) == 0;
}

wxFile::wxFile(const wxString &path) {
	_handle = std::fopen(static_cast<const char *>(path.mb_str()), "rb");
}

wxFile::~wxFile() {
	Close();
}

long wxFile::Length() const {
	if (!_handle)
		return 0;
	long pos = std::ftell(_handle);
	std::fseek(_handle, 0, SEEK_END);
	long len = std::ftell(_handle);
	std::fseek(_handle, pos, SEEK_SET);
	return len;
}

bool wxFile::Open(const wxString &path, const wxString &mode) {
	Close();
	std::string narrowMode(static_cast<const char *>(mode.mb_str()));
	if (narrowMode.find('b') == std::string::npos)
		narrowMode += 'b';
	_handle = std::fopen(static_cast<const char *>(path.mb_str()), narrowMode.c_str());
	return _handle != nullptr;
}

bool wxFile::Eof() const {
	return !_handle || std::feof(_handle) != 0;
}

bool wxFileName::DirExists() const {
	return wxDir::Exists(wxString(GetPath()));
}

bool wxFileName::Mkdir() const {
	return Mkdir(wxString(GetPath()));
}

unsigned long wxFile::Write(const wxString &text) {
	std::string narrow(static_cast<const char *>(text.mb_str()));
	return Write(narrow.data(), static_cast<unsigned long>(narrow.size()));
}

bool wxFileName::MakeAbsolute() {
	std::error_code ec;
	std::filesystem::path absolute = std::filesystem::absolute(_fullPath, ec);
	if (ec)
		return false;
	_fullPath = absolute.wstring();
	return true;
}

bool wxFile::Open(const wxString &path, int mode) {
	Close();
	_handle = std::fopen(static_cast<const char *>(path.mb_str()), mode == 2 ? "r+b" : "rb");
	return _handle != nullptr;
}

unsigned long wxFile::Read(char *buffer, unsigned long size) {
	if (!_handle)
		return 0;
	return static_cast<unsigned long>(std::fread(buffer, 1, size, _handle));
}

unsigned long wxFile::Write(const void *buffer, unsigned long size) {
	if (!_handle)
		return 0;
	return static_cast<unsigned long>(std::fwrite(buffer, 1, size, _handle));
}

void wxFile::Flush() {
	if (_handle)
		std::fflush(_handle);
}

bool wxFile::Seek(unsigned long offset, int mode) {
	if (!_handle)
		return false;
	return std::fseek(_handle, static_cast<long>(offset), mode == 0 ? SEEK_SET : mode) == 0;
}

void wxFile::Close() {
	if (_handle) {
		std::fclose(_handle);
		_handle = nullptr;
	}
}

bool wxDir::Exists(const wxString &path) {
	struct stat st;
	return ::stat(static_cast<const char *>(path.mb_str()), &st) == 0 && (st.st_mode & _S_IFDIR);
}

bool wxDir::Open(const wxString &path) {
	_all.clear();
	_entries.clear();
	_index = 0;
	std::error_code ec;
	if (!std::filesystem::is_directory(path.ToStdWstring(), ec))
		return false;
	for (const auto &entry : std::filesystem::directory_iterator(path.ToStdWstring(), ec))
		_all.push_back(entry.path().filename().wstring());
	return true;
}

static bool wildcardMatch(const wchar_t *pattern, const wchar_t *name) {
	for (; *pattern; pattern++) {
		if (*pattern == L'*') {
			while (pattern[1] == L'*')
				pattern++;
			for (const wchar_t *tail = name;; tail++) {
				if (wildcardMatch(pattern + 1, tail))
					return true;
				if (!*tail)
					return false;
			}
		}
		if (!*name || (*pattern != L'?' && *pattern != *name))
			return false;
		name++;
	}
	return *name == L'\0';
}

bool wxDir::GetFirst(wxString *filename, const wxString &filespec, int /*flags*/) {
	_index = 0;
	// A wildcard-less (or empty) spec means "everything", as in wxWidgets.
	_entries.clear();
	std::wstring spec = filespec.ToStdWstring().empty() ? L"*" : filespec.ToStdWstring();
	for (const std::wstring &name : _all) {
		if (wildcardMatch(spec.c_str(), name.c_str()))
			_entries.push_back(name);
	}
	return GetNext(filename);
}

bool wxDir::GetNext(wxString *filename) {
	if (_index >= _entries.size())
		return false;
	*filename = wxString(_entries[_index++]);
	return true;
}

bool wxRemoveFile(wxString file) {
	return std::remove(static_cast<const char *>(file.mb_str())) == 0;
}

struct tm *wxDateTime::GetTmNow() {
	static struct tm now;
	std::time_t t = std::time(nullptr);
	return localtime_s(&now, &t) == 0 ? &now : nullptr;
}

bool wxFileName::Exists() const {
	return wxFile::Exists(GetFullPath());
}

bool wxFileName::Mkdir(const wxString &dir, int /*permissions*/, int /*flags*/) {
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

void wxFileName::SetCwd() const {
	std::wstring dir = GetPath();
	if (!dir.empty())
		_wchdir(dir.c_str());
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
static wxString EnsureBaseDir(const wchar_t *path) {
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

wxStandardPathsBase &wxGetAppTraitsStandardPaths() {
	static wxStandardPathsBase instance;
	return instance;
}

void wxCriticalSection::Enter() {
}

void wxCriticalSection::Leave() {
}

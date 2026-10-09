#include "WxStub.h"

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <cwctype>
#include <direct.h>
#include <filesystem>
#include <sys/stat.h>
#include <string>
#include <thread>

int wxLog::loglevel = 0;

void wxLog::SetVerbose(bool /*verbose*/) {
}

static wxLogStderr *g_activeLogTarget = nullptr;

wxLogStderr *wxLog::SetActiveTarget(wxLogStderr *target) {
	wxLogStderr *previous = g_activeLogTarget;

	g_activeLogTarget = target;
	return previous;
}

// The text as UTF-8 (the log file is a narrow stream).
static std::string toUtf8(const std::wstring &text) {
	std::string out;

	for (size_t i = 0; i < text.size(); i++) {
		unsigned long c = text[i];

		if (c >= 0xD800 && c < 0xDC00 && i + 1 < text.size() && text[i + 1] >= 0xDC00 && text[i + 1] < 0xE000)
			c = 0x10000 + ((c - 0xD800) << 10) + (text[++i] - 0xDC00);

		if (c < 0x80) {
			out += static_cast<char>(c);
		} else if (c < 0x800) {
			out += static_cast<char>(0xC0 | (c >> 6));
			out += static_cast<char>(0x80 | (c & 0x3F));
		} else if (c < 0x10000) {
			out += static_cast<char>(0xE0 | (c >> 12));
			out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (c & 0x3F));
		} else {
			out += static_cast<char>(0xF0 | (c >> 18));
			out += static_cast<char>(0x80 | ((c >> 12) & 0x3F));
			out += static_cast<char>(0x80 | ((c >> 6) & 0x3F));
			out += static_cast<char>(0x80 | (c & 0x3F));
		}
	}

	return out;
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

	std::wstring text(1024, 0);
	va_list copy;
	va_copy(copy, args);
	int length = std::vswprintf(&text[0], text.size(), portableFmt.c_str(), copy);
	va_end(copy);

	if (length < 0)
		length = 0;

	text.resize(static_cast<size_t>(length));
	va_end(args);

	std::fputws(text.c_str(), stderr);
	std::fputws(L"\n", stderr);

	// the active target (the message log file of the player)
	if (g_activeLogTarget && g_activeLogTarget->GetFile() && g_activeLogTarget->GetFile() != stderr) {
		std::string line = toUtf8(text) + "\n";

		std::fwrite(line.data(), 1, line.size(), g_activeLogTarget->GetFile());
		std::fflush(g_activeLogTarget->GetFile());
	}
}

bool wxInitialize() {
	return true;
}

void wxUninitialize() {
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

wxString CONVTOSTR(const long &value) {
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

bool wxFileName::MakeRelativeTo(const wxFileName &base) {
	std::wstring path = _fullPath;
	std::wstring dir = base._fullPath;
	std::replace(path.begin(), path.end(), L'\\', L'/');
	std::replace(dir.begin(), dir.end(), L'\\', L'/');
	if (!dir.empty() && dir.back() != L'/')
		dir += L'/';

	// Split off the common leading directories, then climb out of the rest of
	// the base with "../".
	std::size_t common = 0;
	for (std::size_t pos = 0; (pos = dir.find(L'/', pos)) != std::wstring::npos; pos++) {
		if (path.compare(0, pos + 1, dir, 0, pos + 1) != 0)
			break;
		common = pos + 1;
	}
	std::wstring result;
	for (std::size_t pos = common; (pos = dir.find(L'/', pos)) != std::wstring::npos; pos++)
		result += L"../";
	_fullPath = result + path.substr(common);
	return true;
}

// ---- wxCmdLineParser -------------------------------------------------------------------------------------------------

static bool isOptionChar(wchar_t c) {
	return std::iswalnum(c) || c == L'_';
}

void wxCmdLineParser::SetCmdLine(int argc, char **argv) {
	_args.clear();
	_programName = (argc > 0) ? wxConvertMB2WX(argv[0]).ToStdWstring() : std::wstring();

	for (int i = 1; i < argc; i++)
		_args.push_back(wxConvertMB2WX(argv[i]).ToStdWstring());
}

void wxCmdLineParser::SetDesc(const wxCmdLineEntryDesc *desc) {
	_options.clear();
	_paramDescs.clear();

	for (; desc->kind != wxCMD_LINE_NONE; desc++) {
		if (desc->kind == wxCMD_LINE_PARAM) {
			_paramDescs.push_back(*desc);
		} else if (desc->kind == wxCMD_LINE_SWITCH || desc->kind == wxCMD_LINE_OPTION) {
			Option option;

			option.desc = *desc;
			_options.push_back(option);
		}
	}
}

wxCmdLineParser::Option *wxCmdLineParser::find(const std::wstring &name, bool shortName) {
	for (Option &option : _options) {
		const char *candidate = shortName ? option.desc.shortName : option.desc.longName;

		if (candidate && *candidate && wxString(candidate).ToStdWstring() == name)
			return &option;
	}

	return nullptr;
}

const wxCmdLineParser::Option *wxCmdLineParser::find(const wxString &name) const {
	auto self = const_cast<wxCmdLineParser *>(this);
	Option *option = self->find(name.ToStdWstring(), true);

	return option ? option : self->find(name.ToStdWstring(), false);
}

// The value of an option, converted to what its description says; false (with the text of the error) when it is not a
// number although it should be.
bool wxCmdLineParser::store(Option &option, const std::wstring &value, std::wstring &error) {
	option.present = true;
	option.text = value;

	if (option.desc.type == wxCMD_LINE_VAL_NUMBER) {
		wxString text(value);

		if (!text.ToLong(&option.number, 10)) {
			error = L"'" + value + L"' is not a correct numeric value for option '" +
			        wxString(option.desc.longName ? option.desc.longName : option.desc.shortName).ToStdWstring() + L"'.";
			return false;
		}
	}

	return true;
}

int wxCmdLineParser::Parse(bool giveUsage) {
	std::vector<std::wstring> errors;
	_params.clear();

	for (Option &option : _options) {
		option.present = false;
		option.text.clear();
		option.number = 0;
	}

	for (size_t i = 0; i < _args.size(); i++) {
		const std::wstring &arg = _args[i];

		if (arg.size() < 2 || arg[0] != L'-') {
			size_t maxParams = _paramDescs.size();
			bool multiple = !_paramDescs.empty() && (_paramDescs.back().flags & wxCMD_LINE_PARAM_MULTIPLE);

			if (_params.size() < maxParams || multiple)
				_params.push_back(arg);
			else
				errors.push_back(L"Unexpected parameter '" + arg + L"'");

			continue;
		}

		const bool isLong = arg.size() > 2 && arg[1] == L'-';
		size_t nameStart = isLong ? 2 : 1;
		size_t nameEnd = nameStart;

		while (nameEnd < arg.size() && isOptionChar(arg[nameEnd]))
			nameEnd++;

		const std::wstring name = arg.substr(nameStart, nameEnd - nameStart);
		Option *option = find(name, !isLong);

		if (!option) {
			errors.push_back(std::wstring(isLong ? L"Unknown long option '" : L"Unknown option '") + name + L"'");
			continue;
		}

		if (option->desc.kind == wxCMD_LINE_SWITCH) {
			option->present = true;
			continue;
		}

		// An option: the value follows after '=' or ':', or is the next argument (when the separator is not required).
		std::wstring value;
		bool haveValue = false;

		if (nameEnd < arg.size() && (arg[nameEnd] == L'=' || arg[nameEnd] == L':')) {
			value = arg.substr(nameEnd + 1);
			haveValue = true;
		} else if (nameEnd < arg.size()) {
			errors.push_back(L"Unexpected characters following option '" + name + L"'.");
			continue;
		} else if (!(option->desc.flags & wxCMD_LINE_NEEDS_SEPARATOR) && i + 1 < _args.size()) {
			value = _args[++i];
			haveValue = true;
		}

		if (!haveValue) {
			errors.push_back(L"Option '" + name + L"' requires a value.");
			continue;
		}

		std::wstring error;

		if (!store(*option, value, error))
			errors.push_back(error);
	}

	for (const Option &option : _options) {
		if ((option.desc.flags & wxCMD_LINE_OPTION_MANDATORY) && !option.present) {
			const char *name = option.desc.longName ? option.desc.longName : option.desc.shortName;

			errors.push_back(L"The required option '" + wxString(name).ToStdWstring() + L"' was not specified.");
		}
	}

	for (size_t i = _params.size(); i < _paramDescs.size(); i++) {
		if (!(_paramDescs[i].flags & wxCMD_LINE_PARAM_OPTIONAL)) {
			errors.push_back(L"The required parameter '" + wxString(_paramDescs[i].description).ToStdWstring() +
			                 L"' was not specified.");
			break;
		}
	}

	if (errors.empty())
		return 0;

	for (const std::wstring &error : errors)
		std::fwprintf(stderr, L"%ls\n", error.c_str());

	if (giveUsage)
		Usage();

	return static_cast<int>(errors.size());
}

void wxCmdLineParser::Usage() const {
	std::wstring line = L"Usage: " + _programName;

	for (const Option &option : _options) {
		const char *name = option.desc.shortName && *option.desc.shortName ? option.desc.shortName : option.desc.longName;
		std::wstring text = std::wstring(L"-") + wxString(name).ToStdWstring();

		if (option.desc.kind == wxCMD_LINE_OPTION)
			text += L" <" + std::wstring(option.desc.type == wxCMD_LINE_VAL_NUMBER ? L"num" : L"str") + L">";

		line += (option.desc.flags & wxCMD_LINE_OPTION_MANDATORY) ? L" " + text : L" [" + text + L"]";
	}

	for (const wxCmdLineEntryDesc &param : _paramDescs) {
		std::wstring text = L"<" + wxString(param.description).ToStdWstring() + L">";

		line += (param.flags & wxCMD_LINE_PARAM_OPTIONAL) ? L" [" + text + L"]" : L" " + text;
	}

	std::fwprintf(stderr, L"%ls\n\n", line.c_str());

	for (const Option &option : _options) {
		std::wstring names;

		if (option.desc.shortName && *option.desc.shortName)
			names += L"-" + wxString(option.desc.shortName).ToStdWstring();

		if (option.desc.longName && *option.desc.longName)
			names += (names.empty() ? L"" : L", ") + std::wstring(L"--") + wxString(option.desc.longName).ToStdWstring();

		std::fwprintf(stderr, L"  %-24ls %ls\n", names.c_str(), wxString(option.desc.description).wc_str());
	}
}

bool wxCmdLineParser::Found(const wxString &name) const {
	const Option *option = find(name);

	return option && option->present;
}

bool wxCmdLineParser::Found(const wxString &name, wxString *value) const {
	const Option *option = find(name);

	if (!option || !option->present || option->desc.kind != wxCMD_LINE_OPTION)
		return false;

	*value = wxString(option->text);
	return true;
}

bool wxCmdLineParser::Found(const wxString &name, long *value) const {
	const Option *option = find(name);

	if (!option || !option->present || option->desc.kind != wxCMD_LINE_OPTION)
		return false;

	*value = option->number;
	return true;
}

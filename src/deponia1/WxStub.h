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
#include <cwctype>
#include <string>
#include <vector>

class wxString {
public:
	wxString() = default;
	wxString(const wchar_t *s) : _data(s) {}
	wxString(const char *s) : _data(s, s + std::char_traits<char>::length(s)) {}
	wxString(const std::wstring &s) : _data(s) {}

	const std::wstring &ToStdWstring() const {
		return _data;
	}
	const wchar_t *wc_str() const {
		return _data.c_str();
	}
	const wchar_t *c_str() const {
		return _data.c_str();
	}

	bool IsEmpty() const {
		return _data.empty();
	}
	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// line 463998) - real wxString::CmpNoCase() is a case-insensitive
	// three-way compare.
	int CmpNoCase(const wxString &other) const {
		std::wstring a(_data), b(other._data);
		for (wchar_t &c : a)
			c = std::towlower(c);
		for (wchar_t &c : b)
			c = std::towlower(c);
		if (a < b)
			return -1;
		if (a > b)
			return 1;
		return 0;
	}

	wxString &operator+=(const wxString &rhs) {
		_data += rhs._data;
		return *this;
	}
	friend wxString operator+(wxString lhs, const wxString &rhs) {
		lhs += rhs;
		return lhs;
	}

	// Stands in for wxString::mb_str(); real wxWidgets returns a
	// wxScopedCharBuffer that is implicitly convertible to const char*.
	class CharBuffer {
	public:
		explicit CharBuffer(std::string s) : _narrow(std::move(s)) {}
		operator const char*() const {
			return _narrow.c_str();
		}

	private:
		std::string _narrow;
	};

	CharBuffer mb_str() const {
		std::string narrow(_data.begin(), _data.end());
		return CharBuffer(std::move(narrow));
	}

private:
	std::wstring _data;
};

class wxFileName {
public:
	wxFileName() = default;
	explicit wxFileName(const std::wstring &fullPath) : _fullPath(fullPath) {}
	// Confirmed call shape only (TGameControl::PreLoad, Deponia_Linux.asm
	// line 463848): real wxFileName(const wxString&, const wxString&) joins
	// a directory path and a file name with a separator.
	wxFileName(const std::wstring &path, const std::wstring &name)
		: _fullPath(path.empty() || path.back() == L'/' ? path + name : path + L"/" + name) {}

	wxString GetFullPath() const {
		return wxString(_fullPath);
	}
	// Real wxFileName::GetPath() default behavior: the directory portion,
	// without a trailing separator (empty if there isn't one).
	std::wstring GetPath() const {
		std::size_t pos = _fullPath.find_last_of(L"/\\");
		return (pos == std::wstring::npos) ? std::wstring() : _fullPath.substr(0, pos);
	}
	// Real wxFileName::GetExt() behavior: everything after the last '.' in
	// the name portion (empty if there isn't one).
	wxString GetExt() const {
		std::size_t slash = _fullPath.find_last_of(L"/\\");
		std::size_t dot = _fullPath.find_last_of(L'.');
		if (dot == std::wstring::npos || (slash != std::wstring::npos && dot < slash))
			return wxString();
		return wxString(_fullPath.substr(dot + 1));
	}
	// Confirmed call shape only (TGameControl::PreLoad, asm line 464085) -
	// real wxFileName::SetExt() replaces (or adds) the extension.
	void SetExt(const wxString &ext) {
		std::size_t slash = _fullPath.find_last_of(L"/\\");
		std::size_t dot = _fullPath.find_last_of(L'.');
		if (dot != std::wstring::npos && (slash == std::wstring::npos || dot > slash))
			_fullPath = _fullPath.substr(0, dot);
		_fullPath += L"." + ext.ToStdWstring();
	}
	bool IsOk() const {
		return !_fullPath.empty();
	}
	// Confirmed call shape only (TGameControl::ReplaceGame, Deponia_Linux.asm
	// line 468671).
	bool Exists() const;
	// Confirmed call shape only (TGameControl::PreLoad, asm line 464151) -
	// distinct from Exists(): real wxFileName::FileExists() specifically
	// checks this is a regular file (Exists() also matches directories).
	bool FileExists() const {
		return Exists();
	}
	// Confirmed call shape only (TGameControl::PreLoad, asm line 463809).
	bool IsAbsolute() const {
		return !_fullPath.empty() && (_fullPath[0] == L'/' || (_fullPath.size() > 1 && _fullPath[1] == L':'));
	}
	// Confirmed call shape only (TGameControl::PreLoad, asm line 463799).
	void Assign(const wxString &fullPath) {
		_fullPath = fullPath.ToStdWstring();
	}
	// Confirmed call shape only (TGameControl::PreLoad, asm line 463921) -
	// real wxFileName::SetCwd() changes the process's current directory to
	// this file's own directory.
	void SetCwd() const;
	void NormalizePath() {}
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463066) -
	// replaces the name+extension portion, keeping any existing directory,
	// matching real wxFileName::SetFullName's documented behavior.
	void SetFullName(const wxString &name) {
		std::size_t pos = _fullPath.find_last_of(L"/\\");
		_fullPath = (pos == std::wstring::npos) ? name.ToStdWstring() : _fullPath.substr(0, pos + 1) + name.ToStdWstring();
	}

	static bool Mkdir(const wxString &dir, int permissions = 0777, int flags = 0);
	static wxString GetCwd();

private:
	std::wstring _fullPath;
};

class wxDir {
public:
	static bool Exists(const wxString &path);
};

// Confirmed call shapes only (TGameControl::LoadEventHandlers,
// Deponia_Linux.asm e.g. lines 475212, 475254) - real wxStringTokenizer
// splits a string on a single delimiter character, skipping empty tokens
// between consecutive delimiters (its default wxTOKEN_STRTOK mode).
class wxStringTokenizer {
public:
	wxStringTokenizer(const wxString &str, wchar_t delimiter) {
		std::wstring s = str.ToStdWstring();
		std::size_t pos = 0;
		while (pos < s.size()) {
			std::size_t next = s.find(delimiter, pos);
			if (next == std::wstring::npos) {
				if (pos < s.size())
					_tokens.push_back(s.substr(pos));
				break;
			}
			if (next > pos)
				_tokens.push_back(s.substr(pos, next - pos));
			pos = next + 1;
		}
	}

	bool HasMoreTokens() const {
		return _index < _tokens.size();
	}
	wxString GetNextToken() {
		return wxString(_tokens[_index++]);
	}
	int CountTokens() const {
		return static_cast<int>(_tokens.size() - _index);
	}

private:
	std::vector<std::wstring> _tokens;
	std::size_t _index = 0;
};

// Real wxWidgets key-code constants (used alongside plain ASCII by
// TGameControl::InitGameActions' fixed action-key table, Deponia_Linux.asm
// data at address 0xD6D220 - the ',' '.' '+' '-' digits/letters in that same
// table are plain ASCII and need no named constant).
constexpr int WXK_BACK = 8;
constexpr int WXK_ESCAPE = 27;

struct wxSize {
	int width = 0;
	int height = 0;
};

struct wxPoint {
	int x = 0;
	int y = 0;

	bool operator==(const wxPoint &other) const {
		return x == other.x && y == other.y;
	}
	bool operator!=(const wxPoint &other) const {
		return !(*this == other);
	}
};

struct wxRect {
	int x = 0;
	int y = 0;
	int width = 0;
	int height = 0;

	int GetWidth() const {
		return width;
	}
	int GetHeight() const {
		return height;
	}
	int GetLeft() const {
		return x;
	}
	int GetTop() const {
		return y;
	}
	int GetRight() const {
		return x + width - 1;
	}
	int GetBottom() const {
		return y + height - 1;
	}
	void SetLeft(int left) {
		x = left;
	}
	void SetTop(int top) {
		y = top;
	}
	void SetWidth(int w) {
		width = w;
	}
	void SetHeight(int h) {
		height = h;
	}
	bool IsEmpty() const {
		return width <= 0 || height <= 0;
	}
	bool Intersects(const wxRect &other) const {
		return x < other.x + other.width && other.x < x + width && y < other.y + other.height &&
		       other.y < y + height;
	}
};

// A floating-point rectangle, used where sub-pixel precision matters (e.g.
// TPictureIO::PreparePaint's scroll-adjusted paint area).
struct FloatRect {
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};

class wxCmdLineParser {
public:
	wxCmdLineParser() = default;
	~wxCmdLineParser() = default;

	// Stub always reports the option as not present.
	bool Found(const wxString &/*name*/, wxString */*value*/) const {
		return false;
	}
};

class wxFile {
public:
	static bool Exists(const wxString &path);
};

class wxLog {
public:
	// Signed so `js` (jump-if-negative) checks against it stay meaningful.
	static int loglevel;

	static void SetVerbose(bool verbose);
	static void SetActiveTarget(class wxLogStderr *target);
	static void logexpanded(const wchar_t *fmt, ...);
};

class wxLogStderr {
public:
	explicit wxLogStderr(std::FILE *fp) : _fp(fp) {}

private:
	std::FILE *_fp;
};

bool wxInitialize();
wxString wxConvertMB2WX(const char *s);

// toUTF(wxString*, const char*) - converts a narrow (assumed UTF-8) C string
// into a wxString, writing into the caller-provided output parameter (this
// matches the calling convention seen at every reversed call site so far).
void toUTF(wxString *out, const char *utf8);

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
wxStandardPathsBase &wxGetAppTraitsStandardPaths();

// wxCriticalSection is a plain mutex wrapper in real wxWidgets; several
// classes (e.g. TComposedFile) embed one as an instance member.
class wxCriticalSection {
public:
	void Enter();
	void Leave();
};

class wxCriticalSectionLocker {
public:
	explicit wxCriticalSectionLocker(wxCriticalSection &cs) : _cs(cs) {
		_cs.Enter();
	}
	~wxCriticalSectionLocker() {
		_cs.Leave();
	}

private:
	wxCriticalSection &_cs;
};

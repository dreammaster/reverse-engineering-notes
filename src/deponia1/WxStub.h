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
#include <cstdlib>
#include <cwctype>
#include <string>
#include <vector>

// Real wxWidgets enumerator values (wx/filefn.h) - used by
// TCharHolder::GetFullPath(wxPathFormat), the only confirmed call site so
// far only ever compares against wxPATH_UNIX.
enum wxPathFormat {
	wxPATH_NATIVE = 0,
	wxPATH_UNIX = 1,
	wxPATH_MAC = 2,
	wxPATH_DOS = 3,
	wxPATH_BEOS = 4,
	wxPATH_WIN = 5,
	wxPATH_VMS = 6,
};

class wxString {
public:
	wxString() = default;
	wxString(const wchar_t *s) : _data(s) {}
	wxString(const char *s) : _data(s, s + std::char_traits<char>::length(s)) {}
	wxString(const std::wstring &s) : _data(s) {}

	// Confirmed call shape only (TMSavegame::GetExistingSaveGames, Deponia_
	// Linux.asm line 164367+) - real wxString::ToLong() parses the whole
	// string as a base-`base` integer, reporting failure (and leaving the
	// output untouched) unless every character is consumed.
	bool ToLong(long *value, int base = 10) const {
		if (_data.empty())
			return false;
		wchar_t *end = nullptr;
		long parsed = std::wcstol(_data.c_str(), &end, base);
		if (*end != L'\0')
			return false;
		*value = parsed;
		return true;
	}
	const std::wstring &ToStdWstring() const {
		return _data;
	}
	// Confirmed call shape only (TCFont, Deponia_Linux.asm line 1394558): real wxString::GetChar() is the
	// character at `index` (the terminating 0 one past the end).
	wchar_t GetChar(size_t index) const {
		return (index < _data.size()) ? _data[index] : L'\0';
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
	size_t Length() const {
		return _data.size();
	}
	size_t Len() const {
		return _data.size();
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
	// Confirmed call shape only (TGameControl::LoadGame(TMSavegame*),
	// Deponia_Linux.asm line 477527) - real wxString::Cmp() is a case-
	// sensitive three-way compare (unlike CmpNoCase() above).
	int Cmp(const wxString &other) const {
		if (_data < other._data)
			return -1;
		if (_data > other._data)
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
	// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
	// line 458539) - real wxString::Contains() is a case-sensitive substring
	// search.
	bool Contains(const wxString &other) const {
		return _data.find(other._data) != std::wstring::npos;
	}
	// Confirmed call shape only (TComposedFileManager::FileExists,
	// Deponia_Linux.asm line 519756) - real wxString::Find() returns the
	// index of the first match, or wxNOT_FOUND (-1) if absent.
	int Find(const wxString &sub) const {
		std::size_t pos = _data.find(sub._data);
		return pos == std::wstring::npos ? -1 : static_cast<int>(pos);
	}
	// Confirmed call shape only (asm line 458581) - real wxString::Replace()
	// replaces every occurrence of strOld with strNew when replaceAll is
	// true (the one call site reversed so far always passes true; a
	// first-occurrence-only mode isn't implemented here).
	int Replace(const wxString &strOld, const wxString &strNew, bool replaceAll = true) {
		int count = 0;
		std::size_t pos = 0;
		while ((pos = _data.find(strOld._data, pos)) != std::wstring::npos) {
			_data.replace(pos, strOld._data.size(), strNew._data);
			pos += strNew._data.size();
			count++;
			if (!replaceAll)
				break;
		}
		return count;
	}

	// Confirmed call shape only (TArgument::SetPath/AddPath, Deponia_Linux.
	// asm lines 1436084, 1439115) - real wxString::StartsWith() is a
	// case-sensitive prefix check.
	bool StartsWith(const wxString &prefix) const {
		return _data.compare(0, prefix._data.size(), prefix._data) == 0;
	}
	// Confirmed call shape only (TArgument::SetPath/AddPath, asm lines
	// 1436104, 1439132) - real wxString::Mid(first, count) returns the
	// substring starting at `first`, `count` characters long (or to the end
	// if `count` is negative, matching wxWidgets' own npos-as-"-1" idiom).
	wxString Mid(int first) const {
		return Mid(first, -1);
	}
	wxString Mid(int first, int count) const {
		if (first < 0 || static_cast<std::size_t>(first) > _data.size())
			return wxString();
		if (count < 0)
			return wxString(_data.substr(first));
		return wxString(_data.substr(first, static_cast<std::size_t>(count)));
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

	// Confirmed call shape only (TCharHolder::Lower, Deponia_Linux.asm line
	// 642619) - real wxString::MakeLower() lowercases in place.
	void MakeLower() {
		for (wchar_t &c : _data)
			c = std::towlower(c);
	}
	// Confirmed call shape only (TCharHolder::ToDouble, Deponia_Linux.asm
	// line 6426FC) - real wxString::ToDouble() parses the whole string as a
	// double, failing (false, *out left unmodified real wx doesn't touch it
	// on failure either) if any trailing non-whitespace remains.
	bool ToDouble(double *out) const {
		std::string narrow(_data.begin(), _data.end());
		char *end = nullptr;
		double value = std::strtod(narrow.c_str(), &end);
		if (end == narrow.c_str())
			return false;
		while (*end == ' ' || *end == '\t')
			++end;
		if (*end != '\0')
			return false;
		*out = value;
		return true;
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
	// Confirmed call shape (TGAction::Execute, Deponia_Linux.asm line 202966): real
	// wxFileName::SameAs() compares the two normalized absolute paths; the path is taken as it
	// is here (the data's paths are all written the same way).
	bool SameAs(const wxFileName &other) const {
		return _fullPath == other._fullPath;
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
	// Confirmed call shape only (TVisionaireGame::LoadDataGame) - back to an
	// empty name.
	void Clear() {
		_fullPath.clear();
	}
	// Confirmed call shape only (TFile::OpenWrite, Deponia_Linux.asm line
	// 523174) - real wxFileName::MakeAbsolute() prefixes the current directory
	// to a relative path.
	bool MakeAbsolute();
	// Confirmed call shapes only (TComposedFile::WriteToDisk(), Deponia_Linux.
	// asm lines 542364-543915) - the instance versions test/create the
	// directory part of the path (not the path itself), non-recursively.
	bool DirExists() const;
	bool Mkdir() const;
	// Confirmed call shape only (TGameControl::LoadGame(TMSavegame*),
	// Deponia_Linux.asm line 477491) - real wxFileName::GetFullName()
	// returns just the name+extension portion, without the directory.
	wxString GetFullName() const {
		std::size_t pos = _fullPath.find_last_of(L"/\\");
		return wxString(pos == std::wstring::npos ? _fullPath : _fullPath.substr(pos + 1));
	}
	// Confirmed call shape only (TComposedFileManager::Export,
	// Deponia_Linux.asm line 519299) - real wxFileName::GetName() returns
	// just the name portion (no directory, no extension).
	wxString GetName() const {
		wxString ext = GetExt();
		std::wstring fullName = GetFullName().ToStdWstring();
		if (ext.ToStdWstring().empty())
			return wxString(fullName);
		return wxString(fullName.substr(0, fullName.size() - ext.ToStdWstring().size() - 1));
	}
	// Confirmed call shape only (TGameControl::SaveGame, asm line 463066) -
	// replaces the name+extension portion, keeping any existing directory,
	// matching real wxFileName::SetFullName's documented behavior.
	void SetFullName(const wxString &name) {
		std::size_t pos = _fullPath.find_last_of(L"/\\");
		_fullPath = (pos == std::wstring::npos) ? name.ToStdWstring() : _fullPath.substr(0, pos + 1) + name.ToStdWstring();
	}

	// Confirmed call shape only (TFieldValue::TFieldValue(wxFileName const&,
	// wxFileName const&), asm line 528389) - real wxFileName::MakeRelativeTo()
	// rewrites this path relative to the given base directory.
	bool MakeRelativeTo(const wxFileName &base);

	static bool Mkdir(const wxString &dir, int permissions = 0777, int flags = 0);
	static wxString GetCwd();

private:
	std::wstring _fullPath;
};

// Confirmed call shapes only (TMSavegame::SavegameExists/GetExistingSaveGames,
// Deponia_Linux.asm lines 163282-164367). GetFirst()/GetNext() list matching
// entries of an opened directory one at a time; `filespec` is a wildcard
// ('*' and '?') matched against the entry name, and a flags value of 0 (the
// only one the call sites reversed so far pass) lists files and directories
// alike, excluding "." and "..".
class wxDir {
public:
	static bool Exists(const wxString &path);

	bool Open(const wxString &path);
	bool GetFirst(wxString *filename, const wxString &filespec, int flags);
	bool GetNext(wxString *filename);

private:
	std::vector<std::wstring> _all;
	std::vector<std::wstring> _entries;
	std::size_t _index = 0;
};

// Confirmed call shape only (TMSavegame::Delete, Deponia_Linux.asm line
// 163096) - real wxRemoveFile() takes its argument by value and reports
// whether the file was deleted.
bool wxRemoveFile(wxString file);

// Confirmed call shape only (TMSavegame::MakeSaveGameName, asm line 161685)
// - the real static GetTmNow() returns a pointer to a freshly filled `struct
// tm` for the current local time (null on failure).
class wxDateTime {
public:
	static struct tm *GetTmNow();
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

// Confirmed a float pair (TPaintControl::SetScrollPos(), Deponia_Linux.asm
// line 749183 reads it with movss/cvttss2si).
struct wxRealPoint {
	float x = 0.0f;
	float y = 0.0f;
};

// A colour of four bytes, red first (the layout of wxColour in the original: Set() stores them in this order).
struct wxColour {
	unsigned char red = 0;
	unsigned char green = 0;
	unsigned char blue = 0;
	unsigned char alpha = 255;

	wxColour() = default;
	wxColour(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) : red(r), green(g), blue(b), alpha(a) {}

	void Set(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) {
		red = r;
		green = g;
		blue = b;
		alpha = a;
	}
};

struct wxSize {
	int width = 0;
	int height = 0;

	void Set(int newWidth, int newHeight) {
		width = newWidth;
		height = newHeight;
	}
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
	// Confirmed a real free operator+ (TGScene::SetCharacter(), Deponia_
	// Linux.asm line 550270 - `_ZplRK7wxPointS1_`), the plain component-wise
	// sum.
	wxPoint operator+(const wxPoint &other) const {
		return wxPoint{x + other.x, y + other.y};
	}
	// Confirmed a real free operator- (TPaintControl::GetRelativePoint(),
	// Deponia_Linux.asm line 749311 - `_ZmiRK7wxPointS1_`).
	wxPoint operator-(const wxPoint &other) const {
		return wxPoint{x - other.x, y - other.y};
	}
	// Confirmed a real free operator+= (TGAnimation::Draw(), Deponia_Linux.asm line
	// 150061 - `_ZpLR7wxPointRKS_`), the component-wise sum in place.
	wxPoint &operator+=(const wxPoint &other) {
		x += other.x;
		y += other.y;
		return *this;
	}
	// Confirmed a real free operator-= (TGObject::IsInside(), Deponia_Linux.asm line 258814 -
	// `_ZmIR7wxPointRKS_`), the component-wise difference in place.
	wxPoint &operator-=(const wxPoint &other) {
		x -= other.x;
		y -= other.y;
		return *this;
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
	// Real wxRect::SetRight()/SetBottom(): the width/height that makes that edge the last pixel.
	void SetRight(int right) {
		width = right - x + 1;
	}
	void SetBottom(int bottom) {
		height = bottom - y + 1;
	}
	bool IsEmpty() const {
		return width <= 0 || height <= 0;
	}
	bool Intersects(const wxRect &other) const {
		return x < other.x + other.width && other.x < x + width && y < other.y + other.height &&
		       other.y < y + height;
	}
	// Confirmed call shape only (TManagedObject::IsInside, Deponia_Linux.asm
	// line 191161) - real wxRect::Contains(wxPoint) is a half-open range
	// test, matching real wxWidgets' own documented behavior.
	bool Contains(const wxPoint &pt) const {
		return pt.x >= x && pt.x < x + width && pt.y >= y && pt.y < y + height;
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

// The kinds of the entries of a command line description (wxCmdLineEntryType) ...
enum wxCmdLineEntryType {
	wxCMD_LINE_SWITCH = 0,
	wxCMD_LINE_OPTION = 1,
	wxCMD_LINE_PARAM = 2,
	wxCMD_LINE_USAGE_TEXT = 3,
	wxCMD_LINE_NONE = 4
};

// ... the type of the value of an option or parameter (wxCmdLineParamType) ...
enum wxCmdLineParamType {
	wxCMD_LINE_VAL_STRING = 0,
	wxCMD_LINE_VAL_NUMBER = 1,
	wxCMD_LINE_VAL_DATE = 2,
	wxCMD_LINE_VAL_DOUBLE = 3,
	wxCMD_LINE_VAL_NONE = 4
};

// ... and the flags (wxCMD_LINE_OPTION_MANDATORY ...). A parameter without wxCMD_LINE_PARAM_OPTIONAL is mandatory.
constexpr int wxCMD_LINE_OPTION_MANDATORY = 0x01;
constexpr int wxCMD_LINE_PARAM_OPTIONAL = 0x02;
constexpr int wxCMD_LINE_PARAM_MULTIPLE = 0x04;
constexpr int wxCMD_LINE_OPTION_HELP = 0x08;
constexpr int wxCMD_LINE_NEEDS_SEPARATOR = 0x10;

// One entry of the description of a command line (as in wxWidgets 3: narrow strings).
struct wxCmdLineEntryDesc {
	wxCmdLineEntryType kind;
	const char *shortName;
	const char *longName;
	const char *description;
	wxCmdLineParamType type;
	int flags;
};

/** The parser of the command line, as wxWidgets' wxCmdLineParser (the part the engine uses): a switch or an option is
 *  given by its short name (`-w`, `-ll warning`, `-ll=warning`) or its long name (`--window`, `--loglevel=warning`);
 *  what is not an option is a parameter. Parse() returns 0 when the line is correct, else the number of the errors
 *  (which it says on stderr with the usage); a mandatory parameter that is missing is an error. */
class wxCmdLineParser {
public:
	wxCmdLineParser() = default;
	~wxCmdLineParser() = default;

	void SetCmdLine(int argc, char **argv);
	/** `desc` ends with an entry of the kind wxCMD_LINE_NONE; the strings must stay alive. */
	void SetDesc(const wxCmdLineEntryDesc *desc);
	int Parse(bool giveUsage = true);
	void Usage() const;

	/** The switch or option is on the command line (by short or long name). */
	bool Found(const wxString &name) const;
	/** The option is on the command line; its value is put into `value`. */
	bool Found(const wxString &name, wxString *value) const;
	bool Found(const wxString &name, long *value) const;

	size_t GetParamCount() const {
		return _params.size();
	}
	wxString GetParam(size_t n = 0) const {
		return (n < _params.size()) ? wxString(_params[n]) : wxString();
	}

private:
	struct Option {
		wxCmdLineEntryDesc desc;
		bool present = false;
		std::wstring text;
		long number = 0;
	};

	Option *find(const std::wstring &name, bool shortName);
	const Option *find(const wxString &name) const;
	bool store(Option &option, const std::wstring &value, std::wstring &error);

	std::vector<std::wstring> _args; ///< the arguments without the name of the program
	std::wstring _programName;
	std::vector<Option> _options;
	std::vector<wxCmdLineEntryDesc> _paramDescs;
	std::vector<std::wstring> _params;
};

class wxFile {
public:
	wxFile() = default;
	// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
	// line 458410) - not reversed beyond that.
	explicit wxFile(const wxString &path);
	~wxFile();

	static bool Exists(const wxString &path);
	// Confirmed call shapes only (TGameControl::InitScripts, asm lines
	// 458421-458435) - not reversed beyond that; implemented for real
	// against the same std::FILE* pattern already used for TFile.
	long Length() const;
	// Confirmed call shapes (TComposedFile::GetMemoryFile, Deponia_Linux.asm
	// line 545901, mode=1; TFile::DecryptHeader, asm line 523692, mode=2) -
	// mode 1 opens read-only, mode 2 read-write (matching DecryptHeader's own
	// subsequent Write() call); the real wxWidgets wxFile::OpenMode ordinals
	// for these two values weren't independently confirmed.
	bool Open(const wxString &path, int mode);
	// Confirmed call shape (TFile::OpenRead/OpenWrite, Deponia_Linux.asm lines
	// 522746-523325): opens with a stdio-style mode string ("r" or "w"; the
	// stub always opens binary - the original runs on Linux, where there's no
	// text/binary distinction).
	bool Open(const wxString &path, const wxString &mode);
	bool IsOpened() const {
		return _handle != nullptr;
	}
	// Confirmed call shape only (TFile::Eof, asm line 523078): true once the
	// last read hit the end of the file.
	bool Eof() const;
	unsigned long Read(char *buffer, unsigned long size);
	unsigned long Read(void *buffer, unsigned long size) {
		return Read(static_cast<char *>(buffer), size);
	}
	// Confirmed call shape only (TFile::DecryptHeader, Deponia_Linux.asm line
	// 523762).
	unsigned long Write(const void *buffer, unsigned long size);
	// Confirmed call shape (TComposedFile::WriteToDisk, asm line 542364+): the
	// string's narrow form, no terminator.
	unsigned long Write(const wxString &text);
	// Confirmed call shape only (TFile::DecryptHeader, asm line 523764).
	void Flush();
	// Confirmed call shape only (TComposedFile::GetMemoryFile, Deponia_Linux.
	// asm line 545987, mode=0 i.e. SEEK_SET at that call site).
	bool Seek(unsigned long offset, int mode);
	void Close();

private:
	std::FILE *_handle = nullptr;
};

class wxLog {
public:
	// Signed so `js` (jump-if-negative) checks against it stay meaningful.
	static int loglevel;

	static void SetVerbose(bool verbose);
	static void SetLogLevel(int level) {
		loglevel = level;
	}
	/** Makes `target` the place the messages go to; returns the one before (whose owner the caller becomes). */
	static class wxLogStderr *SetActiveTarget(class wxLogStderr *target);
	static void logexpanded(const wchar_t *fmt, ...);
};

/** A log target that writes to a file (stderr for wxLogStderr() without one). */
class wxLogStderr {
public:
	explicit wxLogStderr(std::FILE *fp) : _fp(fp) {}

	std::FILE *GetFile() const {
		return _fp;
	}

private:
	std::FILE *_fp;
};

bool wxInitialize();
void wxUninitialize();

// Confirmed call shapes only (CmdGetProperty::Redo(), Deponia_Linux.asm lines 398051-398150): the language of the
// system. The stub knows none (wxLANGUAGE_UNKNOWN).
constexpr int wxLANGUAGE_UNKNOWN = 1;

class wxLocale {
public:
	static int GetSystemLanguage() {
		return wxLANGUAGE_UNKNOWN;
	}
	static wxString GetLanguageName(int /*language*/) {
		return wxString();
	}
	static wxString GetLanguageCode(int /*language*/) {
		return wxString();
	}
};

// Confirmed call shape only (CmdStartDefaultBrowser::Redo(), asm line 398228): opens the URL in the browser of the
// system; the stub opens nothing.
inline bool wxLaunchDefaultBrowser(const wxString &/*url*/) {
	return false;
}
wxString wxConvertMB2WX(const char *s);

// Confirmed call shape only (TTimer::TTimer/SetTime/GetTime, Deponia_Linux.
// asm lines 559379, 559418, 559442) - real wxGetLocalTimeMillis() returns
// milliseconds since the Unix epoch as a wxLongLong; backed by
// system_clock since only elapsed-time differences are ever taken from it.
long long wxGetLocalTimeMillis();
// Confirmed call shape only (TTimer::WaitUntil, Deponia_Linux.asm line
// 559476) - real wxMilliSleep() blocks the calling thread for `ms`
// milliseconds.
void wxMilliSleep(long ms);

// toUTF(wxString*, const char*) - converts a narrow (assumed UTF-8) C string
// into a wxString, writing into the caller-provided output parameter (this
// matches the calling convention seen at every reversed call site so far).
void toUTF(wxString *out, const char *utf8);

// Confirmed a free function returning a wide string by value (TGameControl::
// SaveEventHandlers, Deponia_Linux.asm line 457636, `_Z9CONVTOSTRRKi` -
// CONVTOSTR(int const&)) - not itself reversed, but its result is
// immediately appended to a std::wstring at that call site, so a plain
// decimal rendering (matching its name) is the obvious behavior.
wxString CONVTOSTR(const int &value);
wxString CONVTOSTR(const long &value);

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

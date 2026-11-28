#include "baselib/xmlCommon.h"

#include <cstdlib>
#include <string>

#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/baselib/xmlCommon.cpp";

// Confirmed (asm lines 527454-527574)
int ConvertToInt(const char *begin, const char *end) {
	std::string text(begin, end);
	return (int)strtol(text.c_str(), nullptr, 10);
}

// Confirmed (asm lines 527575-527700)
float ConvertToFloat(const char *begin, const char *end) {
	std::string text(begin, end);
	return (float)strtod(text.c_str(), nullptr);
}

// Confirmed (asm lines 527701-527745)
bool ConvertToBool(const char *begin, const char *end) {
	if (end - begin == 1) {
		int c = *begin & ~0x20;
		if (c == 'F')
			return false;
		if (c == 'T')
			return true;
	}

	x_assert(false, "false", kSourceFile, 0x154);
	return false;
}

// Confirmed (asm lines 527746-527884)
wxString ConvertToString(const char *begin, const char *end) {
	std::string text(begin, end);
	wxString result;
	toUTF(&result, text.c_str());
	return result;
}

// Confirmed (asm lines 527885-528206)
wxFileName ConvertToFileName(const char *begin, const char *end) {
	std::string text(begin, end);
	wxString name;
	toUTF(&name, text.c_str());
	name.Replace(L"\\", L"/", true);

	wxFileName result(name.ToStdWstring());
	result.NormalizePath();
	return result;
}

// Confirmed (asm lines 636174-636260)
long dtol(const char *text) {
	bool negative = (*text == '-');
	if (negative)
		text++;

	int value = 0;
	for (; *text; text++)
		value = value * 10 + (*text - '0');
	return negative ? -value : value;
}

// Confirmed (asm lines 636139-636173)
void normalizepath(char *path) {
	for (; *path; path++) {
		if (*path == '\\')
			*path = '/';
	}
}

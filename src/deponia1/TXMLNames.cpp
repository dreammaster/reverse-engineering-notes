#include "TXMLNames.h"

#include <cstdlib>
#include <cstring>
#include <string>
#include <unordered_map>
#include <vector>

#include "vstables/xmlNamesData.h"

namespace {
bool g_xmlNamesScrambled = false;
bool g_xmlNamesInternInitialized = false;
std::vector<wxString> g_xmlNames;
std::vector<const char *> g_xmlNamesUtf8;
std::unordered_map<std::string, int> g_xmlNamesMap;
}

bool TXMLNames::IsScrambled() {
	return g_xmlNamesScrambled;
}

void TXMLNames::SetScrambled(bool scrambled) {
	g_xmlNamesScrambled = scrambled;
}

// Confirmed (asm lines 527033-527122)
void TXMLNames::CleanUp() {
	for (const char *name : g_xmlNamesUtf8)
		free(const_cast<char *>(name));
	g_xmlNamesUtf8.clear();
	g_xmlNames.clear();
}

// Confirmed (asm lines 527123-527176)
const wxString &TXMLNames::GetString(int id) {
	static const wxString emptyString;

	if (id > 0 && id <= (int)g_xmlNames.size())
		return g_xmlNames[id - 1];
	return emptyString;
}

// Confirmed (asm lines 527177-527207)
const char *TXMLNames::GetStringUtf8(int id) {
	if (id > 0 && id <= (int)g_xmlNamesUtf8.size())
		return g_xmlNamesUtf8[id - 1];
	return "";
}

// Confirmed (asm lines 527208-527339)
int TXMLNames::GetNr(const wxString &name) {
	std::string narrow((const char *)name.mb_str());
	auto it = g_xmlNamesMap.find(narrow);
	return (it == g_xmlNamesMap.end()) ? -1 : it->second;
}

// Confirmed (asm lines 527340-527460)
int TXMLNames::GetNrByUtf8Name(const char *name) {
	if (name[0] == 'T' && name[1] >= '0' && name[1] <= '9')
		return (int)dtol(name + 1);

	auto it = g_xmlNamesMap.find(name);
	return (it == g_xmlNamesMap.end()) ? -1 : it->second;
}

// Confirmed (asm lines 532226-532745). The third argument is never read.
bool TXMLNames::AddXMLName(const wxString &name, int id, bool /*readable*/) {
	if (id - 1 != (int)g_xmlNames.size()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"AddXMLName failed: %ls", name.c_str());
		return false;
	}

	g_xmlNames.push_back(name);
	const char *narrow = strdup((const char *)name.mb_str());
	g_xmlNamesUtf8.push_back(narrow);
	g_xmlNamesMap[narrow] = id;
	return true;
}

// Confirmed (asm lines 533265-535313): ids 1-58 by name, then ids 59-99 all
// as "DSunused" (a loop).
void TXMLNames::InitXMLNamesIntern() {
	if (g_xmlNamesInternInitialized)
		return;
	g_xmlNamesInternInitialized = true;

	int id = 1;
	for (const char *name : kInternXMLNames)
		AddXMLName(wxString(name), id++, false);
	for (; id < 100; id++)
		AddXMLName(wxString("DSunused"), id, false);
}

long dtol(const char *text) {
	bool negative = (*text == '-');
	if (negative)
		text++;

	int value = 0;
	for (; *text; text++)
		value = value * 10 + (*text - '0');
	return negative ? -value : value;
}

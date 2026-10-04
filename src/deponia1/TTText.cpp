#include "TTText.h"

#include "Diagnostics.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vstables/TTText.cpp";

// The two language ids: -1 until a language is set.
int TTText::s_languageId = -1;
int TTText::s_speechLanguageId = -1;
wxString TTText::s_comment;
bool TTText::s_showIdInTextNames = false;

// The id of a Languages-table (table number 0x12) object, -1 for anything else.
static bool languageIdOf(const TVisObjRef &language, int &id) {
	if (language.IsEmpty())
		return false;
	if (language.GetId()[3] != 0x12)
		return false;

	id = language.GetObjectPointer()->GetId24();
	return true;
}

// Confirmed (asm lines 1454372-1454420)
void TTText::SetLanguage(const TVisObjRef &language) {
	int id;

	if (languageIdOf(language, id))
		s_languageId = id;
}

// Confirmed (asm lines 1454437-1454485)
void TTText::SetSpeechLanguage(const TVisObjRef &language) {
	int id;

	if (languageIdOf(language, id))
		s_speechLanguageId = id;
}

static const TTextLanguage &findLanguage(const TTText &text, int languageId) {
	static TTextLanguage empty;

	std::vector<TTextLanguage> *texts = nullptr;
	text.GetTexts(kTextTextLanguages, &texts);

	for (const TTextLanguage &entry : *texts) {
		if (entry.languageId == languageId)
			return entry;
	}
	return empty;
}

// Confirmed (asm lines 1454502-1454625)
const TTextLanguage &TTText::GetTextLanguage() const {
	static TTextLanguage empty;

	if (IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x72);
		return empty;
	}
	return findLanguage(*this, s_languageId);
}

// Confirmed (asm lines 1457263-1457389)
const TTextLanguage &TTText::GetTextLanguage(int languageId) const {
	static TTextLanguage empty;

	if (IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x87);
		return empty;
	}
	return findLanguage(*this, languageId);
}

// Confirmed (asm lines 1460716-1460899)
TTextLanguage &TTText::GetTextLanguageOrCreate(int languageId) {
	static TTextLanguage empty;

	if (IsEmpty()) {
		x_assert(false, "false", kSourceFile, 0x9C);
		return empty;
	}

	std::vector<TTextLanguage> *texts = nullptr;
	GetTexts(kTextTextLanguages, &texts);

	for (TTextLanguage &entry : *texts) {
		if (entry.languageId == languageId)
			return entry;
	}

	TTextLanguage created;
	created.languageId = languageId;
	texts->push_back(created);
	return texts->back();
}

// Confirmed (asm lines 1454626-1454649)
wxString TTText::GetTextString() const {
	return wxString(GetTextLanguage().text);
}

// Confirmed (asm lines 1457390-1457523)
void TTText::GetTextProperties(wxString &text, wxFileName &audioFile) const {
	const TTextLanguage &entry = GetTextLanguage();

	text = wxString(entry.text);
	if (s_speechLanguageId != -1)
		audioFile = wxFileName(GetTextLanguage(s_speechLanguageId).audioFile);
	else
		audioFile = wxFileName(entry.audioFile);
}

// Confirmed (asm lines 1457524-1457971)
bool TTText::ReplaceValues(wxString &text, TVisionaire *visionaire) {
	std::wstring string = text.ToStdWstring();

	for (;;) {
		size_t begin = string.find(L"<v");
		if (begin == std::wstring::npos)
			break;

		size_t end = string.find(L">", begin);
		if (end == std::wstring::npos)
			return false;

		// <v=name> or <vi=name> (an integer), <vs=name> (a string)
		bool isInteger = true;
		size_t nameStart;
		if (string.size() > begin + 2 && string[begin + 2] == L'=') {
			nameStart = begin + 3;
		} else if (string.size() - begin > 5 && string[begin + 3] == L'=' &&
		           (string[begin + 2] == L'i' || string[begin + 2] == L's')) {
			isInteger = string[begin + 2] == L'i';
			nameStart = begin + 4;
		} else {
			return false;
		}

		wxString name(string.substr(nameStart, end - nameStart));
		TVisObjRef value;
		std::wstring replacement;

		if (!visionaire->GetObjectByName(name, 0x14, value))
			replacement = L"'?'";
		else if (isInteger)
			replacement = CONVTOSTR(value.GetInt(kValueInt)).ToStdWstring();
		else
			replacement = value.GetStr(kValueString).ToStdWstring();

		string.replace(begin, end - begin + 1, replacement);
	}

	text = wxString(string);
	return true;
}

// Confirmed (asm lines 1457972-1457988)
void TTText::SetStaticTexts(const wxString &comment) {
	s_comment = comment;
}

// Not yet assert-confirmed to a specific file; stays at the top level.
// Not to be confused with TSText/TGText (scene text objects) - "TT" here
// matches the mangled symbol exactly.
//
// Confirmed (Deponia_Linux.asm lines 1454121-1460716): a typed handle onto a
// text data record (a TVisObjRef, constructed from a plain one) with the
// current text and speech language held in two statics. A text record has a
// list of TTextLanguage entries (its text and audio file per language id);
// the language ids are the ids of objects in the Languages table (18).
//
// Not reconstructed: GetNameInList() (the editor's list label: a long chain
// of parent names, the text itself and, optionally, the id) - see
// vstables/recordCallbacksManual.cpp.
#pragma once

#include "datastruct/typegrp.h"

#include "TTextLanguage.h"
#include "datastruct/visobjref.h"

class TVisionaire;

class TTText : public TVisObjRef {
public:
	// Recovered from the binary's schema (vstables/records.cpp).
	static TTypeGroup &GetTypeGroup();
	static void InitType(int versionLow, int versionHigh);
	static void OnCreate(TVisionaireObject *object);
	static void OnInit(TVisionaireObject *object);
	static wxString GetNameInList(const TVisionaireObject *object);

	TTText() = default;
	explicit TTText(const TVisObjRef &ref) : TVisObjRef(ref) {
	}

	/** The text / speech language: the id of a Languages-table object (any
	 *  other reference is ignored). */
	static void SetLanguage(const TVisObjRef &language);
	static int GetLanguageId() {
		return s_languageId;
	}
	static void SetSpeechLanguage(const TVisObjRef &language);
	static int GetSpeechLanguageId() {
		return s_speechLanguageId;
	}

	/** The entry for the current text language (an empty one if the text has
	 *  none; asserted against for an empty reference). */
	const TTextLanguage &GetTextLanguage() const;
	/** The entry for a language id. */
	const TTextLanguage &GetTextLanguage(int languageId) const;
	/** The same, appending an empty entry for the language if there is none. */
	TTextLanguage &GetTextLanguageOrCreate(int languageId);
	/** The text in the current language. */
	wxString GetTextString() const;
	/** The text in the current language and the audio file of the speech
	 *  language (or of the text language if there is none). */
	void GetTextProperties(wxString &text, wxFileName &audioFile) const;

	/** Replaces the value placeholders of a text: `<v=name>` and `<vi=name>` by
	 *  the integer, `<vs=name>` by the string of the Value object of that name
	 *  ('?' if there is none). False if a placeholder is malformed. */
	static bool ReplaceValues(wxString &text, TVisionaire *visionaire);
	static void SetStaticTexts(const wxString &comment);
	static void ShowIdInTextNames(bool show) {
		s_showIdInTextNames = show;
	}

private:
	static int s_languageId;
	static int s_speechLanguageId;
	static wxString s_comment;
	static bool s_showIdInTextNames;
};

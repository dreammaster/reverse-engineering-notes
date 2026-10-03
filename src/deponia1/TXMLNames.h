// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 527002-527460 and 532226-532745, all 10
// manifest-listed methods; the two generated name tables are in
// vstables/xmlNamesData.h): the registry mapping a game-data XML element/
// attribute name to the integer id the rest of the engine uses (eFieldId in
// vstables/fieldIds.h) and back. Ids are dense and 1-based: AddXMLName() only
// accepts an id that is exactly one past the number of names so far.
//
// The original keeps three parallel static vectors (the wide names, their
// strdup()'d narrow UTF-8 forms, and a "readable" variant that CleanUp()
// frees but nothing here fills) and a hash map from narrow name to id (a
// chained table using a one-at-a-time hash, which std::unordered_map stands
// in for). `Scrambled` is a plain flag set by SetScrambled(); nothing in these
// methods reads it.
//
// InitXMLNamesIntern() (a second 10-call-per-name function, asm line 533265)
// registers ids 1-99 and is guarded to run once; the remaining ids come from
// TVisionaireGame::InitXMLNames().
#pragma once

#include "WxStub.h"

class TXMLNames {
public:
	static bool IsScrambled();
	static void SetScrambled(bool scrambled);

	/** Frees the name tables (the hash map itself is left alone, as in the
	 *  original). */
	static void CleanUp();

	/** The wide name for a 1-based id, or an empty string if out of range. */
	static const wxString &GetString(int id);
	/** The narrow UTF-8 name for a 1-based id, or "" if out of range. */
	static const char *GetStringUtf8(int id);

	/** The id registered for a name, or -1. */
	static int GetNr(const wxString &name);
	/** Like GetNr(), but for a narrow name; a name of the form "T<digit>..."
	 *  is not looked up and is instead read as the number after the "T". */
	static int GetNrByUtf8Name(const char *name);

	/** Registers the next name; `id` must be the current name count + 1,
	 *  otherwise "AddXMLName failed: <name>" is logged and false returned. */
	static bool AddXMLName(const wxString &name, int id, bool readable);

	/** Registers ids 1-99 (once). */
	static void InitXMLNamesIntern();
};

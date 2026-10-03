#include "vstables/visionaireGame.h"

#include "TXMLNames.h"
#include "vstables/xmlNamesData.h"

// Confirmed only as far as the call to InitXMLNames() (asm line 1480547+,
// "TVisionaireGame::TVisionaireGame(void)+2A"); the rest of the constructor
// isn't reversed.
TVisionaireGame::TVisionaireGame() {
	InitXMLNames();
}

void TVisionaireGame::InitXMLNames() {
	static bool init = false;

	if (init)
		return;
	init = true;

	TXMLNames::InitXMLNamesIntern();

	int id = 100;
	for (const char *name : kGameXMLNames)
		TXMLNames::AddXMLName(wxString(name), id++, true);
}

TVisObjRef TVisionaireGame::GetGame() const {
	return TVisObjRef();
}

TVisObjRef TVisionaireGame::GetEmptyObject() const {
	return TVisObjRef();
}

bool TVisionaireGame::LoadSaveGame(const wxFileName &/*file*/, const wxString &/*extra*/) {
	return false;
}

void TVisionaireGame::GetList(int /*fieldId*/, TVList &/*outList*/, bool /*flag*/) const {
}

void SaveGlobalScriptVariables(TVisionaireGame &/*game*/) {
}

void LoadGlobalScriptVariables(TVisionaireGame &/*game*/) {
}

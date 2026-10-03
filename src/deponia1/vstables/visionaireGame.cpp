#include "vstables/visionaireGame.h"

TVisObjRef TVisionaireGame::GetGame() const {
	return TVisObjRef();
}

TVisObjRef TVisionaireGame::GetEmptyObject() const {
	return TVisObjRef();
}

void TVisionaireGame::GetList(int /*fieldId*/, TVList &/*outList*/, bool /*flag*/) const {
}

void SaveGlobalScriptVariables(TVisionaireGame &/*game*/) {
}

void LoadGlobalScriptVariables(TVisionaireGame &/*game*/) {
}

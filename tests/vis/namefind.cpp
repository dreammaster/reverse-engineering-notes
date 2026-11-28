#include <algorithm>
#include <cwctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "graphicslib/graphics.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/vlist.h"
#include "vstables/records.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"

class NullGraphics : public TGraphicsInterface {
};


// Lists the objects of every table whose name contains one of the given texts (not case sensitive): namefind <text> ...; run from
// the game directory (GAME_PASSW).
static std::wstring lower(std::wstring text) {
	std::transform(text.begin(), text.end(), text.begin(), [](wchar_t c) { return (wchar_t)std::towlower(c); });
	return text;
}

int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 0;

	graphics = new NullGraphics();
	if (getenv("GAME_PASSW"))
		passw = wxString(getenv("GAME_PASSW"));
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"data.vis");
	wxString warning;

	if (!control->PreLoad(file, warning, true))
		return 1;
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true))
		return 1;

	TVisionaireGame &game = *control->GetVisionaire();

	for (int table = 0; table < 40; table++) {
		TVList objects;

		game.GetList(table, objects, false);

		for (TVisionaireObject *object : objects) {
			std::wstring name = lower(object->GetName().c_str().c_str());

			for (int i = 1; i < argc; i++) {
				std::wstring wanted = lower(wxString(argv[i]).ToStdWstring());

				if (name.find(wanted) != std::wstring::npos) {
					TVisObjRef parent = TVisObjRef(object).GetParent();

					std::printf("table %d id %d '%ls' (of %ls)\n", table, object->GetId24(), object->GetName().c_str().c_str(),
					            parent.IsEmpty() ? L"-" : parent.GetName().c_str().c_str());
					break;
				}
			}
		}
	}
	return 0;
}

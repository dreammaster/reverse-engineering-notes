#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TMSavegame.h"
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

// Loads a savegame (or bookmark) slot of the real data: slotload <b|s> <slot> (run from the game directory; GAME_PASSW).
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

	if (!control->PreLoad(file, warning, true)) {
		std::printf("not preloaded\n");
		return 1;
	}
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	if (!control->LoadAndInitGame(file, warning, language, true)) {
		std::printf("not loaded\n");
		return 1;
	}

	TMSavegame::InitSaveGamePath();
	bool bookmark = argc > 1 && argv[1][0] == 'b';
	int slot = argc > 2 ? atoi(argv[2]) : 0;
	TMSavegame save(bookmark, slot, 0, 0, control->GetVisionaire());

	save.CheckVisPaths();
	std::printf("loading %s %d\n", bookmark ? "bookmark" : "savegame", slot);
	bool result = control->LoadGame(&save);
	std::printf("result: %d\n", result ? 1 : 0);
	return 0;
}

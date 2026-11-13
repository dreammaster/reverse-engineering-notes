#include <cstdio>
#include <cstdlib>
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

static int marquisSpeed(TVisionaireGame *game) {
	TVList outfits;

	game->GetList(17, outfits, false);

	for (TVisionaireObject *outfit : outfits) {
		if (TVisObjRef(outfit).GetParent().GetName() == TCharHolder(L"Marquis") && outfit->GetName() == TCharHolder(L"standard"))
			return outfit->GetInt(kOutfitCharacterSpeed);
	}
	return -2;
}

// Saves the game of the real data to a slot and loads it again: what happens to the speed of an outfit (run from the game directory; GAME_PASSW).
int main() {
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

	TVisionaireGame *game = control->GetVisionaire();

	std::printf("before: %d\n", marquisSpeed(game));
	control->SaveGame(7);
	std::printf("saved: %d\n", marquisSpeed(game));
	control->LoadGame(7);
	std::printf("after: %d\n", marquisSpeed(game));
	return 0;
}

#define private public
#define protected public
#include <chrono>
#include <thread>
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

	if (argc > 4) {
		// walk: slotload <b|s> <slot> <x> <y>: the current character walks to (x, y); the direction is printed at each step
		TGCharacter *rufus = control->GetCurrentCharacter();
		int x = atoi(argv[3]);
		int y = atoi(argv[4]);

		std::printf("character at %d,%d dir %d state %d outfit speed %d walk anims %d\n", rufus->_position.x, rufus->_position.y,
		            rufus->_objRef.GetInt(kCharacterDirection), rufus->_state, rufus->_outfit.GetInt(kOutfitCharacterSpeed),
		            (int)rufus->_walkAnimations.size());
		for (size_t i = 0; i < rufus->_walkDirections.size(); i++)
			std::printf("walk anim %d direction %d\n", (int)i, rufus->_walkDirections[i]);
		std::printf("harmonize %d\n", rufus->_harmonizeWalk ? 1 : 0);

		const FloatPoint &scroll = control->GetScene()->GetFloatScrollPos();
		wxPoint resolution = *control->GetVisionaire()->GetGame().GetPoint(kGameWindowResolution);

		std::printf("scroll %.0f,%.0f window %d,%d\n", scroll.x, scroll.y, resolution.x, resolution.y);
		rufus->SetFreeDestination(wxPoint{x, y}, false, false, false);
		std::printf("started: state %d dir %d pos %d,%d\n", rufus->_state, rufus->_objRef.GetInt(kCharacterDirection), rufus->_position.x,
		            rufus->_position.y);

		for (int step = 0; step < 600 && rufus->_state == 3; step++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			control->Update();
			rufus->WalkWay();

			int direction = rufus->_objRef.GetInt(kCharacterDirection);

			if (step % 10 == 0)
				std::printf("step %d pos %d,%d dir %d index %d lastWalk %d/%d anim %s\n", step, rufus->_position.x, rufus->_position.y,
				            direction, rufus->_directionIndex, rufus->_lastWalkIndex, rufus->_lastWalkIndex2,
				            rufus->_currentAnimation ? "yes" : "none");
		}
	}
	return 0;
}

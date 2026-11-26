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
#include "TGAction.h"
#include "TGCharacter.h"
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

	if (argc > 3 && std::string(argv[3]) == "act") {
		// action: slotload <b|s> <slot> act <id>: the action with that id (table 7) is started and the game runs for 8 seconds; what the
		// current character does is printed every 100 ms
		TGCharacter *rufus = control->GetCurrentCharacter();
		int id = argc > 4 ? atoi(argv[4]) : 0;

		g_traceFlags |= 2;
		wxLog::loglevel = 3;
		TVList actions;

		control->GetVisionaire()->GetList(7, actions, false);

		for (TVisionaireObject *action : actions) {
			if (action->GetId24() != id)
				continue;

			TVisObjRef ref(action);

			std::printf("starting action %d\n", id);
			TGAction::AddRunningAction(ref);
		}

		TVisObjRef combine;
		TVList animationList;

		control->GetVisionaire()->GetList(9, animationList, false);
		for (TVisionaireObject *animation : animationList) {
			if (animation->GetId24() == 6272)
				combine.Set(animation);
		}

		std::printf("current character %p\n", (void *)rufus);

		for (TGCharacter *character : control->GetAllCharacters())
			std::printf("character %p '%ls' id %d scene '%ls'\n", (void *)character, character->_objRef.GetName().c_str().c_str(),
			            PackVisId(character->_objRef.GetId()), character->_objRef.GetLink(kCharacterScene).GetName().c_str().c_str());
		std::printf("combine parent type %d\n", (int)combine.GetParent().GetId()[3]);

		for (int step = 0; step < 400; step++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			control->Update();

			if (step % 5 == 0) {
				std::printf("step %d combineRunning %d state %d animState %d anim %s runningActions %d\n", step,
				            TGAnimation::AnimationIsRunning(combine) ? 1 : 0, rufus->_state,
				            rufus->_objRef.GetInt(kCharacterAnimState), rufus->_currentAnimation ? "yes" : "none",
				            (int)TGAction::GetRunningActions().size());

				if (rufus->_currentAnimation) {
					TGAnimation *animation = rufus->_currentAnimation;
					TPictureIO *sprite = animation->GetCurrentSprite();

					std::printf("   anim data %d frames %d index %d sprite %s", PackVisId(animation->GetDataObject().GetId()),
					            animation->GetFrameCount(), animation->GetCurrentSpriteIndex(), sprite ? "yes" : "none");

					if (sprite) {
						wxRect rect = sprite->GetDestRect();

						std::printf(" rect %d,%d %dx%d", rect.x, rect.y, rect.width, rect.height);
					}

					wxPoint position = animation->GetCurrentSpritePosition();

					std::printf(" at %d,%d\n", position.x, position.y);
				}
			}
		}
		return 0;
	}

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

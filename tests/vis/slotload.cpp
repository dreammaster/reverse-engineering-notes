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
#include "TGScene.h"
#include "TSceneActionArea.h"
#include "vsplayer/control/masterControl.h"
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

	if (argc > 3 && std::string(argv[3]) == "cond") {
		// cond: slotload <b|s> <slot> cond <part of a name>...: the conditions (table 10) and values (table 20) of the loaded game
		// whose name has one of the texts, with what they are
		for (int table : {10, 20}) {
			TVList list;

			control->GetVisionaire()->GetList(table, list, false);

			for (TVisionaireObject *object : list) {
				std::wstring name = object->GetName().c_str().c_str();

				for (int i = 4; i < argc; i++) {
					if (name.find(wxString(argv[i]).ToStdWstring()) == std::wstring::npos)
						continue;

					if (table == 10)
						std::printf("condition '%ls' = %d\n", name.c_str(), object->GetBool(kConditionValue) ? 1 : 0);
					else
						std::printf("value '%ls' = %d\n", name.c_str(), object->GetInt(kValueInt));
					break;
				}
			}
		}
		return 0;
	}

	if (argc > 3 && std::string(argv[3]) == "click") {
		// click: slotload <b|s> <slot> click <part of an object's name> [double]: the objects of the shown scene whose name has the text
		// are listed, the first is clicked (a double click with `double`) and the game runs for 12 seconds, the character printed
		TGCharacter *rufus = control->GetCurrentCharacter();
		std::wstring wanted = argc > 4 ? wxString(argv[4]).ToStdWstring() : std::wstring();
		bool doubleClick = argc > 5 && std::string(argv[5]) == "double";
		TGScene *scene = control->GetScene();
		const FloatPoint &scroll = scene->GetFloatScrollPos();
		TManagedObject *target = nullptr;

		g_traceFlags |= 2;
		wxLog::loglevel = 3;

		// `way:<id>` as the fifth argument: the way system (table 29) with that id is made the scene's before the click
		if (argc > 5 && std::string(argv[5]).compare(0, 4, "way:") == 0) {
			int wayId = atoi(argv[5] + 4);
			TVList ways;

			control->GetVisionaire()->GetList(29, ways, false);

			for (TVisionaireObject *way : ways) {
				if (way->GetId24() == wayId) {
					TVisObjRef sceneRef = scene->GetRef();
					sceneRef.SetLink(kSceneCurrentWaySystem, TVisObjRef(way), true);
					std::printf("way system set to '%ls'\n", way->GetName().c_str().c_str());
				}
			}
		}

		for (TGCharacter *character : control->GetAllCharacters()) {
			if (character->_objRef.GetLink(kCharacterScene) == scene->GetRef())
				std::printf("character '%ls' at %d,%d active %d state %d\n", character->_objRef.GetName().c_str().c_str(),
				            character->_position.x, character->_position.y, character->_active ? 1 : 0, character->_state);
		}

		std::printf("scene '%ls' scroll %.0f,%.0f character at %d,%d dir %d\n", scene->GetRef().GetName().c_str().c_str(), scroll.x, scroll.y,
		            rufus->_position.x, rufus->_position.y, rufus->_objRef.GetInt(kCharacterDirection));

		// the action areas of the scene, and whether Rufus and the places of the tunnel are in them
		if (std::getenv("SHOWAREAS")) {
			std::list<TSceneActionArea *> *areas = TGScene::GetActionAreas(scene->GetRef());

			if (areas) {
				for (TSceneActionArea *area : *areas)
					std::printf("area '%ls' (id %d): Rufus %d, 1361,672 %d, 1073,686 %d, 1176,686 %d\n", area->GetName().c_str().c_str(),
					            PackVisId(area->GetId()), area->IsInside(rufus->_position) ? 1 : 0, area->IsInside(wxPoint{1361, 672}) ? 1 : 0,
					            area->IsInside(wxPoint{1073, 686}) ? 1 : 0, area->IsInside(wxPoint{1176, 686}) ? 1 : 0);
			}
		}

		for (TManagedObject *object : scene->_objects) {
			std::wstring name = object->GetRef().GetName().c_str().c_str();

			if (wanted.empty() || (wanted != L"ALL" && name.find(wanted) == std::wstring::npos))
				continue;

			wxRect rect = object->GetBoundingRect();

			std::printf("object '%ls' rect %d,%d %dx%d walkable %d center %d position %d,%d active %d\n", name.c_str(), rect.x, rect.y,
			            rect.width, rect.height, object->GetRef().GetBool(kObjectIsWalkable) ? 1 : 0, object->GetCenter(),
			            object->GetRef().GetPoint(kObjectPosition)->x, object->GetRef().GetPoint(kObjectPosition)->y,
			            object->IsActive() ? 1 : 0);
			std::printf("   condition '%ls' negate %d value %d\n", object->GetRef().GetLink(kObjectCondition).GetName().c_str().c_str(),
			            object->GetRef().GetBool(kObjectConditionNegate) ? 1 : 0,
			            object->GetRef().GetLink(kObjectCondition).IsEmpty() ? -1 : (object->GetRef().GetLink(kObjectCondition).GetBool(kConditionValue) ? 1 : 0));

			if (!target) {
				target = object;

				TVList candidates;

				object->GetActionList(candidates);

				for (TVisionaireObject *candidate : candidates)
					std::printf("   action '%ls' type %d\n", candidate->GetName().c_str().c_str(), candidate->GetInt(kActionExecutionType));
			}
		}

		// `@x,y` instead of a name: a click at that place of the scene (a character, for instance)
		bool atPoint = !wanted.empty() && wanted[0] == L'@';
		int pointX = 0;
		int pointY = 0;

		if (atPoint)
			std::swscanf(wanted.c_str() + 1, L"%d,%d", &pointX, &pointY);

		if (target || atPoint) {
			wxRect rect = atPoint ? wxRect() : target->GetBoundingRect();

			if (atPoint) {
				rect.x = pointX;
				rect.y = pointY;
				rect.width = 0;
				rect.height = 0;
			}

			wxPoint screen{(int)(rect.x + rect.width / 2 - scroll.x), (int)(rect.y + rect.height / 2 - scroll.y)};

			// the middle of the bounding rectangle may not be on the object: the first place of the rectangle that is, as the
			// scene finds the object at a place
			if (target && scene->GetObject(wxPoint{(int)(screen.x + scroll.x), (int)(screen.y + scroll.y)}) != target) {
				bool found = false;

				for (int y = rect.y; y < rect.y + rect.height && !found; y += 8) {
					for (int x = rect.x; x < rect.x + rect.width && !found; x += 8) {
						if (scene->GetObject(wxPoint{x, y}) == target) {
							screen = wxPoint{(int)(x - scroll.x), (int)(y - scroll.y)};
							found = true;
						}
					}
				}
			}

			if (atPoint)
				target = nullptr;

			TVisObjRef waySystemRef = scene->GetRef().GetLink(kSceneCurrentWaySystem);

			std::printf("way system '%ls' (id %d) has way system %d, points %d\n", waySystemRef.GetName().c_str().c_str(),
			            PackVisId(waySystemRef.GetId()), rufus->_hasWaySystem ? 1 : 0, (int)rufus->_waySystem._wayPoints.size());

			for (TVisionaireObject *point : rufus->_waySystem._wayPoints)
				std::printf("   way point '%ls' at %d,%d\n", point->GetName().c_str().c_str(), point->GetPoint(kPointPosition)->x,
				            point->GetPoint(kPointPosition)->y);

			for (size_t polygon = 0; target && polygon < rufus->_waySystem._borders.size(); polygon++) {
				std::vector<wxPoint> &points = *rufus->_waySystem._borders[polygon];

				std::printf("   polygon %d (%d points):", (int)polygon, (int)points.size());
				for (const wxPoint &point : points)
					std::printf(" %d,%d", point.x, point.y);
				std::printf("\n");

				wxPoint door = target->GetPosition();

				std::printf("   door inside polygon %d: %d, Rufus inside: %d, Rufus->door cuts polygon: %d\n", (int)polygon,
				            PointInPolygon(door.x, door.y, points) ? 1 : 0, PointInPolygon(rufus->_position.x, rufus->_position.y, points) ? 1 : 0,
				            LineCuts(rufus->_position, door, points) ? 1 : 0);
			}

			if (target) {
				wxPoint objectPosition = target->GetPosition();

				std::printf("object position %d,%d offset %d,%d\n", objectPosition.x, objectPosition.y,
				            target->GetRef().GetPoint(kObjectOffset)->x, target->GetRef().GetPoint(kObjectOffset)->y);
			}
			std::printf("clicking at scene %d,%d (screen %d,%d)\n", rect.x + rect.width / 2, rect.y + rect.height / 2, screen.x, screen.y);
			control->HandleMouseMove(screen, false);

			// (the mouse stays over the object for a while: the actions of the hover run, as for a player)
			for (int hover = 0; hover < 30; hover++) {
				std::this_thread::sleep_for(std::chrono::milliseconds(20));
				control->Update();
			}

			TVisObjRef gameRef = control->GetVisionaire()->GetGame();

			std::printf("before the click: active command '%ls' destination command '%ls' event %d\n",
			            gameRef.GetLink(kGameActiveCommand).GetName().c_str().c_str(),
			            gameRef.GetLink(kGameDestinationCommand).GetName().c_str().c_str(), gameRef.GetInt(kGameDestinationEvent));
			control->HandleMouseUp(screen, doubleClick ? TMouseMessageEnum::kLeftDoubleClick : TMouseMessageEnum::kLeftUp);
			std::printf("after the click: active command '%ls' destination command '%ls' event %d\n",
			            gameRef.GetLink(kGameActiveCommand).GetName().c_str().c_str(),
			            gameRef.GetLink(kGameDestinationCommand).GetName().c_str().c_str(), gameRef.GetInt(kGameDestinationEvent));
		}

		for (int step = 0; step < 1500; step++) {
			std::this_thread::sleep_for(std::chrono::milliseconds(20));
			control->Update();
			rufus = control->GetCurrentCharacter();

			// a click now and then, as a player does to go on with a text (SKIPCLICKS in the environment)
			if (std::getenv("SKIPCLICKS") && step > 100 && step % 50 == 0)
				control->HandleMouseUp(wxPoint{300, 900}, TMouseMessageEnum::kLeftUp);

			if (step % 10 == 0)
				std::printf("step %d scene '%ls' char %d,%d state %d dir %d dest %d,%d destObject %s\n", step,
				            control->GetScene()->GetRef().GetName().c_str().c_str(), rufus->_position.x, rufus->_position.y, rufus->_state,
				            rufus->_objRef.GetInt(kCharacterDirection), rufus->_objRef.GetPoint(kCharacterDestination)->x,
				            rufus->_objRef.GetPoint(kCharacterDestination)->y,
				            rufus->_objRef.GetLink(kCharacterDestinationObject).IsEmpty() ? "none" : "set");

			// where the character is in the draw list: the objects drawn just before and after it, and its alpha
			if (step % 10 == 0 || (std::getenv("EVERYSTEP") && step >= 60 && step <= 500)) {
				std::vector<TManagedObject *> &list = control->GetScene()->_drawList;
				int at = -1;

				for (size_t i = 0; i < list.size(); i++) {
					if (list[i] == rufus)
						at = (int)i;
				}

				std::printf("      draw index %d of %d (center %d, visibility %d, active %d, anim state %d)", at, (int)list.size(),
				            rufus->GetCenter(), rufus->_objRef.GetInt(kCharacterVisibility), rufus->_active ? 1 : 0,
				            rufus->_objRef.GetInt(kCharacterAnimState));

				// (the objects of the tunnel: where they are in the draw list and whether they are shown)
				for (size_t i = 0; i < list.size(); i++) {
					std::wstring objectName = list[i]->GetRef().GetName().c_str().c_str();

					if (objectName.find(L"Aufgang") != std::wstring::npos || objectName.find(L"Vordergrund") != std::wstring::npos)
						std::printf(" [%ls #%d active %d]", objectName.c_str(), (int)i, list[i]->IsActive() ? 1 : 0);
				}

				if (at > 0)
					std::printf(" after '%ls' (center %d)", list[at - 1]->GetRef().GetName().c_str().c_str(), list[at - 1]->GetCenter());
				if (at >= 0 && at + 1 < (int)list.size())
					std::printf(" before '%ls' (center %d)", list[at + 1]->GetRef().GetName().c_str().c_str(), list[at + 1]->GetCenter());
				std::printf("\n");
			}
		}
		return 0;
	}

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

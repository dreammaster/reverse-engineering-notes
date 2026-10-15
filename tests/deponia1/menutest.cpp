#include <chrono>
#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// A menu scene is shown while an action waits: the action is stopped for the time of the menu.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;


	{
		TVisionaireGame game;
		game.NewGame();
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
		scene.SetName(TCharHolder("Hall"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		// a menu scene
		TVisObjRef menu = game.CreateObject(4, gameRef, kGameSceneLinks);
		menu.SetName(TCharHolder("Menu"));
		menu.SetValue(kSceneIsMenu, true, TSendEventEnum::kNoEvent);
		TVisObjRef menuStart = game.CreateObject(6, menu, kSceneObjects);
		menuStart.SetName(TCharHolder("MenuStart"));

		auto makeAction = [&](const char *name) {
			TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
			action.SetName(TCharHolder(name));
			return action;
		};
		auto addPartTo = [&](TVisObjRef action, int command) {
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef toMenu = makeAction("ToMenu");
		TVisObjRef toMenuPart = addPartTo(toMenu, kCommandShowScene);
		toMenuPart.SetLink(kActionPartLink, menu, true);
		toMenuPart.SetValue(kActionPartInt, 1, TSendEventEnum::kNoEvent);
		TVisObjRef toHall = makeAction("ToHall");
		TVisObjRef toHallPart = addPartTo(toHall, kCommandShowScene);
		toHallPart.SetLink(kActionPartLink, scene, true);
		toHallPart.SetValue(kActionPartInt, 1, TSendEventEnum::kNoEvent);

		// a long action: mark, wait 2 seconds, mark
		TVisObjRef longAction = makeAction("Long");
		TVisObjRef markA = addPartTo(longAction, kCommandRunPartScript);
		markA.SetValue(kActionPartString, wxString("step_a = (step_a or 0) + 1"), TSendEventEnum::kNoEvent);
		TVisObjRef wait = addPartTo(longAction, kCommandWait);
		wait.SetValue(kActionPartInt, 2, TSendEventEnum::kNoEvent);
		wait.SetValue(kActionPartAltInt, 1, TSendEventEnum::kNoEvent);
		TVisObjRef markB = addPartTo(longAction, kCommandRunPartScript);
		markB.SetValue(kActionPartString, wxString("step_b = (step_b or 0) + 1"), TSendEventEnum::kNoEvent);

		wxFileName out(L"menu.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"menu.dat");
	wxString warning;

	CHECK(control->PreLoad(file, warning, true));
	InitPlayerCommands(control->GetVisionaire(), wxString(L"."), wxString(L"."), wxString(L"."));
	wxString language;
	bool loaded = control->LoadAndInitGame(file, warning, language, true);

	CHECK(loaded);
	if (loaded) {
		control->RegisterEventHandler();
		control->InitAfterLoadingScreen();
		control->ExecuteStartingAction();

		auto frames = [&](int count) {
			for (int i = 0; i < count; i++) {
				TGAction::ContinueRunningActions(false);
				control->Update();
				control->Draw(true);
				wxMilliSleep(5);
			}
		};
		auto click = [&](int x, int y) {
			control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{x, y});
			frames(3);
			control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{x, y});
			control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{x, y});
		};

		auto count = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			int result = lua_isnil(L, -1) ? 0 : (int)lua_tointeger(L, -1);

			lua_settop(L, 0);
			return result;
		};
		auto actionByName = [&](const char *wanted) {
			TVList actions;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, actions);
			for (TVisionaireObject *object : actions) {
				TVisObjRef ref(object);

				if (std::string(ref.GetName().c_str().mb_str()) == wanted)
					return ref;
			}
			return TVisObjRef();
		};
		auto run = [&](double milliseconds) {
			auto start = std::chrono::steady_clock::now();

			while (std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count() < milliseconds)
				frames(1);
		};

		frames(20);

		TGAction::AddRunningAction(actionByName("Long"));
		run(500);
		printf("running: a %d, b %d\n", count("step_a"), count("step_b"));
		CHECK(count("step_a") == 1);
		CHECK(count("step_b") == 0);

		// the menu is shown for longer than the wait still has to take: the action is stopped meanwhile
		TGAction::AddRunningAction(actionByName("ToMenu"));
		run(100);
		CHECK(control->GetScene()->IsMenu());
		run(2500);
		printf("in the menu: a %d, b %d\n", count("step_a"), count("step_b"));
		CHECK(count("step_b") == 0);

		TGAction::AddRunningAction(actionByName("ToHall"));
		run(300);
		CHECK(!control->GetScene()->IsMenu());
		printf("back: a %d, b %d\n", count("step_a"), count("step_b"));
		CHECK(count("step_b") == 0);

		// and it goes on with what was left of the wait (1.5 seconds, less what has passed)
		run(1600);
		printf("later: a %d, b %d\n", count("step_a"), count("step_b"));
		CHECK(count("step_a") == 1);
		CHECK(count("step_b") == 1);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

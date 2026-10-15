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

// The scripting commands that the player has: tweens, event handlers ...
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

		wxFileName out(L"lua.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"lua.dat");
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

		auto run = [&](const char *code) {
			LuaDoString(code);
		};
		auto number = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			double result = lua_isnil(L, -1) ? -1e9 : lua_tonumber(L, -1);

			lua_settop(L, 0);
			return result;
		};

		frames(20);

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		// the tween of a point and of a number by the method of the object
		run("Characters['Hero']:to(400, {Position = {x = 300, y = 300}})");
		frames(5);
		printf("tween started: hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		frames(80);
		printf("tween over: hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(hero.GetPoint(kCharacterPosition)->x == 300);
		CHECK(hero.GetPoint(kCharacterPosition)->y == 300);
		run("Characters['Hero']:to(400, {AnimIndex = 7})");
		frames(80);
		printf("number tween over: anim index %d\n", hero.GetInt(kCharacterAnimIndex));
		CHECK(hero.GetInt(kCharacterAnimIndex) == 7);

		// an event handler for the main loop, and one for the keys
		run("main_loops = 0; function onMain() main_loops = main_loops + 1 end; registerEventHandler('mainLoop', 'onMain')");
		run("keys_seen = 0; function onKey(msg, name, code, flags) keys_seen = keys_seen + 1; return false end; registerEventHandler('keyEvent', 'onKey')");
		frames(10);
		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyDown, wxString(), 'x', 0);
		printf("main loops %d, keys seen %d\n", (int)number("main_loops"), (int)number("keys_seen"));
		CHECK(number("main_loops") >= 9);
		CHECK(number("keys_seen") == 1);
		run("unregisterEventHandler('mainLoop', 'onMain')");
		double before = number("main_loops");

		frames(10);
		CHECK(number("main_loops") == before);

		// the other commands
		run("inside = isPointInsidePolygon({x = 5, y = 5}, {{x = 0, y = 0}, {x = 10, y = 0}, {x = 10, y = 10}, {x = 0, y = 10}}); outside = isPointInsidePolygon({x = 50, y = 5}, {{x = 0, y = 0}, {x = 10, y = 0}, {x = 10, y = 10}, {x = 0, y = 10}})");
		printf("inside %s, outside %s\n", number("inside") != -1e9 ? "set" : "nil", number("outside") != -1e9 ? "set" : "nil");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TMSavegame.h"
#include "vsplayer/animationGame.h"
#include "fakepictures.h"
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

// A game is saved while an action waits and loaded again: the action goes on where it was.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/a.png", 20, 20);
	writeFakePicture("pics/b.png", 20, 20);

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameFadeEffect, 0, TSendEventEnum::kNoEvent);
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

		// an animation of the start object that an action shows (it never ends)
		TVisObjRef blink = game.CreateObject(9, startObject, kObjectAnimations);
		blink.SetName(TCharHolder("Blink"));
		std::vector<TSprite> blinkSprites(2);
		blinkSprites[0].SetPath(TCharHolder("pics/a.png"));
		blinkSprites[0].SetPause(40);
		blinkSprites[1].SetPath(TCharHolder("pics/b.png"));
		blinkSprites[1].SetPause(40);
		blink.SetValue(kAnimationSprites, blinkSprites, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationPosition, wxPoint{100, 100}, TSendEventEnum::kNoEvent);
		blink.SetValue(kAnimationNumberOfLoops, 0, TSendEventEnum::kNoEvent);

		TVisObjRef showBlink = game.CreateObject(7, gameRef, kGameActions);
		showBlink.SetName(TCharHolder("ShowBlink"));
		TVisObjRef showPart = game.CreateObject(8, showBlink, kActionActionParts);
		showPart.SetValue(kActionPartCommand, kCommandShowAnimation, TSendEventEnum::kNoEvent);
		showPart.SetLink(kActionPartLink, blink, true);
		showPart.SetValue(kActionPartInt, 0, TSendEventEnum::kNoEvent);
		showPart.SetValue(kActionPartAltInt2, 0, TSendEventEnum::kNoEvent);

		TVisObjRef hideBlink = game.CreateObject(7, gameRef, kGameActions);
		hideBlink.SetName(TCharHolder("HideBlink"));
		TVisObjRef hidePart = game.CreateObject(8, hideBlink, kActionActionParts);
		hidePart.SetValue(kActionPartCommand, kCommandShowAnimation, TSendEventEnum::kNoEvent);
		hidePart.SetLink(kActionPartLink, blink, true);
		hidePart.SetValue(kActionPartInt, 1, TSendEventEnum::kNoEvent);

		// a long action: mark, wait, mark, wait, mark
		TVisObjRef longAction = game.CreateObject(7, gameRef, kGameActions);
		longAction.SetName(TCharHolder("Long"));
		auto addPart = [&](int command, int value) {
			TVisObjRef part = game.CreateObject(8, longAction, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, 1, TSendEventEnum::kNoEvent);
			return part;
		};
		auto mark = [&](const char *code) {
			TVisObjRef part = addPart(kCommandRunPartScript, 0);
			part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		mark("step_a = (step_a or 0) + 1");
		addPart(kCommandWait, 2);
		mark("step_b = (step_b or 0) + 1");
		addPart(kCommandWait, 2);
		mark("step_c = (step_c or 0) + 1");

		wxFileName out(L"savegame.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"savegame.dat");
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

		frames(20);

		TMSavegame::InitSaveGamePath();

		// the action is started and is in its first wait (the waits are 2 seconds)
		TGAction::AddRunningAction(actionByName("Long"));
		frames(30);
		printf("running: a %d, b %d, c %d\n", count("step_a"), count("step_b"), count("step_c"));
		CHECK(count("step_a") == 1);
		CHECK(count("step_b") == 0);

		// a variable of the scripts of each kind
		LuaDoString("saved_number = 42; saved_text = 'hello'; saved_flag = true; saved_table = {answer = 7, list = {1, 2, 3}}");

		// saved in the first wait
		control->SaveGame(3);

		LuaDoString("saved_number = 0; saved_text = 'changed'; saved_flag = false; saved_table = nil");

		// the action goes on to its end
		for (int i = 0; i < 2000 && count("step_c") == 0; i++)
			frames(1);
		printf("finished: a %d, b %d, c %d\n", count("step_a"), count("step_b"), count("step_c"));
		CHECK(count("step_a") == 1);
		CHECK(count("step_b") == 1);
		CHECK(count("step_c") == 1);

		// loaded: the action is in its first wait again, and goes on from there without its first part
		CHECK(control->LoadGame(3));
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, "step_b");
		lua_pushnil(L);
		lua_setfield(L, LUA_GLOBALSINDEX, "step_c");
		LuaDoString("saved_text_copy = tostring(saved_text); saved_number_copy = saved_number; saved_flag_copy = tostring(saved_flag)");
		lua_getfield(L, LUA_GLOBALSINDEX, "saved_text_copy");
		std::string text = lua_tostring(L, -1) ? lua_tostring(L, -1) : "?";
		lua_settop(L, 0);
		lua_getfield(L, LUA_GLOBALSINDEX, "saved_flag_copy");
		std::string flag = lua_tostring(L, -1) ? lua_tostring(L, -1) : "?";
		lua_settop(L, 0);
		printf("loaded variables: number %d, text %s, flag %s\n", count("saved_number_copy"), text.c_str(), flag.c_str());
		CHECK(count("saved_number") == 42);
		CHECK(text == "hello");
		CHECK(flag == "true");
		LuaDoString("saved_ok = (saved_flag == true) and saved_table ~= nil and saved_table.answer == 7 and #saved_table.list == 3 and saved_table.list[3] == 3");
		lua_getfield(L, LUA_GLOBALSINDEX, "saved_ok");
		CHECK(lua_toboolean(L, -1));
		lua_settop(L, 0);
		frames(10);
		printf("loaded: a %d, b %d, c %d\n", count("step_a"), count("step_b"), count("step_c"));
		CHECK(count("step_b") == 0);
		for (int i = 0; i < 2000 && count("step_c") == 0; i++)
			frames(1);
		printf("finished again: a %d, b %d, c %d\n", count("step_a"), count("step_b"), count("step_c"));
		CHECK(count("step_a") == 1);
		CHECK(count("step_b") == 1);
		CHECK(count("step_c") == 1);

		// a walk is going on when the game is saved: after the load the character goes on
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		hero.SetValue(kCharacterDestination, wxPoint{500, 100}, TSendEventEnum::kSendEvent);
		frames(40);

		wxPoint walking = *hero.GetPoint(kCharacterPosition);

		printf("walking at %d,%d\n", walking.x, walking.y);
		CHECK(walking.x > 100 && walking.x < 500);
		control->SaveGame(4);
		frames(600);
		CHECK(hero.GetPoint(kCharacterPosition)->x == 500);

		CHECK(control->LoadGame(4));
		hero = control->GetCurrentCharacter()->GetRef();

		wxPoint loaded = *hero.GetPoint(kCharacterPosition);

		printf("loaded at %d,%d\n", loaded.x, loaded.y);
		CHECK(loaded.x == walking.x && loaded.y == walking.y);
		frames(600);
		printf("arrived at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(hero.GetPoint(kCharacterPosition)->x == 500 && hero.GetPoint(kCharacterPosition)->y == 100);

		// an animation that an action shows is running when the game is saved: it runs again after the load, even
		// if it was hidden meanwhile
		TGAction::AddRunningAction(actionByName("ShowBlink"));
		frames(20);
		int runningBefore = (int)TGAnimation::GetRunningAnimations().size();
		printf("animations running: %d\n", runningBefore);
		CHECK(runningBefore >= 1);
		control->SaveGame(5);

		TGAction::AddRunningAction(actionByName("HideBlink"));
		frames(20);
		int runningHidden = (int)TGAnimation::GetRunningAnimations().size();
		printf("animations running after hiding: %d\n", runningHidden);
		CHECK(runningHidden < runningBefore);

		CHECK(control->LoadGame(5));
		frames(20);
		int runningLoaded = (int)TGAnimation::GetRunningAnimations().size();
		printf("animations running after the load: %d\n", runningLoaded);
		CHECK(runningLoaded == runningBefore);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

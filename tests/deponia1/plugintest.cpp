#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "graphicslib/graphics.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

#include <lauxlib.h>
#include <lua.h>

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

static void writeText(const char *path, const char *text) {
	std::FILE *file = std::fopen(path, "wb");

	if (file) {
		std::fputs(text, file);
		std::fclose(file);
	}
}

// What Night of the Rabbit's engine has that Deponia 1's has not (build it with `tools/tbuild.py plugintest.cpp --game nightoftherabbit`; it
// does not build against the other games, which have Lua 5.1): conditions that combine a list of conditions (the list of booleans
// that says which are inverted is saved and loaded with the game), a plugin of the disk (plugins/<directory>/config.ini and player.lua) that
// registers Lua functions for action parts of its own, and the hook that a script puts before the action parts of a command.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	wxFileName::Mkdir(wxString(L"plugins"), 0777, 1);
	wxFileName::Mkdir(wxString(L"plugins/demo"), 0777, 1);
	writeText("plugins/demo/config.ini",
	          "[Plugin]\nName=Demo\nAuthor = \"A Tester\"\nVersion=1.0\nDescription=Two action parts\nActionParts=Beep,Boop\n");
	writeText("plugins/demo/player.lua",
	          "beeps = 0\n"
	          "registerActionPart('Beep', function(action, part) beeps = beeps + 1; return true end)\n"
	          "registerActionPart('Nothing', function() end)\n"
	          "hooked = 0\n"
	          "ran = 0\n");

	{
		TVisionaireGame game;
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
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		// the start of the game: the scene is shown
		TVisObjRef start = game.CreateObject(7, gameRef, kGameActions);
		start.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, start, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, start, true);

		// the plugin and the names of the game's own action parts: Beep is the number 10000, Boop 10001
		TVisObjRef plugin = game.CreateObject(39, gameRef, kGamePlugins);
		plugin.SetName(TCharHolder("Demo"));
		plugin.SetValue(kPluginActive, true, TSendEventEnum::kNoEvent);
		plugin.SetValue(kPluginDirectory, wxString(L"demo"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCustomActionParts, std::vector<TCharHolder>{TCharHolder("Beep"), TCharHolder("Boop")}, TSendEventEnum::kNoEvent);

		// an action of a part of Beep, one of Boop (nobody registered a function), one that runs a script, and one of the command 4711
		TVisObjRef actions = game.CreateObject(7, gameRef, kGameActions);
		actions.SetName(TCharHolder("Parts"));
		auto addPart = [&](int command, const char *code) {
			TVisObjRef part = game.CreateObject(8, actions, kActionActionParts);

			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (code)
				part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		addPart(10000, nullptr);
		addPart(10001, nullptr);
		addPart(4711, nullptr);
		addPart(kCommandRunPartScript, "ran = ran + 1");

		// conditions: variables and ones that combine them (a list names each condition once: the link table refuses a pair twice)
		auto variable = [&](const char *name, bool value) {
			TVisObjRef condition = game.CreateObject(10, scene, kSceneConditions);

			condition.SetName(TCharHolder(name));
			condition.SetValue(kConditionIsVariable, true, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionValue, value, TSendEventEnum::kNoEvent);
			return condition;
		};
		TVisObjRef yes = variable("Yes", true);
		TVisObjRef no = variable("No", false);
		TVisObjRef other = variable("Other", true);
		TVisObjRef yes2 = variable("Yes2", true);
		TVisObjRef no2 = variable("No2", false);
		auto combine = [&](const char *name, int op, std::vector<bool> negate, bool negateAll, std::initializer_list<TVisObjRef> parts) {
			TVisObjRef condition = game.CreateObject(10, scene, kSceneConditions);
			TVList list;

			condition.SetName(TCharHolder(name));
			condition.SetValue(kConditionIsVariable, false, TSendEventEnum::kNoEvent);
			for (const TVisObjRef &part : parts)
				list.push_back(part);
			condition.SetValue(kConditionConditions, list, true);
			condition.SetValue(kConditionConditionsNegate, negate, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionOperator, op, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionReturnNegate, negateAll, TSendEventEnum::kNoEvent);
			return condition;
		};
		combine("And3", 0, {false, false, false}, false, {yes, other, yes2});
		combine("And3WithNo", 0, {false, false, false}, false, {yes, no, other});
		combine("AndNegatedNo", 0, {false, true, false}, false, {yes, no, other});
		combine("Or3", 1, {false, false, false}, false, {no, no2, other});
		combine("OrNone", 1, {false, false}, false, {no, no2});
		combine("OrNegatedAll", 1, {false, false}, true, {no, no2});
		combine("One", 0, {true}, false, {no});
		combine("Empty", 0, {}, false, {});
		combine("OtherOperator", 2, {false, false}, false, {yes, yes2});

		wxFileName out(L"plugin.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"plugin.dat");
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
		auto number = [&](const char *name) {
			lua_getglobal(L, name);
			double result = lua_isnil(L, -1) ? -1e9 : lua_tonumber(L, -1);

			lua_settop(L, 0);
			return result;
		};

		frames(20);

		// the conditions were saved with their lists of booleans and are evaluated from them
		auto isTrue = [&](const char *name) {
			TVList all;

			control->GetVisionaire()->GetList(10, all, false);
			for (TVisionaireObject *object : all) {
				if (object->GetName() == TCharHolder(name)) {
					TTCondition condition{TVisObjRef(object)};

					return condition.IsTrue() ? 1 : 0;
				}
			}
			return -1;
		};
		CHECK(isTrue("Yes") == 1);
		CHECK(isTrue("No") == 0);
		CHECK(isTrue("And3") == 1);
		CHECK(isTrue("And3WithNo") == 0);
		CHECK(isTrue("AndNegatedNo") == 1);
		CHECK(isTrue("Or3") == 1);
		CHECK(isTrue("OrNone") == 0);
		CHECK(isTrue("OrNegatedAll") == 1);
		CHECK(isTrue("One") == 1);
		CHECK(isTrue("Empty") == 0);
		CHECK(isTrue("OtherOperator") == 0);

		// the plugin was loaded from the disk: its Lua ran and Beep was registered (Nothing is not one of its names)
		CHECK(number("beeps") == 0);
		CHECK(number("hooked") == 0);

		// the action parts: Beep calls the function, Boop and 4711 find none; the script part runs
		TVisObjRef actions;
		TVisObjRef parts;
		TVList allActions;

		control->GetVisionaire()->GetList(7, allActions, false);
		for (TVisionaireObject *object : allActions) {
			if (object->GetName() == TCharHolder("Parts"))
				parts = TVisObjRef(object);
		}
		CHECK(!parts.IsEmpty());

		LuaDoString("startAction(Actions['Parts'])");
		frames(30);
		CHECK(number("beeps") == 1);
		CHECK(number("ran") == 1);

		// a hook before the command of RunPartScript: it does the part, and the script does not run
		LuaDoString("system.registerActionPartHook(138, 'partHook'); function partHook(part) hooked = hooked + 1; return true end");
		LuaDoString("startAction(Actions['Parts'])");
		frames(30);
		CHECK(number("hooked") == 1);
		CHECK(number("ran") == 1);
		CHECK(number("beeps") == 2);

		// a hook that answers false does not stop the part, and an empty name takes the hook away
		LuaDoString("function partHook(part) hooked = hooked + 1; return false end");
		LuaDoString("startAction(Actions['Parts'])");
		frames(30);
		CHECK(number("hooked") == 2);
		CHECK(number("ran") == 2);
		LuaDoString("system.registerActionPartHook(138, '')");
		LuaDoString("startAction(Actions['Parts'])");
		frames(30);
		CHECK(number("hooked") == 2);
		CHECK(number("ran") == 3);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

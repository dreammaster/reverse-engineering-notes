#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TMSavegame.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The commands that save and load: the autosaves (saved before or after the part, loaded again), the test for a savegame and
// the deletion of one.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameFadeEffect, 0, TSendEventEnum::kNoEvent);

		TVisObjRef hall = game.CreateObject(4, gameRef, kGameSceneLinks);
		hall.SetName(TCharHolder("Hall"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef startObject = game.CreateObject(6, hall, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);
		TVisObjRef ways = game.CreateObject(29, hall, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		hall.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		auto makeAction = [&](const char *name) {
			TVisObjRef result = game.CreateObject(7, gameRef, kGameActions);
			result.SetName(TCharHolder(name));
			return result;
		};
		auto addPart = [&](TVisObjRef owner, int command, int value, int altValue = 0, int altValue2 = 0) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt2, altValue2, TSendEventEnum::kNoEvent);
			return part;
		};
		auto log = [&](TVisObjRef owner, const char *word) {
			TVisObjRef part = addPart(owner, kCommandRunPartScript, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		TVisObjRef a;

		// saved after the part that saves: it goes on from the next one when loaded
		a = makeAction("SaveAfter");
		log(a, "one");
		addPart(a, kCommandAutosave, 1, 7, 1);
		log(a, "two");
		// saved at the part itself
		a = makeAction("SaveHere");
		log(a, "three");
		addPart(a, kCommandAutosave, 1, 8, 0);
		log(a, "four");
		a = makeAction("SaveOut");
		addPart(a, kCommandAutosave, 1, 150, 0);
		log(a, "out");
		a = makeAction("Load7");
		addPart(a, kCommandAutosave, 0, 7);
		log(a, "unreached");
		a = makeAction("LoadOut");
		addPart(a, kCommandAutosave, 0, 150);
		a = makeAction("If7");
		addPart(a, kCommandIfSavegame, 3, 7);
		log(a, "has7");
		a = makeAction("If8");
		addPart(a, kCommandIfSavegame, 3, 8);
		log(a, "has8");
		a = makeAction("IfAny");
		addPart(a, kCommandIfSavegame, 2, 0);
		log(a, "any");
		a = makeAction("IfSelected");
		addPart(a, kCommandIfSavegame, 0, 0);
		log(a, "selected");
		a = makeAction("IfClicked");
		addPart(a, kCommandIfSavegame, 1, 0);
		log(a, "clicked");
		a = makeAction("Delete7");
		addPart(a, kCommandDeleteSavegame, 1, 7);
		a = makeAction("DeleteSelected");
		addPart(a, kCommandDeleteSavegame, 0, 0);
		a = makeAction("SaveGame");
		addPart(a, kCommandGameSave, 1, 0);
		log(a, "game-saved");
		a = makeAction("LoadGame");
		addPart(a, kCommandGameSave, 0, 0);

		wxFileName out(L"cmd4.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmd4.dat");
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
		auto find = [&](const char *name, int table) {
			TVisObjRef found;

			if (!control->GetVisionaire()->GetObjectByName(wxString(name), table, found))
				printf("no object %s\n", name);
			return found;
		};
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			return result;
		};
		auto reset = [&]() {
			LuaDoString("trace = ''", "reset");
		};
		auto run = [&](const char *name, int count = 30) {
			TGAction::AddRunningAction(find(name, 7));
			frames(count);
		};

		frames(20);
		TMSavegame::InitSaveGamePath();
		reset();

		// (what an earlier run left)
		for (int slot : {7, 8})
			control->DeleteSavegame(slot);

		// nothing is saved yet
		run("If7");
		run("If8");
		run("IfAny");
		CHECK(trace() == "");

		// saved after the part: the part is done, and the action goes on
		run("SaveAfter");
		printf("saved after: %s\n", trace().c_str());
		CHECK(trace() == "one two ");
		reset();
		run("If7");
		CHECK(trace() == "has7 ");
		reset();
		run("IfAny");
		printf("any: %s\n", trace().c_str());
		reset();

		// saved at the part: the part itself is done again when it is loaded
		run("SaveHere");
		CHECK(trace() == "three four ");
		reset();
		run("If8");
		CHECK(trace() == "has8 ");
		reset();

		// a slot that is not one of the autosaves does nothing (the action goes on)
		run("SaveOut");
		printf("save out: %s\n", trace().c_str());
		reset();
		run("LoadOut");
		CHECK(trace() == "");

		// the selected one and the one that was clicked on: there is no savegame area
		run("IfSelected");
		run("IfClicked");
		CHECK(trace() == "");

		// the autosave 7 is loaded: the game is the one of its save, and the action goes on after its saving part
		LuaDoString("trace = 'changed '", "change");
		run("Load7");
		printf("loaded: %s\n", trace().c_str());
		reset();

		// deleted
		run("Delete7");
		run("If7");
		CHECK(trace() == "");
		run("If8");
		CHECK(trace() == "has8 ");
		reset();

		// the commands of the game save (the slot of the scene's selection)
		run("SaveGame");
		printf("game save: %s\n", trace().c_str());
		run("LoadGame");
		run("DeleteSelected");
		control->DeleteSavegame(8);
		CHECK(!control->SavegameExists(8));
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

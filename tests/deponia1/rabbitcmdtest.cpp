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

// The commands 161-167 that Night of the Rabbit's actions have and Deponia 1's have not (build it with
// `tools/tbuild.py rabbitcmdtest.cpp --game nightoftherabbit`): if a character has an item (161), the overlay over the picture (162), if a
// script answers true (163), waiting for a variable (164), for a condition (165) and for an action to be over (167). The sound fade
// (166) needs the audio bus rework of this engine and is not reconstructed yet.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();

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

		// the items: the hero has the key, not the coin
		TVisObjRef key = game.CreateObject(6, scene, kSceneObjects);
		key.SetName(TCharHolder("Key"));
		key.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVisObjRef coin = game.CreateObject(6, scene, kSceneObjects);
		coin.SetName(TCharHolder("Coin"));
		coin.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVList items;

		items.push_back(key);
		hero.SetValue(kCharacterItems, items, true);

		// a variable, and a condition
		TVisObjRef counter = game.CreateObject(20, scene, kSceneValues);
		counter.SetName(TCharHolder("Counter"));
		TVisObjRef flag = game.CreateObject(10, scene, kSceneConditions);
		flag.SetName(TCharHolder("Flag"));
		flag.SetValue(kConditionIsVariable, true, TSendEventEnum::kNoEvent);
		flag.SetValue(kConditionValue, false, TSendEventEnum::kNoEvent);

		auto makeAction = [&](const char *name) {
			TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);

			action.SetName(TCharHolder(name));
			return action;
		};
		auto addPart = [&](TVisObjRef action, int command, TVisObjRef link, TVisObjRef altLink, int number, int altNumber,
		                   const char *text) {
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);

			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			if (!altLink.IsEmpty())
				part.SetLink(kActionPartAltLink, altLink, true);
			part.SetValue(kActionPartInt, number, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altNumber, TSendEventEnum::kNoEvent);
			if (text)
				part.SetValue(kActionPartString, wxString(text), TSendEventEnum::kNoEvent);
			return part;
		};
		TVisObjRef none;

		// 161: IF the hero has the key (yes), IF it has the coin (no), then a part after both
		TVisObjRef hasItem = makeAction("HasItem");
		addPart(hasItem, kCommandIfCharacterHasItem, hero, key, 0, 0, nullptr);
		addPart(hasItem, kCommandRunPartScript, none, none, 0, 0, "itemA = itemA + 1");
		addPart(hasItem, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(hasItem, kCommandIfCharacterHasItem, hero, coin, 0, 0, nullptr);
		addPart(hasItem, kCommandRunPartScript, none, none, 0, 0, "itemB = itemB + 1");
		addPart(hasItem, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(hasItem, kCommandRunPartScript, none, none, 0, 0, "itemC = itemC + 1");

		// 163: IF a script answers true / false / something else (taken as true) / answers with the action it runs in
		TVisObjRef ifScript = makeAction("IfScript");
		addPart(ifScript, kCommandIfScript, none, none, 0, 0, "return true");
		addPart(ifScript, kCommandRunPartScript, none, none, 0, 0, "scriptT = scriptT + 1");
		addPart(ifScript, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(ifScript, kCommandIfScript, none, none, 0, 0, "return false");
		addPart(ifScript, kCommandRunPartScript, none, none, 0, 0, "scriptF = scriptF + 1");
		addPart(ifScript, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(ifScript, kCommandIfScript, none, none, 0, 0, "return 5");
		addPart(ifScript, kCommandRunPartScript, none, none, 0, 0, "scriptN = scriptN + 1");
		addPart(ifScript, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(ifScript, kCommandIfScript, none, none, 0, 0, "x = 1");
		addPart(ifScript, kCommandRunPartScript, none, none, 0, 0, "scriptNothing = scriptNothing + 1");
		addPart(ifScript, kCommandEndIf, none, none, 0, 0, nullptr);
		addPart(ifScript, kCommandIfScript, none, none, 0, 0, "return currentAction ~= nil");
		addPart(ifScript, kCommandRunPartScript, none, none, 0, 0, "scriptCurrent = scriptCurrent + 1");
		addPart(ifScript, kCommandEndIf, none, none, 0, 0, nullptr);

		// 164: wait while the counter is not 3 (mode 1), until it changes (0), while it is greater than 5 (3: until at most 5)
		TVisObjRef waitEqual = makeAction("WaitEqual");
		addPart(waitEqual, kCommandWaitValue, counter, none, 1, 3, nullptr);
		addPart(waitEqual, kCommandRunPartScript, none, none, 0, 0, "waitedEqual = waitedEqual + 1");
		TVisObjRef waitChange = makeAction("WaitChange");
		addPart(waitChange, kCommandWaitValue, counter, none, 0, 0, nullptr);
		addPart(waitChange, kCommandRunPartScript, none, none, 0, 0, "waitedChange = waitedChange + 1");
		TVisObjRef waitGreater = makeAction("WaitGreater");
		addPart(waitGreater, kCommandWaitValue, counter, none, 3, 5, nullptr);
		addPart(waitGreater, kCommandRunPartScript, none, none, 0, 0, "waitedGreater = waitedGreater + 1");
		TVisObjRef waitOther = makeAction("WaitOtherMode");
		addPart(waitOther, kCommandWaitValue, counter, none, 9, 0, nullptr);
		addPart(waitOther, kCommandRunPartScript, none, none, 0, 0, "waitedOther = waitedOther + 1");

		// 165: wait until the condition is true (mode 1), until it is not (2), until it changes (0)
		TVisObjRef untilTrue = makeAction("UntilTrue");
		addPart(untilTrue, kCommandWaitCondition, flag, none, 1, 0, nullptr);
		addPart(untilTrue, kCommandRunPartScript, none, none, 0, 0, "untilTrue = untilTrue + 1");
		TVisObjRef untilFalse = makeAction("UntilFalse");
		addPart(untilFalse, kCommandWaitCondition, flag, none, 2, 0, nullptr);
		addPart(untilFalse, kCommandRunPartScript, none, none, 0, 0, "untilFalse = untilFalse + 1");
		TVisObjRef untilChange = makeAction("UntilChange");
		addPart(untilChange, kCommandWaitCondition, flag, none, 0, 0, nullptr);
		addPart(untilChange, kCommandRunPartScript, none, none, 0, 0, "untilChange = untilChange + 1");

		// 167: an action that waits for another to be over
		TVisObjRef slow = makeAction("Slow");
		addPart(slow, kCommandWait, none, none, 150, 0, nullptr);
		addPart(slow, kCommandRunPartScript, none, none, 0, 0, "slowDone = slowDone + 1");
		TVisObjRef waiter = makeAction("Waiter");
		addPart(waiter, kCommandRunPartScript, none, none, 0, 0, "startAction(Actions['Slow'])");
		addPart(waiter, kCommandWaitAction, slow, none, 0, 0, nullptr);
		addPart(waiter, kCommandRunPartScript, none, none, 0, 0, "waiterDone = waiterDone + 1; waiterSawSlow = slowDone");

		// 162: the overlay goes to 50 percent in 100 ms, in the color 0xFF0000
		TVisObjRef overlay = makeAction("Overlay");
		addPart(overlay, kCommandFadeOverlay, none, none, 100, 50, nullptr).SetValue(kActionPartAltInt2, 0xFF0000, TSendEventEnum::kNoEvent);

		wxFileName out(L"rabbitcmd.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"rabbitcmd.dat");
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
		auto find = [&](int table, const char *name) {
			TVList all;

			control->GetVisionaire()->GetList(table, all, false);
			for (TVisionaireObject *object : all) {
				if (object->GetName() == TCharHolder(name))
					return TVisObjRef(object);
			}
			return TVisObjRef();
		};
		auto start = [&](const char *name) {
			std::string code = std::string("startAction(Actions['") + name + "'])";

			LuaDoString(code);
		};

		LuaDoString("itemA = 0 itemB = 0 itemC = 0 scriptT = 0 scriptF = 0 scriptN = 0 scriptNothing = 0 scriptCurrent = 0");
		LuaDoString("waitedEqual = 0 waitedChange = 0 waitedGreater = 0 waitedOther = 0 untilTrue = 0 untilFalse = 0 untilChange = 0");
		LuaDoString("slowDone = 0 waiterDone = 0 waiterSawSlow = -1");
		frames(20);

		// the three default audio mixes are made when the game is loaded (the data of this game has none)
		CHECK(!find(37, "eMusicVolume").IsEmpty());
		CHECK(!find(37, "eSoundVolume").IsEmpty());
		CHECK(!find(37, "eSpeechVolume").IsEmpty());

		// (the items are not part of the saved data: the hero is given the key now)
		TTCharacter(find(0, "Hero")).AddItem(find(6, "Key"), false);

		// 161: the key is the hero's, the coin is not; the part after the ifs runs
		start("HasItem");
		frames(20);
		CHECK(number("itemA") == 1);
		CHECK(number("itemB") == 0);
		CHECK(number("itemC") == 1);

		// 163: true and the answer that is no boolean (taken as true) run their parts, false and no answer: the nothing is also taken as true
		start("IfScript");
		frames(20);
		CHECK(number("scriptT") == 1);
		CHECK(number("scriptF") == 0);
		CHECK(number("scriptN") == 1);
		CHECK(number("scriptNothing") == 1);
		CHECK(number("scriptCurrent") == 1);
		CHECK(lua_gettop(L) == 0);

		// 164: the counter is 0
		TVisObjRef counter = find(20, "Counter");

		CHECK(!counter.IsEmpty());
		start("WaitEqual");
		start("WaitChange");
		start("WaitGreater");
		start("WaitOtherMode");
		frames(20);
		CHECK(number("waitedEqual") == 0);
		CHECK(number("waitedChange") == 0);
		CHECK(number("waitedGreater") == 1); // 0 is not greater than 5: not waiting
		CHECK(number("waitedOther") == 1);   // a mode that is not known does not wait

		counter.SetValue(kValueInt, 2, TSendEventEnum::kSendEvent);
		frames(10);
		CHECK(number("waitedEqual") == 0);
		CHECK(number("waitedChange") == 1);  // 0 changed to 2

		counter.SetValue(kValueInt, 3, TSendEventEnum::kSendEvent);
		frames(10);
		CHECK(number("waitedEqual") == 1);

		// a greater number waits, a smaller one lets it go
		counter.SetValue(kValueInt, 9, TSendEventEnum::kSendEvent);
		start("WaitGreater");
		frames(10);
		CHECK(number("waitedGreater") == 1);
		counter.SetValue(kValueInt, 4, TSendEventEnum::kSendEvent);
		frames(10);
		CHECK(number("waitedGreater") == 2);

		// 165: the condition is false
		TVisObjRef flag = find(10, "Flag");

		CHECK(!flag.IsEmpty());
		start("UntilTrue");
		start("UntilFalse");
		start("UntilChange");
		frames(20);
		CHECK(number("untilTrue") == 0);
		CHECK(number("untilFalse") == 1);
		CHECK(number("untilChange") == 0);

		TTCondition(flag).SetTo(true);
		frames(10);
		CHECK(number("untilTrue") == 1);
		CHECK(number("untilChange") == 1);
		CHECK(number("untilFalse") == 1);

		start("UntilFalse");
		frames(10);
		CHECK(number("untilFalse") == 1);
		TTCondition(flag).SetTo(false);
		frames(10);
		CHECK(number("untilFalse") == 2);

		// 167: the waiter goes on after the slow action is over, and not before
		start("Waiter");
		frames(10);
		CHECK(number("slowDone") == 0);
		CHECK(number("waiterDone") == 0);
		frames(80);
		CHECK(number("slowDone") == 1);
		CHECK(number("waiterDone") == 1);
		CHECK(number("waiterSawSlow") == 1);

		// 162: the overlay
		TVisObjRef gameRecord = control->GetVisionaire()->GetGame();

		CHECK(gameRecord.GetInt(kGameOverlayVisibility) == 0);
		start("Overlay");
		frames(3);
		CHECK(gameRecord.GetInt(kGameOverlayColor) == 0xFF0000);
		CHECK(gameRecord.GetInt(kGameOverlayVisibility) > 0 && gameRecord.GetInt(kGameOverlayVisibility) < 50);
		frames(60);
		CHECK(gameRecord.GetInt(kGameOverlayVisibility) == 50);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

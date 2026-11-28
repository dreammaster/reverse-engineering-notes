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

// IF / ELSE / END IF (nested), GOTO, values and conditions of an action.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

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

		// the variables of the scripts: a counter and a condition
		TVisObjRef counter = game.CreateObject(20, scene, kSceneValues);
		counter.SetName(TCharHolder("Counter"));
		TVisObjRef flag = game.CreateObject(10, scene, kSceneConditions);
		flag.SetName(TCharHolder("Flag"));

		TVisObjRef program = game.CreateObject(7, gameRef, kGameActions);
		program.SetName(TCharHolder("Program"));
		int partNumber = 0;
		auto addPart = [&](int command, TVisObjRef link, int value, int altValue = 0) {
			TVisObjRef part = game.CreateObject(8, program, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			partNumber++;
			return part;
		};
		TVisObjRef none;
		auto log = [&](const char *word) {
			TVisObjRef part = addPart(kCommandRunPartScript, none, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		// counter = 0; the loop: counter += 1 ...
		addPart(kCommandSetValue, counter, 0, 0);                 // 1
		addPart(kCommandSetValue, counter, 1, 1);                 // 2  (loop start)
		log("a");                                                 // 3
		addPart(kCommandIfValue, counter, 0, 2);                  // 4  IF counter == 2
		log("two");                                               // 5
		addPart(kCommandIfValue, counter, 4, 2);                  // 6    IF counter >= 2
		log("ge");                                                // 7
		addPart(kCommandElse, none, 0);                           // 8    ELSE
		log("lt");                                                // 9
		addPart(kCommandEndIf, none, 0);                          // 10   END IF
		addPart(kCommandElse, none, 0);                           // 11 ELSE
		log("other");                                             // 12
		addPart(kCommandEndIf, none, 0);                          // 13 END IF
		addPart(kCommandIfValue, counter, 3, 3);                  // 14 IF counter < 3
		addPart(kCommandGoto, none, 1, 2);                        // 15   GOTO part 2
		addPart(kCommandEndIf, none, 0);                          // 16 END IF
		log("loop-done");                                         // 17
		// the condition: set, tested in both ways, toggled
		addPart(kCommandSetCondition, flag, 0);                   // 18 flag = true
		addPart(kCommandIfCondition, flag, 0);                    // 19 IF flag
		log("flag-true");                                         // 20
		addPart(kCommandElse, none, 0);                           // 21
		log("flag-false");                                        // 22
		addPart(kCommandEndIf, none, 0);                          // 23
		addPart(kCommandSetCondition, flag, 2);                   // 24 toggle
		addPart(kCommandIfCondition, flag, 0);                    // 25 IF flag
		log("toggled-true");                                      // 26
		addPart(kCommandElse, none, 0);                           // 27
		log("toggled-false");                                     // 28
		addPart(kCommandEndIf, none, 0);                          // 29
		// arithmetic
		addPart(kCommandSetValue, counter, 0, 7);                 // 30 counter = 7
		addPart(kCommandSetValue, counter, 3, 3);                 // 31 counter *= 3   (21)
		addPart(kCommandSetValue, counter, 2, 5);                 // 32 counter -= 5   (16)
		addPart(kCommandSetValue, counter, 4, 4);                 // 33 counter /= 4   (4)
		addPart(kCommandIfValue, counter, 0, 4);                  // 34 IF counter == 4
		log("arithmetic-ok");                                     // 35
		addPart(kCommandEndIf, none, 0);                          // 36
		// a goto that goes on (+2 parts): the next one is skipped
		addPart(kCommandGoto, none, 0, 2);                        // 37 GOTO +2
		log("skipped");                                           // 38
		log("after-goto");                                        // 39

		wxFileName out(L"control.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"control.dat");
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

		TGAction::AddRunningAction(actionByName("Program"));
		frames(40);

		lua_getfield(L, LUA_GLOBALSINDEX, "trace");
		std::string trace = lua_tostring(L, -1) ? lua_tostring(L, -1) : "(nothing)";
		lua_settop(L, 0);
		printf("trace: %s\n", trace.c_str());
		CHECK(trace == "a other a two ge a other loop-done flag-true toggled-false arithmetic-ok after-goto ");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

#include <cmath>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TManagedObject.h"
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

// Objects with conditions: a variable, and a condition made of two others (AND / OR / negated). The object is active when
// its condition is true (the other way round when negated), and follows the condition when a script changes it.
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
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		auto makeVariable = [&](const char *name) {
			TVisObjRef condition = game.CreateObject(10, scene, kSceneConditions);
			condition.SetName(TCharHolder(name));
			condition.SetValue(kConditionIsVariable, true, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionValue, false, TSendEventEnum::kNoEvent);
			return condition;
		};
		auto makeCombination = [&](const char *name, TVisObjRef first, TVisObjRef second, int op, bool negateFirst, bool negateAll) {
			TVisObjRef condition = game.CreateObject(10, scene, kSceneConditions);
			condition.SetName(TCharHolder(name));
			condition.SetValue(kConditionIsVariable, false, TSendEventEnum::kNoEvent);
			condition.SetLink(kConditionCondition1, first, true);
			condition.SetLink(kConditionCondition2, second, true);
			condition.SetValue(kConditionOperator, op, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionCondition1Negate, negateFirst, TSendEventEnum::kNoEvent);
			condition.SetValue(kConditionReturnNegate, negateAll, TSendEventEnum::kNoEvent);
			return condition;
		};
		TVisObjRef a = makeVariable("A");
		TVisObjRef b = makeVariable("B");
		TVisObjRef both = makeCombination("Both", a, b, 0, false, false);
		TVisObjRef either = makeCombination("Either", a, b, 1, false, false);
		TVisObjRef aNotB = makeCombination("ANotB", a, b, 0, false, true);          // not (A and B)
		TVisObjRef notAOrB = makeCombination("NotAOrB", a, b, 1, true, false);      // (not A) or B
		TVisObjRef nested = makeCombination("Nested", both, either, 1, false, false); // (A and B) or (A or B)

		auto makeObject = [&](const char *name, TVisObjRef condition, bool negate, int x) {
			TVisObjRef object = game.CreateObject(6, scene, kSceneObjects);
			object.SetName(TCharHolder(name));
			object.SetValue(kObjectPosition, wxPoint{x, 300}, TSendEventEnum::kNoEvent);
			if (!condition.IsEmpty())
				object.SetLink(kObjectCondition, condition, true);
			object.SetValue(kObjectConditionNegate, negate, TSendEventEnum::kNoEvent);
			return object;
		};
		makeObject("OnA", a, false, 50);
		makeObject("OffA", a, true, 100);
		makeObject("OnBoth", both, false, 150);
		makeObject("OnEither", either, false, 200);
		makeObject("OnNotBoth", aNotB, false, 250);
		makeObject("OnNotAOrB", notAOrB, false, 300);
		makeObject("OnNested", nested, false, 350);
		makeObject("Free", TVisObjRef(), false, 400);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		auto makeAction = [&](const char *name, TVisObjRef variable, int mode) {
			TVisObjRef result = game.CreateObject(7, gameRef, kGameActions);
			result.SetName(TCharHolder(name));
			TVisObjRef part = game.CreateObject(8, result, kActionActionParts);
			part.SetValue(kActionPartCommand, kCommandSetCondition, TSendEventEnum::kNoEvent);
			part.SetLink(kActionPartLink, variable, true);
			part.SetValue(kActionPartInt, mode, TSendEventEnum::kNoEvent);
			return result;
		};
		makeAction("SetA", a, 0);
		makeAction("ClearA", a, 1);
		makeAction("SetB", b, 0);
		makeAction("ClearB", b, 1);
		makeAction("ToggleB", b, 2);

		wxFileName out(L"cond.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cond.dat");
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
		auto actionByName = [&](const char *wanted) {
			TVList actions;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, actions);
			for (TVisionaireObject *object : actions) {
				TVisObjRef ref(object);

				if (std::string(ref.GetName().c_str().mb_str()) == wanted)
					return ref;
			}
			printf("no action %s\n", wanted);
			return TVisObjRef();
		};
		auto isActive = [&](const char *name) {
			TVisObjRef ref;

			if (!control->GetVisionaire()->GetObjectByName(wxString(name), 6, ref)) {
				printf("no object %s\n", name);
				return false;
			}

			TManagedObject *object = control->GetObject(ref);

			return object && object->IsActive();
		};
		auto run = [&](const char *name) {
			TGAction::AddRunningAction(actionByName(name));
			frames(10);
		};
		auto state = [&]() {
			std::string result;

			for (const char *name : {"OnA", "OffA", "OnBoth", "OnEither", "OnNotBoth", "OnNotAOrB", "OnNested", "Free"})
				result += isActive(name) ? '1' : '0';
			return result;
		};

		frames(20);

		// (the objects: on A, off A, on A and B, on A or B, on not (A and B), on (not A) or B, on nested, free)
		printf("A=0 B=0: %s\n", state().c_str());
		CHECK(state() == "01001101");
		run("SetA");
		printf("A=1 B=0: %s\n", state().c_str());
		CHECK(state() == "10011011");
		run("SetB");
		printf("A=1 B=1: %s\n", state().c_str());
		CHECK(state() == "10110111");
		run("ClearA");
		printf("A=0 B=1: %s\n", state().c_str());
		CHECK(state() == "01011111");
		run("ToggleB");
		printf("A=0 B=0: %s\n", state().c_str());
		CHECK(state() == "01001101");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

#include <cmath>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGInterface.h"
#include "TGScene.h"
#include "TManagedObject.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// The commands of an action that work on the interfaces: the active command, show / hide / fade an interface,
// another interface for the character, the object under the mouse.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/bar.png", 200, 100);

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

		// two interfaces of two classes (the editor makes five classes for a new game)
		TVList classes;

		gameRef.GetLinks(kGameInterfaceClasses, TypeOrder::kValue0, classes);
		CHECK(classes.size() >= 2);

		auto classAt = [](const TVList &list, int index) {
			int n = 0;

			for (TVisionaireObject *object : list) {
				if (n++ == index)
					return TVisObjRef(object);
			}
			return TVisObjRef();
		};
		auto makeInterface = [&](const char *name, TVisObjRef interfaceClass) {
			TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
			interface.SetName(TCharHolder(name));
			TSprite background;
			background.SetPath(TCharHolder("pics/bar.png"));
			interface.SetValue(kInterfaceSprite, background, TSendEventEnum::kNoEvent);
			std::vector<wxPoint> shape = {{0, 0}, {200, 0}, {200, 100}, {0, 100}};
			interface.SetValue(kInterfaceBorder, shape, TSendEventEnum::kNoEvent);
			interface.SetValue(kInterfaceDisplacement, 4, TSendEventEnum::kNoEvent);
			interface.SetLink(kInterfaceInterfaceClass, interfaceClass, true);
			return interface;
		};
		auto makeCommand = [&](TVisObjRef interface, const char *name, int x) {
			TVisObjRef command = game.CreateObject(2, interface, kInterfaceButtons);
			command.SetName(TCharHolder(name));
			command.SetValue(kButtonType, 3, TSendEventEnum::kNoEvent);
			command.SetValue(kButtonCommandType, 1, TSendEventEnum::kNoEvent);
			std::vector<wxPoint> area = {{x, 10}, {x + 40, 10}, {x + 40, 50}, {x, 50}};
			command.SetValue(kButtonPolygon, area, TSendEventEnum::kNoEvent);
			return command;
		};

		TVisObjRef bar = makeInterface("Bar", classAt(classes, 0));
		TVisObjRef useCommand = makeCommand(bar, "Use", 10);
		TVisObjRef lookCommand = makeCommand(bar, "Look", 60);
		TVisObjRef hiddenCommand = makeCommand(bar, "Hidden", 85);
		TVisObjRef never = game.CreateObject(10, scene, kSceneConditions);
		never.SetName(TCharHolder("Never"));
		hiddenCommand.SetLink(kButtonCondition, never, true);
		TVisObjRef talkCommand = makeCommand(bar, "Talk", 110);
		bar.SetLink(kInterfaceStandardCommand, useCommand, true);
		TVisObjRef other = makeInterface("Other", classAt(classes, 1));
		TVisObjRef grabCommand = makeCommand(other, "Grab", 10);
		other.SetLink(kInterfaceStandardCommand, grabCommand, true);
		hero.SetLink(kCharacterInterfaces, bar, true);

		// a rock under the mouse, and a statue that is an item holder
		TVisObjRef rock = game.CreateObject(6, scene, kSceneObjects);
		rock.SetName(TCharHolder("Rock"));
		rock.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> rockPolygon = {{380, 280}, {420, 280}, {420, 320}, {380, 320}};
		rock.SetValue(kObjectPolygon, rockPolygon, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		TVisObjRef none;
		auto makeAction = [&](const char *name) {
			TVisObjRef result = game.CreateObject(7, gameRef, kGameActions);
			result.SetName(TCharHolder(name));
			return result;
		};
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value, int altValue = 0, int altValue2 = 0,
		                   TVisObjRef altLink = TVisObjRef()) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			if (!altLink.IsEmpty())
				part.SetLink(kActionPartAltLink, altLink, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt2, altValue2, TSendEventEnum::kNoEvent);
			return part;
		};
		auto log = [&](TVisObjRef owner, const char *word) {
			TVisObjRef part = addPart(owner, kCommandRunPartScript, none, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		TVisObjRef a;

		a = makeAction("SetLook");
		addPart(a, kCommandSetCommand, lookCommand, 0);
		a = makeAction("SetStandard");
		addPart(a, kCommandSetCommand, none, 1);
		a = makeAction("SetNext");
		addPart(a, kCommandSetCommand, none, 2);
		a = makeAction("IfUse");
		addPart(a, kCommandIfCommand, useCommand, 0);
		log(a, "use");
		addPart(a, kCommandElse, none, 0);
		log(a, "not-use");
		addPart(a, kCommandEndIf, none, 0);
		a = makeAction("HideBar");
		addPart(a, kCommandShowHideInterface, classAt(classes, 0), 1);
		a = makeAction("ShowBar");
		addPart(a, kCommandShowHideInterface, classAt(classes, 0), 0);
		a = makeAction("ToggleBar");
		addPart(a, kCommandShowHideInterface, classAt(classes, 0), 2);
		a = makeAction("FadeBar");
		addPart(a, kCommandFadeInterface, classAt(classes, 0), 40, 50);
		a = makeAction("OtherInterface");
		addPart(a, kCommandSetInterface, other, 0);
		a = makeAction("UnsetInterface");
		addPart(a, kCommandSetInterface, none, 0);
		a = makeAction("ObjectUnder");
		addPart(a, kCommandIfCurrentObject, none, 0);
		log(a, "object-under");
		addPart(a, kCommandElse, none, 0);
		log(a, "nothing-under");
		addPart(a, kCommandEndIf, none, 0);
		a = makeAction("NoObjectUnder");
		addPart(a, kCommandIfCurrentObject, none, 1);
		log(a, "no-object");
		addPart(a, kCommandEndIf, none, 0);
		a = makeAction("TalkInactive");
		addPart(a, kCommandSetObjectActive, talkCommand, 0);
		a = makeAction("TalkActive");
		addPart(a, kCommandSetObjectActive, talkCommand, 1);

		wxFileName out(L"cmdinterface.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmdinterface.dat");
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
		auto byName = [&](const char *wanted, int table = -1) {
			if (table >= 0) {
				TVisObjRef found;

				if (!control->GetVisionaire()->GetObjectByName(wxString(wanted), table, found))
					printf("no object %s\n", wanted);
				return found;
			}

			TVList objects;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, objects);
			for (TVisionaireObject *object : objects) {
				TVisObjRef ref(object);

				if (std::string(ref.GetName().c_str().mb_str()) == wanted)
					return ref;
			}
			printf("no action %s\n", wanted);
			return TVisObjRef();
		};
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			LuaDoString("trace = ''", "reset");
			return result;
		};
		auto run = [&](const char *name, int count = 10) {
			TGAction::AddRunningAction(byName(name));
			frames(count);
		};

		frames(20);
		LuaDoString("trace = ''", "reset");

		TVisObjRef game = control->GetVisionaire()->GetGame();
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		TVisObjRef bar = byName("Bar", 1);
		TVisObjRef other = byName("Other", 1);
		TVisObjRef useCommand = byName("Use", 2);
		TVisObjRef lookCommand = byName("Look", 2);
		TVisObjRef talkCommand = byName("Talk", 2);

		// the active command
		auto nameOf = [&](TVisObjRef ref) {
			return ref.IsEmpty() ? std::string("(none)") : std::string(ref.GetName().c_str().mb_str());
		};

		printf("active command: %s\n", nameOf(game.GetLink(kGameActiveCommand)).c_str());
		run("IfUse");
		CHECK(trace() == "use ");
		run("SetLook");
		CHECK(game.GetLink(kGameActiveCommand) == lookCommand);
		run("IfUse");
		CHECK(trace() == "not-use ");
		run("SetStandard");
		CHECK(game.GetLink(kGameActiveCommand) == useCommand);
		run("SetNext");
		printf("after the next command: %s\n", nameOf(game.GetLink(kGameActiveCommand)).c_str());
		CHECK(game.GetLink(kGameActiveCommand) == lookCommand);
		run("SetNext");
		CHECK(game.GetLink(kGameActiveCommand) == talkCommand);
		run("SetNext");
		CHECK(game.GetLink(kGameActiveCommand) == useCommand);

		// show, hide, toggle and fade an interface of a class
		CHECK(bar.GetBool(kInterfaceVisible));
		run("HideBar");
		CHECK(!bar.GetBool(kInterfaceVisible));
		run("ShowBar");
		CHECK(bar.GetBool(kInterfaceVisible));
		run("ToggleBar");
		CHECK(!bar.GetBool(kInterfaceVisible));
		run("ToggleBar");
		CHECK(bar.GetBool(kInterfaceVisible));
		run("FadeBar");
		printf("bar to %d in %d\n", bar.GetInt(kInterfaceDestVisibility), bar.GetInt(kInterfaceTimeToDestVisibility));
		CHECK(bar.GetInt(kInterfaceDestVisibility) == 40);
		CHECK(bar.GetInt(kInterfaceTimeToDestVisibility) == 50);

		// another interface for the hero
		{
			TVList interfaces;

			hero.GetLinks(kCharacterInterfaces, TypeOrder::kValue0, interfaces);
			CHECK(interfaces.size() == 1);
		}
		run("OtherInterface");
		{
			TVList interfaces;

			hero.GetLinks(kCharacterInterfaces, TypeOrder::kValue0, interfaces);
			printf("the hero has %d interfaces\n", (int)interfaces.size());
			CHECK(interfaces.size() == 2);
		}
		run("UnsetInterface");

		// the object under the mouse
		run("NoObjectUnder");
		CHECK(trace() == "no-object ");
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{400, 300});
		frames(5);
		run("ObjectUnder");
		printf("with the mouse on the rock: %s\n", trace().c_str());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

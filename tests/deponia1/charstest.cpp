#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "graphicslib/graphics.h"
#include "TGInterface.h"
#include "TGPlaceHolder.h"
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

// Two characters in two scenes: the controlled one is changed and the scene and the interface follow.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/bar.png", 200, 200);
	
	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		auto makeScene = [&](const char *name, wxPoint startAt, const char *characterName, TVisObjRef &startOut, TVisObjRef &characterOut, const char *interfaceName, TVisObjRef &interfaceOut) {
			TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
			scene.SetName(TCharHolder(name));
			TVisObjRef character = game.CreateObject(0, gameRef, kGameCharacterLinks);
			character.SetName(TCharHolder(characterName));
			TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
			startObject.SetName(TCharHolder("Start"));
			startObject.SetValue(kObjectPosition, startAt, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterStartObject, startObject, true);
			TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
			std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
			ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
			scene.SetLink(kSceneCurrentWaySystem, ways, true);
			TVisObjRef outfit = game.CreateObject(17, character, kCharacterOutfits);
			outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterCurrentOutfit, outfit, true);

			// the interface of the character
			TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
			interface.SetName(TCharHolder(interfaceName));
			TSprite background;
			background.SetPath(TCharHolder("pics/bar.png"));
			interface.SetValue(kInterfaceSprite, background, TSendEventEnum::kNoEvent);
			std::vector<wxPoint> bar = {{0, 0}, {200, 0}, {200, 200}, {0, 200}};
			interface.SetValue(kInterfaceBorder, bar, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterInterfaces, interface, true);

			startOut = startObject;
			characterOut = character;
			interfaceOut = interface;
			return scene;
		};

		TVisObjRef hallStart, hero, heroBar, kitchenStart, girl, girlBar;
		TVisObjRef hall = makeScene("Hall", wxPoint{100, 200}, "Hero", hallStart, hero, "HeroBar", heroBar);
		TVisObjRef kitchen = makeScene("Kitchen", wxPoint{300, 300}, "Girl", kitchenStart, girl, "GirlBar", girlBar);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		// the start of the game: the hall is shown; a second action switches to the girl
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, hallStart, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		TVisObjRef toGirl = game.CreateObject(7, gameRef, kGameActions);
		toGirl.SetName(TCharHolder("ToGirl"));
		TVisObjRef switchToGirl = game.CreateObject(8, toGirl, kActionActionParts);
		switchToGirl.SetValue(kActionPartCommand, kCommandChangeCharacter, TSendEventEnum::kNoEvent);
		switchToGirl.SetLink(kActionPartLink, girl, true);
		switchToGirl.SetValue(kActionPartInt, 1, TSendEventEnum::kNoEvent);

		TVisObjRef toHero = game.CreateObject(7, gameRef, kGameActions);
		toHero.SetName(TCharHolder("ToHero"));
		TVisObjRef switchToHero = game.CreateObject(8, toHero, kActionActionParts);
		switchToHero.SetValue(kActionPartCommand, kCommandChangeCharacter, TSendEventEnum::kNoEvent);
		switchToHero.SetLink(kActionPartLink, hero, true);
		switchToHero.SetValue(kActionPartInt, 1, TSendEventEnum::kNoEvent);

		wxFileName out(L"chars.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"chars.dat");
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
			frames(10);
		};

		auto name = [&](const TVisObjRef &ref) {
			return std::string(ref.GetName().c_str().mb_str());
		};
		auto activeBars = [&]() {
			std::string result;

			for (TGInterface *interface : control->GetActiveInterfaces())
				result += name(interface->GetRef()) + " ";
			return result;
		};
		auto actionByName = [&](const char *wanted) {
			TVList actions;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, actions);
			for (TVisionaireObject *object : actions) {
				TVisObjRef ref(object);

				if (name(ref) == wanted)
					return ref;
			}
			return TVisObjRef();
		};

		frames(30);
		printf("start: character %s, scene %s, bars %s\n", name(control->GetCurrentCharacter()->GetRef()).c_str(), name(control->GetScene()->GetRef()).c_str(), activeBars().c_str());
		CHECK(name(control->GetCurrentCharacter()->GetRef()) == "Hero");
		CHECK(name(control->GetScene()->GetRef()) == "Hall");
		CHECK(activeBars() == "HeroBar ");

		// to the girl, who is in the kitchen
		TGAction::AddRunningAction(actionByName("ToGirl"));
		frames(60);
		printf("switched: character %s, scene %s, bars %s\n", name(control->GetCurrentCharacter()->GetRef()).c_str(), name(control->GetScene()->GetRef()).c_str(), activeBars().c_str());
		CHECK(name(control->GetCurrentCharacter()->GetRef()) == "Girl");
		CHECK(name(control->GetScene()->GetRef()) == "Kitchen");
		CHECK(activeBars() == "GirlBar ");
		TVisObjRef girlRef = control->GetCurrentCharacter()->GetRef();
		CHECK(name(control->GetVisionaire()->GetGame().GetLink(kGameCurrentCharacter)) == "Girl");
		CHECK(name(control->GetVisionaire()->GetGame().GetLink(kGameCurrentScene)) == "Kitchen");

		// and back
		TGAction::AddRunningAction(actionByName("ToHero"));
		frames(60);
		printf("switched back: character %s, scene %s, bars %s\n", name(control->GetCurrentCharacter()->GetRef()).c_str(), name(control->GetScene()->GetRef()).c_str(), activeBars().c_str());
		CHECK(name(control->GetCurrentCharacter()->GetRef()) == "Hero");
		CHECK(name(control->GetScene()->GetRef()) == "Hall");
		CHECK(activeBars() == "HeroBar ");

		// the girl is in the kitchen all the time; the hero walks in the hall
		CHECK(name(girlRef.GetLink(kCharacterScene)) == "Kitchen");
		click(500, 300);
		frames(500);
		printf("hero at %d,%d\n", control->GetCurrentCharacter()->GetRef().GetPoint(kCharacterPosition)->x, control->GetCurrentCharacter()->GetRef().GetPoint(kCharacterPosition)->y);
		CHECK(control->GetCurrentCharacter()->GetRef().GetPoint(kCharacterPosition)->x == 500);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

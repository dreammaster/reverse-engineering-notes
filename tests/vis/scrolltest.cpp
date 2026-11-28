#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include <algorithm>
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

// A scene twice as wide as the window: the view follows the walking character.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/wide.png", 1280, 480);

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameScrollSpeed, 200, TSendEventEnum::kNoEvent);
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

		// a scene twice as wide as the window
		TSprite background;
		background.SetPath(TCharHolder("pics/wide.png"));
		scene.SetValue(kSceneSprite, background, TSendEventEnum::kNoEvent);
		scene.SetValue(kSceneScrollOnEdges, true, TSendEventEnum::kNoEvent);
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {1280, 0}, {1280, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);
		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		outfit.SetValue(kOutfitCharacterSpeed, 200, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"scroll.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"scroll.dat");
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

		frames(30);

		TGCharacter *character = control->GetCurrentCharacter();
		TVisObjRef hero = character->GetRef();
		auto scroll = [&]() {
			return control->GetScene()->GetScrollPos().x;
		};

		printf("start: hero at %d, scroll %d, worktop %d\n", hero.GetPoint(kCharacterPosition)->x, scroll(), control->GetScene()->GetWorktopWidth());
		CHECK(control->GetScene()->GetWorktopWidth() == 1280);
		CHECK(scroll() == 0);

		// the hero walks to the right end: the view follows him up to the end of the scene
		control->ProcessMessage(static_cast<TMouseMessageEnum>(1), wxPoint{600, 300});
		frames(3);
		control->ProcessMessage(static_cast<TMouseMessageEnum>(3), wxPoint{600, 300});
		control->ProcessMessage(static_cast<TMouseMessageEnum>(4), wxPoint{600, 300});
		int farthest = 0;

		for (int i = 0; i < 300; i++) {
			frames(1);
			farthest = std::max(farthest, scroll());
		}
		printf("hero at %d, scroll %d\n", hero.GetPoint(kCharacterPosition)->x, scroll());
		CHECK(hero.GetPoint(kCharacterPosition)->x == 600);

		// the click was on the visible part: the hero is at 600 and the view has not moved much
		hero.SetValue(kCharacterDestination, wxPoint{1200, 300}, TSendEventEnum::kSendEvent);
		for (int i = 0; i < 600; i++) {
			frames(1);
			farthest = std::max(farthest, scroll());
		}
		printf("hero at %d, scroll %d, farthest %d\n", hero.GetPoint(kCharacterPosition)->x, scroll(), farthest);
		CHECK(hero.GetPoint(kCharacterPosition)->x == 1200);
		CHECK(scroll() > 400);
		CHECK(scroll() <= 640);

		// and back to the left end
		hero.SetValue(kCharacterDestination, wxPoint{100, 300}, TSendEventEnum::kSendEvent);
		for (int i = 0; i < 1000; i++)
			frames(1);
		printf("hero at %d, scroll %d\n", hero.GetPoint(kCharacterPosition)->x, scroll());
		CHECK(hero.GetPoint(kCharacterPosition)->x == 100);
		CHECK(scroll() < 100);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

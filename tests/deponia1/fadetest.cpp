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

// A scene with a rock that runs a script when it is clicked, and a floor the character walks on when it is clicked.
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

		// a rock that counts its clicks
		TVisObjRef rock = game.CreateObject(6, scene, kSceneObjects);
		rock.SetName(TCharHolder("Rock"));
		rock.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> rockPolygon = {{380, 280}, {420, 280}, {420, 320}, {380, 320}};
		rock.SetValue(kObjectPolygon, rockPolygon, TSendEventEnum::kNoEvent);
		TVisObjRef rockAction = game.CreateObject(7, rock, kObjectActions);
		rockAction.SetName(TCharHolder("ClickRock"));
		rockAction.SetValue(kActionExecutionType, 7, TSendEventEnum::kNoEvent);
		TVisObjRef rockScript = game.CreateObject(8, rockAction, kActionActionParts);
		rockScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		rockScript.SetValue(kActionPartString, wxString("rock_clicked = (rock_clicked or 0) + 1"), TSendEventEnum::kNoEvent);

		// a second scene, with an object to go to
		TVisObjRef scene2 = game.CreateObject(4, gameRef, kGameSceneLinks);
		scene2.SetName(TCharHolder("Cellar"));
		TVisObjRef arrival = game.CreateObject(6, scene2, kSceneObjects);
		arrival.SetName(TCharHolder("Arrival"));
		arrival.SetValue(kObjectPosition, wxPoint{300, 300}, TSendEventEnum::kNoEvent);
		TVisObjRef ways2 = game.CreateObject(29, scene2, kSceneWaySystems);
		ways2.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene2.SetLink(kSceneCurrentWaySystem, ways2, true);
		gameRef.SetValue(kGameFadeDelay, 60, TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"click.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"click.dat");
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

		frames(20);

		TVisObjRef game = control->GetVisionaire()->GetGame();
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		TVisObjRef cellar = control->GetVisionaire()->GetGame().GetLink(kGameCurrentScene);
		TVisObjRef hall = cellar;
		TVisObjRef arrivalObject;
		TVisObjRef startObject;
		TVList scenes;

		control->GetVisionaire()->GetList(4, scenes, false);

		for (TVisionaireObject *object : scenes) {
			TVisObjRef scene(object);

			if (scene.GetName() == TCharHolder("Cellar"))
				cellar = scene;
		}

		TVList cellarObjects;

		cellar.GetLinks(kSceneObjects, TypeOrder::kValue0, cellarObjects);
		arrivalObject = TVisObjRef(cellarObjects.front());
		startObject = hero.GetLink(kCharacterStartObject);

		const int effects[] = {0, 1, 2, 3, 4, 5, 6, 8, 9, 11};

		for (int effect : effects) {
			for (int back = 0; back < 2; back++) {
				game.SetValue(kGameFadeEffect, effect, TSendEventEnum::kNoEvent);

				TVisObjRef target = back ? startObject : arrivalObject;

				control->GetSceneControl()->ChangeScene(hero, target, true, -1);
				frames(10);

				TVisObjRef where = hero.GetLink(kCharacterScene);

				if (!(where == target.GetParent())) {
					printf("effect %d (%s): the hero is in the wrong scene\n", effect, back ? "back" : "there");
					failures++;
				}

				CHECK(!control->GetSceneControl()->FadingToNewScene());
				CHECK(control->GetScene()->GetRef() == target.GetParent());
			}
		}

		printf("scene changes done\n");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

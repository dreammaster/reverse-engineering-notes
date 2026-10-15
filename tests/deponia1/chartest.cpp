#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
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

// A character with walking and standing animations (four directions, three pictures each): the right one is shown.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/h1.png", 30, 60);
	writeFakePicture("pics/h2.png", 30, 60);
	writeFakePicture("pics/h3.png", 30, 60);

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

		// walking and standing, for four directions
		for (int kind = 0; kind < 2; kind++) {
			for (int direction = 0; direction < 360; direction += 90) {
				TVisObjRef anim = game.CreateObject(9, outfit, kind == 0 ? kOutfitWalkAnimations : kOutfitStandingAnimations);
				anim.SetName(TCharHolder(kind == 0 ? "Walk" : "Stand"));
				anim.SetValue(kAnimationDirection, direction, TSendEventEnum::kNoEvent);
				std::vector<TSprite> sprites(3);
				for (int i = 0; i < 3; i++) {
					sprites[i].SetPath(TCharHolder(i == 0 ? "pics/h1.png" : i == 1 ? "pics/h2.png" : "pics/h3.png"));
					sprites[i].SetPause(40);
				}
				anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
				if (kind == 0)
					anim.SetValue(kAnimationWalkSteps, std::vector<float>{6.0f, 6.0f, 6.0f}, TSendEventEnum::kNoEvent);
			}
		}

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"char.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"char.dat");
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
		auto distance = [&](int x, int y) {
			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			return std::sqrt((double)(position->x - x) * (position->x - x) + (double)(position->y - y) * (position->y - y));
		};
		auto shown = [&]() {
			// the kind of animation that is shown and its direction (-1 for none)
			TVisObjRef anim = character->GetCharacterAnim();

			return anim.IsEmpty() ? -1 : anim.GetLink(kAnimationSavedObject).GetInt(kAnimationDirection);
		};

		printf("standing: anim state %d, shown direction %d, direction of the hero %d\n", hero.GetInt(kCharacterAnimState), shown(), hero.GetInt(kCharacterDirection));
		CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kStand);
		CHECK(shown() != -1);

		// walking to the right: a walk animation for that direction
		click(500, 200);
		frames(20);
		int walkDirection = hero.GetInt(kCharacterDirection);
		int shownWalking = shown();

		printf("walking right: anim state %d, shown direction %d, direction of the hero %d\n", hero.GetInt(kCharacterAnimState), shownWalking, walkDirection);
		CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kWalk);

		// the animation is the one nearest to the direction
		int nearest = ((walkDirection + 45) / 90 * 90) % 360;

		CHECK(shownWalking == nearest);

		frames(400);
		printf("arrived: at %d,%d, anim state %d, shown direction %d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, hero.GetInt(kCharacterAnimState), shown());
		CHECK(hero.GetPoint(kCharacterPosition)->x == 500);
		CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kStand);

		// walking to the left
		click(100, 200);
		frames(20);
		printf("walking left: anim state %d, shown direction %d, direction of the hero %d\n", hero.GetInt(kCharacterAnimState), shown(), hero.GetInt(kCharacterDirection));
		CHECK(shown() == ((hero.GetInt(kCharacterDirection) + 45) / 90 * 90) % 360);
		CHECK(shown() != shownWalking);
		frames(400);
		CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kStand);

		// and in all other directions: down, up and the diagonals
		const wxPoint targets[] = {{100, 400}, {100, 100}, {300, 300}, {100, 200}};

		for (const wxPoint &target : targets) {
			click(target.x, target.y);
			frames(20);

			int direction = hero.GetInt(kCharacterDirection);
			int walking = shown();

			printf("walking to %d,%d: direction %d, walk animation %d\n", target.x, target.y, direction, walking);
			CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kWalk);
			CHECK(walking == ((direction + 45) / 90 * 90) % 360);

			frames(600);

			direction = hero.GetInt(kCharacterDirection);
			printf("arrived at %d,%d: anim state %d, direction %d, stand animation %d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, hero.GetInt(kCharacterAnimState), direction, shown());
			CHECK(distance(target.x, target.y) < 3);
			CHECK(hero.GetInt(kCharacterAnimState) == (int)TCharacterAnimEnum::kStand);
			CHECK(shown() == ((direction + 45) / 90 * 90) % 360);
		}
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

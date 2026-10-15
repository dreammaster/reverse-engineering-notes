#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TPolygonList.h"
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

// A hall with a wall (a hole in the outline) in the middle: the character walks around it, along the way points.
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

		// the floor with a wall in it, and way points over the wall
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}, kPolygonSeparator, {300, 100}, {340, 100}, {340, 380}, {300, 380}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		TVisObjRef left = game.CreateObject(5, ways, kWaySystemPoints);
		left.SetName(TCharHolder("Left"));
		left.SetValue(kPointPosition, wxPoint{250, 60}, TSendEventEnum::kNoEvent);
		TVisObjRef right = game.CreateObject(5, ways, kWaySystemPoints);
		right.SetName(TCharHolder("Right"));
		right.SetValue(kPointPosition, wxPoint{390, 60}, TSendEventEnum::kNoEvent);
		left.SetLink(kPointRelations, right, true);
		right.SetLink(kPointRelations, left, true);
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

		wxFileName out(L"way.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"way.dat");
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

		frames(20);

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		auto distance = [&](int x, int y) {
			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			return std::sqrt((double)(position->x - x) * (position->x - x) + (double)(position->y - y) * (position->y - y));
		};

		printf("hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);

		// across the wall: along the way points
		double nearLeft = 1e9;
		double nearRight = 1e9;
		bool insideWall = false;

		click(600, 240);
		for (int i = 0; i < 600; i++) {
			frames(1);
			nearLeft = std::min(nearLeft, distance(250, 60));
			nearRight = std::min(nearRight, distance(390, 60));

			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			if (position->x > 300 && position->x < 340 && position->y > 100 && position->y < 380)
				insideWall = true;
		}
		printf("hero after the click across the wall at %d,%d (passed %.0f from the left point, %.0f from the right one)\n",
		       hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, nearLeft, nearRight);
		CHECK(distance(600, 240) < 5);
		CHECK(nearLeft < 5);
		CHECK(nearRight < 5);
		CHECK(!insideWall);

		// and back along the same way
		nearLeft = 1e9;
		nearRight = 1e9;
		click(100, 300);
		for (int i = 0; i < 700; i++) {
			frames(1);
			nearLeft = std::min(nearLeft, distance(250, 60));
			nearRight = std::min(nearRight, distance(390, 60));
		}
		printf("hero after the click back at %d,%d (passed %.0f from the left point, %.0f from the right one)\n",
		       hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, nearLeft, nearRight);
		CHECK(distance(100, 300) < 5);
		CHECK(nearLeft < 5);
		CHECK(nearRight < 5);

		// free walking (the controller): a destination in the floor is walked to straight; one in the wall
		// is stopped at the wall
		click(280, 240);
		frames(300);
		CHECK(distance(280, 240) < 5);

		control->GetCurrentCharacter()->SetFreeDestination(wxPoint{285, 200}, false, true, true);
		frames(100);
		printf("hero after the free walk on the floor at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(distance(285, 200) < 3);

		bool insideWall2 = false;

		control->GetCurrentCharacter()->SetFreeDestination(wxPoint{330, 200}, false, true, true);
		for (int i = 0; i < 200; i++) {
			frames(1);

			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			if (position->x > 300 && position->x < 340 && position->y > 100 && position->y < 380)
				insideWall2 = true;
		}
		printf("hero after the free walk into the wall at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(!insideWall2);
		CHECK(hero.GetPoint(kCharacterPosition)->x > 285);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

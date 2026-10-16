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
		startObject.SetValue(kObjectPosition, wxPoint{100, 420}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		// the floor with a bar across it (open at the right end), and way points above the bar: from below no way point can be
		// seen, so the way goes round the end of the bar
		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}, kPolygonSeparator, {0, 300}, {600, 300}, {600, 320}, {0, 320}};
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

		wxFileName out(L"way2.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"way2.dat");
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

		// (a walk in a straight line first: a way system that has not been used says that the way goes along way points, so the
		// first walk of the other kind ends at the corner, as in the original)
		click(200, 400);
		frames(200);
		CHECK(distance(200, 400) < 5);

		// from below the bar to above it
		bool insideBar = false;
		int rightmost = 0;

		click(300, 100);
		for (int i = 0; i < 1500; i++) {
			frames(1);

			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			if (position->y > 300 && position->y < 320 && position->x < 600)
				insideBar = true;
			if (position->y >= 295 && position->y <= 325)
				rightmost = std::max(rightmost, position->x);
		}
		printf("hero after the click above the bar at %d,%d (rightmost at the bar: %d)\n", hero.GetPoint(kCharacterPosition)->x,
		       hero.GetPoint(kCharacterPosition)->y, rightmost);
		CHECK(distance(300, 100) < 6);
		CHECK(!insideBar);
		CHECK(rightmost >= 595);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

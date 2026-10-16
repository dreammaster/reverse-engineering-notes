#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGWaySystem.h"
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

// A wall across the floor that can be passed above and below: way points at both passages, one of them with two relations
// (their order is by the distance to the destination), and the positions next to the floor that are put on it.
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
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}, kPolygonSeparator,
		                               {300, 40}, {320, 40}, {320, 440}, {300, 440}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);

		auto addPoint = [&](const char *name, int x, int y) {
			TVisObjRef point = game.CreateObject(5, ways, kWaySystemPoints);

			point.SetName(TCharHolder(name));
			point.SetValue(kPointPosition, wxPoint{x, y}, TSendEventEnum::kNoEvent);
			return point;
		};
		TVisObjRef top = addPoint("Top", 310, 20);
		TVisObjRef topFar = addPoint("TopFar", 310, 8);
		TVisObjRef bottom = addPoint("Bottom", 310, 460);

		top.SetLink(kPointRelations, topFar, true);
		top.SetLink(kPointRelations, bottom, true);
		topFar.SetLink(kPointRelations, top, true);
		bottom.SetLink(kPointRelations, top, true);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);

		TVisObjRef outfit = game.CreateObject(17, hero, kCharacterOutfits);
		outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, outfit, true);

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"way3.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"way3.dat");
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

		TGCharacter *character = control->GetCurrentCharacter();
		TVisObjRef hero = character->GetRef();
		auto distance = [&](int x, int y) {
			const wxPoint *position = hero.GetPoint(kCharacterPosition);

			return std::sqrt((double)(position->x - x) * (position->x - x) + (double)(position->y - y) * (position->y - y));
		};

		// a straight walk first (see way2test)
		click(100, 300);
		frames(200);
		CHECK(distance(100, 300) < 5);

		// across the wall: round its upper end (the nearer), by way points
		int highest = 480;

		click(500, 200);
		for (int i = 0; i < 1500; i++) {
			frames(1);
			highest = std::min(highest, hero.GetPoint(kCharacterPosition)->y);
		}
		printf("hero at %d,%d, highest %d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, highest);
		CHECK(distance(500, 200) < 6);
		CHECK(highest < 40);

		// the positions next to the floor (and in the hole of the wall): put on it. A point on the floor stays, a point just outside is put on the edge, a
		// point far outside stays where it is (it is not on any triangle)
		TGWaySystem &floor = character->GetWaySystem();

		CHECK((floor.CheckPosition(wxPoint{200, 200}) == wxPoint{200, 200}));

		wxPoint beside = floor.CheckPosition(wxPoint{-4, 200});

		printf("-4,200 -> %d,%d\n", beside.x, beside.y);
		CHECK(beside.x == 0 && beside.y >= 190 && beside.y <= 210);

		wxPoint above = floor.CheckPosition(wxPoint{200, -6});

		printf("200,-6 -> %d,%d\n", above.x, above.y);
		// (the nearest point of the first triangle that the position is near, which is not always the nearest of the floor: here it
		// is a point of the edge from the corner to the hole)
		CHECK(above.y > -6 && above.y <= 40);

		wxPoint inWall = floor.CheckPosition(wxPoint{311, 200});

		printf("311,200 -> %d,%d\n", inWall.x, inWall.y);
		wxPoint far = floor.CheckPosition(wxPoint{-300, -300});

		CHECK((far == wxPoint{-300, -300}));
		CHECK(!floor.IsWalkingToWayPoint());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

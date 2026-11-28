#include <cmath>
#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "AppGlobals.h"
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

// The matrices that the scripts set for the drawing: the mouse position goes back through the inverse one before the game
// sees it, and the points can be moved through the matrix itself.
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

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"matrix.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"matrix.dat");
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

		// no matrix: the click is where it is
		CHECK(!hasInverseMatrix());
		CHECK(!hasMatrix());
		click(300, 300);
		frames(300);
		CHECK(distance(300, 300) < 5);

		// the inverse matrix of the scripts moves the click by 50 to the right (the point is a row vector times the matrix)
		LuaDoString("graphics.invMatrix1 = {1, 0, 0,  0, 1, 0,  50, 0, 1}", "inverse");
		CHECK(hasInverseMatrix());

		wxPoint moved = transformByInverseMatrix(wxPoint{10, 20});

		printf("the point 10,20 through the inverse matrix: %d,%d\n", moved.x, moved.y);
		CHECK((moved == wxPoint{60, 20}));
		click(100, 100);
		frames(300);
		printf("hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(distance(150, 100) < 5);

		// scaled: the numbers are cut to whole ones
		LuaDoString("graphics.invMatrix1 = {0.5, 0, 0,  0, 0.5, 0,  0, 0, 1}", "scaled");
		moved = transformByInverseMatrix(wxPoint{101, 51});
		CHECK((moved == wxPoint{50, 25}));
		click(400, 200);
		frames(400);
		CHECK(distance(200, 100) < 5);

		// not nine numbers: none
		LuaDoString("graphics.invMatrix1 = {1, 2, 3}", "short");
		CHECK(!hasInverseMatrix());
		LuaDoString("graphics.invMatrix1 = {}", "none");
		CHECK(!hasInverseMatrix());
		click(300, 300);
		frames(300);
		CHECK(distance(300, 300) < 5);

		// the matrix of the drawing: the points are moved through it, the numbers are not cut
		LuaDoString("graphics.matrix1 = {0.5, 0, 0,  0, 0.5, 0,  10, 20, 1}", "matrix");
		CHECK(hasMatrix());

		idVec3 through = transformByMatrix(wxPoint{101, 51});

		printf("through the matrix: %f %f %f\n", through.x, through.y, through.z);
		CHECK(std::fabs(through.x - 60.5f) < 0.001f);
		CHECK(std::fabs(through.y - 45.5f) < 0.001f);
		CHECK(std::fabs(through.z - 1.0f) < 0.001f);
		frames(5);
		LuaDoString("graphics.matrix1 = {}", "clear");
		CHECK(!hasMatrix());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

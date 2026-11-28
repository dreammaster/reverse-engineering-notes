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

// An area of the scene runs actions when the hero walks in and when it walks out.
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

		// the area, with an action for the way in and one for the way out, and an area that is for another character
		TVisObjRef other = game.CreateObject(0, gameRef, kGameCharacterLinks);
		other.SetName(TCharHolder("Other"));

		auto makeAction = [&](const char *name, int executionType, const char *code) {
			TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
			action.SetName(TCharHolder(name));
			action.SetValue(kActionExecutionType, executionType, TSendEventEnum::kNoEvent);
			TVisObjRef part = game.CreateObject(8, action, kActionActionParts);
			part.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
			return action;
		};
		auto makeArea = [&](std::vector<wxPoint> polygon, TVisObjRef forCharacter, TVisObjRef enter, TVisObjRef leave) {
			TVisObjRef area = game.CreateObject(32, scene, kSceneActionAreas);
			area.SetValue(kActionAreaPolygon, polygon, TSendEventEnum::kNoEvent);
			TVisObjRef inAction = game.CreateObject(33, area, kActionAreaActions);
			inAction.SetLink(kAreaActionAction, enter, true);
			if (!forCharacter.IsEmpty())
				inAction.SetLink(kAreaActionCharacter, forCharacter, true);
			else
				inAction.SetLinkAnyObject(kAreaActionCharacter, true);
			TVisObjRef outAction = game.CreateObject(33, area, kActionAreaActions);
			outAction.SetLink(kAreaActionAction, leave, true);
			if (!forCharacter.IsEmpty())
				outAction.SetLink(kAreaActionCharacter, forCharacter, true);
			else
				outAction.SetLinkAnyObject(kAreaActionCharacter, true);
			return area;
		};

		TVisObjRef none;
		TVisObjRef enter = makeAction("Enter", 0x1D, "entered = (entered or 0) + 1");
		TVisObjRef leave = makeAction("Leave", 0x1E, "left = (left or 0) + 1");
		makeArea({{300, 150}, {400, 150}, {400, 250}, {300, 250}}, none, enter, leave);

		TVisObjRef enterOther = makeAction("EnterOther", 0x1D, "entered_other = (entered_other or 0) + 1");
		TVisObjRef leaveOther = makeAction("LeaveOther", 0x1E, "left_other = (left_other or 0) + 1");
		makeArea({{300, 300}, {400, 300}, {400, 400}, {300, 400}}, other, enterOther, leaveOther);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"area.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"area.dat");
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
		auto count = [&](const char *name) {
			lua_getfield(L, LUA_GLOBALSINDEX, name);
			int result = lua_isnil(L, -1) ? 0 : (int)lua_tointeger(L, -1);

			lua_settop(L, 0);
			return result;
		};

		frames(20);

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		CHECK(count("entered") == 0);

		// into the area: the way in runs once
		click(350, 200);
		frames(400);
		printf("hero at %d,%d: entered %d, left %d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y,
		       count("entered"), count("left"));
		CHECK(hero.GetPoint(kCharacterPosition)->x == 350);
		CHECK(count("entered") == 1);
		CHECK(count("left") == 0);

		// staying in it changes nothing
		frames(50);
		CHECK(count("entered") == 1);

		// out of it: the way out runs once
		click(100, 300);
		frames(500);
		CHECK(count("entered") == 1);
		CHECK(count("left") == 1);

		// through the other area, which is for another character: nothing runs
		click(350, 350);
		frames(500);
		CHECK(count("entered_other") == 0);
		CHECK(count("left_other") == 0);
		click(100, 300);
		frames(500);
		CHECK(count("entered_other") == 0);
		CHECK(count("left_other") == 0);

		// in again, and out again
		click(350, 200);
		frames(400);
		click(100, 300);
		frames(500);
		CHECK(count("entered") == 2);
		CHECK(count("left") == 2);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

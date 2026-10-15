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

		// an action of the game that a key starts
		TVisObjRef keyAction = game.CreateObject(7, gameRef, kGameActions);
		keyAction.SetName(TCharHolder("KeyK"));
		keyAction.SetValue(kActionExecutionType, 'k', TSendEventEnum::kNoEvent);
		TVisObjRef keyScript = game.CreateObject(8, keyAction, kActionActionParts);
		keyScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		keyScript.SetValue(kActionPartString, wxString("key_pressed = (key_pressed or 0) + 1"), TSendEventEnum::kNoEvent);

		// an action of the game that the release of a key starts (the key + 10000)
		TVisObjRef upAction = game.CreateObject(7, gameRef, kGameActions);
		upAction.SetName(TCharHolder("KeyJ"));
		upAction.SetValue(kActionExecutionType, 'j' + 10000, TSendEventEnum::kNoEvent);
		TVisObjRef upScript = game.CreateObject(8, upAction, kActionActionParts);
		upScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		upScript.SetValue(kActionPartString, wxString("key_released = (key_released or 0) + 1"), TSendEventEnum::kNoEvent);

		// the action that a long click (the button held) starts
		TVisObjRef holdAction = game.CreateObject(7, gameRef, kGameActions);
		holdAction.SetName(TCharHolder("Hold"));
		TVisObjRef holdScript = game.CreateObject(8, holdAction, kActionActionParts);
		holdScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		holdScript.SetValue(kActionPartString, wxString("hold_seen = (hold_seen or 0) + 1"), TSendEventEnum::kNoEvent);
		gameRef.SetLink(kGameLeftHoldingAction, holdAction, true);

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
		printf("cursor active %d, scene active %d\n", (int)control->GetCursorControl()->IsActive(), (int)control->GetScene()->IsActive());

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		printf("hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);

		click(400, 300);
		frames(300);
		printf("hero after the click on the rock at %d,%d (destination %d,%d, object %d)\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, hero.GetPoint(kCharacterDestination)->x, hero.GetPoint(kCharacterDestination)->y, (int)!hero.GetLink(kCharacterDestinationObject).IsEmpty());
		lua_getfield(L, LUA_GLOBALSINDEX, "rock_clicked");
		printf("rock clicked: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);

		control->ProcessMessage(static_cast<TMouseMessageEnum>(6), wxPoint{300, 300});
		frames(10);
		lua_getfield(L, LUA_GLOBALSINDEX, "hold_seen");
		printf("hold action: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) >= 1);
		lua_settop(L, 0);

		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyDown, wxString(), 'k', 0);
		frames(10);
		lua_getfield(L, LUA_GLOBALSINDEX, "key_pressed");
		printf("key pressed: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);

		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyDown, wxString(), 'j', 0);
		frames(10);
		lua_getfield(L, LUA_GLOBALSINDEX, "key_released");
		CHECK(lua_isnil(L, -1));
		lua_settop(L, 0);
		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyUp, wxString(), 'j', 0);
		frames(10);
		lua_getfield(L, LUA_GLOBALSINDEX, "key_released");
		printf("key released: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);

		click(100, 400);
		frames(150);
		printf("hero after the click on the floor at %d,%d (destination %d,%d)\n", hero.GetPoint(kCharacterPosition)->x,
		       hero.GetPoint(kCharacterPosition)->y, hero.GetPoint(kCharacterDestination)->x,
		       hero.GetPoint(kCharacterDestination)->y);
		CHECK(hero.GetPoint(kCharacterPosition)->y > 300);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

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

// A second character stands in the scene: a click on it (not on the floor) walks the hero to it and runs the action of
// the character for the arrival when the hero is there.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/h1.png", 30, 60);
	writeFakePicture("pics/bar.png", 100, 60);

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
		TVisObjRef friendRef = game.CreateObject(0, gameRef, kGameCharacterLinks);
		friendRef.SetName(TCharHolder("Friend"));
		TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		TVisObjRef friendStart = game.CreateObject(6, scene, kSceneObjects);
		friendStart.SetName(TCharHolder("FriendStart"));
		friendStart.SetValue(kObjectPosition, wxPoint{450, 300}, TSendEventEnum::kNoEvent);
		friendRef.SetLink(kCharacterStartObject, friendStart, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);

		for (TVisObjRef character : {hero, friendRef}) {
			TVisObjRef outfit = game.CreateObject(17, character, kCharacterOutfits);
			outfit.SetValue(kOutfitCharacterSpeed, 200, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterCurrentOutfit, outfit, true);

			// one picture for the walking and the standing of every direction
			for (int kind = 0; kind < 2; kind++) {
				TVList animations;

				outfit.GetLinks(kind == 0 ? kOutfitWalkAnimations : kOutfitStandingAnimations, TypeOrder::kValue0, animations);
				int direction = 0;

				for (TVisionaireObject *object : animations) {
					TVisObjRef anim(object);

					anim.SetValue(kAnimationDirection, direction, TSendEventEnum::kNoEvent);
					direction += 90;

					std::vector<TSprite> sprites(1);
					sprites[0].SetPath(TCharHolder("pics/h1.png"));
					sprites[0].SetPause(40);
					anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
					if (kind == 0)
						anim.SetValue(kAnimationWalkSteps, std::vector<float>{150.0f}, TSendEventEnum::kNoEvent);
				}
			}
		}

		// an interface with a command that is used on objects and characters (its use is 0), the standard one
		TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
		interface.SetName(TCharHolder("Bar"));
		TSprite barBackground;
		barBackground.SetPath(TCharHolder("pics/bar.png"));
		interface.SetValue(kInterfaceSprite, barBackground, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> bar = {{0, 0}, {100, 0}, {100, 60}, {0, 60}};
		interface.SetValue(kInterfaceBorder, bar, TSendEventEnum::kNoEvent);
		interface.SetValue(kInterfaceDisplacement, 4, TSendEventEnum::kNoEvent);
		TVisObjRef talkCommand = game.CreateObject(2, interface, kInterfaceButtons);
		talkCommand.SetName(TCharHolder("Talk"));
		talkCommand.SetValue(kButtonType, 3, TSendEventEnum::kNoEvent);
		talkCommand.SetValue(kButtonCommandType, 1, TSendEventEnum::kNoEvent);
		talkCommand.SetValue(kButtonUse, 0, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> talkArea = {{10, 10}, {50, 10}, {50, 50}, {10, 50}};
		talkCommand.SetValue(kButtonPolygon, talkArea, TSendEventEnum::kNoEvent);
		interface.SetLink(kInterfaceStandardCommand, talkCommand, true);
		hero.SetLink(kCharacterInterfaces, interface, true);

		// the action of the friend: the one of the arrival
		TVisObjRef reached = game.CreateObject(7, friendRef, kCharacterActions);
		reached.SetName(TCharHolder("ReachFriend"));
		reached.SetValue(kActionExecutionType, 6, TSendEventEnum::kNoEvent);
		reached.SetLink(kActionCommand, talkCommand, true);
		TVisObjRef reachedScript = game.CreateObject(8, reached, kActionActionParts);
		reachedScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		reachedScript.SetValue(kActionPartString, wxString("friend_reached = (friend_reached or 0) + 1; hero_x = Characters['Hero'].Position.x"), TSendEventEnum::kNoEvent);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		wxFileName out(L"talk.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"talk.dat");
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
		auto number = [&](const char *name) {
			lua_getglobal(L, name);
			int value = lua_isnumber(L, -1) ? (int)lua_tonumber(L, -1) : -1;
			lua_settop(L, 0);
			return value;
		};

		frames(30);

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		TVisObjRef friendRef;
		control->GetVisionaire()->GetObjectByName(wxString(L"Friend"), 0, friendRef);
		printf("hero at %d,%d, friend at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y,
		       friendRef.GetPoint(kCharacterPosition)->x, friendRef.GetPoint(kCharacterPosition)->y);

		// a click next to the friend walks to the floor
		click(300, 100);
		frames(300);
		CHECK(number("friend_pressed") == -1);

		printf("action place of the friend %d,%d\n", friendRef.GetPoint(kCharacterActionDestPosition)->x, friendRef.GetPoint(kCharacterActionDestPosition)->y);

		// a click on the friend
		click(460, 320);
		frames(5);
		CHECK(number("friend_reached") == -1);
		printf("pressed %d reached %d\n", number("friend_pressed"), number("friend_reached"));
		frames(500);
		printf("pressed %d reached %d, hero at %d,%d\n", number("friend_pressed"), number("friend_reached"),
		       hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(number("friend_reached") == 1);
		CHECK(number("hero_x") > 300);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

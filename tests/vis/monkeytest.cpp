#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "graphicslib/graphics.h"
#include "TGInterface.h"
#include "TGPlaceHolder.h"
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

// A scene with a rock that runs a script when it is clicked, and a floor the character walks on when it is clicked.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/bar.png", 200, 200);
	writeFakePicture("pics/key.png", 40, 40);

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

		// an interface with a button that counts its clicks
		TVisObjRef clickAction = game.CreateObject(7, gameRef, kGameActions);
		clickAction.SetName(TCharHolder("Click"));
		clickAction.SetValue(kActionExecutionType, 7, TSendEventEnum::kNoEvent);
		TVisObjRef clickScript = game.CreateObject(8, clickAction, kActionActionParts);
		clickScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		clickScript.SetValue(kActionPartString, wxString("button_clicked = (button_clicked or 0) + 1"), TSendEventEnum::kNoEvent);
		TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
		interface.SetName(TCharHolder("Bar"));
		TSprite barBackground;
		barBackground.SetPath(TCharHolder("pics/bar.png"));
		interface.SetValue(kInterfaceSprite, barBackground, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> bar = {{0, 0}, {200, 0}, {200, 200}, {0, 200}};
		interface.SetValue(kInterfaceBorder, bar, TSendEventEnum::kNoEvent);
		interface.SetValue(kInterfaceDisplacement, 4, TSendEventEnum::kNoEvent);  // fixed, at its offset (0,0)
		TVisObjRef button = game.CreateObject(2, interface, kInterfaceButtons);
		button.SetName(TCharHolder("Go"));
		std::vector<wxPoint> square = {{10, 10}, {60, 10}, {60, 60}, {10, 60}};
		button.SetValue(kButtonPolygon, square, TSendEventEnum::kNoEvent);
		button.SetLink(kButtonActions, clickAction, true);
		// a place holder for the items of the hero and a command to use them
		TVisObjRef holder = game.CreateObject(2, interface, kInterfaceButtons);
		holder.SetName(TCharHolder("Slot"));
		holder.SetValue(kButtonType, 0, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> slot = {{70, 10}, {130, 10}, {130, 60}, {70, 60}};
		holder.SetValue(kButtonPolygon, slot, TSendEventEnum::kNoEvent);
		TVisObjRef useCommand = game.CreateObject(2, interface, kInterfaceButtons);
		useCommand.SetName(TCharHolder("Use"));
		useCommand.SetValue(kButtonType, 3, TSendEventEnum::kNoEvent);
		useCommand.SetValue(kButtonCommandType, 1, TSendEventEnum::kNoEvent);
		useCommand.SetValue(kButtonDraggable, true, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> useArea = {{10, 100}, {60, 100}, {60, 150}, {10, 150}};
		useCommand.SetValue(kButtonPolygon, useArea, TSendEventEnum::kNoEvent);
		interface.SetLink(kInterfaceStandardCommand, useCommand, true);

		// the key: an item with a picture
		TVisObjRef key = game.CreateObject(6, gameRef, kGameItems);
		key.SetName(TCharHolder("Key"));
		key.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVisObjRef noParent;
		TVisObjRef keySprite = game.CreateObject(13, noParent, -1);
		TSprite keyPicture;
		keyPicture.SetPath(TCharHolder("pics/key.png"));
		keySprite.SetValue(kSpriteSprite, keyPicture, TSendEventEnum::kNoEvent);
		key.SetLink(kObjectSprite, keySprite, true);

		// a rock in the scene: the key used on it runs a script
		TVisObjRef rock = game.CreateObject(6, scene, kSceneObjects);
		rock.SetName(TCharHolder("Rock"));
		rock.SetValue(kObjectPosition, wxPoint{400, 300}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> rockPolygon = {{380, 280}, {420, 280}, {420, 320}, {380, 320}};
		rock.SetValue(kObjectPolygon, rockPolygon, TSendEventEnum::kNoEvent);
		TVisObjRef rockAction = game.CreateObject(7, rock, kObjectActions);
		rockAction.SetName(TCharHolder("UseKeyOnRock"));
		rockAction.SetValue(kActionExecutionType, 6, TSendEventEnum::kNoEvent);
		rockAction.SetLink(kActionCommand, useCommand, true);
		rockAction.SetLink(kActionFixture, key, true);
		TVisObjRef dropAction = game.CreateObject(7, rock, kObjectActions);
		dropAction.SetName(TCharHolder("DropKeyOnRock"));
		dropAction.SetValue(kActionExecutionType, 3, TSendEventEnum::kNoEvent);
		dropAction.SetLink(kActionFixture, key, true);
		TVisObjRef dropScript = game.CreateObject(8, dropAction, kActionActionParts);
		dropScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		dropScript.SetValue(kActionPartString, wxString("key_dropped = (key_dropped or 0) + 1"), TSendEventEnum::kNoEvent);
		TVisObjRef rockScript = game.CreateObject(8, rockAction, kActionActionParts);
		rockScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		rockScript.SetValue(kActionPartString, wxString("key_used = (key_used or 0) + 1"), TSendEventEnum::kNoEvent);
		// a statue: the "use" command on it runs a script, once the hero is there
		TVisObjRef statue = game.CreateObject(6, scene, kSceneObjects);
		statue.SetName(TCharHolder("Statue"));
		statue.SetValue(kObjectPosition, wxPoint{500, 400}, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> statuePolygon = {{480, 380}, {520, 380}, {520, 420}, {480, 420}};
		statue.SetValue(kObjectPolygon, statuePolygon, TSendEventEnum::kNoEvent);
		TVisObjRef statueAction = game.CreateObject(7, statue, kObjectActions);
		statueAction.SetName(TCharHolder("UseStatue"));
		statueAction.SetValue(kActionExecutionType, 6, TSendEventEnum::kNoEvent);
		statueAction.SetLink(kActionCommand, useCommand, true);
		TVisObjRef statueScript = game.CreateObject(8, statueAction, kActionActionParts);
		statueScript.SetValue(kActionPartCommand, kCommandRunPartScript, TSendEventEnum::kNoEvent);
		statueScript.SetValue(kActionPartString, wxString("statue_used = (statue_used or 0) + 1; statue_hero_x = Characters['Hero'].Position.x"), TSendEventEnum::kNoEvent);
		// scroll buttons for the items (one place holder shows one of three items)
		interface.SetValue(kInterfaceScrollStepSize, 1, TSendEventEnum::kNoEvent);
		TVisObjRef scrollBack = game.CreateObject(2, interface, kInterfaceButtons);
		scrollBack.SetName(TCharHolder("Back"));
		scrollBack.SetValue(kButtonType, 1, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> backArea = {{140, 70}, {190, 70}, {190, 120}, {140, 120}};
		scrollBack.SetValue(kButtonPolygon, backArea, TSendEventEnum::kNoEvent);
		TVisObjRef scrollForward = game.CreateObject(2, interface, kInterfaceButtons);
		scrollForward.SetName(TCharHolder("Forward"));
		scrollForward.SetValue(kButtonType, 2, TSendEventEnum::kNoEvent);
		std::vector<wxPoint> forwardArea = {{140, 10}, {190, 10}, {190, 60}, {140, 60}};
		scrollForward.SetValue(kButtonPolygon, forwardArea, TSendEventEnum::kNoEvent);
		TVisObjRef coin = game.CreateObject(6, gameRef, kGameItems);
		coin.SetName(TCharHolder("Coin"));
		coin.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVisObjRef rope = game.CreateObject(6, gameRef, kGameItems);
		rope.SetName(TCharHolder("Rope"));
		rope.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);

		hero.SetLink(kCharacterInterfaces, interface, true);

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		TVisObjRef giveKey = game.CreateObject(8, action, kActionActionParts);
		giveKey.SetValue(kActionPartCommand, kCommandCharacterItem, TSendEventEnum::kNoEvent);
		giveKey.SetLink(kActionPartLink, key, true);
		giveKey.SetValue(kActionPartInt, 0, TSendEventEnum::kNoEvent);
		giveKey.SetLink(kActionPartAltLink, hero, true);
		for (TVisObjRef extra : {coin, rope}) {
			TVisObjRef give = game.CreateObject(8, action, kActionActionParts);
			give.SetValue(kActionPartCommand, kCommandCharacterItem, TSendEventEnum::kNoEvent);
			give.SetLink(kActionPartLink, extra, true);
			give.SetValue(kActionPartInt, 0, TSendEventEnum::kNoEvent);
			give.SetLink(kActionPartAltLink, hero, true);
		}
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

		// random mouse moves, presses, releases, clicks, double clicks, keys and mouse wheels, for a few thousand steps
		unsigned int state = getenv("MONKEY_SEED") ? (unsigned int)atoi(getenv("MONKEY_SEED")) : 987654321u;
		int total = getenv("MONKEY_STEPS") ? atoi(getenv("MONKEY_STEPS")) : 600;
		auto random = [&](int limit) {
			state = state * 1664525u + 1013904223u;
			return (int)((state >> 8) % (unsigned int)limit);
		};
		const int keys[] = {'a', 'k', 'j', ' ', 27, 13, 9, 'x', 'z', 'q'};
		int steps = 0;

		for (; steps < total; steps++) {
			wxPoint where{random(660) - 10, random(500) - 10};

			switch (random(9)) {
			case 0:
			case 1:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(1), where);
				break;
			case 2:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(3), where);
				break;
			case 3:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(4), where);
				break;
			case 4:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(2), where);
				break;
			case 5:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(8), where);
				control->ProcessMessage(static_cast<TMouseMessageEnum>(9), where);
				break;
			case 6:
				control->HandleKeyEvent(TKeyboardMessageEnum::kKeyDown, wxString(), keys[random(10)], 0);
				control->HandleKeyEvent(TKeyboardMessageEnum::kKeyUp, wxString(), keys[random(10)], 0);
				break;
			case 7:
				control->ProcessMessage(static_cast<TMouseMessageEnum>(random(2) ? 12 : 13), where);
				break;
			default:
				click(where.x, where.y);
				break;
			}

			frames(1 + random(3));
		}

		printf("%d random steps done\n", steps);
		CHECK(steps == total);
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

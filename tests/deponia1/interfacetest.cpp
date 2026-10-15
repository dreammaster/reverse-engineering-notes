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
		TVisObjRef keySprite = game.CreateObject(13, gameRef, kSprites);
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
		printf("interfaces: all %d active %d\n", (int)control->GetAllInterfaces().size(), (int)control->GetActiveInterfaces().size());

		for (TGInterface *interface : control->GetActiveInterfaces()) {
			printf("worktop %dx%d inside(30,30) %d inside(300,300) %d\n", interface->GetWorktopWidth(), interface->GetWorktopHeight(), (int)interface->IsInside(wxPoint{30, 30}), (int)interface->IsInside(wxPoint{300, 300}));
			CHECK(interface->GetWorktopWidth() == 200);
			CHECK(interface->IsInside(wxPoint{30, 30}));
			CHECK(!interface->IsInside(wxPoint{300, 300}));
		}

		{
			TVList items;
			control->GetCurrentCharacter()->GetRef().GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			printf("items of the hero: %d\n", (int)items.size());
			CHECK(items.size() == 3);
		}
		for (TGInterface *interface : control->GetActiveInterfaces()) {
			TManagedObject *inSlot = interface->GetObject(wxPoint{100, 30});
			printf("object at the slot: %p, active command: %s\n", (void *)inSlot, interface->GetActiveCommand().IsEmpty() ? "none" : "set");
			CHECK(inSlot != nullptr);
			CHECK(!interface->GetActiveCommand().IsEmpty());
		}

		click(30, 30);
		lua_getfield(L, LUA_GLOBALSINDEX, "button_clicked");
		printf("button clicked: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);

		// the command "use" is chosen, then the item is clicked: it is the used one
		click(30, 125);
		click(100, 30);
		{
			TVisObjRef g = control->GetVisionaire()->GetGame();
			printf("used item set: %d picked %d\n", (int)!g.GetLink(kGameUsedItem).IsEmpty(), (int)g.GetBool(kGameUsedItemPicked));
			CHECK(!g.GetLink(kGameUsedItem).IsEmpty());
		}

		// the key is used on the rock (the hero walks there first, so there is time for it)
		click(400, 300);
		frames(300);
		lua_getfield(L, LUA_GLOBALSINDEX, "key_used");
		printf("key used on the rock: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);

		// the use command on the statue: the hero walks there, the script runs when it is there
		click(30, 125);
		click(500, 400);
		lua_getfield(L, LUA_GLOBALSINDEX, "statue_used");
		printf("statue used at once: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_isnil(L, -1));
		lua_settop(L, 0);
		frames(400);
		lua_getfield(L, LUA_GLOBALSINDEX, "statue_used");
		lua_getfield(L, LUA_GLOBALSINDEX, "statue_hero_x");
		printf("statue used: %d, hero at x %d\n", (int)lua_tointeger(L, -2), (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -2) == 1);
		CHECK(lua_tointeger(L, -1) > 350);
		lua_settop(L, 0);

		// the items are dragged: the item sticks to the cursor and is dropped on the rock
		control->GetVisionaire()->GetGame().SetValue(kGameDraggableItems, true, TSendEventEnum::kNoEvent);
		control->GetVisionaire()->GetGame().SetValue(kGameCommandBehaviour, 0, TSendEventEnum::kNoEvent);
		click(30, 125);
		click(100, 30);
		{
			TVisObjRef g = control->GetVisionaire()->GetGame();

			printf("dragged: used item set %d picked %d\n", (int)!g.GetLink(kGameUsedItem).IsEmpty(), (int)g.GetBool(kGameUsedItemPicked));
			CHECK(!g.GetLink(kGameUsedItem).IsEmpty());
			CHECK(g.GetBool(kGameUsedItemPicked));
		}
		click(400, 300);
		frames(300);
		lua_getfield(L, LUA_GLOBALSINDEX, "key_dropped");
		printf("key dropped on the rock: %d\n", (int)lua_tointeger(L, -1));
		CHECK(lua_tointeger(L, -1) == 1);
		lua_settop(L, 0);
		{
			TVisObjRef g = control->GetVisionaire()->GetGame();

			printf("after the drop: used item set %d picked %d\n", (int)!g.GetLink(kGameUsedItem).IsEmpty(), (int)g.GetBool(kGameUsedItemPicked));
			CHECK(g.GetLink(kGameUsedItem).IsEmpty());
		}

		// the items scroll: one place holder shows one of the three
		auto shownItem = [&]() {
			for (TGInterface *iface : control->GetActiveInterfaces()) {
				TManagedObject *object = iface->GetObject(wxPoint{100, 30});

				if (object)
					return std::string(object->GetRef().GetName().c_str().mb_str());
			}
			return std::string("-");
		};

		printf("shown: %s\n", shownItem().c_str());
		CHECK(shownItem() == "Key");
		click(160, 30);
		printf("after forward: %s\n", shownItem().c_str());
		CHECK(shownItem() == "Coin");
		click(160, 30);
		CHECK(shownItem() == "Rope");
		click(160, 30);
		printf("at the end: %s\n", shownItem().c_str());
		CHECK(shownItem() == "Rope");
		click(160, 90);
		CHECK(shownItem() == "Coin");
		click(160, 90);
		CHECK(shownItem() == "Key");
		click(160, 90);
		printf("back at the start: %s\n", shownItem().c_str());
		CHECK(shownItem() == "Key");
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

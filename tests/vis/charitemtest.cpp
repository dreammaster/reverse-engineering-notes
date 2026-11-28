#include <cstdio>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGEventInfo.h"
#include "graphicslib/graphics.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/eCommand.h"
#include "vstables/fieldIds.h"
#include "vstables/records.h"
#include "vstables/visionaireGame.h"
#include "fakepictures.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

class NullGraphics : public TGraphicsInterface {
};

// Items passing from a character to another (the item dropped on a character, the methods of the character that give), the
// standing of a character, and how a character is turned to an object.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/h1.png", 30, 60);

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
		TVisObjRef buddy = game.CreateObject(0, gameRef, kGameCharacterLinks);
		buddy.SetName(TCharHolder("Buddy"));
		game.CreateObject(1, buddy, kCharacterInterfaces);
		TVisObjRef loner = game.CreateObject(0, gameRef, kGameCharacterLinks);
		loner.SetName(TCharHolder("Loner"));
		TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		buddy.SetLink(kCharacterStartObject, startObject, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
		std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
		ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
		scene.SetLink(kSceneCurrentWaySystem, ways, true);

		// what the characters stand in
		TCharHolder standing("pics/h1.png");

		for (TVisObjRef character : {hero, buddy}) {
			TVisObjRef outfit = game.CreateObject(17, character, kCharacterOutfits);

			character.SetLink(kCharacterCurrentOutfit, outfit, true);

			TVList animations;

			outfit.GetLinks(kOutfitStandingAnimations, TypeOrder::kValue0, animations);
			int direction = 0;

			for (TVisionaireObject *object : animations) {
				TVisObjRef anim(object);

				anim.SetValue(kAnimationDirection, direction, TSendEventEnum::kNoEvent);
				direction += 90;

				std::vector<TSprite> sprites(1);

				sprites[0].SetPath(standing);
				sprites[0].SetPause(40);
				anim.SetValue(kAnimationSprites, sprites, TSendEventEnum::kNoEvent);
			}
		}

		// an item of the hero, and an object that looks to the left
		TVisObjRef key = game.CreateObject(6, scene, kSceneObjects);
		key.SetName(TCharHolder("Key"));
		key.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVisObjRef coin = game.CreateObject(6, scene, kSceneObjects);
		coin.SetName(TCharHolder("Coin"));
		coin.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);
		TVisObjRef statue = game.CreateObject(6, scene, kSceneObjects);
		statue.SetName(TCharHolder("Statue"));
		statue.SetValue(kObjectPosition, wxPoint{400, 200}, TSendEventEnum::kNoEvent);
		statue.SetValue(kObjectDirection, 270, TSendEventEnum::kNoEvent);

		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		// the turn to the statue
		TVisObjRef turn = game.CreateObject(7, gameRef, kGameActions);
		turn.SetName(TCharHolder("Turn"));
		TVisObjRef turnPart = game.CreateObject(8, turn, kActionActionParts);
		turnPart.SetValue(kActionPartCommand, kCommandTurnCharacter, TSendEventEnum::kNoEvent);
		turnPart.SetLink(kActionPartLink, hero, true);
		turnPart.SetLink(kActionPartAltLink, statue, true);

		wxFileName out(L"charitem.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"charitem.dat");
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
		auto find = [&](const char *name, int table) {
			TVList list;

			control->GetVisionaire()->GetList(table, list, false);

			for (TVisionaireObject *object : list) {
				TVisObjRef ref(object);

				if (ref.GetName() == TCharHolder(name))
					return ref;
			}
			return TVisObjRef();
		};
		auto count = [&](TVisObjRef character) {
			TVList items;

			character.GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			return (int)items.size();
		};

		frames(30);

		TGCharacter *heroCharacter = control->GetCurrentCharacter();
		TVisObjRef hero = heroCharacter->GetRef();
		TVisObjRef buddy = find("Buddy", 0);
		TVisObjRef key = find("Key", 6);
		TVisObjRef coin = find("Coin", 6);
		TVisObjRef statue = find("Statue", 6);
		TGCharacter *buddyCharacter = control->GetCharacter(buddy);

		CHECK(!buddy.IsEmpty() && !key.IsEmpty() && !statue.IsEmpty());
		CHECK(buddyCharacter && buddyCharacter != heroCharacter);

		if (buddyCharacter && buddyCharacter != heroCharacter) {
			TTCharacter(hero).AddItem(key, false);
			TTCharacter(hero).AddItem(coin, false);
			CHECK(count(hero) == 2);
			CHECK(count(buddy) == 0);

			// to nobody, or nothing
			TVisObjRef nobody;

			CHECK(!heroCharacter->GiveItemTo(nobody, key));
			CHECK(!heroCharacter->GiveItemTo(buddy, nobody));
			CHECK(count(hero) == 2);

			CHECK(heroCharacter->GiveItemTo(buddy, key));
			CHECK(count(hero) == 1);
			CHECK(count(buddy) == 1);

			// the item dropped on the character: the character brings it, the other has to have an interface
			TGEventInfo info;

			info.action = coin;
			info.character = heroCharacter;
			// (the character himself takes nothing from himself)
			CHECK(!heroCharacter->ReceiveItem(info));
			CHECK(count(hero) == 1);

			// (a character without an interface to show it in takes nothing)
			TVisObjRef loner = find("Loner", 0);
			TGCharacter *lonerCharacter = control->GetCharacter(loner);

			CHECK(lonerCharacter && lonerCharacter != heroCharacter);
			CHECK(!lonerCharacter->ReceiveItem(info));
			CHECK(count(hero) == 1);
			CHECK(count(loner) == 0);

			// the one with an interface takes it from the hero
			CHECK(buddyCharacter->ReceiveItem(info));
			CHECK(count(hero) == 0);
			CHECK(count(buddy) == 2);
			TTCharacter(hero).AddItem(coin, false);
			TTCharacter(buddy).RemoveItem(coin);
			CHECK(count(buddy) == 1);

			TGEventInfo noItem;

			noItem.character = heroCharacter;
			CHECK(!buddyCharacter->ReceiveItem(noItem));

			// with all the items
			heroCharacter->GiveAllItemsTo(buddy);
			CHECK(count(hero) == 0);
			CHECK(count(buddy) == 2);
			buddyCharacter->GiveAllItemsTo(nobody);
			CHECK(count(buddy) == 2);
		}

		// standing: where it is when it stands, anywhere while the state is 0
		wxPoint position = *hero.GetPoint(kCharacterPosition);

		printf("hero at %d,%d, state %d\n", position.x, position.y, hero.GetInt(kCharacterState));
		CHECK(hero.GetInt(kCharacterState) == 2);
		CHECK(heroCharacter->StandingAt(position));
		CHECK(!heroCharacter->StandingAt(wxPoint{position.x + 10, position.y}));

		wxSize dimension = heroCharacter->GetCurrentDimension();

		printf("dimension %dx%d\n", dimension.width, dimension.height);
		CHECK(dimension.width > 0 && dimension.height > 0);
		CHECK(heroCharacter->GetStandingDirection() == -1 || heroCharacter->GetStandingDirection() >= 0);

		// aligned to an object: its direction (none when it has none)
		heroCharacter->AlignToObject(statue);
		CHECK(hero.GetInt(kCharacterDirection) == 270);
		statue.SetValue(kObjectDirection, -1, TSendEventEnum::kNoEvent);
		hero.SetValue(kCharacterDirection, 90, TSendEventEnum::kNoEvent);
		heroCharacter->AlignToObject(statue);
		CHECK(hero.GetInt(kCharacterDirection) == 90);

		// turned to the statue by an action: to the right (0 degrees, counted anticlockwise from the x axis)
		TVisObjRef turn = find("Turn", 7);

		CHECK(!turn.IsEmpty());
		TGAction::AddRunningAction(turn);
		frames(10);
		printf("turned to direction %d\n", hero.GetInt(kCharacterDirection));
		CHECK(hero.GetInt(kCharacterDirection) == 0);
		CHECK(!heroCharacter->TurnCharacter());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

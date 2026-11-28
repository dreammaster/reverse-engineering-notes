#include <cmath>
#include <cstdio>
#include <string>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "THGameControl.h"
#include "TSceneControl.h"
#include "TGAction.h"
#include "TGCharacter.h"
#include "TGScene.h"
#include "TManagedObject.h"
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

// The commands of the actions that no other test runs: the characters (turn, place, walk, outfit, items, follow), the
// scene (scroll, way system, scrollable area), values and objects (random value, move object).
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		TVisObjRef hall = game.CreateObject(4, gameRef, kGameSceneLinks);
		hall.SetName(TCharHolder("Hall"));
		TVisObjRef cellar = game.CreateObject(4, gameRef, kGameSceneLinks);
		cellar.SetName(TCharHolder("Cellar"));
		TVisObjRef hero = game.CreateObject(0, gameRef, kGameCharacterLinks);
		hero.SetName(TCharHolder("Hero"));
		TVisObjRef friendRef = game.CreateObject(0, gameRef, kGameCharacterLinks);
		friendRef.SetName(TCharHolder("Friend"));

		TVisObjRef startObject = game.CreateObject(6, hall, kSceneObjects);
		startObject.SetName(TCharHolder("Start"));
		startObject.SetValue(kObjectPosition, wxPoint{100, 200}, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterStartObject, startObject, true);
		TVisObjRef friendStart = game.CreateObject(6, cellar, kSceneObjects);
		friendStart.SetName(TCharHolder("FriendStart"));
		friendStart.SetValue(kObjectPosition, wxPoint{50, 50}, TSendEventEnum::kNoEvent);
		friendRef.SetLink(kCharacterStartObject, friendStart, true);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		TVisObjRef door = game.CreateObject(6, hall, kSceneObjects);
		door.SetName(TCharHolder("Door"));
		door.SetValue(kObjectPosition, wxPoint{500, 250}, TSendEventEnum::kNoEvent);
		door.SetValue(kObjectDirection, 90, TSendEventEnum::kNoEvent);
		TVisObjRef crate = game.CreateObject(6, hall, kSceneObjects);
		crate.SetName(TCharHolder("Crate"));
		crate.SetValue(kObjectPosition, wxPoint{200, 350}, TSendEventEnum::kNoEvent);

		TVisObjRef key = game.CreateObject(6, hall, kSceneObjects);
		key.SetName(TCharHolder("Key"));
		key.SetValue(kObjectIsItem, true, TSendEventEnum::kNoEvent);

		TVisObjRef firstWays;
		for (TVisObjRef scene : {hall, cellar}) {
			TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
			ways.SetName(TCharHolder("First"));
			if (scene == hall)
				firstWays = ways;
			std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
			ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
			scene.SetLink(kSceneCurrentWaySystem, ways, true);
		}

		TVisObjRef ways2 = game.CreateObject(29, hall, kSceneWaySystems);
		ways2.SetName(TCharHolder("Second"));
		std::vector<wxPoint> border2 = {{0, 0}, {320, 0}, {320, 480}, {0, 480}};
		ways2.SetValue(kWaySystemBorder, border2, TSendEventEnum::kNoEvent);

		TVisObjRef heroOutfit = game.CreateObject(17, hero, kCharacterOutfits);
		heroOutfit.SetName(TCharHolder("Plain"));
		heroOutfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		hero.SetLink(kCharacterCurrentOutfit, heroOutfit, true);
		TVisObjRef heroOutfit2 = game.CreateObject(17, hero, kCharacterOutfits);
		heroOutfit2.SetName(TCharHolder("Fancy"));
		heroOutfit2.SetValue(kOutfitCharacterSpeed, 400, TSendEventEnum::kNoEvent);
		TVisObjRef friendOutfit = game.CreateObject(17, friendRef, kCharacterOutfits);
		friendOutfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
		friendRef.SetLink(kCharacterCurrentOutfit, friendOutfit, true);

		TVisObjRef counter = game.CreateObject(20, hall, kSceneValues);
		counter.SetName(TCharHolder("Counter"));

		// the start of the game: the scene is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, startObject, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		TVisObjRef none;

		// an action of one part (and a log of what it did: "trace" of the script)
		auto makeAction = [&](const char *name) {
			TVisObjRef result = game.CreateObject(7, gameRef, kGameActions);
			result.SetName(TCharHolder(name));
			return result;
		};
		auto addPart = [&](TVisObjRef owner, int command, TVisObjRef link, int value, int altValue = 0, int altValue2 = 0,
		                   TVisObjRef altLink = TVisObjRef()) {
			TVisObjRef part = game.CreateObject(8, owner, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			if (!link.IsEmpty())
				part.SetLink(kActionPartLink, link, true);
			if (!altLink.IsEmpty())
				part.SetLink(kActionPartAltLink, altLink, true);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt2, altValue2, TSendEventEnum::kNoEvent);
			return part;
		};
		auto log = [&](TVisObjRef owner, const char *word) {
			TVisObjRef part = addPart(owner, kCommandRunPartScript, none, 0);
			part.SetValue(kActionPartString, wxString((std::string("trace = (trace or '') .. '") + word + " '").c_str()), TSendEventEnum::kNoEvent);
		};

		TVisObjRef a;

		a = makeAction("Turn");
		addPart(a, kCommandTurnCharacter, none, 0, 0, 0, door);
		a = makeAction("TurnTo");
		addPart(a, kCommandTurnCharacter, hero, 270);
		a = makeAction("FriendWhere");
		addPart(a, kCommandIfCharacterInScene, friendRef, 0);
		log(a, "friend-here");
		addPart(a, kCommandElse, none, 0);
		log(a, "friend-away");
		addPart(a, kCommandEndIf, none, 0);
		a = makeAction("PlaceFriend");
		addPart(a, kCommandPlaceCharacter, friendRef, -1, 0, 0, door);
		a = makeAction("PlaceFriendAt");
		addPart(a, kCommandPlaceCharacterAt, friendRef, 400, 300, 180, hall);
		a = makeAction("GoToCrate");
		addPart(a, kCommandCharacterGoToObject, hero, 1, 0, 0, crate);
		log(a, "at-crate");
		a = makeAction("GoToPoint");
		addPart(a, kCommandCharacterGoTo, hero, 400, 100, 1);
		log(a, "at-point");
		a = makeAction("GoToDoor");
		addPart(a, kCommandCharacterGoToPoint, hero, 0, 0, 0, door);
		log(a, "at-door");
		a = makeAction("Direction");
		addPart(a, kCommandIfCharacterDirection, hero, 270, 0, 10);
		log(a, "dir-270");
		addPart(a, kCommandElse, none, 0);
		log(a, "dir-other");
		addPart(a, kCommandEndIf, none, 0);
		a = makeAction("Random");
		addPart(a, kCommandRandomValue, counter, 5, 9);
		a = makeAction("Speed");
		addPart(a, kCommandSetOutfitSpeed, hero, 222);
		a = makeAction("Outfit");
		addPart(a, kCommandChangeOutfit, heroOutfit2, 0);
		a = makeAction("MoveBy");
		addPart(a, kCommandMoveObjectBy, crate, 40, 20, 100);
		a = makeAction("MoveTo");
		addPart(a, kCommandMoveObjectTo, crate, 300, 400, 100);
		a = makeAction("ScrollPoint");
		addPart(a, kCommandScrollToPoint, none, 33, 44);
		a = makeAction("Inactive");
		addPart(a, kCommandCharacterActive, friendRef, 1);
		a = makeAction("Active");
		addPart(a, kCommandCharacterActive, friendRef, 0);
		a = makeAction("FarWalk");
		addPart(a, kCommandCharacterGoTo, hero, 600, 450, 0);
		a = makeAction("Stop");
		addPart(a, kCommandStopWalking, hero, 0);
		a = makeAction("GiveKey");
		addPart(a, kCommandCharacterItem, key, 0, 0, 0, hero);
		a = makeAction("GiveAll");
		addPart(a, kCommandGiveAllItems, hero, 0, 0, 0, friendRef);
		a = makeAction("ClearHero");
		addPart(a, kCommandClearItems, hero, 0);
		a = makeAction("ClearFriend");
		addPart(a, kCommandClearItems, friendRef, 0);
		a = makeAction("Inner");
		log(a, "inner");
		TVisObjRef inner = a;
		a = makeAction("Outer");
		addPart(a, kCommandStartAction, inner, 0);
		log(a, "outer");
		a = makeAction("Ways");
		addPart(a, kCommandSetWaySystem, ways2, 0);
		a = makeAction("WaysBack");
		addPart(a, kCommandSetWaySystem, firstWays, 0);
		a = makeAction("Area");
		addPart(a, kCommandSetScrollableAreaX, none, 10, 500);
		a = makeAction("TextSpeed");
		addPart(a, kCommandSetTextSpeed, none, 7);
		a = makeAction("CurrentIsHero");
		addPart(a, kCommandIfCurrentCharacter, hero, 0);
		log(a, "is-hero");
		addPart(a, kCommandElse, none, 0);
		log(a, "not-hero");
		addPart(a, kCommandEndIf, none, 0);

		// a second set: scrolling, waiting, the game's switches, scripts, following
		TVisObjRef script = game.CreateObject(30, gameRef, kGameScriptLinks);
		script.SetName(TCharHolder("Sc"));
		script.SetValue(kScriptScript, wxString(L"trace = (trace or '') .. 'script '"), TSendEventEnum::kNoEvent);
		TVisObjRef language = game.CreateObject(18, gameRef, kGameLanguages);
		language.SetName(TCharHolder("German"));
		TVisObjRef commentSet = game.CreateObject(16, hero, kCharacterCommentSets);
		commentSet.SetName(TCharHolder("Jokes"));

		a = makeAction("ScrollObject");
		addPart(a, kCommandScrollToObject, door, 0);
		a = makeAction("ScrollChar");
		addPart(a, kCommandScrollToCharacter, friendRef, 0);
		a = makeAction("WaitChar");
		addPart(a, kCommandCharacterGoTo, hero, 50, 400, 0);
		addPart(a, kCommandWaitCharacter, hero, 0);
		log(a, "waited");
		a = makeAction("Language");
		addPart(a, kCommandSetLanguage, language, 0);
		a = makeAction("Comments");
		addPart(a, kCommandSetCommentSet, commentSet, 0, 0, 0, hero);
		a = makeAction("WalkSound");
		a.SetName(TCharHolder("WalkSound"));
		{
			TVisObjRef part = addPart(a, kCommandSetWalkingSound, hero, 0);
			part.SetValue(kActionPartPath, wxFileName(L"sounds/steps.ogg"), TSendEventEnum::kNoEvent);
		}
		a = makeAction("Brightness");
		addPart(a, kCommandSetBrightness, none, 35);
		a = makeAction("HideCursor");
		addPart(a, kCommandHideCursor, none, 1);
		a = makeAction("ShowCursor");
		addPart(a, kCommandHideCursor, none, 0);
		a = makeAction("HideInterfaces");
		addPart(a, kCommandHideInterfaces, none, 1);
		a = makeAction("TextOutput");
		addPart(a, kCommandSetTextOutput, none, 2);
		a = makeAction("AnimIndex");
		addPart(a, kCommandSetAnimationIndex, hero, 3);
		a = makeAction("RunScript");
		addPart(a, kCommandRunScript, script, 0);
		a = makeAction("Follow");
		addPart(a, kCommandFollowCharacter, friendRef, 1, 30, 0, inner);
		a = makeAction("StopFollow");
		addPart(a, kCommandStopFollowing, hero, 0);
		a = makeAction("Cutscene");
		addPart(a, kCommandCutscene, none, 0);
		log(a, "in-cutscene");
		addPart(a, kCommandCutscene, none, 1);
		log(a, "after-cutscene");

		wxFileName out(L"cmd.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"cmd.dat");
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
		auto byName = [&](const char *wanted, int table = -1) {
			if (table >= 0) {
				TVisObjRef found;

				if (!control->GetVisionaire()->GetObjectByName(wxString(wanted), table, found))
					printf("no object %s\n", wanted);
				return found;
			}

			TVList objects;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, objects);
			for (TVisionaireObject *object : objects) {
				TVisObjRef ref(object);

				if (std::string(ref.GetName().c_str().mb_str()) == wanted)
					return ref;
			}
			printf("no action %s\n", wanted);
			return TVisObjRef();
		};
		auto trace = [&]() {
			lua_getfield(L, LUA_GLOBALSINDEX, "trace");
			std::string result = lua_tostring(L, -1) ? lua_tostring(L, -1) : "";
			lua_settop(L, 0);
			LuaDoString("trace = ''", "reset");
			return result;
		};
		auto run = [&](const char *name, int count = 40) {
			TGAction::AddRunningAction(byName(name));
			frames(count);
		};

		frames(20);

		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();
		TVisObjRef friendRef = byName("Friend", 0);
		TVisObjRef door = byName("Door", 6);
		TVisObjRef crate = byName("Crate", 6);
		TVisObjRef game = control->GetVisionaire()->GetGame();
		auto distance = [&](TVisObjRef character, int x, int y) {
			const wxPoint *position = character.GetPoint(kCharacterPosition);

			return std::sqrt((double)(position->x - x) * (position->x - x) + (double)(position->y - y) * (position->y - y));
		};

		LuaDoString("trace = ''", "reset");

		// turn to an object, and to a direction
		run("Turn");
		{
			const wxPoint *position = hero.GetPoint(kCharacterPosition);
			int expected = GetAngle((float)(500 - position->x), (float)(250 - position->y));

			printf("hero turned to the door: %d (expected %d)\n", hero.GetInt(kCharacterDirection), expected);
			CHECK(hero.GetInt(kCharacterDirection) == expected);
		}
		run("TurnTo");
		CHECK(hero.GetInt(kCharacterDirection) == 270);

		run("Direction");
		CHECK(trace() == "dir-270 ");

		run("CurrentIsHero");
		CHECK(trace() == "is-hero ");

		// a character in another scene is placed in this one
		run("FriendWhere");
		CHECK(trace() == "friend-away ");
		run("PlaceFriend");
		run("FriendWhere");
		CHECK(trace() == "friend-here ");
		printf("friend at %d,%d direction %d\n", friendRef.GetPoint(kCharacterPosition)->x, friendRef.GetPoint(kCharacterPosition)->y,
		       friendRef.GetInt(kCharacterDirection));
		CHECK(distance(friendRef, 500, 250) < 3);
		CHECK(friendRef.GetInt(kCharacterDirection) == 90);
		run("PlaceFriendAt");
		CHECK(distance(friendRef, 400, 300) < 3);
		CHECK(friendRef.GetInt(kCharacterDirection) == 90); // (a character that is in the scene already keeps its direction)

		// walks
		run("GoToCrate", 400);
		CHECK(trace() == "at-crate ");
		CHECK(distance(hero, 200, 350) < 3);
		run("GoToPoint", 400);
		CHECK(trace() == "at-point ");
		CHECK(distance(hero, 400, 100) < 3);
		run("GoToDoor", 400);
		CHECK(trace() == "at-door ");
		CHECK(distance(hero, 500, 250) < 3);

		// a walk that is stopped
		run("FarWalk", 10);
		CHECK(control->GetCurrentCharacter()->IsWalking());
		run("Stop", 5);
		frames(10);
		CHECK(!control->GetCurrentCharacter()->IsWalking());
		printf("hero stopped at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(distance(hero, 600, 450) > 20);

		// values, outfits
		run("Random");
		{
			int value = byName("Counter", 20).GetInt(kValueInt);

			printf("random value %d\n", value);
			CHECK(value >= 5 && value <= 9);
		}
		run("Speed");
		CHECK(hero.GetLink(kCharacterCurrentOutfit).GetInt(kOutfitCharacterSpeed) == 222);
		run("Outfit");
		CHECK(hero.GetLink(kCharacterCurrentOutfit).GetInt(kOutfitCharacterSpeed) == 400);

		// objects move
		run("MoveBy", 100);
		printf("crate offset %d,%d\n", crate.GetPoint(kObjectOffset)->x, crate.GetPoint(kObjectOffset)->y);
		CHECK(crate.GetPoint(kObjectOffset)->x == 40 && crate.GetPoint(kObjectOffset)->y == 20);
		run("MoveTo", 100);
		printf("crate offset %d,%d\n", crate.GetPoint(kObjectOffset)->x, crate.GetPoint(kObjectOffset)->y);
		CHECK(crate.GetPoint(kObjectOffset)->x == 300 - 200 && crate.GetPoint(kObjectOffset)->y == 400 - 350);

		// the scene
		run("ScrollPoint");
		CHECK(game.GetPoint(kGameScrollToPoint)->x == 33 && game.GetPoint(kGameScrollToPoint)->y == 44);
		run("Area");
		{
			const wxRect *area = control->GetScene()->GetRef().GetRect(kSceneScrollableArea);

			printf("scrollable area %d,%d %dx%d\n", area->x, area->y, area->width, area->height);
			CHECK(area->x == 10 && area->x + area->width - 1 == 500);
		}
		run("Ways");
		CHECK(control->GetScene()->GetRef().GetLink(kSceneCurrentWaySystem).GetName() == TCharHolder("Second"));
		run("WaysBack");
		run("TextSpeed");
		CHECK(game.GetInt(kGameTextSpeed) == 7);

		// the character is switched off and on
		run("Inactive");
		CHECK(!friendRef.GetBool(kCharacterActive));
		run("Active");
		CHECK(friendRef.GetBool(kCharacterActive));

		// items
		run("GiveKey");
		{
			TVList items;

			hero.GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			CHECK(items.size() == 1);
		}
		run("GiveAll");
		{
			TVList items;

			friendRef.GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			CHECK(items.size() == 1);
		}
		run("ClearFriend");
		{
			TVList items;

			friendRef.GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			CHECK(items.size() == 0);
		}
		run("ClearHero");
		{
			TVList items;

			hero.GetLinks(kCharacterItems, TypeOrder::kValue0, items);
			CHECK(items.size() == 0);
		}

		// scrolling
		run("ScrollObject");
		{
			const wxSize &size = control->GetScene()->GetVisibleSize();

			printf("scroll to object: %d,%d (visible %dx%d)\n", game.GetPoint(kGameScrollToPoint)->x, game.GetPoint(kGameScrollToPoint)->y,
			       size.width, size.height);
			CHECK(game.GetPoint(kGameScrollToPoint)->x == 500 - size.width / 2);
			CHECK(game.GetPoint(kGameScrollToPoint)->y == 250 - size.height / 2);
		}
		run("ScrollChar");
		CHECK(game.GetBool(kGameScrollCenterCharacter));
		CHECK(game.GetLink(kGameScrollCharacter) == friendRef);

		// waiting for a walk
		printf("before: state %d anim %d dest %d,%d pos %d,%d\n", hero.GetInt(kCharacterState), hero.GetInt(kCharacterAnimState),
		       hero.GetPoint(kCharacterDestination)->x, hero.GetPoint(kCharacterDestination)->y, hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		run("WaitChar", 1);
		printf("after: state %d anim %d dest %d,%d\n", hero.GetInt(kCharacterState), hero.GetInt(kCharacterAnimState),
		       hero.GetPoint(kCharacterDestination)->x, hero.GetPoint(kCharacterDestination)->y);
		printf("hero right after the wait at %d,%d walking %d trace %s\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y,
		       (int)control->GetCurrentCharacter()->IsWalking(), trace().c_str());
		frames(400);
		printf("hero at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(trace() == "waited ");
		CHECK(distance(hero, 50, 400) < 3);

		// the switches of the game
		run("Language");
		CHECK(game.GetLink(kGameStandardLanguage).GetName() == TCharHolder("German"));
		run("Comments");
		CHECK(hero.GetLink(kCharacterCurrentCommentSet).GetName() == TCharHolder("Jokes"));
		run("WalkSound");
		CHECK(hero.GetPath(kCharacterWalkingSound).GetFullPath().ToStdWstring() == L"sounds/steps.ogg");
		run("Brightness");
		CHECK(control->GetScene()->GetRef().GetInt(kSceneBrightness) == 35);
		run("HideCursor");
		CHECK(game.GetBool(kGameHideCursor));
		run("ShowCursor");
		CHECK(!game.GetBool(kGameHideCursor));
		run("HideInterfaces");
		CHECK(game.GetBool(kGameHideInterfaces));
		run("TextOutput");
		CHECK(game.GetInt(kGameTextOutput) == 2);
		run("AnimIndex");
		CHECK(hero.GetInt(kCharacterAnimIndex) == 3);
		run("RunScript");
		CHECK(trace() == "script ");

		// following
		run("Follow");
		CHECK(hero.GetLink(kCharacterFollowCharacter) == friendRef);
		CHECK(hero.GetInt(kCharacterFollowReachDistance) == 30);
		run("StopFollow");
		CHECK(hero.GetLink(kCharacterFollowCharacter).IsEmpty());

		// a cutscene
		run("Cutscene");
		{
			std::string result = trace();

			printf("cutscene trace: %s\n", result.c_str());
			CHECK(result == "in-cutscene after-cutscene ");
		}
		CHECK(!game.GetBool(kGameHideCursor));

		// an action started by an action
		run("Outer");
		printf("trace: %s\n", trace().c_str());
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

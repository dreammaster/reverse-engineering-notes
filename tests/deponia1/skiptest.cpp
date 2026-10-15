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

// A cutscene (a walk, a text, a wait, a change of scene) is skipped with the escape key.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	useFakePictures();
	writeFakePicture("pics/bar.png", 200, 200);
	
	{
		TVisionaireGame game;
		TVisObjRef gameRef = game.GetGame();
		gameRef.SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameCompanyName, wxString(L"Test Company"), TSendEventEnum::kNoEvent);
		gameRef.SetValue(kGameGameName, wxString(L"Test Game"), TSendEventEnum::kNoEvent);

		auto makeScene = [&](const char *name, wxPoint startAt, const char *characterName, TVisObjRef &startOut, TVisObjRef &characterOut, const char *interfaceName, TVisObjRef &interfaceOut) {
			TVisObjRef scene = game.CreateObject(4, gameRef, kGameSceneLinks);
			scene.SetName(TCharHolder(name));
			TVisObjRef character = game.CreateObject(0, gameRef, kGameCharacterLinks);
			character.SetName(TCharHolder(characterName));
			TVisObjRef startObject = game.CreateObject(6, scene, kSceneObjects);
			startObject.SetName(TCharHolder("Start"));
			startObject.SetValue(kObjectPosition, startAt, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterStartObject, startObject, true);
			TVisObjRef ways = game.CreateObject(29, scene, kSceneWaySystems);
			std::vector<wxPoint> border = {{0, 0}, {640, 0}, {640, 480}, {0, 480}};
			ways.SetValue(kWaySystemBorder, border, TSendEventEnum::kNoEvent);
			scene.SetLink(kSceneCurrentWaySystem, ways, true);
			TVisObjRef outfit = game.CreateObject(17, character, kCharacterOutfits);
			outfit.SetValue(kOutfitCharacterSpeed, 100, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterCurrentOutfit, outfit, true);

			// the interface of the character
			TVisObjRef interface = game.CreateObject(1, gameRef, kGameInterfaces);
			interface.SetName(TCharHolder(interfaceName));
			TSprite background;
			background.SetPath(TCharHolder("pics/bar.png"));
			interface.SetValue(kInterfaceSprite, background, TSendEventEnum::kNoEvent);
			std::vector<wxPoint> bar = {{0, 0}, {200, 0}, {200, 200}, {0, 200}};
			interface.SetValue(kInterfaceBorder, bar, TSendEventEnum::kNoEvent);
			character.SetLink(kCharacterInterfaces, interface, true);

			startOut = startObject;
			characterOut = character;
			interfaceOut = interface;
			return scene;
		};

		TVisObjRef hallStart, hero, heroBar, kitchenStart, girl, girlBar;
		TVisObjRef hall = makeScene("Hall", wxPoint{100, 200}, "Hero", hallStart, hero, "HeroBar", heroBar);
		TVisObjRef kitchen = makeScene("Kitchen", wxPoint{300, 300}, "Girl", kitchenStart, girl, "GirlBar", girlBar);
		gameRef.SetLink(kGameFirstCharacter, hero, true);

		// the start of the game: the hall is shown
		TVisObjRef action = game.CreateObject(7, gameRef, kGameActions);
		action.SetName(TCharHolder("Start"));
		TVisObjRef change = game.CreateObject(8, action, kActionActionParts);
		change.SetValue(kActionPartCommand, kCommandChangeScene, TSendEventEnum::kNoEvent);
		change.SetLink(kActionPartLink, hallStart, true);
		change.SetLink(kActionPartAltLink, hero, true);
		gameRef.SetLink(kGameStartAction, action, true);

		// a language, a font and a text the hero says
		TVisObjRef language = game.CreateObject(18, gameRef, kGameLanguages);
		language.SetName(TCharHolder("English"));
		gameRef.SetLink(kGameStandardLanguage, language, true);
		TVisObjRef font = game.CreateObject(3, gameRef, kGameFontLinks);
		font.SetName(TCharHolder("Font"));
		std::vector<wxRect> rects;
		for (int k = 0; k < 26; k++) {
			wxRect r;
			r.x = k * 8;
			r.y = 0;
			r.width = 8;
			r.height = 12;
			rects.push_back(r);
		}
		font.SetValue(kFontLetters, rects, TSendEventEnum::kNoEvent);
		font.SetValue(kFontAlphabet, wxString(L"abcdefghijklmnopqrstuvwxyz"), TSendEventEnum::kNoEvent);
		gameRef.SetLink(kGameActionTextFont, font, true);
		TVisObjRef noParent;
		TVisObjRef text = game.CreateObject(14, noParent, -1);
		text.SetName(TCharHolder("Hello"));
		std::vector<TTextLanguage> texts(1);
		texts[0].text = TCharHolder("hello world");
		texts[0].languageId = PackVisId(language.GetId());
		text.SetValue(kTextTextLanguages, texts, TSendEventEnum::kNoEvent);

		// the cutscene
		TVisObjRef cutscene = game.CreateObject(7, gameRef, kGameActions);
		cutscene.SetName(TCharHolder("Cutscene"));
		auto addPart = [&](int command, int value, int altValue = 0) {
			TVisObjRef part = game.CreateObject(8, cutscene, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			return part;
		};
		auto mark = [&](const char *code) {
			TVisObjRef part = addPart(kCommandRunPartScript, 0);
			part.SetValue(kActionPartString, wxString(code), TSendEventEnum::kNoEvent);
		};
		addPart(kCommandCutscene, 0);
		mark("cut_1 = true");
		TVisObjRef walk = addPart(kCommandCharacterGoTo, 1, 0);
		walk.SetLink(kActionPartLink, hero, true);
		walk.SetValue(kActionPartInt, 500, TSendEventEnum::kNoEvent);
		walk.SetValue(kActionPartAltInt, 100, TSendEventEnum::kNoEvent);
		walk.SetValue(kActionPartAltInt2, 1, TSendEventEnum::kNoEvent);
		mark("cut_2 = true");
		TVisObjRef say = addPart(kCommandShowText, 0);
		say.SetLink(kActionPartLink, text, true);
		say.SetLink(kActionPartAltLink, hero, true);
		mark("cut_3 = true");
		addPart(kCommandWait, 5, 1);
		TVisObjRef toKitchen = addPart(kCommandChangeScene, 0);
		toKitchen.SetLink(kActionPartLink, kitchenStart, true);
		toKitchen.SetLink(kActionPartAltLink, hero, true);
		mark("cut_4 = true");
		addPart(kCommandCutscene, 1);
		mark("cut_5 = true");

		// a cutscene that only walks and waits
		TVisObjRef walkOnly = game.CreateObject(7, gameRef, kGameActions);
		walkOnly.SetName(TCharHolder("WalkOnly"));
		auto addPart2 = [&](int command, int value, int altValue = 0) {
			TVisObjRef part = game.CreateObject(8, walkOnly, kActionActionParts);
			part.SetValue(kActionPartCommand, command, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartInt, value, TSendEventEnum::kNoEvent);
			part.SetValue(kActionPartAltInt, altValue, TSendEventEnum::kNoEvent);
			return part;
		};
		addPart2(kCommandCutscene, 0);
		TVisObjRef walk2 = addPart2(kCommandCharacterGoTo, 1, 0);
		walk2.SetLink(kActionPartLink, hero, true);
		walk2.SetValue(kActionPartInt, 450, TSendEventEnum::kNoEvent);
		walk2.SetValue(kActionPartAltInt, 120, TSendEventEnum::kNoEvent);
		walk2.SetValue(kActionPartAltInt2, 1, TSendEventEnum::kNoEvent);
		addPart2(kCommandWait, 5, 1);
		TVisObjRef markPart = addPart2(kCommandRunPartScript, 0);
		markPart.SetValue(kActionPartString, wxString("walk_only_done = true"), TSendEventEnum::kNoEvent);
		addPart2(kCommandCutscene, 1);

		wxFileName out(L"skip.dat");
		CHECK(game.BinarySave(out, nullptr, false, nullptr));
	}

	graphics = new NullGraphics();
	THGameControl *control = new THGameControl();
	g_pGameControl = control;
	TVisionaire::SetVisPlayerMode(true);

	wxString file(L"skip.dat");
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

		auto name = [&](const TVisObjRef &ref) {
			return std::string(ref.GetName().c_str().mb_str());
		};
		auto isSet = [&](const char *variable) {
			lua_getfield(L, LUA_GLOBALSINDEX, variable);
			bool result = !lua_isnil(L, -1);

			lua_settop(L, 0);
			return result;
		};
		auto actionByName = [&](const char *wanted) {
			TVList actions;

			control->GetVisionaire()->GetGame().GetLinks(kGameActions, TypeOrder::kValue0, actions);
			for (TVisionaireObject *object : actions) {
				TVisObjRef ref(object);

				if (name(ref) == wanted)
					return ref;
			}
			return TVisObjRef();
		};

		frames(30);
		TVisObjRef gameRef = control->GetVisionaire()->GetGame();
		TVisObjRef hero = control->GetCurrentCharacter()->GetRef();

		TGAction::AddRunningAction(actionByName("Cutscene"));
		frames(10);
		printf("cutscene started: cut_1 %d, cursor hidden %d, interfaces hidden %d, scene %s\n", (int)isSet("cut_1"), (int)gameRef.GetBool(kGameHideCursor), (int)gameRef.GetBool(kGameHideInterfaces), name(control->GetScene()->GetRef()).c_str());
		CHECK(isSet("cut_1"));
		CHECK(gameRef.GetBool(kGameHideCursor));
		CHECK(gameRef.GetBool(kGameHideInterfaces));
		CHECK(!gameRef.GetLink(kGameCutsceneAction).IsEmpty());
		CHECK(!isSet("cut_2"));

		// the walk takes a while: the hero is on the way when the cutscene is skipped
		frames(40);
		printf("walking: at %d,%d\n", hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(hero.GetPoint(kCharacterPosition)->x < 500);
		CHECK(!isSet("cut_2"));

		// the escape key (released) skips the cutscene
		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyUp, wxString(), 0x1B, 0);
		frames(30);
		printf("skipped: cut_2 %d cut_3 %d cut_4 %d cut_5 %d, hero at %d,%d, scene %s, cursor hidden %d, interfaces hidden %d, cutscene action %d\n", (int)isSet("cut_2"), (int)isSet("cut_3"), (int)isSet("cut_4"), (int)isSet("cut_5"), hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y, name(control->GetScene()->GetRef()).c_str(), (int)gameRef.GetBool(kGameHideCursor), (int)gameRef.GetBool(kGameHideInterfaces), (int)!gameRef.GetLink(kGameCutsceneAction).IsEmpty());
		CHECK(isSet("cut_2"));
		CHECK(isSet("cut_3"));
		CHECK(isSet("cut_4"));
		CHECK(isSet("cut_5"));
		CHECK(hero.GetPoint(kCharacterPosition)->x == 300 && hero.GetPoint(kCharacterPosition)->y == 300);
		CHECK(name(control->GetScene()->GetRef()) == "Kitchen");
		CHECK(!gameRef.GetBool(kGameHideCursor));
		CHECK(!gameRef.GetBool(kGameHideInterfaces));
		CHECK(gameRef.GetLink(kGameCutsceneAction).IsEmpty());

		// the walk of the second one is finished by the skip: the hero is where it went
		TGAction::AddRunningAction(actionByName("WalkOnly"));
		frames(40);
		hero = control->GetCurrentCharacter()->GetRef();
		CHECK(!isSet("walk_only_done"));
		CHECK(!(hero.GetPoint(kCharacterPosition)->x == 450 && hero.GetPoint(kCharacterPosition)->y == 120));
		control->HandleKeyEvent(TKeyboardMessageEnum::kKeyUp, wxString(), 0x1B, 0);
		frames(30);
		printf("second skipped: done %d, hero at %d,%d\n", (int)isSet("walk_only_done"), hero.GetPoint(kCharacterPosition)->x, hero.GetPoint(kCharacterPosition)->y);
		CHECK(isSet("walk_only_done"));
		CHECK(hero.GetPoint(kCharacterPosition)->x == 450 && hero.GetPoint(kCharacterPosition)->y == 120);
		CHECK(!gameRef.GetBool(kGameHideCursor));
	}

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}
